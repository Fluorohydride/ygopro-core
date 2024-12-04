/*
 * group.h
 *
 *  Created on: 2010-5-6
 *      Author: Argon
 */

#ifndef GROUP_H_
#define GROUP_H_

#include "common.h"
#include "sort.h"
#include <set>
#include <list>

class card;
class duel;

using card_set = std::set<card*, card_sort>;

constexpr int GTYPE_DEFAULT = 0;
constexpr int GTYPE_READ_ONLY = 1;
constexpr int GTYPE_KEEP_ALIVE = 2;

class group {
public:
	duel* pduel;
	card_set container;
	card_set::iterator it;
	int32_t ref_handle{ 0 };
	uint32_t is_readonly{ GTYPE_DEFAULT };
	bool is_iterator_dirty{ true };
	
	bool has_card(card* c) {
		return container.find(c) != container.end();
	}
	
	explicit group(duel* pd);
	group(duel* pd, card* pcard);
	group(duel* pd, const card_set& cset);
	~group() = default;
};

#endif /* GROUP_H_ */
