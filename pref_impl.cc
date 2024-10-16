#include "sim.h"

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
