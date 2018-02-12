/*
 * duel.h
 *
 *  Created on: 2010-4-8
 *      Author: Argon
 */

#ifndef DUEL_H_
#define DUEL_H_

#include "common.h"
#include "mtrandom.h"
#include <set>
#include <unordered_set>

class card;
class group;
class effect;
class field;
class interpreter;

class duel {
public:
	typedef std::set<card*, card_sort> card_set;
	char strbuffer[256];
	byte buffer[0x1000];
	uint32 bufferlen;
	byte* bufferp;
	interpreter* lua;
	field* game_field;
	mtrandom random;
	std::unordered_set<card*> cards;
	std::unordered_set<card*> assumes;
	std::unordered_set<group*> groups;
	std::unordered_set<group*> sgroups;
	std::unordered_set<effect*> effects;
	std::unordered_set<effect*> uncopy;
	
	duel();
	~duel();
	void clear();
	
	card* new_card(uint32 code);
	group* new_group();
	group* new_group(card* pcard);
	group* new_group(const card_set& cset);
	effect* new_effect();
	void delete_card(card* pcard);
	void delete_group(group* pgroup);
	void delete_effect(effect* peffect);
	void release_script_group();
	void restore_assumes();
	int32 read_buffer(byte* buf);
	void write_buffer32(uint32 value);
	void write_buffer16(uint16 value);
	void write_buffer8(uint8 value);
	void clear_buffer();
	void set_responsei(uint32 resp);
	void set_responseb(byte* resp);
	int32 get_next_integer(int32 l, int32 h);
private:
	group* register_group(group* pgroup);
};

//Player
#define PLAYER_NONE		2	//
#define PLAYER_ALL		3	//
//Phase
#define PHASE_DRAW			0x01
#define PHASE_STANDBY		0x02
#define PHASE_MAIN1			0x04
#define PHASE_BATTLE_START	0x08
#define PHASE_BATTLE_STEP	0x10
#define PHASE_DAMAGE		0x20
#define PHASE_DAMAGE_CAL	0x40
#define PHASE_BATTLE		0x80
#define PHASE_MAIN2			0x100
#define PHASE_END			0x200
//Options
#define DUEL_TEST_MODE			0x01
#define DUEL_ATTACK_FIRST_TURN	0x02
//#define DUEL_NO_CHAIN_HINT		0x04
#define DUEL_OBSOLETE_RULING	0x08
#define DUEL_PSEUDO_SHUFFLE		0x10
#define DUEL_TAG_MODE			0x20
#define DUEL_SIMPLE_AI			0x40
#define DUEL_OBSOLETE_IGNITION	0x100
#define DUEL_1ST_TURN_DRAW		0x200
#define DUEL_1_FIELD			0x400
#define DUEL_PZONE				0x800
#define DUEL_EMZONE				0x1000
#define MASTER_RULE_1			0x700
#define MASTER_RULE_2			0x600
#define MASTER_RULE_3			0x800
#define MASTER_RULE_4			0x1800
#endif /* DUEL_H_ */
