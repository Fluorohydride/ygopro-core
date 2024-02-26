#ifndef CARD_DATA_H_
#define CARD_DATA_H_

#include "common.h"

constexpr int CARD_ARTWORK_VERSIONS_OFFSET = 20;
constexpr int SIZE_SETCODE = 16;
constexpr int CARD_BLACK_LUSTER_SOLDIER2 = 5405695;

struct card_data {
	uint32 code{};
	uint32 alias{};
	uint16_t setcode[SIZE_SETCODE]{};
	uint32 type{};
	uint32 level{};
	uint32 attribute{};
	uint32 race{};
	int32 attack{};
	int32 defense{};
	uint32 lscale{};
	uint32 rscale{};
	uint32 link_marker{};

	void clear() {
		code = 0;
		alias = 0;
		for (auto& x : setcode)
			x = 0;
		type = 0;
		level = 0;
		attribute = 0;
		race = 0;
		attack = 0;
		defense = 0;
		lscale = 0;
		rscale = 0;
		link_marker = 0;
	}

	bool is_setcode(uint32 value) const {
		uint16_t settype = value & 0x0fff;
		uint16_t setsubtype = value & 0xf000;
		for (auto& x : setcode) {
			if ((x & 0x0fff) == settype && (x & 0xf000 & setsubtype) == setsubtype)
				return true;
			if (!x)
				return false;
		}
		return false;
	}

	bool is_alternative() const {
		if (code == CARD_BLACK_LUSTER_SOLDIER2)
			return false;
		return alias && (alias < code + CARD_ARTWORK_VERSIONS_OFFSET) && (code < alias + CARD_ARTWORK_VERSIONS_OFFSET);
	}

	void set_setcode(uint64 value) {
		int ctr = 0;
		while (value) {
			if (value & 0xffff) {
				setcode[ctr] = value & 0xffff;
				++ctr;
			}
			value >>= 16;
		}
		for (int i = ctr; i < SIZE_SETCODE; ++i)
			setcode[i] = 0;
	}
};

#endif /* CARD_DATA_H_ */
