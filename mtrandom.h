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
	const unsigned int rand_max{ std::mt19937::max() };

	mt19937() :
		rng() {}
	explicit mt19937(uint_fast32_t value) :
		rng(value) {}
	mt19937(uint32_t seq[], size_t len) {
		std::seed_seq q(seq, seq + len);
		rng.seed(q);
	}
	
	mt19937(const mt19937& other) = delete;
	void operator=(const mt19937& other) = delete;

	// mersenne_twister_engine
	void seed(uint32_t seq[], size_t len) {
		std::seed_seq q(seq, seq + len);
		rng.seed(q);
	}
	void seed(uint_fast32_t value) {
		rng.seed(value);
	}
	uint_fast32_t rand() {
		return rng();
	}
	void discard(unsigned long long z) {
		rng.discard(z);
	}

	// old vesion, discard too many numbers
	int get_random_integer_v1(int l, int h) {
		uint32_t range = (h - l + 1);
		uint32_t secureMax = rand_max - rand_max % range;
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
