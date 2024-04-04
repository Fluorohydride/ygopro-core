/*
 * group.cpp
 *
 *  Created on: 2010-8-3
 *      Author: Argon
 */

#include "group.h"
#include "card.h"
#include "duel.h"

group::group(duel* pd) {
	pduel = pd;
	it = container.begin();
}
group::group(duel* pd, card* pcard) {
	container.insert(pcard);
	pduel = pd;
	it = container.begin();
}
group::group(duel* pd, const card_set& cset): container(cset) {
	pduel = pd;
	it = container.begin();
}
