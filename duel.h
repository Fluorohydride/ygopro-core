/*
 * duel.h
 *
 *  Created on: 2010-4-8
 *      Author: Argon
 */

#ifndef DUEL_H_
#define DUEL_H_

#include "common.h"
#include "sort.h"
#include "mtrandom.h"
#include <set>
#include <unordered_set>
#include <vector>

class card;
class group;
class effect;
class field;
class interpreter;

using card_set = std::set<card*, card_sort>;

class duel {
public:
	char strbuffer[256]{};
	int32_t rng_version{ 2 };
	interpreter* lua;
	field* game_field;
	mt19937 random;

	std::vector<byte> message_buffer;
	std::unordered_set<card*> cards;
	std::unordered_set<card*> assumes;
	std::unordered_set<group*> groups;
	std::unordered_set<group*> sgroups;
	std::unordered_set<effect*> effects;
	std::unordered_set<effect*> uncopy;
	
	duel();
	~duel();
	void clear();
	
	uint32_t buffer_size() const {
		return (uint32_t)message_buffer.size() & PROCESSOR_BUFFER_LEN;
	}
	card* new_card(uint32_t code);
	group* new_group();
	group* new_group(card* pcard);
	group* new_group(const card_set& cset);
	effect* new_effect();
	void delete_card(card* pcard);
	void delete_group(group* pgroup);
	void delete_effect(effect* peffect);
	void release_script_group();
	void restore_assumes();
	int32_t read_buffer(byte* buf);
	void write_buffer(const void* data, int size);
	void write_buffer32(uint32_t value);
	void write_buffer16(uint16_t value);
	void write_buffer8(uint8_t value);
	void clear_buffer();
	void set_responsei(uint32_t resp);
	void set_responseb(byte* resp);
	int32_t get_next_integer(int32_t l, int32_t h);
private:
	group* register_group(group* pgroup);
};

#endif /* DUEL_H_ */
