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

class mtrandom {
public:
	const unsigned int rand_max{ std::mt19937::max() };

	mtrandom() :
		rng() {}
	mtrandom(uint32_t seq[], size_t len) {
		std::seed_seq q(seq, seq + len);
		rng.seed(q);
	}
	explicit mtrandom(uint_fast32_t value) :
		rng(value) {}
	mtrandom(std::seed_seq& q) :
		rng(q) {}
	
	mtrandom(const mtrandom& other) = delete;
	void operator=(const mtrandom& other) = delete;

	// mersenne_twister_engine
	void seed(uint32_t seq[], size_t len) {
		std::seed_seq q(seq, seq + len);
		rng.seed(q);
	}
	void seed(uint_fast32_t value) {
		rng.seed(value);
	}
	void seed(std::seed_seq& q) {
		rng.seed(q);
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

	// N % k = (N - k) % k = (-k) % k
	// discard (N % range) numbers from the left end so that it is a multiple of range
#pragma warning(disable:4146)
	int get_random_integer_v2(int l, int h) {
		uint32_t range = (h - l + 1);
		uint32_t bound = -range % range;
		auto x = rng();
		while (x < bound) {
			x = rng();
		}
		return l + (int)(x % range);
	}
#pragma warning(default:4146)

	// Fisher-Yates shuffle [first, last)
	template<typename T>
	void shuffle_vector(std::vector<T>& v, int first = 0, int last = INT32_MAX, int version = 2) {
		if ((size_t)last > v.size())
			last = (int)v.size();
		auto distribution = &mtrandom::get_random_integer_v2;
		if (version == 1)
			distribution = &mtrandom::get_random_integer_v1;
		for (int i = first; i < last - 1; ++i) {
			int r = (this->*distribution)(i, last - 1);
			std::swap(v[i], v[r]);
		}
	}

private:
	std::mt19937 rng;
};


#endif /* MTRANDOM_H_ */
