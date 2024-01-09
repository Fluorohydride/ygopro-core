#ifndef CARD_DATA_H_
#define CARD_DATA_H_

#include "common.h"
#include <vector>

struct card_data {
	uint32 code{};
	uint32 alias{};
	std::vector<uint16_t> setcode;
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
		setcode.clear();
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
		}
		return false;
	}

	void set_setcode(uint64 value) {
		setcode.clear();
		while (value) {
			if (value & 0xffff)
				setcode.push_back(value & 0xffff);
			value >>= 16;
		}
	}
};

#endif /* CARD_DATA_H_ */
