/*
 * libdebug.cpp
 *
 *  Created on: 2012-2-8
 *      Author: Argon
 */

#include <cstring>
#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "ocgapi.h"

int32_t scriptlib::debug_message(lua_State *L) {
	check_param_count(L, 1);
	duel* pduel = interpreter::get_duel_info(L);
	lua_getglobal(L, "tostring");
	lua_pushvalue(L, -2);
	lua_pcall(L, 1, 1, 0);
	interpreter::sprintf(pduel->strbuffer, "%s", lua_tostring(L, -1));
	handle_message(pduel, 2);
	return 0;
}
int32_t scriptlib::debug_add_card(lua_State *L) {
	check_param_count(L, 6);
	duel* pduel = interpreter::get_duel_info(L);
	int32_t code = (int32_t)lua_tointeger(L, 1);
	int32_t owner = (int32_t)lua_tointeger(L, 2);
	int32_t playerid = (int32_t)lua_tointeger(L, 3);
	int32_t location = (int32_t)lua_tointeger(L, 4);
	int32_t sequence = (int32_t)lua_tointeger(L, 5);
	int32_t position = (int32_t)lua_tointeger(L, 6);
	int32_t proc = lua_toboolean(L, 7);
	if (!check_playerid(owner))
		return 0;
	if (!check_playerid(playerid))
		return 0;
	if(pduel->game_field->is_location_useable(playerid, location, sequence)) {
		card* pcard = pduel->new_card(code);
		pcard->owner = owner;
		if(location == LOCATION_EXTRA && position == 0)
			position = POS_FACEDOWN_DEFENSE;
		pcard->sendto_param.position = position;
		if(location == LOCATION_PZONE) {
			int32_t seq = pduel->game_field->get_pzone_sequence(sequence);
			pduel->game_field->add_card(playerid, pcard, LOCATION_SZONE, seq, TRUE);
		} else {
			pduel->game_field->add_card(playerid, pcard, location, sequence);
		}
		pcard->current.position = position;
		if(!(location & (LOCATION_ONFIELD | LOCATION_PZONE)) || (position & POS_FACEUP)) {
			pcard->enable_field_effect(true);
			pduel->game_field->adjust_instant();
		}
		if(proc)
			pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
		interpreter::card2value(L, pcard);
		return 1;
	} else if(location == LOCATION_MZONE) {
		card* fcard = pduel->game_field->get_field_card(playerid, location, sequence);
		if (!fcard || !(fcard->data.type & TYPE_XYZ))
			return 0;
		card* pcard = pduel->new_card(code);
		pcard->owner = owner;
		fcard->xyz_add(pcard);
		if(proc)
			pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
		interpreter::card2value(L, pcard);
		return 1;
	}
	return 0;
}
int32_t scriptlib::debug_set_player_info(lua_State *L) {
	check_param_count(L, 4);
	duel* pduel = interpreter::get_duel_info(L);
	int32_t playerid = (int32_t)lua_tointeger(L, 1);
	int32_t lp = (int32_t)lua_tointeger(L, 2);
	int32_t startcount = (int32_t)lua_tointeger(L, 3);
	int32_t drawcount = (int32_t)lua_tointeger(L, 4);
	if(playerid != 0 && playerid != 1)
		return 0;
	pduel->game_field->player[playerid].lp = lp;
	pduel->game_field->player[playerid].start_count = startcount;
	pduel->game_field->player[playerid].draw_count = drawcount;
	return 0;
}
int32_t scriptlib::debug_pre_summon(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t summon_type = (uint32_t)lua_tointeger(L, 2);
	uint8_t summon_location = 0;
	if(lua_gettop(L) > 2)
		summon_location = (uint8_t)lua_tointeger(L, 3);
	pcard->summon_info = summon_type | (summon_location << 16);
	return 0;
}
int32_t scriptlib::debug_pre_equip(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* equip_card = *(card**) lua_touserdata(L, 1);
	card* target = *(card**) lua_touserdata(L, 2);
	if((equip_card->current.location != LOCATION_SZONE)
	        || (target->current.location != LOCATION_MZONE)
	        || (target->current.position & POS_FACEDOWN))
		lua_pushboolean(L, 0);
	else {
		equip_card->equip(target, FALSE);
		equip_card->effect_target_cards.insert(target);
		target->effect_target_owner.insert(equip_card);
		lua_pushboolean(L, 1);
	}
	return 1;
}
int32_t scriptlib::debug_pre_set_target(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* t_card = *(card**) lua_touserdata(L, 1);
	card* target = *(card**) lua_touserdata(L, 2);
	t_card->add_card_target(target);
	return 0;
}
int32_t scriptlib::debug_pre_add_counter(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t countertype = (uint32_t)lua_tointeger(L, 2);
	uint16_t count = (uint16_t)lua_tointeger(L, 3);
	uint16_t cttype = countertype;
	auto pr = pcard->counters.emplace(cttype, 0);
	auto cmit = pr.first;
	cmit->second += count;
	return 0;
}
int32_t scriptlib::debug_reload_field_begin(lua_State *L) {
	check_param_count(L, 1);
	duel* pduel = interpreter::get_duel_info(L);
	uint32_t flag = (uint32_t)lua_tointeger(L, 1);
	int32_t rule = (int32_t)lua_tointeger(L, 2);
	pduel->clear();
	pduel->game_field->core.duel_options |= flag;
	if (rule)
		pduel->game_field->core.duel_rule = rule;
	else if (flag & DUEL_OBSOLETE_RULING)
		pduel->game_field->core.duel_rule = 1;
	else
		pduel->game_field->core.duel_rule = CURRENT_RULE;
	return 0;
}
int32_t scriptlib::debug_reload_field_end(lua_State *L) {
	duel* pduel = interpreter::get_duel_info(L);
	pduel->game_field->core.shuffle_hand_check[0] = FALSE;
	pduel->game_field->core.shuffle_hand_check[1] = FALSE;
	pduel->game_field->core.shuffle_deck_check[0] = FALSE;
	pduel->game_field->core.shuffle_deck_check[1] = FALSE;
	pduel->game_field->reload_field_info();
	return 0;
}
int32_t scriptlib::debug_set_ai_name(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_STRING, 1);
	duel* pduel = interpreter::get_duel_info(L);
	pduel->write_buffer8(MSG_AI_NAME);
	const char* pstr = lua_tostring(L, 1);
	int len = (int)std::strlen(pstr);
	if(len > SIZE_AI_NAME -1)
		len = SIZE_AI_NAME - 1;
	pduel->write_buffer16(len);
	pduel->write_buffer(pstr, len);
	pduel->write_buffer8(0);
	return 0;
}
int32_t scriptlib::debug_show_hint(lua_State *L) {
	check_param_count(L, 1);
	duel* pduel = interpreter::get_duel_info(L);
	lua_getglobal(L, "tostring");
	lua_pushvalue(L, -2);
	lua_pcall(L, 1, 1, 0);
	pduel->write_buffer8(MSG_SHOW_HINT);
	const char* pstr = lua_tostring(L, -1);
	int len = (int)std::strlen(pstr);
	if (len > SIZE_HINT_MSG - 1)
		len = SIZE_HINT_MSG - 1;
	pduel->write_buffer16(len);
	pduel->write_buffer(pstr, len);
	pduel->write_buffer8(0);
	return 0;
}

static const struct luaL_Reg debuglib[] = {
	{ "Message", scriptlib::debug_message },
	{ "AddCard", scriptlib::debug_add_card },
	{ "SetPlayerInfo", scriptlib::debug_set_player_info },
	{ "PreSummon", scriptlib::debug_pre_summon },
	{ "PreEquip", scriptlib::debug_pre_equip },
	{ "PreSetTarget", scriptlib::debug_pre_set_target },
	{ "PreAddCounter", scriptlib::debug_pre_add_counter },
	{ "ReloadFieldBegin", scriptlib::debug_reload_field_begin },
	{ "ReloadFieldEnd", scriptlib::debug_reload_field_end },
	{ "SetAIName", scriptlib::debug_set_ai_name },
	{ "ShowHint", scriptlib::debug_show_hint },
	{ nullptr, nullptr }
};
void scriptlib::open_debuglib(lua_State *L) {
	luaL_newlib(L, debuglib);
	lua_setglobal(L, "Debug");
}
