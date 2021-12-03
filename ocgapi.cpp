/*
 * interface.cpp
 *
 *  Created on: 2010-5-2
 *      Author: Argon
 */
#include <stdio.h>
#include <string.h>
#include "ocgapi.h"
#include "duel.h"
#include "card.h"
#include "group.h"
#include "effect.h"
#include "field.h"
#include "interpreter.h"
#include <set>

static script_reader sreader = default_script_reader;
static card_reader creader = default_card_reader;
static message_handler mhandler = default_message_handler;
static byte buffer[0x20000];
static std::set<duel*> duel_set;

extern "C" DECL_DLLEXPORT void set_script_reader(script_reader f) {
	sreader = f;
}
extern "C" DECL_DLLEXPORT void set_card_reader(card_reader f) {
	creader = f;
}
extern "C" DECL_DLLEXPORT void set_message_handler(message_handler f) {
	mhandler = f;
}
byte* read_script(const char* script_name, int* len) {
	return sreader(script_name, len);
}
uint32 read_card(uint32 code, card_data* data) {
	return creader(code, data);
}
uint32 handle_message(void* pduel, uint32 msg_type) {
	return mhandler(pduel, msg_type);
}
byte* default_script_reader(const char* script_name, int* slen) {
	FILE *fp;
	fp = fopen(script_name, "rb");
	if (!fp)
		return 0;
	int len = (int)fread(buffer, 1, sizeof(buffer), fp);
	fclose(fp);
	if(len >= sizeof(buffer))
		return 0;
	*slen = len;
	return buffer;
}
uint32 default_card_reader(uint32 code, card_data* data) {
	return 0;
}
uint32 default_message_handler(void* pduel, uint32 message_type) {
	return 0;
}
extern "C" DECL_DLLEXPORT intptr_t create_duel(uint32 seed) {
	duel* pduel = new duel();
	duel_set.insert(pduel);
	pduel->random.reset(seed);
	return (intptr_t)pduel;
}
extern "C" DECL_DLLEXPORT void start_duel(intptr_t pduel, int32 options) {
	duel* pd = (duel*)pduel;
	pd->game_field->core.duel_options |= options & 0xffff;
	int32 duel_rule = options >> 16;
	if(duel_rule)
		pd->game_field->core.duel_rule = duel_rule;
	else if(options & DUEL_OBSOLETE_RULING)		//provide backward compatibility with replay
		pd->game_field->core.duel_rule = 1;
	else if(!pd->game_field->core.duel_rule)
		pd->game_field->core.duel_rule = 5;
	pd->game_field->core.shuffle_hand_check[0] = FALSE;
	pd->game_field->core.shuffle_hand_check[1] = FALSE;
	pd->game_field->core.shuffle_deck_check[0] = FALSE;
	pd->game_field->core.shuffle_deck_check[1] = FALSE;
	if(pd->game_field->player[0].start_count > 0)
		pd->game_field->draw(0, REASON_RULE, PLAYER_NONE, 0, pd->game_field->player[0].start_count);
	if(pd->game_field->player[1].start_count > 0)
		pd->game_field->draw(0, REASON_RULE, PLAYER_NONE, 1, pd->game_field->player[1].start_count);
	if(options & DUEL_TAG_MODE) {
		for(int i = 0; i < pd->game_field->player[0].start_count && pd->game_field->player[0].tag_list_main.size(); ++i) {
			card* pcard = pd->game_field->player[0].tag_list_main.back();
			pd->game_field->player[0].tag_list_main.pop_back();
			pd->game_field->player[0].tag_list_hand.push_back(pcard);
			pcard->current.controler = 0;
			pcard->current.location = LOCATION_HAND;
			pcard->current.sequence = (uint8)pd->game_field->player[0].tag_list_hand.size() - 1;
			pcard->current.position = POS_FACEDOWN;
		}
		for(int i = 0; i < pd->game_field->player[1].start_count && pd->game_field->player[1].tag_list_main.size(); ++i) {
			card* pcard = pd->game_field->player[1].tag_list_main.back();
			pd->game_field->player[1].tag_list_main.pop_back();
			pd->game_field->player[1].tag_list_hand.push_back(pcard);
			pcard->current.controler = 1;
			pcard->current.location = LOCATION_HAND;
			pcard->current.sequence = (uint8)pd->game_field->player[1].tag_list_hand.size() - 1;
			pcard->current.position = POS_FACEDOWN;
		}
	}
	pd->game_field->add_process(PROCESSOR_TURN, 0, 0, 0, 0, 0);
}
extern "C" DECL_DLLEXPORT void end_duel(intptr_t pduel) {
	duel* pd = (duel*)pduel;
	if(duel_set.count(pd)) {
		duel_set.erase(pd);
		delete pd;
	}
}
extern "C" DECL_DLLEXPORT void set_player_info(intptr_t pduel, int32 playerid, int32 lp, int32 startcount, int32 drawcount) {
	duel* pd = (duel*)pduel;
	if(lp > 0)
		pd->game_field->player[playerid].lp = lp;
	if(startcount >= 0)
		pd->game_field->player[playerid].start_count = startcount;
	if(drawcount >= 0)
		pd->game_field->player[playerid].draw_count = drawcount;
}
extern "C" DECL_DLLEXPORT void get_log_message(intptr_t pduel, byte* buf) {
	strcpy((char*)buf, ((duel*)pduel)->strbuffer);
}
extern "C" DECL_DLLEXPORT int32 get_message(intptr_t pduel, byte* buf) {
	int32 len = ((duel*)pduel)->read_buffer(buf);
	((duel*)pduel)->clear_buffer();
	return len;
}
extern "C" DECL_DLLEXPORT int32 process(intptr_t pduel) {
	duel* pd = (duel*)pduel;
	int result = pd->game_field->process();
	while((result & 0xffff) == 0 && (result & 0xf0000) == 0)
		result = pd->game_field->process();
	return result;
}
extern "C" DECL_DLLEXPORT void new_card(intptr_t pduel, uint32 code, uint8 owner, uint8 playerid, uint8 location, uint8 sequence, uint8 position) {
	duel* ptduel = (duel*)pduel;
	if(ptduel->game_field->is_location_useable(playerid, location, sequence)) {
		card* pcard = ptduel->new_card(code);
		pcard->owner = owner;
		ptduel->game_field->add_card(playerid, pcard, location, sequence);
		pcard->current.position = position;
		if(!(location & LOCATION_ONFIELD) || (position & POS_FACEUP)) {
			pcard->enable_field_effect(true);
			ptduel->game_field->adjust_instant();
		} if(location & LOCATION_ONFIELD) {
			if(location == LOCATION_MZONE)
				pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
		}
	}
}
extern "C" DECL_DLLEXPORT void new_tag_card(intptr_t pduel, uint32 code, uint8 owner, uint8 location) {
	duel* ptduel = (duel*)pduel;
	if(owner > 1 || !(location & (LOCATION_DECK | LOCATION_EXTRA)))
		return;
	card* pcard = ptduel->new_card(code);
	switch(location) {
	case LOCATION_DECK:
		ptduel->game_field->player[owner].tag_list_main.push_back(pcard);
		pcard->owner = owner;
		pcard->current.controler = owner;
		pcard->current.location = LOCATION_DECK;
		pcard->current.sequence = (uint8)ptduel->game_field->player[owner].tag_list_main.size() - 1;
		pcard->current.position = POS_FACEDOWN_DEFENSE;
		break;
	case LOCATION_EXTRA:
		ptduel->game_field->player[owner].tag_list_extra.push_back(pcard);
		pcard->owner = owner;
		pcard->current.controler = owner;
		pcard->current.location = LOCATION_EXTRA;
		pcard->current.sequence = (uint8)ptduel->game_field->player[owner].tag_list_extra.size() - 1;
		pcard->current.position = POS_FACEDOWN_DEFENSE;
		break;
	}
}
extern "C" DECL_DLLEXPORT int32 query_card(intptr_t pduel, uint8 playerid, uint8 location, uint8 sequence, int32 query_flag, byte* buf, int32 use_cache) {
	if(playerid != 0 && playerid != 1)
		return 0;
	duel* ptduel = (duel*)pduel;
	card* pcard = 0;
	location &= 0x7f;
	if(location & LOCATION_ONFIELD)
		pcard = ptduel->game_field->get_field_card(playerid, location, sequence);
	else {
		field::card_vector* lst = 0;
		if(location == LOCATION_HAND)
			lst = &ptduel->game_field->player[playerid].list_hand;
		else if(location == LOCATION_GRAVE)
			lst = &ptduel->game_field->player[playerid].list_grave;
		else if(location == LOCATION_REMOVED)
			lst = &ptduel->game_field->player[playerid].list_remove;
		else if(location == LOCATION_EXTRA)
			lst = &ptduel->game_field->player[playerid].list_extra;
		else if(location == LOCATION_DECK)
			lst = &ptduel->game_field->player[playerid].list_main;
		if(!lst || sequence >= lst->size())
			pcard = 0;
		else
			pcard = (*lst)[sequence];
	}
	if (pcard) {
		return pcard->get_infos(buf, query_flag, use_cache);
	}
	else {
		*((int32*)buf) = 4;
		return 4;
	}
}
extern "C" DECL_DLLEXPORT int32 query_field_count(intptr_t pduel, uint8 playerid, uint8 location) {
	duel* ptduel = (duel*)pduel;
	if(playerid != 0 && playerid != 1)
		return 0;
	auto& player = ptduel->game_field->player[playerid];
	if(location == LOCATION_HAND)
		return (int32)player.list_hand.size();
	if(location == LOCATION_GRAVE)
		return (int32)player.list_grave.size();
	if(location == LOCATION_REMOVED)
		return (int32)player.list_remove.size();
	if(location == LOCATION_EXTRA)
		return (int32)player.list_extra.size();
	if(location == LOCATION_DECK)
		return (int32)player.list_main.size();
	if(location == LOCATION_MZONE) {
		uint32 count = 0;
		for(auto& pcard : player.list_mzone)
			if(pcard) count++;
		return count;
	}
	if(location == LOCATION_SZONE) {
		uint32 count = 0;
		for(auto& pcard : player.list_szone)
			if(pcard) count++;
		return count;
	}
	return 0;
}
extern "C" DECL_DLLEXPORT int32 query_field_card(intptr_t pduel, uint8 playerid, uint8 location, int32 query_flag, byte* buf, int32 use_cache) {
	if(playerid != 0 && playerid != 1)
		return 0;
	duel* ptduel = (duel*)pduel;
	auto& player = ptduel->game_field->player[playerid];
	byte* p = buf;
	if(location == LOCATION_MZONE) {
		for(auto& pcard : player.list_mzone) {
			if(pcard) {
				uint32 clen = pcard->get_infos(p, query_flag, use_cache);
				p += clen;
			} else {
				*((int32*)p) = 4;
				p += 4;
			}
		}
	}
	else if(location == LOCATION_SZONE) {
		for(auto& pcard : player.list_szone) {
			if(pcard) {
				uint32 clen = pcard->get_infos(p, query_flag, use_cache);
				p += clen;
			} else {
				*((int32*)p) = 4;
				p += 4;
			}
		}
	}
	else {
		field::card_vector* lst = 0;
		if(location == LOCATION_HAND)
			lst = &player.list_hand;
		else if(location == LOCATION_GRAVE)
			lst = &player.list_grave;
		else if(location == LOCATION_REMOVED)
			lst = &player.list_remove;
		else if(location == LOCATION_EXTRA)
			lst = &player.list_extra;
		else if(location == LOCATION_DECK)
			lst = &player.list_main;
		for(auto& pcard : *lst) {
			uint32 clen = pcard->get_infos(p, query_flag, use_cache);
			p += clen;
		}
	}
	return (int32)(p - buf);
}
extern "C" DECL_DLLEXPORT int32 query_field_info(intptr_t pduel, byte* buf) {
	duel* ptduel = (duel*)pduel;
	byte* p = buf;
	*p++ = MSG_RELOAD_FIELD;
	*p++ = ptduel->game_field->core.duel_rule;
	for(int playerid = 0; playerid < 2; ++playerid) {
		auto& player = ptduel->game_field->player[playerid];
		*((int*)p) = player.lp;
		p += 4;
		for(auto& pcard : player.list_mzone) {
			if(pcard) {
				*p++ = 1;
				*p++ = pcard->current.position;
				*p++ = (uint8)pcard->xyz_materials.size();
			} else {
				*p++ = 0;
			}
		}
		for(auto& pcard : player.list_szone) {
			if(pcard) {
				*p++ = 1;
				*p++ = pcard->current.position;
			} else {
				*p++ = 0;
			}
		}
		*p++ = (uint8)player.list_main.size();
		*p++ = (uint8)player.list_hand.size();
		*p++ = (uint8)player.list_grave.size();
		*p++ = (uint8)player.list_remove.size();
		*p++ = (uint8)player.list_extra.size();
		*p++ = (uint8)player.extra_p_count;
	}
	*p++ = (uint8)ptduel->game_field->core.current_chain.size();
	for(const auto& ch : ptduel->game_field->core.current_chain) {
		effect* peffect = ch.triggering_effect;
		*((int*)p) = peffect->get_handler()->data.code;
		p += 4;
		*((int*)p) = peffect->get_handler()->get_info_location();
		p += 4;
		*p++ = ch.triggering_controler;
		*p++ = (uint8)ch.triggering_location;
		*p++ = ch.triggering_sequence;
		*((int*)p) = peffect->description;
		p += 4;
	}
	return (int32)(p - buf);
}
extern "C" DECL_DLLEXPORT void set_responsei(intptr_t pduel, int32 value) {
	((duel*)pduel)->set_responsei(value);
}
extern "C" DECL_DLLEXPORT void set_responseb(intptr_t pduel, byte* buf) {
	((duel*)pduel)->set_responseb(buf);
}
extern "C" DECL_DLLEXPORT int32 preload_script(intptr_t pduel, const char* script, int32 len) {
	return ((duel*)pduel)->lua->load_script(script);
}
