// #define DEBUG
#include "sim.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <sys/types.h>

/*  "argc" holds the number of command-line arguments.
		"argv[]" holds the arguments themselves.

Example:
./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
argc = 9
argv[0] = "./sim"
argv[1] = "32"
argv[2] = "8192"
... and so on
*/
int main(int argc, char *argv[]) {
	FILE *fp;			   // File pointer.
	char *trace_file;	   // This variable holds the trace file name.
	cache_params_t params; // Look at the sim.h header file for the definition
						   // of struct cache_params_t.
	char rw;			   // This variable holds the request's type (read or write) obtained
						   // from the trace.
	uint32_t addr;		   // This variable holds the request's address obtained from the
						   // trace. The header file <inttypes.h> above defines signed and
						   // unsigned integers of various sizes in a machine-agnostic
						   // way.  "uint32_t" is an unsigned integer of 32 bits.

	// Exit with an error if the number of command-line arguments is incorrect.
	if (argc != 9) {
		printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
		exit(EXIT_FAILURE);
	}

	// "atoi()" (included by <stdlib.h>) converts a string (char *) to an
	// integer (int).
	params.BLOCKSIZE = (uint32_t)atoi(argv[1]);
	params.L1_SIZE = (uint32_t)atoi(argv[2]);
	params.L1_ASSOC = (uint32_t)atoi(argv[3]);
	params.L2_SIZE = (uint32_t)atoi(argv[4]);
	params.L2_ASSOC = (uint32_t)atoi(argv[5]);
	params.PREF_N = (uint32_t)atoi(argv[6]);
	params.PREF_M = (uint32_t)atoi(argv[7]);
	trace_file = argv[8];

	// Open the trace file for reading.
	fp = fopen(trace_file, "r");
	if (fp == (FILE *)NULL) {
		// Exit with an error if file open failed.
		printf("Error: Unable to open file %s\n", trace_file);
		exit(EXIT_FAILURE);
	}

	// Print simulator configuration.
	printf("===== Simulator configuration =====\n");
	printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
	printf("L1_SIZE:    %u\n", params.L1_SIZE);
	printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
	printf("L2_SIZE:    %u\n", params.L2_SIZE);
	printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
	printf("PREF_N:     %u\n", params.PREF_N);
	printf("PREF_M:     %u\n", params.PREF_M);
	printf("trace_file: %s\n", trace_file);
	printf("\n");
	// Check if L2 cache exists and create it if necessary
	CACHE *L2 = nullptr;
	if (params.L2_SIZE != 0) {
		// L2 is the last level, so it gets the prefetcher
		L2 = new CACHE(params.L2_SIZE, params.L2_ASSOC, params.BLOCKSIZE, 2, nullptr, params.PREF_N,
					   params.PREF_M);
	}

	// Create L1 cache
	// If L2 cache does not exist, L1 is the last level and gets the prefetcher
	uint32_t L1_prefetch_N = 0;
	uint32_t L1_prefetch_M = 0;

	if (params.L2_SIZE == 0) {
		// L1 is the last level, so it gets the prefetcher
		L1_prefetch_N = params.PREF_N;
		L1_prefetch_M = params.PREF_M;
	}

	CACHE L1(params.L1_SIZE, params.L1_ASSOC, params.BLOCKSIZE, 1, L2, L1_prefetch_N,
			 L1_prefetch_M);
	// Read requests from the trace file and echo them back.
	// uint32_t i = 1; //DEBUG
	while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) { // Stay in the loop if fscanf() successfully
													 // parsed two tokens as specified.
#ifdef DEBUG
		printf("%u=%c %x \n", i, rw, addr);
		i++;
#endif // DEBUG
		if (rw == 'r')
			// printf("r %x\n", addr);
			L1.cache_access(rw, addr);
		else if (rw == 'w')
			// printf("w %x\n", addr);
			L1.cache_access(rw, addr);
		else {
			printf("Error: Unknown request type %c.\n", rw);
			exit(EXIT_FAILURE);
		}
	}
	// instruction name
	// L1.debugger();
	//
	L1.elaborate_cache();
	if (L2 != nullptr) {
		L2->elaborate_cache();
		L2->elaborate_prefetcher();
	} else {
		L1.elaborate_prefetcher();
	}
	L1.display_cache_measurements();

	return (0);
}

void CACHE::cache_access(char mode, u32 addr) {
	update_access_count(mode);
	bool hit = cache_hit_or_miss(mode, find_address_calc(addr, true), addr);
	u32 block_addr = cache_block_addr_calc(addr);
	u32 sbuffr_target_row = 0, sbuffr_target_col = 0;

	if (hit) {
		handle_prefetcher_hit_2(block_addr, sbuffr_target_row, sbuffr_target_col, true);
	} else {
		handle_miss(mode, addr, block_addr, sbuffr_target_row, sbuffr_target_col);
		place_cache_block(mode, addr);
	}
}

bool CACHE::cache_hit_or_miss(char rw, u32 set_number, u32 address) {
	bool hit = false;
	u32 target_tag = find_address_calc(address, false);
	for (auto &block : m_cache[set_number]) {
		if (block.m_valid_bit && block.m_blocktag == target_tag) {
			hit = true;
			if (rw == 'w') {
				block.m_dirty_bit = true;
			}
			update_cache(set_number, &block);
		}
	}
	return hit;
}

void CACHE::place_cache_block(char rw, u32 addr) {
	u32 index = find_address_calc(addr, true);
	bool allocated = place_at_invalid_space(rw, index, addr);
	u32 block_addr = cache_block_addr_calc(addr);
	u32 sbuffr_target_row = 0, sbuffr_target_col = 0;
	BLOCK *victim = nullptr;
	index = find_address_calc(addr, true);
	if (!allocated) {
		victim = search_for_victim_block(index);
		handle_victim_block(victim, rw, addr, block_addr, sbuffr_target_row, sbuffr_target_col);
	}
}

bool CACHE::place_at_invalid_space(char rw, u32 index, u32 addr) {
	u32 sbuffr_target_row = 0, sbuffr_target_col = 0;
	foru(a, m_assoc) {
		BLOCK &block = m_cache[index][a];
		if (!block.m_valid_bit) {
			if (m_prefetcher != nullptr) {
				handle_prefetcher_hit_1(cache_block_addr_calc(addr), sbuffr_target_row,
										sbuffr_target_col, false);
			} else {
				if (m_next_CACHE != nullptr) {
					m_next_CACHE->cache_access('r', addr);
				}
			}
			cache_block_upgrade(rw, &block, addr, index);
			return true;
		}
	}
	return false;
}

BLOCK *CACHE::search_for_victim_block(u32 index) {
	BLOCK *victim = nullptr;
	u32 highest_lru = 0;
	for (auto &block : m_cache[index]) {
		if (block.m_LRU >= highest_lru) {
			victim = &block;
			highest_lru = block.m_LRU;
		}
	}
	return victim;
}

void CACHE::update_cache(u32 index, BLOCK *access_block) {
	for (auto &block : m_cache[index]) {
		if (block.m_LRU < access_block->m_LRU) {
			block.m_LRU++;
		}
	}
	access_block->m_valid_bit = true;
	access_block->m_LRU = 0;
}

void CACHE::cache_block_upgrade(char rw, BLOCK *victim, u32 addr, u32 index) {
	victim->m_blockaddr = addr;
	victim->m_blocktag = find_address_calc(addr, false);
	victim->m_dirty_bit = (rw == 'w');
	update_cache(index, victim);
}

void CACHE::handle_miss(char mode, u32 addr, u32 block_addr, u32 &row, u32 &col) {
	if (m_prefetcher != nullptr) {
		bool sbuffr_hit = m_prefetcher->sbuffr_hit_or_miss(block_addr, row, col);
		if (!sbuffr_hit) {
			update_miss_count(mode);
		}
	} else {
		update_miss_count(mode);
	}
}

void CACHE::handle_victim_block(BLOCK *victim, char rw, u32 addr, u32 block_addr, u32 &row,
								u32 &col) {
	if (m_next_CACHE != nullptr) {
		if (victim->m_dirty_bit) {
			writebacks++;
			m_next_CACHE->cache_access('w', victim->m_blockaddr);
		}
		m_next_CACHE->cache_access('r', addr);
	} else {
		if (victim->m_dirty_bit) {
			writebacks++;
		}
		handle_prefetcher_hit_1(block_addr, row, col, false);
	}
	cache_block_upgrade(rw, victim, addr, find_address_calc(addr, true));
}

void CACHE::handle_prefetcher_hit_1(u32 block_addr, u32 &row, u32 &col, bool is_hit) {
	if (m_prefetcher != nullptr) {
		bool sbuffr_hit = m_prefetcher->sbuffr_hit_or_miss(block_addr, row, col);
		if (sbuffr_hit) {
			m_prefetcher->pref_hit_on_cache_miss(block_addr, row, col, is_hit);
			total_prefetches_calc(col);
		} else {
			m_prefetcher->pref_miss_on_cache_miss(block_addr);
			u32 ind = m_prefetcher->mM - 1;
			total_prefetches_calc(ind);
		}
	}
}

void CACHE::handle_prefetcher_hit_2(u32 block_addr, u32 &row, u32 &col, bool is_hit) {
	if (m_prefetcher != nullptr) {
		bool sbuffr_hit = m_prefetcher->sbuffr_hit_or_miss(block_addr, row, col);
		if (sbuffr_hit) {
			m_prefetcher->pref_hit_on_cache_miss(block_addr, row, col, is_hit);
			total_prefetches_calc(col);
		}
	}
}

void CACHE::update_access_count(char mode) {
	if (mode == 'r') {
		reads++;
	} else if (mode == 'w') {
		writes++;
	}
}

void CACHE::update_miss_count(char mode) {
	if (mode == 'r') {
		read_misses++;
	} else if (mode == 'w') {
		write_misses++;
	}
}

u32 CACHE::cache_block_addr_calc(u32 addr) { return addr >> static_cast<u32>(log2(m_blocksize)); }

u32 CACHE::find_address_calc(u32 addr, bool find_index) {
	u32 component, blockoffset_bits, index_bits, bitmask, shift_addr;

	blockoffset_bits = static_cast<u32>(log2(m_blocksize));
	index_bits = static_cast<u32>(log2(m_sets));
	if (find_index) {
		shift_addr = addr >> blockoffset_bits;
		bitmask = (1 << index_bits) - 1;
		component = shift_addr & bitmask;
	} else {
		component = addr >> (blockoffset_bits + index_bits);
	}
	return component;
}

void CACHE::total_prefetches_calc(u32 hit_index) {
	auto number_of_blocks = hit_index + 1;
	prefetches += number_of_blocks;
}
void CACHE::elaborate_cache() {
	printf("===== L%x contents =====\n", m_level);

	foru(set_id, m_sets) {
		printf("set %d: ", set_id);

		std::vector<BLOCK> sorted_blocks = m_cache[set_id];
		std::sort(sorted_blocks.begin(), sorted_blocks.end(),
				  [](const BLOCK &a, const BLOCK &b) { return a.m_LRU < b.m_LRU; });

		foru(assoc, m_assoc) { sorted_blocks[assoc].elaborate_block(); }

		printf("\n");
	}
	printf("\n");
}

void BLOCK::elaborate_block() {
	printf("%x", m_blocktag);
	if (m_dirty_bit == 1) {
		printf(" D  ");
	} else {
		printf("	");
	}
}

void CACHE::elaborate_prefetcher() {
	if (m_prefetcher == nullptr) {
		return;
	}
	printf("===== Stream Buffer(s) contents=====\n");

	vector<u32> sorted_indices;
	foru(i, m_prefetcher->nN) { sorted_indices.push_back(i); }

	sort(sorted_indices.begin(), sorted_indices.end(), [&](u32 i1, u32 i2) {
		return m_prefetcher->buffer_lru[i1] < m_prefetcher->buffer_lru[i2];
	});

	for (u32 n : sorted_indices) {
		if (m_prefetcher->valid_b[n]) {
			foru(m, m_prefetcher->mM) { printf("%8x\t", m_prefetcher->buffer[n][m].m_blockaddr); }
			printf("\n");
		}
	}
	printf("\n");
}

void CACHE::display_cache_measurements() {
	printf("===== Measurements =====\n");
	printf("a. L1 reads:                   %d\n", reads);
	printf("b. L1 read misses:             %d\n", read_misses);
	printf("c. L1 writes:                  %d\n", writes);
	printf("d. L1 write misses:            %d\n", write_misses);
	printf("e. L1 miss rate:               %.4f\n",
		   (reads > 0) ? (float)(read_misses + write_misses) / (reads + writes) : 1.0);
	printf("f. L1 writebacks:              %d\n", writebacks);
	if (m_prefetcher != nullptr) {
		printf("g. L1 prefetches:              %d\n", prefetches);
	} else {
		printf("g. L1 prefetches:              0\n");
	}
	if (m_next_CACHE) {
		CACHE *L2_cache = m_next_CACHE;
		printf("h. L2 reads (demand):          %d\n", L2_cache->reads);
		printf("i. L2 read misses (demand):    %d\n", L2_cache->read_misses);
		printf("j. L2 reads (prefetch):        0\n");
		printf("k. L2 read misses (prefetch):  0\n");
		printf("l. L2 writes:                  %d\n", L2_cache->writes);
		printf("m. L2 write misses:            %d\n", L2_cache->write_misses);
		printf("n. L2 miss rate:               %.4f\n",
			   (L2_cache->reads > 0) ? (float)L2_cache->read_misses / L2_cache->reads : 0.0);
		printf("o. L2 writebacks:              %d\n", L2_cache->writebacks);
		printf("p. L2 prefetches:              %d\n", L2_cache->prefetches);
		m_next_CACHE->mem_traffic = m_next_CACHE->read_misses + m_next_CACHE->write_misses +
									m_next_CACHE->writebacks + m_next_CACHE->prefetches;
		printf("q. memory traffic:             %d\n", L2_cache->mem_traffic);
	} else {
		printf("h. L2 reads (demand):          0\n");
		printf("i. L2 read misses (demand):    0\n");
		printf("j. L2 reads (prefetch):        0\n");
		printf("k. L2 read misses (prefetch):  0\n");
		printf("l. L2 writes:                  0\n");
		printf("m. L2 write misses:            0\n");
		printf("n. L2 miss rate:               0.0000\n");
		printf("o. L2 writebacks:              0\n");
		printf("p. L2 prefetches:              0\n");
		mem_traffic = read_misses + write_misses + writebacks + prefetches;
		printf("q. memory traffic:             %d\n", mem_traffic);
	}
}

void STREAM_BUFF::pref_miss_on_cache_miss(u32 block_addr) {
	u32 target_row = 0, highest_lru_found = 0;

	foru(nn, nN) {
		if (valid_b[nn])
			continue;
		sbuffr_place(block_addr, nn);
		return;
	}

	foru(nn, nN) {
		if (buffer_lru[nn] < highest_lru_found)
			continue;
		target_row = nn;
		highest_lru_found = buffer_lru[nn];
	}

	sbuffr_place(block_addr, target_row);
}

void STREAM_BUFF::pref_hit_on_cache_miss(u32 block_addr, u32 hit_row, u32 hit_col, bool scenario4) {
	vector<BLOCK> replace_buffer_row(mM);

	u32 hit_lru = buffer_lru[hit_row];

	foru(nn, nN) {
		if (nn == hit_row || !valid_b[nn])
			continue;
		if (buffer_lru[nn] < hit_lru) {
			buffer_lru[nn]++;
		}
	}

	buffer_lru[hit_row] = 0;
	replace_buffer_row[0] = buffer[hit_row][hit_col];
	replace_buffer_row[0].m_blockaddr += 1;

	foru(r, mM - 1) {
		replace_buffer_row[r + 1].m_blockaddr = replace_buffer_row[r].m_blockaddr + 1;
		buffer[hit_row][r + 1] = replace_buffer_row[r + 1];
	}

	buffer[hit_row][0] = replace_buffer_row[0];
}

bool STREAM_BUFF::sbuffr_hit_or_miss(u32 block_addr, u32 &row_n, u32 &col_m) {
	bool found = false;
	u32 lowest_lru_faced = 0xffffffff;
	foru(nn, nN) {
		if (!valid_b[nn])
			continue;
		foru(mm, mM) {
			if (buffer[nn][mm].m_blockaddr != block_addr)
				continue;
			if (buffer_lru[nn] < lowest_lru_faced) {
				row_n = nn;
				col_m = mm;
				lowest_lru_faced = buffer_lru[nn];
				found = true;
			}
			break;
		}
	}
	return found;
}
void STREAM_BUFF::sbuffr_place(u32 block_addr, u32 target_row) {
	u32 data = block_addr + 1;

	foru(nn, nN) {
		if (!valid_b[nn])
			continue;
		buffer_lru[nn] += 1;
	}

	valid_b[target_row] = true;
	buffer_lru[target_row] = 0;

	foru(mm, mM) { buffer[target_row][mm].m_blockaddr = data++; }
}
