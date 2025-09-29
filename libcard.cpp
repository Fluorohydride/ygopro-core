/*
 * libcard.cpp
 *
 *  Created on: 2010-5-6
 *      Author: Argon
 */

#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"

int32_t scriptlib::card_get_code(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_code());
	uint32_t otcode = pcard->get_another_code();
	if(otcode) {
		lua_pushinteger(L, otcode);
		return 2;
	}
	return 1;
}
int32_t scriptlib::card_get_origin_code(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_original_code());
	return 1;
}
int32_t scriptlib::card_get_origin_code_rule(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	auto codes = pcard->get_original_code_rule();
	uint32_t code1 = std::get<0>(codes);
	uint32_t code2 = std::get<1>(codes);
	if (code2) {
		lua_pushinteger(L, code1);
		lua_pushinteger(L, code2);
		return 2;
	}
	else {
		lua_pushinteger(L, code1);
		return 1;
	}
}
int32_t scriptlib::card_get_fusion_code(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_code());
	int32_t count = 1;
	uint32_t otcode = pcard->get_another_code();
	if(otcode) {
		lua_pushinteger(L, otcode);
		++count;
	}
	if(pcard->pduel->game_field->core.not_material)
		return count;
	effect_set eset;
	pcard->filter_effect(EFFECT_ADD_FUSION_CODE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i)
		lua_pushinteger(L, eset[i]->get_value(pcard));
	return count + (int32_t)eset.size();
}
int32_t scriptlib::card_get_link_code(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_code());
	int32_t count = 1;
	uint32_t otcode = pcard->get_another_code();
	if(otcode) {
		lua_pushinteger(L, otcode);
		++count;
	}
	effect_set eset;
	pcard->filter_effect(EFFECT_ADD_LINK_CODE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i)
		lua_pushinteger(L, eset[i]->get_value(pcard));
	return count + (int32_t)eset.size();
}
int32_t scriptlib::card_is_fusion_code(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	effect_set eset;
	pcard->filter_effect(EFFECT_ADD_FUSION_CODE, &eset);
	if(!eset.size() || pcard->pduel->game_field->core.not_material)
		return card_is_code(L);
	uint32_t code1 = pcard->get_code();
	uint32_t code2 = pcard->get_another_code();
	std::unordered_set<uint32_t> fcode;
	fcode.insert(code1);
	if(code2)
		fcode.insert(code2);
	for(effect_set::size_type i = 0; i < eset.size(); ++i)
		fcode.insert(eset[i]->get_value(pcard));
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t tcode = (uint32_t)lua_tointeger(L, i + 2);
		if(fcode.find(tcode) != fcode.end()) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_link_code(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	effect_set eset;
	pcard->filter_effect(EFFECT_ADD_LINK_CODE, &eset);
	if(!eset.size())
		return card_is_code(L);
	uint32_t code1 = pcard->get_code();
	uint32_t code2 = pcard->get_another_code();
	std::unordered_set<uint32_t> fcode;
	fcode.insert(code1);
	if(code2)
		fcode.insert(code2);
	for(effect_set::size_type i = 0; i < eset.size(); ++i)
		fcode.insert(eset[i]->get_value(pcard));
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t tcode = (uint32_t)lua_tointeger(L, i + 2);
		if(fcode.find(tcode) != fcode.end()) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_set_card(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t set_code = (uint32_t)lua_tointeger(L, i + 2);
		if(pcard->is_set_card(set_code)) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_origin_set_card(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t set_code = (uint32_t)lua_tointeger(L, i + 2);
		if(pcard->is_origin_set_card(set_code)) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_pre_set_card(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t set_code = (uint32_t)lua_tointeger(L, i + 2);
		if(pcard->is_pre_set_card(set_code)) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_fusion_set_card(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t set_code = (uint32_t)lua_tointeger(L, i + 2);
		if(pcard->is_fusion_set_card(set_code)) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_link_set_card(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t set_code = (uint32_t)lua_tointeger(L, i + 2);
		if(pcard->is_link_set_card(set_code)) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_special_summon_set_card(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t set_code = (uint32_t)lua_tointeger(L, i + 2);
		if(pcard->is_special_summon_set_card(set_code)) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_get_type(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_type());
	return 1;
}
int32_t scriptlib::card_get_origin_type(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->data.type);
	return 1;
}
int32_t scriptlib::card_get_fusion_type(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_fusion_type());
	return 1;
}
int32_t scriptlib::card_get_synchro_type(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_synchro_type());
	return 1;
}
int32_t scriptlib::card_get_xyz_type(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_xyz_type());
	return 1;
}
int32_t scriptlib::card_get_link_type(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_link_type());
	return 1;
}
int32_t scriptlib::card_get_level(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_level());
	return 1;
}
int32_t scriptlib::card_get_rank(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_rank());
	return 1;
}
int32_t scriptlib::card_get_link(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_link());
	return 1;
}
int32_t scriptlib::card_get_synchro_level(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* scard = *(card**) lua_touserdata(L, 2);
	lua_pushinteger(L, pcard->get_synchro_level(scard));
	return 1;
}
int32_t scriptlib::card_get_ritual_level(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* scard = *(card**) lua_touserdata(L, 2);
	lua_pushinteger(L, pcard->get_ritual_level(scard));
	return 1;
}
int32_t scriptlib::card_get_origin_level(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if((pcard->data.type & (TYPE_XYZ | TYPE_LINK)) || (pcard->status & STATUS_NO_LEVEL))
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.level);
	return 1;
}
int32_t scriptlib::card_get_origin_rank(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(!(pcard->data.type & TYPE_XYZ))
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.level);
	return 1;
}
int32_t scriptlib::card_is_xyz_level(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* xyzcard = *(card**) lua_touserdata(L, 2);
	uint32_t lv = (uint32_t)lua_tointeger(L, 3);
	lua_pushboolean(L, pcard->check_xyz_level(xyzcard, lv));
	return 1;
}
int32_t scriptlib::card_get_lscale(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_lscale());
	return 1;
}
int32_t scriptlib::card_get_origin_lscale(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->data.lscale);
	return 1;
}
int32_t scriptlib::card_get_rscale(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_rscale());
	return 1;
}
int32_t scriptlib::card_get_origin_rscale(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->data.rscale);
	return 1;
}
int32_t scriptlib::card_get_current_scale(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	if (pcard->current.pzone) {
		if (pcard->current.sequence == pcard->pduel->game_field->get_pzone_sequence(0))
			lua_pushinteger(L, pcard->get_lscale());
		else
			lua_pushinteger(L, pcard->get_rscale());
	}
	else
		lua_pushinteger(L, pcard->data.lscale);
	return 1;
}
int32_t scriptlib::card_is_link_marker(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t dir = (uint32_t)lua_tointeger(L, 2);
	lua_pushboolean(L, pcard->is_link_marker(dir));
	return 1;
}
int32_t scriptlib::card_get_linked_group(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card_set cset;
	pcard->get_linked_cards(&cset);
	group* pgroup = pcard->pduel->new_group(cset);
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::card_get_linked_group_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card_set cset;
	pcard->get_linked_cards(&cset);
	lua_pushinteger(L, cset.size());
	return 1;
}
int32_t scriptlib::card_get_linked_zone(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t zone = pcard->get_linked_zone();
	int32_t cp = pcard->current.controler;
	if(lua_gettop(L) >= 2 && !lua_isnil(L, 2))
		cp = (int32_t)lua_tointeger(L, 2);
	if(cp == 1 - pcard->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
int32_t scriptlib::card_get_mutual_linked_group(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	card_set cset;
	pcard->get_mutual_linked_cards(&cset);
	group* pgroup = pcard->pduel->new_group(cset);
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::card_get_mutual_linked_group_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	card_set cset;
	pcard->get_mutual_linked_cards(&cset);
	lua_pushinteger(L, cset.size());
	return 1;
}
int32_t scriptlib::card_get_mutual_linked_zone(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t zone = pcard->get_mutual_linked_zone();
	int32_t cp = pcard->current.controler;
	if(lua_gettop(L) >= 2 && !lua_isnil(L, 2))
		cp = (int32_t)lua_tointeger(L, 2);
	if(cp == 1 - pcard->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
int32_t scriptlib::card_is_link_state(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_link_state());
	return 1;
}
int32_t scriptlib::card_is_extra_link_state(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_extra_link_state());
	return 1;
}
int32_t scriptlib::card_get_column_group(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card_set cset;
	pcard->get_column_cards(&cset);
	group* pgroup = pcard->pduel->new_group(cset);
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::card_get_column_group_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card_set cset;
	pcard->get_column_cards(&cset);
	lua_pushinteger(L, cset.size());
	return 1;
}
int32_t scriptlib::card_get_column_zone(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t loc = (int32_t)lua_tointeger(L, 2);
	int32_t cp = pcard->current.controler;
	if(lua_gettop(L) >= 3 && !lua_isnil(L, 3))
		cp = (int32_t)lua_tointeger(L, 3);
	uint32_t zone = pcard->get_column_zone(loc);
	if(cp == 1 - pcard->current.controler)
		lua_pushinteger(L, (((zone & 0xffff) << 16) | (zone >> 16)));
	else
		lua_pushinteger(L, zone);
	return 1;
}
int32_t scriptlib::card_is_all_column(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_all_column());
	return 1;
}
int32_t scriptlib::card_get_attribute(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_attribute());
	return 1;
}
int32_t scriptlib::card_get_origin_attribute(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.attribute);
	return 1;
}
int32_t scriptlib::card_get_fusion_attribute(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	int32_t playerid = PLAYER_NONE;
	if(lua_gettop(L) > 1 && !lua_isnil(L, 2))
		playerid = (int32_t)lua_tointeger(L, 2);
	else
		playerid = pcard->pduel->game_field->core.reason_player;
	lua_pushinteger(L, pcard->get_fusion_attribute(playerid));
	return 1;
}
int32_t scriptlib::card_get_link_attribute(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	int32_t playerid = PLAYER_NONE;
	if(lua_gettop(L) > 1 && !lua_isnil(L, 2))
		playerid = (int32_t)lua_tointeger(L, 2);
	else
		playerid = pcard->pduel->game_field->core.reason_player;
	lua_pushinteger(L, pcard->get_link_attribute(playerid));
	return 1;
}
int32_t scriptlib::card_get_attribute_in_grave(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	int32_t playerid = PLAYER_NONE;
	if(lua_gettop(L) > 1 && !lua_isnil(L, 2))
		playerid = (int32_t)lua_tointeger(L, 2);
	else
		playerid = pcard->pduel->game_field->core.reason_player;
	lua_pushinteger(L, pcard->get_grave_attribute(playerid));
	return 1;
}
int32_t scriptlib::card_get_race(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_race());
	return 1;
}
int32_t scriptlib::card_get_origin_race(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.race);
	return 1;
}
int32_t scriptlib::card_get_link_race(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	int32_t playerid = PLAYER_NONE;
	if(lua_gettop(L) > 1 && !lua_isnil(L, 2))
		playerid = (int32_t)lua_tointeger(L, 2);
	else
		playerid = pcard->pduel->game_field->core.reason_player;
	lua_pushinteger(L, pcard->get_link_race(playerid));
	return 1;
}
int32_t scriptlib::card_get_race_in_grave(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	int32_t playerid = PLAYER_NONE;
	if(lua_gettop(L) > 1 && !lua_isnil(L, 2))
		playerid = (int32_t)lua_tointeger(L, 2);
	else
		playerid = pcard->pduel->game_field->core.reason_player;
	lua_pushinteger(L, pcard->get_grave_race(playerid));
	return 1;
}
int32_t scriptlib::card_get_attack(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t atk = pcard->get_attack();
	if(atk < 0)
		atk = 0;
	lua_pushinteger(L, atk);
	return 1;
}
int32_t scriptlib::card_get_origin_attack(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t atk = pcard->get_base_attack();
	if(atk < 0)
		atk = 0;
	lua_pushinteger(L, atk);
	return 1;
}
int32_t scriptlib::card_get_text_attack(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.attack);
	return 1;
}
int32_t scriptlib::card_get_defense(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t def = pcard->get_defense();
	if(def < 0)
		def = 0;
	lua_pushinteger(L, def);
	return 1;
}
int32_t scriptlib::card_get_origin_defense(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t def = pcard->get_base_defense();
	if(def < 0)
		def = 0;
	lua_pushinteger(L, def);
	return 1;
}
int32_t scriptlib::card_get_text_defense(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->status & STATUS_NO_LEVEL)
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->data.defense);
	return 1;
}
int32_t scriptlib::card_get_previous_code_onfield(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.code);
	if(pcard->previous.code2) {
		lua_pushinteger(L, pcard->previous.code2);
		return 2;
	}
	return 1;
}
int32_t scriptlib::card_get_previous_type_onfield(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.type);
	return 1;
}
int32_t scriptlib::card_get_previous_level_onfield(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.level);
	return 1;
}
int32_t scriptlib::card_get_previous_rank_onfield(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.rank);
	return 1;
}
int32_t scriptlib::card_get_previous_attribute_onfield(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.attribute);
	return 1;
}
int32_t scriptlib::card_get_previous_race_onfield(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.race);
	return 1;
}
int32_t scriptlib::card_get_previous_attack_onfield(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.attack);
	return 1;
}
int32_t scriptlib::card_get_previous_defense_onfield(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.defense);
	return 1;
}
int32_t scriptlib::card_get_previous_overlay_count_onfield(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->xyz_materials_previous_count_onfield);
	return 1;
}
int32_t scriptlib::card_get_owner(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->owner);
	return 1;
}
int32_t scriptlib::card_get_controler(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->current.controler);
	return 1;
}
int32_t scriptlib::card_get_previous_controler(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.controler);
	return 1;
}
int32_t scriptlib::card_set_reason(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t reason = (uint32_t)lua_tointeger(L, 2);
	pcard->current.reason = reason;
	return 0;
}
int32_t scriptlib::card_get_reason(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->current.reason);
	return 1;
}
int32_t scriptlib::card_get_reason_card(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	interpreter::card2value(L, pcard->current.reason_card);
	return 1;
}
int32_t scriptlib::card_get_reason_player(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->current.reason_player);
	return 1;
}
int32_t scriptlib::card_get_reason_effect(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	interpreter::effect2value(L, pcard->current.reason_effect);
	return 1;
}
int32_t scriptlib::card_get_position(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->current.position);
	return 1;
}
int32_t scriptlib::card_get_previous_position(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.position);
	return 1;
}
int32_t scriptlib::card_get_battle_position(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->temp.position);
	return 1;
}
int32_t scriptlib::card_get_location(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->is_treated_as_not_on_field())
		lua_pushinteger(L, 0);
	else
		lua_pushinteger(L, pcard->current.location);
	return 1;
}
int32_t scriptlib::card_get_previous_location(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.location);
	return 1;
}
int32_t scriptlib::card_get_sequence(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->current.sequence);
	return 1;
}
int32_t scriptlib::card_get_previous_sequence(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->previous.sequence);
	return 1;
}
int32_t scriptlib::card_get_summon_type(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->summon_info & DEFAULT_SUMMON_TYPE);
	return 1;
}
int32_t scriptlib::card_get_summon_location(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, (pcard->summon_info >> 16) & 0xff);
	return 1;
}
int32_t scriptlib::card_get_summon_player(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->summon_player);
	return 1;
}
int32_t scriptlib::card_get_special_summon_info(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t args = lua_gettop(L) - 1;
	for(uint32_t i = 0; i < args; ++i) {
		uint32_t flag = (uint32_t)lua_tointeger(L, 2 + i);
		switch(flag) {
			case SUMMON_INFO_CODE:
				lua_pushinteger(L, pcard->spsummon.code);
				break;
			case SUMMON_INFO_CODE2:
				lua_pushinteger(L, pcard->spsummon.code2);
				break;
			case SUMMON_INFO_TYPE:
				lua_pushinteger(L, pcard->spsummon.type);
				break;
			case SUMMON_INFO_LEVEL:
				lua_pushinteger(L, pcard->spsummon.level);
				break;
			case SUMMON_INFO_RANK:
				lua_pushinteger(L, pcard->spsummon.rank);
				break;
			case SUMMON_INFO_ATTRIBUTE:
				lua_pushinteger(L, pcard->spsummon.attribute);
				break;
			case SUMMON_INFO_RACE:
				lua_pushinteger(L, pcard->spsummon.race);
				break;
			case SUMMON_INFO_ATTACK:
				lua_pushinteger(L, pcard->spsummon.attack);
				break;
			case SUMMON_INFO_DEFENSE:
				lua_pushinteger(L, pcard->spsummon.defense);
				break;
			case SUMMON_INFO_REASON_EFFECT:
				interpreter::effect2value(L, pcard->spsummon.reason_effect);
				break;
			case SUMMON_INFO_REASON_PLAYER:
				lua_pushinteger(L, pcard->spsummon.reason_player);
				break;
			default:
				lua_pushnil(L);
				break;
		}
	}
	return args;
}
int32_t scriptlib::card_get_destination(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->sendto_param.location);
	return 1;
}
int32_t scriptlib::card_get_leave_field_dest(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->leave_field_redirect(REASON_EFFECT));
	return 1;
}
int32_t scriptlib::card_get_turnid(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->turnid);
	return 1;
}
int32_t scriptlib::card_get_fieldid(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->fieldid);
	return 1;
}
int32_t scriptlib::card_get_fieldidr(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->fieldid_r);
	return 1;
}
int32_t scriptlib::card_is_origin_code_rule(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	auto codes = pcard->get_original_code_rule();
	uint32_t code1 = std::get<0>(codes);
	uint32_t code2 = std::get<1>(codes);
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t tcode = (uint32_t)lua_tointeger(L, i + 2);
		if(code1 == tcode || (code2 && code2 == tcode)) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_code(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t code1 = pcard->get_code();
	uint32_t code2 = pcard->get_another_code();
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t tcode = (uint32_t)lua_tointeger(L, i + 2);
		if(code1 == tcode || (code2 && code2 == tcode)) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t ttype = (uint32_t)lua_tointeger(L, 2);
	if(pcard->get_type() & ttype)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_all_types(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t ttype = (uint32_t)lua_tointeger(L, 2);
	if((pcard->get_type() & ttype) == ttype)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_fusion_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t ttype = (uint32_t)lua_tointeger(L, 2);
	if(pcard->get_fusion_type() & ttype)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_synchro_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t ttype = (uint32_t)lua_tointeger(L, 2);
	if(pcard->get_synchro_type() & ttype)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_xyz_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t ttype = (uint32_t)lua_tointeger(L, 2);
	if(pcard->get_xyz_type() & ttype)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_link_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t ttype = (uint32_t)lua_tointeger(L, 2);
	if(pcard->get_link_type() & ttype)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_level(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t level = pcard->get_level();
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t tlevel = (uint32_t)lua_tointeger(L, i + 2);
		if(level == tlevel) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_rank(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t rank = pcard->get_rank();
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t trank = (uint32_t)lua_tointeger(L, i + 2);
		if(rank == trank) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_link(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t link = pcard->get_link();
	uint32_t count = lua_gettop(L) - 1;
	uint32_t result = FALSE;
	for(uint32_t i = 0; i < count; ++i) {
		if(lua_isnil(L, i + 2))
			continue;
		uint32_t tlink = (uint32_t)lua_tointeger(L, i + 2);
		if(link == tlink) {
			result = TRUE;
			break;
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::card_is_attack(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER))
		lua_pushboolean(L, 0);
	else {
		uint32_t atk = pcard->get_attack();
		uint32_t count = lua_gettop(L) - 1;
		uint32_t result = FALSE;
		for(uint32_t i = 0; i < count; ++i) {
			if(lua_isnil(L, i + 2))
				continue;
			uint32_t tatk = (uint32_t)lua_tointeger(L, i + 2);
			if(atk == tatk) {
				result = TRUE;
				break;
			}
		}
		lua_pushboolean(L, result);
	}
	return 1;
}
int32_t scriptlib::card_is_defense(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if((pcard->data.type & TYPE_LINK)
		|| (!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER)))
		lua_pushboolean(L, 0);
	else {
		uint32_t def = pcard->get_defense();
		uint32_t count = lua_gettop(L) - 1;
		uint32_t result = FALSE;
		for(uint32_t i = 0; i < count; ++i) {
			if(lua_isnil(L, i + 2))
				continue;
			uint32_t tdef = (uint32_t)lua_tointeger(L, i + 2);
			if(def == tdef) {
				result = TRUE;
				break;
			}
		}
		lua_pushboolean(L, result);
	}
	return 1;
}
int32_t scriptlib::card_is_race(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t trace = (uint32_t)lua_tointeger(L, 2);
	if(pcard->get_race() & trace)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_link_race(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t trace = (uint32_t)lua_tointeger(L, 2);
	int32_t playerid = PLAYER_NONE;
	if(lua_gettop(L) > 2 && !lua_isnil(L, 3))
		playerid = (int32_t)lua_tointeger(L, 3);
	else
		playerid = pcard->pduel->game_field->core.reason_player;
	if(pcard->get_link_race(playerid) & trace)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_attribute(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t tattrib = (uint32_t)lua_tointeger(L, 2);
	if(pcard->get_attribute() & tattrib)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_fusion_attribute(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t tattrib = (uint32_t)lua_tointeger(L, 2);
	int32_t playerid = PLAYER_NONE;
	if(lua_gettop(L) > 2 && !lua_isnil(L, 3))
		playerid = (int32_t)lua_tointeger(L, 3);
	else
		playerid = pcard->pduel->game_field->core.reason_player;
	if(pcard->get_fusion_attribute(playerid) & tattrib)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_link_attribute(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t tattrib = (uint32_t)lua_tointeger(L, 2);
	int32_t playerid = PLAYER_NONE;
	if(lua_gettop(L) > 2 && !lua_isnil(L, 3))
		playerid = (int32_t)lua_tointeger(L, 3);
	else
		playerid = pcard->pduel->game_field->core.reason_player;
	if(pcard->get_link_attribute(playerid) & tattrib)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_non_attribute(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t tattrib = (uint32_t)lua_tointeger(L, 2);
	if(pcard->get_attribute() & (ATTRIBUTE_ALL & ~tattrib))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_extra_deck_monster(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_extra_deck_monster());
	return 1;
}
int32_t scriptlib::card_is_reason(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t treason = (uint32_t)lua_tointeger(L, 2);
	if(pcard->current.reason & treason)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_all_reasons(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t treason = (uint32_t)lua_tointeger(L, 2);
	if((pcard->current.reason & treason) == treason)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_summon_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t ttype = (uint32_t)lua_tointeger(L, 2);
	if(((pcard->summon_info & DEFAULT_SUMMON_TYPE) & ttype) == ttype)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_summon_location(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t tloc = (uint32_t)lua_tointeger(L, 2);
	if(((pcard->summon_info >> 16) & 0xff) & tloc)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_summon_player(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t con = (uint32_t)lua_tointeger(L, 2);
	if(pcard->summon_player == con)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_status(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t tstatus = (uint32_t)lua_tointeger(L, 2);
	if(pcard->status & tstatus)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_not_tuner(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* scard = *(card**) lua_touserdata(L, 2);
	lua_pushboolean(L, pcard->is_not_tuner(scard));
	return 1;
}
int32_t scriptlib::card_is_tuner(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**)lua_touserdata(L, 1);
	card* scard = *(card**)lua_touserdata(L, 2);
	lua_pushboolean(L, pcard->is_tuner(scard));
	return 1;
}
int32_t scriptlib::card_is_original_effect_property(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	card* pcard = *(card**)lua_touserdata(L, 1);
	int32_t filter = interpreter::get_function_handle(L, 2);
	lua_pushboolean(L, pcard->is_original_effect_property(filter));
	return 1;
}
int32_t scriptlib::card_is_effect_property(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	card* pcard = *(card**)lua_touserdata(L, 1);
	int32_t filter = interpreter::get_function_handle(L, 2);
	lua_pushboolean(L, pcard->is_effect_property(filter));
	return 1;
}
int32_t scriptlib::card_set_status(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->status & STATUS_COPYING_EFFECT)
		return 0;
	uint32_t tstatus = (uint32_t)lua_tointeger(L, 2);
	int32_t enable = lua_toboolean(L, 3);
	pcard->set_status(tstatus, enable);
	return 0;
}
int32_t scriptlib::card_is_dual_state(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t ret = 0;
	if(pcard->is_affected_by_effect(EFFECT_DUAL_STATUS))
		ret = 1;
	else
		ret = 0;
	lua_pushboolean(L, ret);
	return 1;
}
int32_t scriptlib::card_enable_dual_state(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	duel* pduel = pcard->pduel;
	effect* deffect = pduel->new_effect();
	deffect->owner = pcard;
	deffect->code = EFFECT_DUAL_STATUS;
	deffect->type = EFFECT_TYPE_SINGLE;
	deffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_CLIENT_HINT;
	deffect->description = 64;
	deffect->reset_flag = RESET_EVENT + 0x1fe0000;
	pcard->add_effect(deffect);
	return 0;
}
int32_t scriptlib::card_set_turn_counter(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t ct = (int32_t)lua_tointeger(L, 2);
	pcard->count_turn(ct);
	return 0;
}
int32_t scriptlib::card_get_turn_counter(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->turn_counter);
	return 1;
}
int32_t scriptlib::card_set_material(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(!lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_GROUP, 2);
		group* pgroup = *(group**) lua_touserdata(L, 2);
		pcard->set_material(&pgroup->container);
	} else
		pcard->set_material(0);
	return 0;
}
int32_t scriptlib::card_get_material(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	group* pgroup = pcard->pduel->new_group(pcard->material_cards);
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::card_get_material_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->material_cards.size());
	return 1;
}
int32_t scriptlib::card_get_equip_group(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	group* pgroup = pcard->pduel->new_group(pcard->equiping_cards);
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::card_get_equip_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->equiping_cards.size());
	return 1;
}
int32_t scriptlib::card_get_equip_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	interpreter::card2value(L, pcard->equiping_target);
	return 1;
}
int32_t scriptlib::card_get_pre_equip_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	interpreter::card2value(L, pcard->pre_equip_target);
	return 1;
}
int32_t scriptlib::card_check_equip_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* target = *(card**) lua_touserdata(L, 2);
	if(pcard->is_affected_by_effect(EFFECT_EQUIP_LIMIT, target)
		&& ((!pcard->is_affected_by_effect(EFFECT_OLDUNION_STATUS) || target->get_union_count() == 0)
			&& (!pcard->is_affected_by_effect(EFFECT_UNION_STATUS) || target->get_old_union_count() == 0)))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_check_union_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* target = *(card**) lua_touserdata(L, 2);
	if(pcard->is_affected_by_effect(EFFECT_UNION_LIMIT, target)
		&& ((!pcard->is_affected_by_effect(EFFECT_OLDUNION_STATUS) || target->get_union_count() == 0)
			&& (!pcard->is_affected_by_effect(EFFECT_UNION_STATUS) || target->get_old_union_count() == 0)))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_get_union_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->get_union_count());
	lua_pushinteger(L, pcard->get_old_union_count());
	return 2;
}
int32_t scriptlib::card_get_overlay_group(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	group* pgroup = pcard->pduel->new_group();
	pgroup->container.insert(pcard->xyz_materials.begin(), pcard->xyz_materials.end());
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::card_get_overlay_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->xyz_materials.size());
	return 1;
}
int32_t scriptlib::card_get_overlay_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	interpreter::card2value(L, pcard->overlay_target);
	return 1;
}
int32_t scriptlib::card_check_remove_overlay_card(lua_State *L) {
	check_param_count(L, 4);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	int32_t count = (int32_t)lua_tointeger(L, 3);
	int32_t reason = (int32_t)lua_tointeger(L, 4);
	duel* pduel = pcard->pduel;
	lua_pushboolean(L, pduel->game_field->is_player_can_remove_overlay_card(playerid, pcard, 0, 0, count, reason));
	return 1;
}
int32_t scriptlib::card_remove_overlay_card(lua_State *L) {
	check_action_permission(L);
	check_param_count(L, 5);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	int32_t min = (int32_t)lua_tointeger(L, 3);
	int32_t max = (int32_t)lua_tointeger(L, 4);
	int32_t reason = (int32_t)lua_tointeger(L, 5);
	duel* pduel = pcard->pduel;
	pduel->game_field->remove_overlay_card(reason, pcard, playerid, 0, 0, min, max);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State *L, int32_t status, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		lua_pushinteger(L, pduel->game_field->returns.ivalue[0]);
		return 1;
	});
}
int32_t scriptlib::card_get_attacked_group(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	group* pgroup = pcard->pduel->new_group();
	for(auto& cit : pcard->attacked_cards) {
		if(cit.second.first)
			pgroup->container.insert(cit.second.first);
	}
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::card_get_attacked_group_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->attacked_cards.size());
	return 1;
}
int32_t scriptlib::card_get_attacked_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->attacked_count);
	return 1;
}
int32_t scriptlib::card_get_battled_group(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	group* pgroup = pcard->pduel->new_group();
	for(auto& cit : pcard->battled_cards) {
		if(cit.second.first)
			pgroup->container.insert(cit.second.first);
	}
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::card_get_battled_group_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->battled_cards.size());
	return 1;
}
int32_t scriptlib::card_get_attack_announced_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->attack_announce_count);
	return 1;
}
int32_t scriptlib::card_is_direct_attacked(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->attacked_cards.findcard(0))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_set_card_target(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* ocard = *(card**) lua_touserdata(L, 2);
	pcard->add_card_target(ocard);
	return 0;
}
int32_t scriptlib::card_get_card_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	group* pgroup = pcard->pduel->new_group(pcard->effect_target_cards);
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::card_get_first_card_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->effect_target_cards.size())
		interpreter::card2value(L, *pcard->effect_target_cards.begin());
	else lua_pushnil(L);
	return 1;
}
int32_t scriptlib::card_get_card_target_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->effect_target_cards.size());
	return 1;
}
int32_t scriptlib::card_is_has_card_target(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* rcard = *(card**) lua_touserdata(L, 2);
	lua_pushboolean(L, (int32_t)pcard->effect_target_cards.count(rcard));
	return 1;
}
int32_t scriptlib::card_cancel_card_target(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* rcard = *(card**) lua_touserdata(L, 2);
	pcard->cancel_card_target(rcard);
	return 0;
}
int32_t scriptlib::card_get_owner_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	group* pgroup = pcard->pduel->new_group(pcard->effect_target_owner);
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::card_get_owner_target_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushinteger(L, pcard->effect_target_owner.size());
	return 1;
}
int32_t scriptlib::card_get_activate_effect(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t count = 0;
	for(auto& eit : pcard->field_effect) {
		if(eit.second->type & EFFECT_TYPE_ACTIVATE) {
			interpreter::effect2value(L, eit.second);
			++count;
		}
	}
	return count;
}
int32_t scriptlib::card_check_activate_effect(lua_State *L) {
	check_param_count(L, 4);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t neglect_con = lua_toboolean(L, 2);
	int32_t neglect_cost = lua_toboolean(L, 3);
	int32_t copy_info = lua_toboolean(L, 4);
	duel* pduel = pcard->pduel;
	tevent pe;
	for(auto eit = pcard->field_effect.begin(); eit != pcard->field_effect.end();) {
		effect* peffect = eit->second;
		++eit;
		if((peffect->type & EFFECT_TYPE_ACTIVATE)
		        && pduel->game_field->check_event_c(peffect, pduel->game_field->core.reason_player, neglect_con, neglect_cost, copy_info, &pe)) {
			if(!copy_info || (peffect->code == EVENT_FREE_CHAIN)) {
				interpreter::effect2value(L, peffect);
				return 1;
			} else {
				interpreter::effect2value(L, peffect);
				interpreter::group2value(L, pe.event_cards);
				lua_pushinteger(L, pe.event_player);
				lua_pushinteger(L, pe.event_value);
				interpreter::effect2value(L, pe.reason_effect);
				lua_pushinteger(L, pe.reason);
				lua_pushinteger(L, pe.reason_player);
				return 7;
			}
		}
	}
	return 0;
}
int32_t scriptlib::card_get_tuner_limit(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	effect* peffect = pcard->is_affected_by_effect(EFFECT_TUNER_MATERIAL_LIMIT);
	if(peffect) {
		interpreter::effect2value(L, peffect);
		if(peffect->target)
			interpreter::function2value(L, peffect->target);
		else
			lua_pushnil(L);
		if(peffect->value)
			lua_pushinteger(L, peffect->value);
		else
			lua_pushnil(L);
		uint16_t min = 0;
		uint16_t max = 99;
		if(peffect->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
			if(peffect->s_range && peffect->s_range > min)
				min = peffect->s_range;
			if(peffect->o_range && peffect->o_range < max)
				max = peffect->o_range;
		}
		lua_pushinteger(L, min);
		lua_pushinteger(L, max);
		return 5;
	}
	return 0;
}
int32_t scriptlib::card_get_hand_synchro(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	if(!(pcard->current.location & LOCATION_MZONE))
		return 0;
	effect* peffect = pcard->is_affected_by_effect(EFFECT_HAND_SYNCHRO);
	if(peffect) {
		interpreter::effect2value(L, peffect);
		if(peffect->target)
			interpreter::function2value(L, peffect->target);
		else
			lua_pushnil(L);
		uint16_t min = 0;
		uint16_t max = 99;
		if(peffect->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
			if(peffect->s_range && peffect->s_range > min)
				min = peffect->s_range;
			if(peffect->o_range && peffect->o_range < max)
				max = peffect->o_range;
		}
		lua_pushinteger(L, min);
		lua_pushinteger(L, max);
		return 4;
	}
	peffect = pcard->is_affected_by_effect(EFFECT_TUNER_MATERIAL_LIMIT);
	if(peffect && (peffect->value & LOCATION_HAND)) {
		interpreter::effect2value(L, peffect);
		if(peffect->target)
			interpreter::function2value(L, peffect->target);
		else
			lua_pushnil(L);
		uint16_t min = 0;
		uint16_t max = 99;
		if(peffect->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
			if(peffect->s_range && peffect->s_range > min)
				min = peffect->s_range;
			if(peffect->o_range && peffect->o_range < max)
				max = peffect->o_range;
		}
		lua_pushinteger(L, min);
		lua_pushinteger(L, max);
		return 4;
	}
	return 0;
}
int32_t scriptlib::card_register_effect(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_EFFECT, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 2);
	int32_t forced = lua_toboolean(L, 3);
	duel* pduel = pcard->pduel;
	if(peffect->owner == pduel->game_field->temp_card)
		return 0;
	if(!forced && pduel->game_field->core.reason_effect && !pcard->is_affect_by_effect(pduel->game_field->core.reason_effect)) {
		pduel->game_field->core.reseted_effects.insert(peffect);
		return 0;
	}
	for (auto& entry : category_checklist) {
		if (peffect->category & entry.first)
			peffect->flag[0] |= entry.second;
	}
	int32_t id;
	if (peffect->handler)
		id = -1;
	else
		id = pcard->add_effect(peffect);
	lua_pushinteger(L, id);
	return 1;
}
int32_t scriptlib::card_is_has_effect(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t code = (uint32_t)lua_tointeger(L, 2);
	if(!pcard) {
		lua_pushnil(L);
		return 1;
	}
	effect_set eset;
	pcard->filter_effect(code, &eset);
	int32_t check_player = PLAYER_NONE;
	if(lua_gettop(L) >= 3) {
		check_player = (int32_t)lua_tointeger(L, 3);
		if(check_player > PLAYER_NONE)
			check_player = PLAYER_NONE;
	}
	int32_t size = 0;
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(check_player == PLAYER_NONE || eset[i]->check_count_limit(check_player)) {
			interpreter::effect2value(L, eset[i]);
			++size;
		}
	}
	if(!size) {
		lua_pushnil(L);
		return 1;
	}
	return size;
}
int32_t scriptlib::card_reset_effect(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t code = (uint32_t)lua_tointeger(L, 2);
	uint32_t type = (uint32_t)lua_tointeger(L, 3);
	pcard->reset(code, type);
	return 0;
}
int32_t scriptlib::card_get_effect_count(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t code = (uint32_t)lua_tointeger(L, 2);
	effect_set eset;
	pcard->filter_effect(code, &eset);
	lua_pushinteger(L, eset.size());
	return 1;
}
int32_t scriptlib::card_register_flag_effect(lua_State *L) {
	check_param_count(L, 5);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t code = (lua_tointeger(L, 2) & MAX_CARD_ID) | EFFECT_FLAG_EFFECT;
	int32_t reset = (int32_t)lua_tointeger(L, 3);
	uint64_t flag = lua_tointeger(L, 4);
	int32_t count = (int32_t)lua_tointeger(L, 5);
	lua_Integer lab = 0;
	int32_t desc = 0;
	if(lua_gettop(L) >= 6)
		lab = lua_tointeger(L, 6);
	if(lua_gettop(L) >= 7)
		desc = (int32_t)lua_tointeger(L, 7);
	if(count == 0)
		count = 1;
	if(reset & (RESET_PHASE) && !(reset & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	duel* pduel = pcard->pduel;
	effect* peffect = pduel->new_effect();
	peffect->effect_owner = pduel->game_field->core.reason_player;
	peffect->owner = pcard;
	peffect->handler = 0;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = code;
	peffect->reset_flag = reset;
	peffect->flag[0] = flag | EFFECT_FLAG_CANNOT_DISABLE;
	peffect->reset_count = count;
	peffect->label.push_back(lab);
	peffect->description = desc;
	pcard->add_effect(peffect);
	interpreter::effect2value(L, peffect);
	return 1;
}
int32_t scriptlib::card_get_flag_effect(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t code = (lua_tointeger(L, 2) & MAX_CARD_ID) | EFFECT_FLAG_EFFECT;
	lua_pushinteger(L, pcard->single_effect.count(code));
	return 1;
}
int32_t scriptlib::card_reset_flag_effect(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t code = (lua_tointeger(L, 2) & MAX_CARD_ID) | EFFECT_FLAG_EFFECT;
	pcard->reset(code, RESET_CODE);
	return 0;
}
int32_t scriptlib::card_set_flag_effect_label(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t code = (lua_tointeger(L, 2) & MAX_CARD_ID) | EFFECT_FLAG_EFFECT;
	auto lab = lua_tointeger(L, 3);
	auto eit = pcard->single_effect.find(code);
	if(eit == pcard->single_effect.end())
		lua_pushboolean(L, FALSE);
	else {
		eit->second->label.clear();
		eit->second->label.push_back(lab);
		lua_pushboolean(L, TRUE);
	}
	return 1;
}
int32_t scriptlib::card_get_flag_effect_label(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t code = (lua_tointeger(L, 2) & MAX_CARD_ID) | EFFECT_FLAG_EFFECT;
	auto rg = pcard->single_effect.equal_range(code);
	int32_t count = 0;
	for(; rg.first != rg.second; ++rg.first, ++count)
		lua_pushinteger(L, rg.first->second->label.size() ? rg.first->second->label[0] : 0);
	if(!count) {
		lua_pushnil(L);
		return 1;
	}
	return count;
}
int32_t scriptlib::card_create_relation(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* rcard = *(card**) lua_touserdata(L, 2);
	uint32_t reset = (uint32_t)lua_tointeger(L, 3);
	pcard->create_relation(rcard, reset);
	return 0;
}
int32_t scriptlib::card_release_relation(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* rcard = *(card**) lua_touserdata(L, 2);
	pcard->release_relation(rcard);
	return 0;
}
int32_t scriptlib::card_create_effect_relation(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_EFFECT, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 2);
	pcard->create_relation(peffect);
	return 0;
}
int32_t scriptlib::card_release_effect_relation(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_EFFECT, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 2);
	pcard->release_relation(peffect);
	return 0;
}
int32_t scriptlib::card_clear_effect_relation(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	pcard->clear_relate_effect();
	return 0;
}
int32_t scriptlib::card_is_relate_to_effect(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_EFFECT, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 2);
	if(pcard && pcard->is_has_relation(peffect))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_relate_to_chain(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t chain_count = 0;
	if(lua_gettop(L) >= 2)
		chain_count = (uint32_t)lua_tointeger(L, 2);
	duel* pduel = pcard->pduel;
	if(chain_count > pduel->game_field->core.current_chain.size() || chain_count < 1)
		chain_count = (uint32_t)pduel->game_field->core.current_chain.size();
	if(pcard && pcard->is_has_relation(pduel->game_field->core.current_chain[chain_count - 1]))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_relate_to_card(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* rcard = *(card**) lua_touserdata(L, 2);
	if(pcard && pcard->is_has_relation(rcard))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_relate_to_battle(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	duel* pduel = pcard->pduel;
	if(pcard->fieldid_r == pduel->game_field->core.pre_field[0] || pcard->fieldid_r == pduel->game_field->core.pre_field[1])
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_copy_effect(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t code = (uint32_t)lua_tointeger(L, 2);
	uint32_t reset = (uint32_t)lua_tointeger(L, 3);
	int32_t count = (int32_t)lua_tointeger(L, 4);
	if(count == 0)
		count = 1;
	if(reset & RESET_PHASE && !(reset & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	lua_pushinteger(L, pcard->copy_effect(code, reset, count));
	return 1;
}
int32_t scriptlib::card_replace_effect(lua_State * L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t code = (uint32_t)lua_tointeger(L, 2);
	uint32_t reset = (uint32_t)lua_tointeger(L, 3);
	int32_t count = (int32_t)lua_tointeger(L, 4);
	if(count == 0)
		count = 1;
	if(reset & RESET_PHASE && !(reset & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		reset |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	lua_pushinteger(L, pcard->replace_effect(code, reset, count));
	return 1;
}
int32_t scriptlib::card_enable_revive_limit(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	duel* pduel = pcard->pduel;
	if(!pcard->is_status(STATUS_COPYING_EFFECT)) {
		effect* peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->code = EFFECT_REVIVE_LIMIT;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_UNCOPYABLE;
		pcard->add_effect(peffect);
	}
	return 0;
}
int32_t scriptlib::card_complete_procedure(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
	return 0;
}
int32_t scriptlib::card_is_disabled(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_status(STATUS_DISABLED));
	return 1;
}
int32_t scriptlib::card_is_destructable(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	effect* peffect = nullptr;
	if(lua_gettop(L) > 1) {
		check_param(L, PARAM_TYPE_EFFECT, 2);
		peffect = *(effect**) lua_touserdata(L, 2);
	}
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(peffect)
		lua_pushboolean(L, pcard->is_destructable_by_effect(peffect, pcard->pduel->game_field->core.reason_player));
	else
		lua_pushboolean(L, pcard->is_destructable());
	return 1;
}
int32_t scriptlib::card_is_summonable(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_summonable_card());
	return 1;
}
int32_t scriptlib::card_is_special_summonable_card(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_spsummonable_card());
	return 1;
}
int32_t scriptlib::card_is_fusion_summonable_card(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t summon_type = 0;
	if(lua_gettop(L) > 1)
		summon_type = (uint32_t)lua_tointeger(L, 2);
	lua_pushboolean(L, pcard->is_fusion_summonable_card(summon_type));
	return 1;
}
int32_t scriptlib::card_is_msetable(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->pduel->game_field->core.effect_damage_step) {
		lua_pushboolean(L, FALSE);
		return 1;
	}
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	uint32_t ign = lua_toboolean(L, 2);
	effect* peffect = nullptr;
	if(!lua_isnil(L, 3)) {
		check_param(L, PARAM_TYPE_EFFECT, 3);
		peffect = *(effect**)lua_touserdata(L, 3);
	}
	uint32_t minc = 0;
	if(lua_gettop(L) >= 4)
		minc = (uint32_t)lua_tointeger(L, 4);
	uint32_t zone = 0x1f;
	if(lua_gettop(L) >= 5)
		zone = (uint32_t)lua_tointeger(L, 5);
	lua_pushboolean(L, pcard->is_setable_mzone(p, ign, peffect, minc, zone));
	return 1;
}
int32_t scriptlib::card_is_ssetable(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	uint32_t ign = FALSE;
	if(lua_gettop(L) >= 2)
		ign = lua_toboolean(L, 2);
	lua_pushboolean(L, pcard->is_setable_szone(p, ign));
	return 1;
}
int32_t scriptlib::card_is_special_summonable(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t sumtype = 0;
	if(lua_gettop(L) >= 2)
		sumtype = (uint32_t)lua_tointeger(L, 2);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	lua_pushboolean(L, pcard->is_special_summonable(p, sumtype));
	return 1;
}
int32_t scriptlib::card_is_synchro_summonable(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(!(pcard->data.type & TYPE_SYNCHRO) || pcard->pduel->game_field->core.effect_damage_step) {
		lua_pushboolean(L, FALSE);
		return 1;
	}
	card* tuner = nullptr;
	group* mg = nullptr;
	if(!lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_CARD, 2);
		tuner = *(card**) lua_touserdata(L, 2);
	}
	if(lua_gettop(L) >= 3) {
		if(!lua_isnil(L, 3)) {
			check_param(L, PARAM_TYPE_GROUP, 3);
			mg = *(group**) lua_touserdata(L, 3);
		}
	}
	int32_t minc = 0;
	if(lua_gettop(L) >= 4)
		minc = (int32_t)lua_tointeger(L, 4);
	int32_t maxc = 0;
	if(lua_gettop(L) >= 5)
		maxc = (int32_t)lua_tointeger(L, 5);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	material_info info;
	info.limit_tuner = tuner;
	info.limit_syn = mg;
	info.limit_syn_minc = minc;
	info.limit_syn_maxc = maxc;
	int32_t res = pcard->is_special_summonable(p, SUMMON_TYPE_SYNCHRO, info);
	lua_pushboolean(L, res);
	return 1;
}
int32_t scriptlib::card_is_xyz_summonable(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(!(pcard->data.type & TYPE_XYZ) || pcard->pduel->game_field->core.effect_damage_step) {
		lua_pushboolean(L, FALSE);
		return 1;
	}
	group* materials = nullptr;
	if(!lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_GROUP, 2);
		materials = *(group**) lua_touserdata(L, 2);
	}
	int32_t minc = 0;
	if(lua_gettop(L) >= 3)
		minc = (int32_t)lua_tointeger(L, 3);
	int32_t maxc = 0;
	if(lua_gettop(L) >= 4)
		maxc = (int32_t)lua_tointeger(L, 4);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	material_info info;
	info.limit_xyz = materials;
	info.limit_xyz_minc = minc;
	info.limit_xyz_maxc = maxc;
	int32_t res = pcard->is_special_summonable(p, SUMMON_TYPE_XYZ, info);
	lua_pushboolean(L, res);
	return 1;
}
int32_t scriptlib::card_is_link_summonable(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	if(!(pcard->data.type & TYPE_LINK) || pcard->pduel->game_field->core.effect_damage_step) {
		lua_pushboolean(L, FALSE);
		return 1;
	}
	group* materials = nullptr;
	card* lcard = nullptr;
	if(!lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_GROUP, 2);
		materials = *(group**)lua_touserdata(L, 2);
	}
	if(lua_gettop(L) >= 3) {
		if(!lua_isnil(L, 3)) {
			check_param(L, PARAM_TYPE_CARD, 3);
			lcard = *(card**)lua_touserdata(L, 3);
		}
	}
	int32_t minc = 0;
	if(lua_gettop(L) >= 4)
		minc = (int32_t)lua_tointeger(L, 4);
	int32_t maxc = 0;
	if(lua_gettop(L) >= 5)
		maxc = (int32_t)lua_tointeger(L, 5);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	material_info info;
	info.limit_link = materials;
	info.limit_link_card = lcard;
	info.limit_link_minc = minc;
	info.limit_link_maxc = maxc;
	int32_t res = pcard->is_special_summonable(p, SUMMON_TYPE_LINK, info);
	lua_pushboolean(L, res);
	return 1;
}
int32_t scriptlib::card_is_can_be_summoned(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->pduel->game_field->core.effect_damage_step) {
		lua_pushboolean(L, FALSE);
		return 1;
	}
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	uint32_t ign = lua_toboolean(L, 2);
	effect* peffect = nullptr;
	if(!lua_isnil(L, 3)) {
		check_param(L, PARAM_TYPE_EFFECT, 3);
		peffect = *(effect**)lua_touserdata(L, 3);
	}
	uint32_t minc = 0;
	if(lua_gettop(L) >= 4)
		minc = (uint32_t)lua_tointeger(L, 4);
	uint32_t zone = 0x1f;
	if(lua_gettop(L) >= 5)
		zone = (uint32_t)lua_tointeger(L, 5);
	lua_pushboolean(L, pcard->is_can_be_summoned(p, ign, peffect, minc, zone));
	return 1;
}
int32_t scriptlib::card_is_can_be_special_summoned(lua_State *L) {
	check_param_count(L, 6);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_EFFECT, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 2);
	uint32_t sumtype = (uint32_t)lua_tointeger(L, 3);
	uint32_t sumplayer = (uint32_t)lua_tointeger(L, 4);
	uint32_t nocheck = lua_toboolean(L, 5);
	uint32_t nolimit = lua_toboolean(L, 6);
	uint32_t sumpos = POS_FACEUP;
	uint32_t toplayer = sumplayer;
	uint32_t zone = 0xff;
	if(lua_gettop(L) >= 7)
		sumpos = (uint32_t)lua_tointeger(L, 7);
	if(lua_gettop(L) >= 8)
		toplayer = (uint32_t)lua_tointeger(L, 8);
	if(lua_gettop(L) >= 9)
		zone = (uint32_t)lua_tointeger(L, 9);
	if(pcard->is_can_be_special_summoned(peffect, sumtype, sumpos, sumplayer, toplayer, nocheck, nolimit, zone))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_can_be_placed_on_field(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t toplayer = pcard->pduel->game_field->core.reason_player;
	if(lua_gettop(L) >= 2)
		toplayer = (uint32_t)lua_tointeger(L, 2);
	if(toplayer != 0 && toplayer != 1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	uint32_t tolocation = LOCATION_SZONE;
	if(lua_gettop(L) >= 3)
		tolocation = (uint32_t)lua_tointeger(L, 3);
	if(pcard->is_status(STATUS_FORBIDDEN)
		|| pcard->pduel->game_field->check_unique_onfield(pcard, toplayer, tolocation, nullptr))
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, 1);
	return 1;
}
int32_t scriptlib::card_is_able_to_hand(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	if(lua_gettop(L) >= 2)
		p = (uint32_t)lua_tointeger(L, 2);
	if(pcard->is_capable_send_to_hand(p))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_able_to_grave(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	if(pcard->is_capable_send_to_grave(p))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_able_to_deck(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	if(pcard->is_capable_send_to_deck(p))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_able_to_extra(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	if(pcard->is_capable_send_to_extra(p))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_able_to_remove(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	if(lua_gettop(L) >= 2)
		p = (uint32_t)lua_tointeger(L, 2);
	uint8_t pos = POS_FACEUP;
	if(lua_gettop(L) >= 3)
		pos = (uint8_t)lua_tointeger(L, 3);
	uint32_t reason = REASON_EFFECT;
	if(lua_gettop(L) >= 4)
		reason = (uint32_t)lua_tointeger(L, 4);
	if(pcard->is_removeable(p, pos, reason))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_able_to_hand_as_cost(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	if(pcard->is_capable_cost_to_hand(p))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_able_to_grave_as_cost(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	if(pcard->is_capable_cost_to_grave(p))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_able_to_deck_as_cost(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	if(pcard->is_capable_cost_to_deck(p))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_able_to_extra_as_cost(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	if(pcard->is_capable_cost_to_extra(p))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_able_to_deck_or_extra_as_cost(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	int32_t val = pcard->is_extra_deck_monster() ? pcard->is_capable_cost_to_extra(p) : pcard->is_capable_cost_to_deck(p);
	if(val)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_able_to_remove_as_cost(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	uint8_t pos = POS_FACEUP;
	if(lua_gettop(L) >= 2)
		pos = (uint8_t)lua_tointeger(L, 2);
	if(pcard->is_removeable_as_cost(p, pos))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_releasable(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	uint32_t reason = REASON_COST;
	if (lua_gettop(L) >= 2)
		reason = (uint32_t)lua_tointeger(L, 2);
	if(pcard->is_releasable_by_nonsummon(p, reason))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_releasable_by_effect(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	effect* re = pcard->pduel->game_field->core.reason_effect;
	if(pcard->is_releasable_by_nonsummon(p, REASON_EFFECT) && pcard->is_releasable_by_effect(p, re))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_discardable(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t p = pcard->pduel->game_field->core.reason_player;
	effect* pe = pcard->pduel->game_field->core.reason_effect;
	uint32_t reason = REASON_COST;
	if(lua_gettop(L) > 1)
		reason = (uint32_t)lua_tointeger(L, 2);
	if((reason != REASON_COST || !pcard->is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
	        && pcard->pduel->game_field->is_player_can_discard_hand(p, pcard, pe, reason))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_attackable(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_capable_attack());
	return 1;
}
int32_t scriptlib::card_is_chain_attackable(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	int32_t monsteronly = FALSE;
	card* pcard = *(card**) lua_touserdata(L, 1);
	duel* pduel = pcard->pduel;
	int32_t ac = 2;
	if(lua_gettop(L) > 1)
		ac = (int32_t)lua_tointeger(L, 2);
	if(lua_gettop(L) > 2)
		monsteronly = lua_toboolean(L, 3);
	card* attacker = pduel->game_field->core.attacker;
	if(attacker->is_status(STATUS_BATTLE_DESTROYED)
			|| attacker->current.controler != pduel->game_field->infos.turn_player
			|| attacker->fieldid_r != pduel->game_field->core.pre_field[0]
			|| !attacker->is_capable_attack_announce(pduel->game_field->infos.turn_player)
			|| (ac != 0 && attacker->announce_count >= ac)
			|| (ac == 2 && attacker->is_affected_by_effect(EFFECT_EXTRA_ATTACK))) {
		lua_pushboolean(L, 0);
		return 1;
	}
	card_vector cv;
	pduel->game_field->get_attack_target(attacker, &cv, TRUE);
	if(cv.size() == 0 && (monsteronly || attacker->direct_attackable == 0))
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, 1);
	return 1;
}
int32_t scriptlib::card_is_faceup(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_position(POS_FACEUP));
	return 1;
}
int32_t scriptlib::card_is_faceup_ex(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_position(POS_FACEUP) | (pcard->current.location & (LOCATION_HAND | LOCATION_GRAVE | LOCATION_DECK)));
	return 1;
}
int32_t scriptlib::card_is_attack_pos(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_position(POS_ATTACK));
	return 1;
}
int32_t scriptlib::card_is_facedown(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_position(POS_FACEDOWN));
	return 1;
}
int32_t scriptlib::card_is_defense_pos(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_position(POS_DEFENSE));
	return 1;
}
int32_t scriptlib::card_is_position(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t pos = (uint32_t)lua_tointeger(L, 2);
	lua_pushboolean(L, pcard->is_position(pos));
	return 1;
}
int32_t scriptlib::card_is_pre_position(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t pos = (uint32_t)lua_tointeger(L, 2);
	lua_pushboolean(L, pcard->previous.position & pos);
	return 1;
}
int32_t scriptlib::card_is_controler(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t con = (uint32_t)lua_tointeger(L, 2);
	if(pcard->current.controler == con)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_pre_controler(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t con = (uint32_t)lua_tointeger(L, 2);
	if(pcard->previous.controler == con)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_onfield(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if((pcard->current.location & LOCATION_ONFIELD) && !pcard->is_treated_as_not_on_field())
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_location(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t loc = (uint32_t)lua_tointeger(L, 2);
	if(pcard->is_treated_as_not_on_field()) {
		lua_pushboolean(L, 0);
	} else if(pcard->current.location == LOCATION_MZONE) {
		if(loc & LOCATION_MZONE)
			lua_pushboolean(L, 1);
		else
			lua_pushboolean(L, 0);
	} else if(pcard->current.location == LOCATION_SZONE) {
		if(pcard->current.is_location(loc))
			lua_pushboolean(L, 1);
		else
			lua_pushboolean(L, 0);
	} else
		lua_pushboolean(L, pcard->current.location & loc);
	return 1;
}
int32_t scriptlib::card_is_pre_location(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t loc = (uint32_t)lua_tointeger(L, 2);
	lua_pushboolean(L, pcard->previous.is_location(loc));
	return 1;
}
int32_t scriptlib::card_is_level_below(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t lvl = (uint32_t)lua_tointeger(L, 2);
	uint32_t plvl = pcard->get_level();
	if(plvl == 0)
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, plvl <= lvl);
	return 1;
}
int32_t scriptlib::card_is_level_above(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t lvl = (uint32_t)lua_tointeger(L, 2);
	uint32_t plvl = pcard->get_level();
	if(plvl == 0)
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, plvl >= lvl);
	return 1;
}
int32_t scriptlib::card_is_rank_below(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t rnk = (uint32_t)lua_tointeger(L, 2);
	uint32_t prnk = pcard->get_rank();
	if(prnk == 0)
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, prnk <= rnk);
	return 1;
}
int32_t scriptlib::card_is_rank_above(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t rnk = (uint32_t)lua_tointeger(L, 2);
	uint32_t prnk = pcard->get_rank();
	if(prnk == 0)
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, prnk >= rnk);
	return 1;
}
int32_t scriptlib::card_is_link_below(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t lnk = (uint32_t)lua_tointeger(L, 2);
	uint32_t plnk = pcard->get_link();
	if(plnk == 0)
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, plnk <= lnk);
	return 1;
}
int32_t scriptlib::card_is_link_above(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t lnk = (uint32_t)lua_tointeger(L, 2);
	uint32_t plnk = pcard->get_link();
	if(plnk == 0)
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, plnk >= lnk);
	return 1;
}
int32_t scriptlib::card_is_attack_below(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t atk = (int32_t)lua_tointeger(L, 2);
	if(!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER))
		lua_pushboolean(L, 0);
	else {
		int32_t _atk = pcard->get_attack();
		lua_pushboolean(L, _atk >= 0 && _atk <= atk);
	}
	return 1;
}
int32_t scriptlib::card_is_attack_above(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t atk = (int32_t)lua_tointeger(L, 2);
	if(!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER))
		lua_pushboolean(L, 0);
	else {
		int32_t _atk = pcard->get_attack();
		lua_pushboolean(L, _atk >= atk);
	}
	return 1;
}
int32_t scriptlib::card_is_defense_below(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t def = (int32_t)lua_tointeger(L, 2);
	if((pcard->data.type & TYPE_LINK)
		|| (!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER)))
		lua_pushboolean(L, 0);
	else {
		int32_t _def = pcard->get_defense();
		lua_pushboolean(L, _def >= 0 && _def <= def);
	}
	return 1;
}
int32_t scriptlib::card_is_defense_above(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t def = (int32_t)lua_tointeger(L, 2);
	if((pcard->data.type & TYPE_LINK)
		|| (!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER)))
		lua_pushboolean(L, 0);
	else {
		int32_t _def = pcard->get_defense();
		lua_pushboolean(L, _def >= def);
	}
	return 1;
}
int32_t scriptlib::card_is_has_level(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if((pcard->data.type & (TYPE_XYZ | TYPE_LINK))
		|| (pcard->status & STATUS_NO_LEVEL)
		|| (!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER)))
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, 1);
	return 1;
}
int32_t scriptlib::card_is_has_defense(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if((pcard->data.type & TYPE_LINK)
		|| (!(pcard->data.type & TYPE_MONSTER) && !(pcard->get_type() & TYPE_MONSTER) && !pcard->is_affected_by_effect(EFFECT_PRE_MONSTER)))
		lua_pushboolean(L, 0);
	else
		lua_pushboolean(L, 1);
	return 1;
}
int32_t scriptlib::card_is_public(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->is_position(POS_FACEUP))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_forbidden(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_status(STATUS_FORBIDDEN));
	return 1;
}
int32_t scriptlib::card_is_able_to_change_controler(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->is_capable_change_control())
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_is_controler_can_be_changed(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t ign = FALSE;
	if(lua_gettop(L) >= 2)
		ign = lua_toboolean(L, 2);
	uint32_t zone = 0xff;
	if(lua_gettop(L) >= 3)
		zone = (uint32_t)lua_tointeger(L, 3);
	if(pcard->is_control_can_be_changed(ign, zone))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_add_counter(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t countertype = (uint32_t)lua_tointeger(L, 2);
	uint32_t count = (uint32_t)lua_tointeger(L, 3);
	uint8_t singly = FALSE;
	if(lua_gettop(L) > 3)
		singly = lua_toboolean(L, 4);
	if(pcard->is_affect_by_effect(pcard->pduel->game_field->core.reason_effect))
		lua_pushboolean(L, pcard->add_counter(pcard->pduel->game_field->core.reason_player, countertype, count, singly));
	else lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::card_remove_counter(lua_State *L) {
	check_action_permission(L);
	check_param_count(L, 5);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t rplayer = (uint32_t)lua_tointeger(L, 2);
	uint32_t countertype = (uint32_t)lua_tointeger(L, 3);
	uint32_t count = (uint32_t)lua_tointeger(L, 4);
	uint32_t reason = (uint32_t)lua_tointeger(L, 5);
	duel* pduel = pcard->pduel;
	if(countertype == 0) {
		// c38834303: remove all counters
		for(const auto& cmit : pcard->counters) {
			pduel->write_buffer8(MSG_REMOVE_COUNTER);
			pduel->write_buffer16(cmit.first);
			pduel->write_buffer8(pcard->current.controler);
			pduel->write_buffer8(pcard->current.location);
			pduel->write_buffer8(pcard->current.sequence);
			pduel->write_buffer16(cmit.second);
		}
		pcard->counters.clear();
		return 0;
	} else {
		pduel->game_field->remove_counter(reason, pcard, rplayer, 0, 0, countertype, count);
		return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State *L, int32_t status, lua_KContext ctx) {
			duel* pduel = (duel*)ctx;
			lua_pushboolean(L, pduel->game_field->returns.ivalue[0]);
			return 1;
		});
	}
}
int32_t scriptlib::card_get_counter(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t countertype = (uint32_t)lua_tointeger(L, 2);
	if(countertype == 0)
		lua_pushinteger(L, pcard->counters.size());
	else
		lua_pushinteger(L, pcard->get_counter(countertype));
	return 1;
}
int32_t scriptlib::card_enable_counter_permit(lua_State *L) {
	check_param_count(L, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t countertype = (int32_t)lua_tointeger(L, 2);
	uint32_t prange;
	if(lua_gettop(L) > 2)
		prange = (uint32_t)lua_tointeger(L, 3);
	else if(pcard->data.type & TYPE_MONSTER)
		prange = LOCATION_MZONE;
	else
		prange = LOCATION_SZONE | LOCATION_FZONE;
	effect* peffect = pcard->pduel->new_effect();
	peffect->owner = pcard;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_COUNTER_PERMIT | countertype;
	peffect->flag[0] = EFFECT_FLAG_SINGLE_RANGE;
	peffect->range = prange;
	if(lua_gettop(L) > 3 && lua_isfunction(L, 4))
		peffect->condition = interpreter::get_function_handle(L, 4);
	pcard->add_effect(peffect);
	return 0;
}
int32_t scriptlib::card_set_counter_limit(lua_State *L) {
	check_param_count(L, 3);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t countertype = (int32_t)lua_tointeger(L, 2);
	int32_t limit = (int32_t)lua_tointeger(L, 3);
	effect* peffect = pcard->pduel->new_effect();
	peffect->owner = pcard;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_COUNTER_LIMIT | countertype;
	peffect->value = limit;
	pcard->add_effect(peffect);
	return 0;
}
int32_t scriptlib::card_is_can_change_position(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_capable_change_position_by_effect(pcard->pduel->game_field->core.reason_player));
	return 1;
}
int32_t scriptlib::card_is_can_turn_set(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	lua_pushboolean(L, pcard->is_capable_turn_set(pcard->pduel->game_field->core.reason_player));
	return 1;
}
int32_t scriptlib::card_is_can_add_counter(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t countertype = (uint32_t)lua_tointeger(L, 2);
	uint32_t count = (uint32_t)lua_tointeger(L, 3);
	uint8_t singly = FALSE;
	if(lua_gettop(L) > 3)
		singly = lua_toboolean(L, 4);
	uint32_t loc = 0;
	if(lua_gettop(L) > 4)
		loc = (uint32_t)lua_tointeger(L, 5);
	lua_pushboolean(L, pcard->is_can_add_counter(pcard->pduel->game_field->core.reason_player, countertype, count, singly, loc));
	return 1;
}
int32_t scriptlib::card_is_can_remove_counter(lua_State *L) {
	check_param_count(L, 5);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	uint32_t countertype = (uint32_t)lua_tointeger(L, 3);
	uint32_t count = (uint32_t)lua_tointeger(L, 4);
	uint32_t reason = (uint32_t)lua_tointeger(L, 5);
	lua_pushboolean(L, pcard->pduel->game_field->is_player_can_remove_counter(playerid, pcard, 0, 0, countertype, count, reason));
	return 1;
}
int32_t scriptlib::card_is_can_have_counter(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**)lua_touserdata(L, 1);
	uint32_t countertype = (uint32_t)lua_tointeger(L, 2);
	lua_pushboolean(L, pcard->is_can_have_counter(countertype));
	return 1;
}
int32_t scriptlib::card_is_can_overlay(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint8_t playerid = pcard->pduel->game_field->core.reason_player;
	if(lua_gettop(L) > 1 && !lua_isnil(L, 2))
		playerid = (uint8_t)lua_tointeger(L, 2);
	lua_pushboolean(L, pcard->is_capable_overlay(playerid));
	return 1;
}
int32_t scriptlib::card_is_can_be_fusion_material(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* fcard = nullptr;
	if(lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_CARD, 2);
		fcard = *(card**)lua_touserdata(L, 2);
	}
	uint32_t summon_type = SUMMON_TYPE_FUSION;
	if(lua_gettop(L) >= 3)
		summon_type = (uint32_t)lua_tointeger(L, 3);
	lua_pushboolean(L, pcard->is_can_be_fusion_material(fcard, summon_type));
	return 1;
}
int32_t scriptlib::card_is_can_be_synchro_material(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* scard = nullptr;
	card* tuner = nullptr;
	if(lua_gettop(L) >= 2) {
		check_param(L, PARAM_TYPE_CARD, 2);
		scard = *(card**) lua_touserdata(L, 2);
	}
	if(lua_gettop(L) >= 3) {
		check_param(L, PARAM_TYPE_CARD, 3);
		tuner = *(card**) lua_touserdata(L, 3);
	}
	lua_pushboolean(L, pcard->is_can_be_synchro_material(scard, tuner));
	return 1;
}
int32_t scriptlib::card_is_can_be_ritual_material(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* scard = nullptr;
	if(lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_CARD, 2);
		scard = *(card**) lua_touserdata(L, 2);
	}
	lua_pushboolean(L, pcard->is_can_be_ritual_material(scard));
	return 1;
}
int32_t scriptlib::card_is_can_be_xyz_material(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* scard = nullptr;
	if(lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_CARD, 2);
		scard = *(card**) lua_touserdata(L, 2);
	}
	lua_pushboolean(L, pcard->is_can_be_xyz_material(scard));
	return 1;
}
int32_t scriptlib::card_is_can_be_link_material(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* scard = nullptr;
	if(lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_CARD, 2);
		scard = *(card**) lua_touserdata(L, 2);
	}
	lua_pushboolean(L, pcard->is_can_be_link_material(scard));
	return 1;
}
int32_t scriptlib::card_check_fusion_material(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t chkf = PLAYER_NONE;
	group* pgroup = nullptr;
	if(lua_gettop(L) > 1 && !lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_GROUP, 2);
		pgroup = *(group**) lua_touserdata(L, 2);
	}
	card* cg = nullptr;
	if(lua_gettop(L) > 2 && !lua_isnil(L, 3)) {
		check_param(L, PARAM_TYPE_CARD, 3);
		cg = *(card**) lua_touserdata(L, 3);
	}
	if(lua_gettop(L) > 3)
		chkf = (uint32_t)lua_tointeger(L, 4);
	uint8_t not_material = FALSE;
	if(lua_gettop(L) > 4)
		not_material = lua_toboolean(L, 5);
	lua_pushboolean(L, pcard->fusion_check(pgroup, cg, chkf, not_material));
	return 1;
}
int32_t scriptlib::card_check_fusion_substitute(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* fcard = *(card**) lua_touserdata(L, 2);
	lua_pushboolean(L, pcard->check_fusion_substitute(fcard));
	return 1;
}
int32_t scriptlib::card_is_immune_to_effect(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_EFFECT, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 2);
	lua_pushboolean(L, (int)!pcard->is_affect_by_effect(peffect));
	return 1;
}
int32_t scriptlib::card_is_can_be_disabled_by_effect(lua_State* L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_EFFECT, 2);
	card* pcard = *(card**)lua_touserdata(L, 1);
	effect* peffect = *(effect**)lua_touserdata(L, 2);
	bool is_monster_effect = true;
	if (lua_gettop(L) > 2)
		is_monster_effect = lua_toboolean(L, 3);
	lua_pushboolean(L, pcard->is_can_be_disabled_by_effect(peffect, is_monster_effect));
	return 1;
}
int32_t scriptlib::card_is_can_be_effect_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	duel* pduel = pcard->pduel;
	effect* peffect = pduel->game_field->core.reason_effect;
	if(lua_gettop(L) > 1) {
		check_param(L, PARAM_TYPE_EFFECT, 2);
		peffect = *(effect**) lua_touserdata(L, 2);
	}
	lua_pushboolean(L, pcard->is_capable_be_effect_target(peffect, pduel->game_field->core.reason_player));
	return 1;
}
int32_t scriptlib::card_is_can_be_battle_target(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	card* pcard = *(card**) lua_touserdata(L, 1);
	card* bcard = *(card**) lua_touserdata(L, 2);
	lua_pushboolean(L, pcard->is_capable_be_battle_target(bcard));
	return 1;
}
int32_t scriptlib::card_add_monster_attribute(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	int32_t type = (int32_t)lua_tointeger(L, 2);
	int32_t attribute = (int32_t)lua_tointeger(L, 3);
	int32_t race = (int32_t)lua_tointeger(L, 4);
	int32_t level = (int32_t)lua_tointeger(L, 5);
	int32_t atk = (int32_t)lua_tointeger(L, 6);
	int32_t def = (int32_t)lua_tointeger(L, 7);
	card* pcard = *(card**) lua_touserdata(L, 1);
	duel* pduel = pcard->pduel;
	pcard->set_status(STATUS_NO_LEVEL, FALSE);
	// pre-monster
	effect* peffect = pduel->new_effect();
	peffect->owner = pcard;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_PRE_MONSTER;
	peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
	peffect->reset_flag = RESET_CHAIN + RESET_EVENT + 0x47e0000;
	peffect->value = type;
	pcard->add_effect(peffect);
	//attribute
	if(attribute) {
		peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_ADD_ATTRIBUTE;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT + 0x47e0000;
		peffect->value = attribute;
		pcard->add_effect(peffect);
	}
	//race
	if(race) {
		peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_ADD_RACE;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT + 0x47e0000;
		peffect->value = race;
		pcard->add_effect(peffect);
	}
	//level
	if(level) {
		peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_CHANGE_LEVEL;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT + 0x47e0000;
		peffect->value = level;
		pcard->add_effect(peffect);
	}
	//atk
	if(atk) {
		peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_SET_BASE_ATTACK;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT + 0x47e0000;
		peffect->value = atk;
		pcard->add_effect(peffect);
	}
	//def
	if(def) {
		peffect = pduel->new_effect();
		peffect->owner = pcard;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_SET_BASE_DEFENSE;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = RESET_EVENT + 0x47e0000;
		peffect->value = def;
		pcard->add_effect(peffect);
	}
	return 0;
}
int32_t scriptlib::card_cancel_to_grave(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	bool cancel = true;
	if(lua_gettop(L) > 1)
		cancel = lua_toboolean(L, 2) != 0;
	if(cancel)
		pcard->set_status(STATUS_LEAVE_CONFIRMED, FALSE);
	else {
		pcard->pduel->game_field->core.leave_confirmed.insert(pcard);
		pcard->set_status(STATUS_LEAVE_CONFIRMED, TRUE);
	}
	return 0;
}
int32_t scriptlib::card_get_tribute_requirement(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t rcount = pcard->get_summon_tribute_count();
	lua_pushinteger(L, rcount & 0xffff);
	lua_pushinteger(L, (rcount >> 16) & 0xffff);
	return 2;
}
int32_t scriptlib::card_get_battle_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	duel* pduel = pcard->pduel;
	if(pduel->game_field->core.attacker == pcard)
		interpreter::card2value(L, pduel->game_field->core.attack_target);
	else if(pduel->game_field->core.attack_target == pcard)
		interpreter::card2value(L, pduel->game_field->core.attacker);
	else lua_pushnil(L);
	return 1;
}
int32_t scriptlib::card_get_attackable_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	duel* pduel = pcard->pduel;
	card_vector targets;
	uint8_t chain_attack = FALSE;
	if(pduel->game_field->core.chain_attacker_id == pcard->fieldid)
		chain_attack = TRUE;
	pduel->game_field->get_attack_target(pcard, &targets, chain_attack);
	group* newgroup = pduel->new_group();
	newgroup->container.insert(targets.begin(), targets.end());
	interpreter::group2value(L, newgroup);
	lua_pushboolean(L, (int32_t)pcard->direct_attackable);
	return 2;
}
int32_t scriptlib::card_set_hint(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	duel* pduel = pcard->pduel;
	uint32_t type = (uint32_t)lua_tointeger(L, 2);
	uint32_t value = (uint32_t)lua_tointeger(L, 3);
	if(type >= CHINT_DESC_ADD)
		return 0;
	pduel->write_buffer8(MSG_CARD_HINT);
	pduel->write_buffer32(pcard->get_info_location());
	pduel->write_buffer8(type);
	pduel->write_buffer32(value);
	return 0;
}
int32_t scriptlib::card_reverse_in_deck(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->current.location != LOCATION_DECK)
		return 0;
	pcard->current.position = POS_FACEUP_DEFENSE;
	duel* pduel = pcard->pduel;
	if(pcard->current.sequence == pduel->game_field->player[pcard->current.controler].list_main.size() - 1) {
		pduel->write_buffer8(MSG_DECK_TOP);
		pduel->write_buffer8(pcard->current.controler);
		pduel->write_buffer8(0);
		pduel->write_buffer32(pcard->data.code | 0x80000000);
	}
	return 0;
}
int32_t scriptlib::card_set_unique_onfield(lua_State *L) {
	check_param_count(L, 4);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	pcard->unique_pos[0] = (uint8_t)lua_tointeger(L, 2);
	pcard->unique_pos[1] = (uint8_t)lua_tointeger(L, 3);
	if(lua_isfunction(L, 4)) {
		pcard->unique_code = 1;
		pcard->unique_function = interpreter::get_function_handle(L, 4);
	} else
		pcard->unique_code = (uint32_t)lua_tointeger(L, 4);
	uint32_t location = LOCATION_ONFIELD;
	if(lua_gettop(L) > 4)
		location = lua_tointeger(L, 5) & LOCATION_ONFIELD;
	pcard->unique_location = location;
	effect* peffect = pcard->pduel->new_effect();
	peffect->owner = pcard;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_UNIQUE_CHECK;
	pcard->add_effect(peffect);
	pcard->unique_effect = peffect;
	if(pcard->current.location & location)
		pcard->pduel->game_field->add_unique_card(pcard);
	return 0;
}
int32_t scriptlib::card_check_unique_onfield(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	uint32_t check_player = (uint32_t)lua_tointeger(L, 2);
	uint32_t check_location = LOCATION_ONFIELD;
	if(lua_gettop(L) > 2)
		check_location = lua_tointeger(L, 3) & LOCATION_ONFIELD;
	card* icard = nullptr;
	if(lua_gettop(L) > 3) {
		if(check_param(L, PARAM_TYPE_CARD, 4, TRUE))
			icard = *(card**)lua_touserdata(L, 4);
	}
	lua_pushboolean(L, pcard->pduel->game_field->check_unique_onfield(pcard, check_player, check_location, icard) ? 0 : 1);
	return 1;
}
int32_t scriptlib::card_reset_negate_effect(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	int32_t count = lua_gettop(L) - 1;
	for(int32_t i = 0; i < count; ++i)
		pcard->reset((uint32_t)lua_tointeger(L, i + 2), RESET_CARD);
	return 0;
}
int32_t scriptlib::card_assume_prop(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	pcard->assume_type = (uint8_t)lua_tointeger(L, 2);
	pcard->assume_value = (uint32_t)lua_tointeger(L, 3);
	pcard->pduel->assumes.insert(pcard);
	return 0;
}
int32_t scriptlib::card_set_spsummon_once(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	if(pcard->status & STATUS_COPYING_EFFECT)
		return 0;
	pcard->spsummon_code = (uint32_t)lua_tointeger(L, 2);
	pcard->pduel->game_field->core.global_flag |= GLOBALFLAG_SPSUMMON_ONCE;
	return 0;
}
int32_t scriptlib::card_check_spsummon_once(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	auto playerid = (int32_t)lua_tointeger(L, 2);
	auto pduel = pcard->pduel;
	lua_pushboolean(L, pduel->game_field->check_spsummon_once(pcard, playerid));
	return 1;
}

static const struct luaL_Reg cardlib[] = {
	{ "GetCode", scriptlib::card_get_code },
	{ "GetOriginalCode", scriptlib::card_get_origin_code },
	{ "GetOriginalCodeRule", scriptlib::card_get_origin_code_rule },
	{ "GetFusionCode", scriptlib::card_get_fusion_code },
	{ "GetLinkCode", scriptlib::card_get_link_code },
	{ "IsFusionCode", scriptlib::card_is_fusion_code },
	{ "IsLinkCode", scriptlib::card_is_link_code },
	{ "IsSetCard", scriptlib::card_is_set_card },
	{ "IsOriginalSetCard", scriptlib::card_is_origin_set_card },
	{ "IsPreviousSetCard", scriptlib::card_is_pre_set_card },
	{ "IsFusionSetCard", scriptlib::card_is_fusion_set_card },
	{ "IsLinkSetCard", scriptlib::card_is_link_set_card },
	{ "IsSpecialSummonSetCard", scriptlib::card_is_special_summon_set_card },
	{ "GetType", scriptlib::card_get_type },
	{ "GetOriginalType", scriptlib::card_get_origin_type },
	{ "GetFusionType", scriptlib::card_get_fusion_type },
	{ "GetSynchroType", scriptlib::card_get_synchro_type },
	{ "GetXyzType", scriptlib::card_get_xyz_type },
	{ "GetLinkType", scriptlib::card_get_link_type },
	{ "GetLevel", scriptlib::card_get_level },
	{ "GetRank", scriptlib::card_get_rank },
	{ "GetLink", scriptlib::card_get_link },
	{ "GetSynchroLevel", scriptlib::card_get_synchro_level },
	{ "GetRitualLevel", scriptlib::card_get_ritual_level },
	{ "GetOriginalLevel", scriptlib::card_get_origin_level },
	{ "GetOriginalRank", scriptlib::card_get_origin_rank },
	{ "IsXyzLevel", scriptlib::card_is_xyz_level },
	{ "GetLeftScale", scriptlib::card_get_lscale },
	{ "GetOriginalLeftScale", scriptlib::card_get_origin_lscale },
	{ "GetRightScale", scriptlib::card_get_rscale },
	{ "GetOriginalRightScale", scriptlib::card_get_origin_rscale },
	{ "GetCurrentScale", scriptlib::card_get_current_scale },
	{ "IsLinkMarker", scriptlib::card_is_link_marker },
	{ "GetLinkedGroup", scriptlib::card_get_linked_group },
	{ "GetLinkedGroupCount", scriptlib::card_get_linked_group_count },
	{ "GetLinkedZone", scriptlib::card_get_linked_zone },
	{ "GetMutualLinkedGroup", scriptlib::card_get_mutual_linked_group },
	{ "GetMutualLinkedGroupCount", scriptlib::card_get_mutual_linked_group_count },
	{ "GetMutualLinkedZone", scriptlib::card_get_mutual_linked_zone },
	{ "IsLinkState", scriptlib::card_is_link_state },
	{ "IsExtraLinkState", scriptlib::card_is_extra_link_state },
	{ "GetColumnGroup", scriptlib::card_get_column_group },
	{ "GetColumnGroupCount", scriptlib::card_get_column_group_count },
	{ "GetColumnZone", scriptlib::card_get_column_zone },
	{ "IsAllColumn", scriptlib::card_is_all_column },
	{ "GetAttribute", scriptlib::card_get_attribute },
	{ "GetOriginalAttribute", scriptlib::card_get_origin_attribute },
	{ "GetFusionAttribute", scriptlib::card_get_fusion_attribute },
	{ "GetLinkAttribute", scriptlib::card_get_link_attribute },
	{ "GetAttributeInGrave", scriptlib::card_get_attribute_in_grave },
	{ "GetRace", scriptlib::card_get_race },
	{ "GetOriginalRace", scriptlib::card_get_origin_race },
	{ "GetLinkRace", scriptlib::card_get_link_race },
	{ "GetRaceInGrave", scriptlib::card_get_race_in_grave },
	{ "GetAttack", scriptlib::card_get_attack },
	{ "GetBaseAttack", scriptlib::card_get_origin_attack },
	{ "GetTextAttack", scriptlib::card_get_text_attack },
	{ "GetDefense",	scriptlib::card_get_defense },
	{ "GetBaseDefense", scriptlib::card_get_origin_defense },
	{ "GetTextDefense", scriptlib::card_get_text_defense },
	{ "GetPreviousCodeOnField", scriptlib::card_get_previous_code_onfield },
	{ "GetPreviousTypeOnField", scriptlib::card_get_previous_type_onfield },
	{ "GetPreviousLevelOnField", scriptlib::card_get_previous_level_onfield },
	{ "GetPreviousRankOnField", scriptlib::card_get_previous_rank_onfield },
	{ "GetPreviousAttributeOnField", scriptlib::card_get_previous_attribute_onfield },
	{ "GetPreviousRaceOnField", scriptlib::card_get_previous_race_onfield },
	{ "GetPreviousAttackOnField", scriptlib::card_get_previous_attack_onfield },
	{ "GetPreviousDefenseOnField", scriptlib::card_get_previous_defense_onfield },
	{ "GetPreviousOverlayCountOnField", scriptlib::card_get_previous_overlay_count_onfield },
	{ "GetOwner", scriptlib::card_get_owner },
	{ "GetControler", scriptlib::card_get_controler },
	{ "GetPreviousControler", scriptlib::card_get_previous_controler },
	{ "SetReason", scriptlib::card_set_reason },
	{ "GetReason", scriptlib::card_get_reason },
	{ "GetReasonCard", scriptlib::card_get_reason_card },
	{ "GetReasonPlayer", scriptlib::card_get_reason_player },
	{ "GetReasonEffect", scriptlib::card_get_reason_effect },
	{ "GetPosition", scriptlib::card_get_position },
	{ "GetPreviousPosition", scriptlib::card_get_previous_position },
	{ "GetBattlePosition", scriptlib::card_get_battle_position },
	{ "GetLocation", scriptlib::card_get_location },
	{ "GetPreviousLocation", scriptlib::card_get_previous_location },
	{ "GetSequence", scriptlib::card_get_sequence },
	{ "GetPreviousSequence", scriptlib::card_get_previous_sequence },
	{ "GetSummonType", scriptlib::card_get_summon_type },
	{ "GetSummonLocation", scriptlib::card_get_summon_location },
	{ "GetSummonPlayer", scriptlib::card_get_summon_player },
	{ "GetSpecialSummonInfo", scriptlib::card_get_special_summon_info },
	{ "GetDestination", scriptlib::card_get_destination },
	{ "GetLeaveFieldDest", scriptlib::card_get_leave_field_dest },
	{ "GetTurnID", scriptlib::card_get_turnid },
	{ "GetFieldID", scriptlib::card_get_fieldid },
	{ "GetRealFieldID", scriptlib::card_get_fieldidr },
	{ "IsOriginalCodeRule", scriptlib::card_is_origin_code_rule },
	{ "IsCode", scriptlib::card_is_code },
	{ "IsType", scriptlib::card_is_type },
	{ "IsAllTypes", scriptlib::card_is_all_types },
	{ "IsFusionType", scriptlib::card_is_fusion_type },
	{ "IsSynchroType", scriptlib::card_is_synchro_type },
	{ "IsXyzType", scriptlib::card_is_xyz_type },
	{ "IsLinkType", scriptlib::card_is_link_type },
	{ "IsLevel", scriptlib::card_is_level },
	{ "IsRank", scriptlib::card_is_rank },
	{ "IsLink", scriptlib::card_is_link },
	{ "IsAttack", scriptlib::card_is_attack },
	{ "IsDefense", scriptlib::card_is_defense },
	{ "IsRace", scriptlib::card_is_race },
	{ "IsLinkRace", scriptlib::card_is_link_race },
	{ "IsAttribute", scriptlib::card_is_attribute },
	{ "IsFusionAttribute", scriptlib::card_is_fusion_attribute },
	{ "IsLinkAttribute", scriptlib::card_is_link_attribute },
	{ "IsNonAttribute", scriptlib::card_is_non_attribute },
	{ "IsExtraDeckMonster", scriptlib::card_is_extra_deck_monster },
	{ "IsReason", scriptlib::card_is_reason },
	{ "IsAllReasons", scriptlib::card_is_all_reasons },
	{ "IsSummonType", scriptlib::card_is_summon_type },
	{ "IsSummonLocation", scriptlib::card_is_summon_location },
	{ "IsSummonPlayer", scriptlib::card_is_summon_player },
	{ "IsStatus", scriptlib::card_is_status },
	{ "IsNotTuner", scriptlib::card_is_not_tuner },
	{ "IsTuner", scriptlib::card_is_tuner },
	{ "IsOriginalEffectProperty", scriptlib::card_is_original_effect_property },
	{ "IsEffectProperty", scriptlib::card_is_effect_property },
	{ "SetStatus", scriptlib::card_set_status },
	{ "IsDualState", scriptlib::card_is_dual_state },
	{ "EnableDualState", scriptlib::card_enable_dual_state },
	{ "SetTurnCounter",	scriptlib::card_set_turn_counter },
	{ "GetTurnCounter", scriptlib::card_get_turn_counter },
	{ "SetMaterial", scriptlib::card_set_material },
	{ "GetMaterial", scriptlib::card_get_material },
	{ "GetMaterialCount", scriptlib::card_get_material_count },
	{ "GetEquipGroup", scriptlib::card_get_equip_group },
	{ "GetEquipCount", scriptlib::card_get_equip_count },
	{ "GetEquipTarget", scriptlib::card_get_equip_target },
	{ "GetPreviousEquipTarget", scriptlib::card_get_pre_equip_target },
	{ "CheckEquipTarget", scriptlib::card_check_equip_target },
	{ "CheckUnionTarget", scriptlib::card_check_union_target },
	{ "GetUnionCount", scriptlib::card_get_union_count },
	{ "GetOverlayGroup", scriptlib::card_get_overlay_group },
	{ "GetOverlayCount", scriptlib::card_get_overlay_count },
	{ "GetOverlayTarget", scriptlib::card_get_overlay_target },
	{ "CheckRemoveOverlayCard", scriptlib::card_check_remove_overlay_card },
	{ "RemoveOverlayCard", scriptlib::card_remove_overlay_card },
	{ "GetAttackedGroup", scriptlib::card_get_attacked_group },
	{ "GetAttackedGroupCount", scriptlib::card_get_attacked_group_count },
	{ "GetAttackedCount", scriptlib::card_get_attacked_count },
	{ "GetBattledGroup", scriptlib::card_get_battled_group },
	{ "GetBattledGroupCount", scriptlib::card_get_battled_group_count },
	{ "GetAttackAnnouncedCount", scriptlib::card_get_attack_announced_count },
	{ "IsDirectAttacked", scriptlib::card_is_direct_attacked },
	{ "SetCardTarget", scriptlib::card_set_card_target },
	{ "GetCardTarget", scriptlib::card_get_card_target },
	{ "GetFirstCardTarget", scriptlib::card_get_first_card_target },
	{ "GetCardTargetCount", scriptlib::card_get_card_target_count },
	{ "IsHasCardTarget", scriptlib::card_is_has_card_target },
	{ "CancelCardTarget", scriptlib::card_cancel_card_target },
	{ "GetOwnerTarget", scriptlib::card_get_owner_target },
	{ "GetOwnerTargetCount", scriptlib::card_get_owner_target_count },
	{ "GetActivateEffect", scriptlib::card_get_activate_effect },
	{ "CheckActivateEffect", scriptlib::card_check_activate_effect },
	{ "GetTunerLimit", scriptlib::card_get_tuner_limit },
	{ "GetHandSynchro", scriptlib::card_get_hand_synchro },
	{ "RegisterEffect", scriptlib::card_register_effect },
	{ "IsHasEffect", scriptlib::card_is_has_effect },
	{ "ResetEffect", scriptlib::card_reset_effect },
	{ "GetEffectCount", scriptlib::card_get_effect_count },
	{ "RegisterFlagEffect", scriptlib::card_register_flag_effect },
	{ "GetFlagEffect", scriptlib::card_get_flag_effect },
	{ "ResetFlagEffect", scriptlib::card_reset_flag_effect },
	{ "SetFlagEffectLabel", scriptlib::card_set_flag_effect_label },
	{ "GetFlagEffectLabel", scriptlib::card_get_flag_effect_label },
	{ "CreateRelation", scriptlib::card_create_relation },
	{ "ReleaseRelation", scriptlib::card_release_relation },
	{ "CreateEffectRelation", scriptlib::card_create_effect_relation },
	{ "ReleaseEffectRelation", scriptlib::card_release_effect_relation },
	{ "ClearEffectRelation", scriptlib::card_clear_effect_relation },
	{ "IsRelateToEffect", scriptlib::card_is_relate_to_effect },
	{ "IsRelateToChain", scriptlib::card_is_relate_to_chain },
	{ "IsRelateToCard", scriptlib::card_is_relate_to_card },
	{ "IsRelateToBattle", scriptlib::card_is_relate_to_battle },
	{ "CopyEffect", scriptlib::card_copy_effect },
	{ "ReplaceEffect", scriptlib::card_replace_effect },
	{ "EnableReviveLimit", scriptlib::card_enable_revive_limit },
	{ "CompleteProcedure", scriptlib::card_complete_procedure },
	{ "IsDisabled", scriptlib::card_is_disabled },
	{ "IsDestructable", scriptlib::card_is_destructable },
	{ "IsSummonableCard", scriptlib::card_is_summonable },
	{ "IsSpecialSummonableCard", scriptlib::card_is_special_summonable_card },
	{ "IsFusionSummonableCard", scriptlib::card_is_fusion_summonable_card },
	{ "IsSpecialSummonable", scriptlib::card_is_special_summonable },
	{ "IsSynchroSummonable", scriptlib::card_is_synchro_summonable },
	{ "IsXyzSummonable", scriptlib::card_is_xyz_summonable },
	{ "IsLinkSummonable", scriptlib::card_is_link_summonable },
	{ "IsSummonable", scriptlib::card_is_can_be_summoned },
	{ "IsMSetable", scriptlib::card_is_msetable },
	{ "IsSSetable", scriptlib::card_is_ssetable },
	{ "IsCanBeSpecialSummoned", scriptlib::card_is_can_be_special_summoned },
	{ "IsCanBePlacedOnField", scriptlib::card_is_can_be_placed_on_field },
	{ "IsAbleToHand", scriptlib::card_is_able_to_hand },
	{ "IsAbleToDeck", scriptlib::card_is_able_to_deck },
	{ "IsAbleToExtra", scriptlib::card_is_able_to_extra },
	{ "IsAbleToGrave", scriptlib::card_is_able_to_grave },
	{ "IsAbleToRemove", scriptlib::card_is_able_to_remove },
	{ "IsAbleToHandAsCost", scriptlib::card_is_able_to_hand_as_cost },
	{ "IsAbleToDeckAsCost", scriptlib::card_is_able_to_deck_as_cost },
	{ "IsAbleToExtraAsCost", scriptlib::card_is_able_to_extra_as_cost },
	{ "IsAbleToDeckOrExtraAsCost", scriptlib::card_is_able_to_deck_or_extra_as_cost },
	{ "IsAbleToGraveAsCost", scriptlib::card_is_able_to_grave_as_cost },
	{ "IsAbleToRemoveAsCost", scriptlib::card_is_able_to_remove_as_cost },
	{ "IsReleasable", scriptlib::card_is_releasable },
	{ "IsReleasableByEffect", scriptlib::card_is_releasable_by_effect },
	{ "IsDiscardable", scriptlib::card_is_discardable },
	{ "IsAttackable", scriptlib::card_is_attackable },
	{ "IsChainAttackable", scriptlib::card_is_chain_attackable },
	{ "IsFaceup", scriptlib::card_is_faceup },
	{ "IsFaceupEx", scriptlib::card_is_faceup_ex },
	{ "IsAttackPos", scriptlib::card_is_attack_pos },
	{ "IsFacedown", scriptlib::card_is_facedown },
	{ "IsDefensePos", scriptlib::card_is_defense_pos },
	{ "IsPosition", scriptlib::card_is_position },
	{ "IsPreviousPosition", scriptlib::card_is_pre_position },
	{ "IsControler", scriptlib::card_is_controler },
	{ "IsPreviousControler", scriptlib::card_is_pre_controler },
	{ "IsOnField", scriptlib::card_is_onfield },
	{ "IsLocation", scriptlib::card_is_location },
	{ "IsPreviousLocation", scriptlib::card_is_pre_location },
	{ "IsLevelBelow", scriptlib::card_is_level_below },
	{ "IsLevelAbove", scriptlib::card_is_level_above },
	{ "IsRankBelow", scriptlib::card_is_rank_below },
	{ "IsRankAbove", scriptlib::card_is_rank_above },
	{ "IsLinkBelow", scriptlib::card_is_link_below },
	{ "IsLinkAbove", scriptlib::card_is_link_above },
	{ "IsAttackBelow", scriptlib::card_is_attack_below },
	{ "IsAttackAbove", scriptlib::card_is_attack_above },
	{ "IsDefenseBelow", scriptlib::card_is_defense_below },
	{ "IsDefenseAbove", scriptlib::card_is_defense_above },
	{ "IsHasLevel", scriptlib::card_is_has_level },
	{ "IsHasDefense", scriptlib::card_is_has_defense },
	{ "IsPublic", scriptlib::card_is_public },
	{ "IsForbidden", scriptlib::card_is_forbidden },
	{ "IsAbleToChangeControler", scriptlib::card_is_able_to_change_controler },
	{ "IsControlerCanBeChanged", scriptlib::card_is_controler_can_be_changed },
	{ "AddCounter", scriptlib::card_add_counter },
	{ "RemoveCounter", scriptlib::card_remove_counter },
	{ "GetCounter", scriptlib::card_get_counter },
	{ "EnableCounterPermit", scriptlib::card_enable_counter_permit },
	{ "SetCounterLimit", scriptlib::card_set_counter_limit },
	{ "IsCanChangePosition", scriptlib::card_is_can_change_position },
	{ "IsCanTurnSet", scriptlib::card_is_can_turn_set },
	{ "IsCanAddCounter", scriptlib::card_is_can_add_counter },
	{ "IsCanRemoveCounter", scriptlib::card_is_can_remove_counter },
	{ "IsCanHaveCounter", scriptlib::card_is_can_have_counter },
	{ "IsCanOverlay", scriptlib::card_is_can_overlay },
	{ "IsCanBeFusionMaterial", scriptlib::card_is_can_be_fusion_material },
	{ "IsCanBeSynchroMaterial", scriptlib::card_is_can_be_synchro_material },
	{ "IsCanBeRitualMaterial", scriptlib::card_is_can_be_ritual_material },
	{ "IsCanBeXyzMaterial", scriptlib::card_is_can_be_xyz_material },
	{ "IsCanBeLinkMaterial", scriptlib::card_is_can_be_link_material },
	{ "CheckFusionMaterial", scriptlib::card_check_fusion_material },
	{ "CheckFusionSubstitute", scriptlib::card_check_fusion_substitute },
	{ "IsImmuneToEffect", scriptlib::card_is_immune_to_effect },
	{ "IsCanBeDisabledByEffect", scriptlib::card_is_can_be_disabled_by_effect },
	{ "IsCanBeEffectTarget", scriptlib::card_is_can_be_effect_target },
	{ "IsCanBeBattleTarget", scriptlib::card_is_can_be_battle_target },
	{ "AddMonsterAttribute", scriptlib::card_add_monster_attribute },
	{ "CancelToGrave", scriptlib::card_cancel_to_grave },
	{ "GetTributeRequirement", scriptlib::card_get_tribute_requirement },
	{ "GetBattleTarget", scriptlib::card_get_battle_target },
	{ "GetAttackableTarget", scriptlib::card_get_attackable_target },
	{ "SetHint", scriptlib::card_set_hint },
	{ "ReverseInDeck", scriptlib::card_reverse_in_deck },
	{ "SetUniqueOnField", scriptlib::card_set_unique_onfield },
	{ "CheckUniqueOnField", scriptlib::card_check_unique_onfield },
	{ "ResetNegateEffect", scriptlib::card_reset_negate_effect },
	{ "AssumeProperty", scriptlib::card_assume_prop },
	{ "SetSPSummonOnce", scriptlib::card_set_spsummon_once },
	{ "CheckSPSummonOnce", scriptlib::card_check_spsummon_once },
	{ nullptr, nullptr }
};
void scriptlib::open_cardlib(lua_State *L) {
	luaL_newlib(L, cardlib);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Card");
}
