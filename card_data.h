#ifndef CARD_DATA_H_
#define CARD_DATA_H_

#include <unordered_map>
#include "common.h"

constexpr int CARD_ARTWORK_VERSIONS_OFFSET = 20;
constexpr int SIZE_SETCODE = 16;
constexpr uint32_t CARD_BLACK_LUSTER_SOLDIER2 = 5405695;

//double name
constexpr uint32_t CARD_MARINE_DOLPHIN = 78734254;
constexpr uint32_t CARD_TWINKLE_MOSS = 13857930;
constexpr uint32_t CARD_TIMAEUS = 1784686;
constexpr uint32_t CARD_CRITIAS = 11082056;
constexpr uint32_t CARD_HERMOS = 46232525;

const std::unordered_map<uint32_t, uint32_t> second_code = {
	{CARD_MARINE_DOLPHIN, 17955766u},
	{CARD_TWINKLE_MOSS, 17732278u},
	{CARD_TIMAEUS, 10000050u},
	{CARD_CRITIAS, 10000060u},
	{CARD_HERMOS, 10000070u},
};

struct card_data {
	uint32_t code{};
	uint32_t alias{};
	uint16_t setcode[SIZE_SETCODE]{};
	uint32_t type{};
	uint32_t level{};
	uint32_t attribute{};
	uint32_t race{};
	int32_t attack{};
	int32_t defense{};
	uint32_t lscale{};
	uint32_t rscale{};
	uint32_t link_marker{};

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

	bool is_setcode(uint32_t value) const {
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

	void set_setcode(uint64_t value) {
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

	uint32_t get_original_code() const {
		return is_alternative() ? alias : code;
	}
};

#endif /* CARD_DATA_H_ */
