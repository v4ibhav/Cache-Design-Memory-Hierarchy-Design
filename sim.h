#ifndef SIM_CACHE_H
#define SIM_CACHE_H
#include <algorithm>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <endian.h>
#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <vector>

using namespace std;
using u32 = uint32_t;
#define foru(i, n) for (u32 i = 0; i < n; i++)

typedef struct {
	u32 BLOCKSIZE;
	u32 L1_SIZE;
	u32 L1_ASSOC;
	u32 L2_SIZE;
	u32 L2_ASSOC;
	u32 PREF_N;
	u32 PREF_M;
} cache_params_t;

// Put additional data structures here as per your requirement.

class BLOCK {
  public:
	bool m_valid_bit = false;
	bool m_dirty_bit = false;
	u32 m_blockindex = 0;
	u32 m_blockaddr = 0;
	u32 m_blocktag = 0;
	u32 m_LRU = 0xffffffff;

	BLOCK() = default;

	void elaborate_block();
};

class STREAM_BUFF {

  public:
	u32 nN, mM;
	vector<bool> valid_b;
	vector<u32> buffer_lru;

	vector<vector<BLOCK>> buffer;

	STREAM_BUFF(u32 N, u32 M) {
		nN = N;
		mM = M;
		buffer.resize(nN, vector<BLOCK>(mM));
		valid_b.resize(nN, false);
		buffer_lru.resize(nN, 0);
	}

	void pref_miss_on_cache_miss(u32);
	void pref_hit_on_cache_miss(u32, u32, u32, bool);
	bool sbuffr_hit_or_miss(u32, u32 &, u32 &);
	void sbuffr_place(u32, u32);
};

class CACHE {

  public:
	u32 m_blocksize;
	u32 m_size;
	u32 m_assoc;
	u32 m_sets;
	vector<vector<BLOCK>> m_cache;

	u32 m_level;
	CACHE *m_next_CACHE;
	STREAM_BUFF *m_prefetcher;

	CACHE(u32 size, u32 assoc, u32 blocksize, u32 level, CACHE *next_level, u32 pref_N,
		  u32 pref_M) {
		m_size = size;
		m_assoc = assoc;
		m_blocksize = blocksize;
		m_level = level;
		m_next_CACHE = next_level;

		if (m_assoc != 0) {
			m_sets = m_size / (m_blocksize * m_assoc);
		} else {
			m_sets = 0;
		}

		m_cache = vector<vector<BLOCK>>(m_sets, vector<BLOCK>(m_assoc));

		if (m_next_CACHE == nullptr && pref_N != 0 && pref_M != 0) {
			m_prefetcher = new STREAM_BUFF(pref_N, pref_M);
		} else {
			m_prefetcher = nullptr;
		}
	}

	void cache_access(char, u32);

	bool cache_hit_or_miss(char rw, u32 set_number, u32 address);
	void place_cache_block(char, u32);
	bool place_at_invalid_space(char, u32, u32);
	BLOCK *search_for_victim_block(u32);
	void update_cache(u32, BLOCK *);
	void cache_block_upgrade(char rw, BLOCK *, u32, u32);

	void handle_miss(char mode, u32 addr, u32 block_addr, u32 &row, u32 &col);
	void handle_victim_block(BLOCK *victim, char rw, u32 addr, u32 block_addr, u32 &row, u32 &col);
	void handle_prefetcher_hit_1(u32 block_addr, u32 &row, u32 &col, bool is_hit);
	void handle_prefetcher_hit_2(u32 block_addr, u32 &row, u32 &col, bool is_hit);

	void update_access_count(char mode);
	void update_miss_count(char mode);

	u32 cache_block_addr_calc(u32);
	u32 find_address_calc(u32, bool);
	void total_prefetches_calc(u32);
	// MEASUREMENTS
	u32 reads = 0;
	u32 read_misses = 0;
	u32 writes = 0;
	u32 write_misses = 0;
	u32 writebacks = 0;
	u32 mem_traffic = 0;
	u32 prefetches = 0;

	// OUTPUTS
	void elaborate_cache();
	void elaborate_prefetcher();
	void display_cache_measurements();
};
#endif
