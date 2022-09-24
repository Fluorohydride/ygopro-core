/*
 * mtrandom.h
 *
 *  Created on: 2009-10-18
 *      Author: Argon.Sun
 */

#ifndef MTRANDOM_H_
#define MTRANDOM_H_

#include <random>

class mt19937 {
public:
	const unsigned int rand_max;

	mt19937() :
		rng(), rand_max((rng.max)()) {}
	explicit mt19937(unsigned int seed) :
		rng(seed), rand_max((rng.max)()) {}

	// mersenne_twister_engine
	void reset(unsigned int seed) {
		rng.seed(seed);
	}
	unsigned int rand() {
		return rng();
	}

	// uniform_int_distribution
	int get_random_integer(int l, int h) {
		unsigned int range = (unsigned int)(h - l + 1);
		unsigned int secureMax = rand_max - rand_max % range;
		unsigned int x;
		do {
			x = rng();
		} while (x >= secureMax);
		return (int)(l + x % range);
	}
	int get_random_integer_old(int l, int h) {
		int result = (int)((double)rng() / rand_max * ((double)h - l + 1)) + l;
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
