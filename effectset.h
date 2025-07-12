/*
 * effectset.h
 *
 *  Created on: 2011-10-8
 *      Author: Argon
 */

#ifndef EFFECTSET_H_
#define EFFECTSET_H_

#include <vector>
#include <algorithm>

class effect;

bool effect_sort_id(const effect* e1, const effect* e2);

using effect_set = std::vector<effect*>;
using effect_set_v = effect_set;

#endif //EFFECTSET_H_
