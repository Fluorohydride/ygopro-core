/*
 * mtrandom.h
 *
 *  Created on: 2009-10-18
 *      Author: Argon.Sun
 */

#ifndef MTRANDOM_H_
#define MTRANDOM_H_

#include <random>
#include <vector>
#include <utility>

class mt19937 {
public:
	const unsigned int rand_max;

	mt19937() :
		rng(), rand_max(rng.max()) {}
	explicit mt19937(uint_fast32_t seed) :
		rng(seed), rand_max(rng.max()) {}

	// mersenne_twister_engine
	void reset(uint_fast32_t seed) {
		rng.seed(seed);
	}
	uint_fast32_t rand() {
		return rng();
	}

	// uniform_int_distribution
	int get_random_integer(int l, int h) {
		uint_fast32_t range = (uint_fast32_t)(h - l + 1);
		uint_fast32_t secureMax = rng.max() - rng.max() % range;
		uint_fast32_t x;
		do {
			x = rng();
		} while (x >= secureMax);
		return l + (int)(x % range);
	}
	int get_random_integer_old(int l, int h) {
		int result = (int)((double)rng() / rng.max() * ((double)h - l + 1)) + l;
		if (result > h)
			result = h;
		return result;
	}
	
	// Fisher-Yates shuffle v[a]~v[b]
	template<typename T>
	void shuffle_vector(std::vector<T>& v, int a = -1, int b = -1) {
		if (a < 0)
			a = 0;
		if (b < 0)
			b = (int)v.size() - 1;
		for (int i = a; i < b; ++i) {
			int r = get_random_integer(i, b);
			std::swap(v[i], v[r]);
		}
	}
	template<typename T>
	void shuffle_vector_old(std::vector<T>& v, int a = -1, int b = -1) {
		if (a < 0)
			a = 0;
		if (b < 0)
			b = (int)v.size() - 1;
		for (int i = a; i < b; ++i) {
			int r = get_random_integer_old(i, b);
			std::swap(v[i], v[r]);
		}
	}

private:
	std::mt19937 rng;
};


#endif /* MTRANDOM_H_ */
