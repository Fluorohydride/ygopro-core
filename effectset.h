/*
 * effectset.h
 *
 *  Created on: 2011-10-8
 *      Author: Argon
 */

#ifndef EFFECTSET_H_
#define EFFECTSET_H_

#include <array>
#include <vector>
#include <algorithm>

class effect;

bool effect_sort_id(const effect* e1, const effect* e2);

struct effect_set {
	void add_item(effect* peffect) {
		if (count >= 64)
			return;
		container[count++] = peffect;
	}
	void remove_item(int index) {
		if (index < 0 || index >= count)
			return;
		if(index == count - 1) {
			--count;
			return;
		}
		for(int i = index; i < count - 1; ++i)
			container[i] = container[i + 1];
		--count;
	}
	void clear() {
		count = 0;
	}
	int size() const {
		return count;
	}
	void sort() {
		if(count < 2)
			return;
		std::sort(container.begin(), container.begin() + count, effect_sort_id);
	}
	effect* const& get_last() const {
		assert(count);
		return container[count - 1];
	}
	effect*& get_last() {
		assert(count);
		return container[count - 1];
	}
	effect* const& operator[] (int index) const {
		return container[index];
	}
	effect*& operator[] (int index) {
		return container[index];
	}
	effect* const& at(int index) const {
		return container[index];
	}
	effect*& at(int index) {
		return container[index];
	}
private:
	std::array<effect*, 64> container{ nullptr };
	int count{ 0 };
};

struct effect_set_v {
	void add_item(effect* peffect) {
		container.push_back(peffect);
	}
	void remove_item(int index) {
		if (index < 0 || index >= (int)container.size())
			return;
		container.erase(container.begin() + index);
	}
	void clear() {
		container.clear();
	}
	int size() const {
		return (int)container.size();
	}
	void sort() {
		int count = (int)container.size();
		if(count < 2)
			return;
		std::sort(container.begin(), container.begin() + count, effect_sort_id);
	}
	effect* const& get_last() const {
		assert(container.size());
		return container.back();
	}
	effect*& get_last() {
		assert(container.size());
		return container.back();
	}
	effect* const& operator[] (int index) const {
		return container[index];
	}
	effect*& operator[] (int index) {
		return container[index];
	}
	effect* const& at(int index) const {
		return container[index];
	}
	effect*& at(int index) {
		return container[index];
	}
private:
	std::vector<effect*> container;
};

#endif //EFFECTSET_H_
