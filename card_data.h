#ifndef CARD_DATA_H_
#define CARD_DATA_H_

#include <cstring>
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

inline bool check_setcode(uint16_t setcode, uint32_t value) {
	const uint32_t settype = value & 0x0fffU;
	const uint32_t setsubtype = value & 0xf000U;
	return setcode && (setcode & 0x0fffU) == settype && (setcode & setsubtype) == setsubtype;
}

inline void write_setcode(uint16_t list[], uint64_t value) {
	if (!list)
		return;
	int len = 0;
	while (value) {
		if (value & 0xffff) {
			list[len] = value & 0xffff;
			++len;
		}
		value >>= 16;
	}
	if (len < SIZE_SETCODE)
		std::memset(list + len, 0, (SIZE_SETCODE - len) * sizeof(uint16_t));
}

inline bool is_alternative(uint32_t code, uint32_t alias) {
	if (code == CARD_BLACK_LUSTER_SOLDIER2)
		return false;
	return alias && (alias < code + CARD_ARTWORK_VERSIONS_OFFSET) && (code < alias + CARD_ARTWORK_VERSIONS_OFFSET);
}

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
		std::memset(this, 0, sizeof(card_data));
	}

	bool is_setcode(uint32_t value) const {
		for (auto& x : setcode) {
			if (!x)
				return false;
			if (check_setcode(x, value))
				return true;
		}
		return false;
	}

	uint32_t get_original_code() const {
		return is_alternative(code, alias) ? alias : code;
	}
};

#endif /* CARD_DATA_H_ */
