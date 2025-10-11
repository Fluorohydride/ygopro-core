/*
 * libgroup.cpp
 *
 *  Created on: 2010-5-6
 *      Author: Argon
 */

#include "scriptlib.h"
#include "group.h"
#include "card.h"
#include "effect.h"
#include "duel.h"

int32_t scriptlib::group_new(lua_State *L) {
	duel* pduel = interpreter::get_duel_info(L);
	group* pgroup = pduel->new_group();
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::group_clone(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_GROUP, 1);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	group* newgroup = pduel->new_group(pgroup->container);
	interpreter::group2value(L, newgroup);
	return 1;
}
int32_t scriptlib::group_from_cards(lua_State *L) {
	duel* pduel = interpreter::get_duel_info(L);
	group* pgroup = pduel->new_group();
	for(int32_t i = 0; i < lua_gettop(L); ++i) {
		if(!lua_isnil(L, i + 1)) {
			check_param(L, PARAM_TYPE_CARD, i + 1);
			card* pcard = *(card**) lua_touserdata(L, i + 1);
			pgroup->container.insert(pcard);
		}
	}
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::group_delete(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_GROUP, 1);
	duel* pduel = interpreter::get_duel_info(L);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	if(pgroup->is_readonly != GTYPE_KEEP_ALIVE)
		return 0;
	pgroup->is_iterator_dirty = true;
	pgroup->is_readonly = GTYPE_DEFAULT;
	pduel->sgroups.insert(pgroup);
	return 0;
}
int32_t scriptlib::group_keep_alive(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_GROUP, 1);
	duel* pduel = interpreter::get_duel_info(L);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	if(pgroup->is_readonly == GTYPE_READ_ONLY)
		return 0;
	pgroup->is_readonly = GTYPE_KEEP_ALIVE;
	pduel->sgroups.erase(pgroup);
	return 0;
}
int32_t scriptlib::group_clear(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_GROUP, 1);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	if (pgroup->is_readonly != GTYPE_READ_ONLY) {
		pgroup->is_iterator_dirty = true;
		pgroup->container.clear();
	}
	return 0;
}
int32_t scriptlib::group_add_card(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	card* pcard = *(card**) lua_touserdata(L, 2);
	if (pgroup->is_readonly != GTYPE_READ_ONLY) {
		pgroup->is_iterator_dirty = true;
		pgroup->container.insert(pcard);
	}
	return 0;
}
int32_t scriptlib::group_remove_card(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	card* pcard = *(card**) lua_touserdata(L, 2);
	if (pgroup->is_readonly != GTYPE_READ_ONLY) {
		pgroup->is_iterator_dirty = true;
		pgroup->container.erase(pcard);
	}
	return 0;
}
int32_t scriptlib::group_get_next(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_GROUP, 1);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	if (pgroup->is_iterator_dirty) {
		return luaL_error(L, "Called Group.GetNext without calling Group.GetFirst first.");
	}
	if(pgroup->it == pgroup->container.end())
		lua_pushnil(L);
	else {
		++pgroup->it;
		if (pgroup->it == pgroup->container.end())
			lua_pushnil(L);
		else
			interpreter::card2value(L, (*(pgroup->it)));
	}
	return 1;
}
int32_t scriptlib::group_get_first(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_GROUP, 1);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	pgroup->is_iterator_dirty = false;
	if (pgroup->container.size()) {
		pgroup->it = pgroup->container.begin();
		interpreter::card2value(L, (*(pgroup->it)));
	} else
		lua_pushnil(L);
	return 1;
}
int32_t scriptlib::group_get_count(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_GROUP, 1);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	lua_pushinteger(L, pgroup->container.size());
	return 1;
}
int32_t scriptlib::group_filter(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	card_set cset(pgroup->container);
	if(check_param(L, PARAM_TYPE_CARD, 3, TRUE)) {
		card* pexception = *(card**) lua_touserdata(L, 3);
		cset.erase(pexception);
	} else if(check_param(L, PARAM_TYPE_GROUP, 3, TRUE)) {
		group* pexgroup = *(group**) lua_touserdata(L, 3);
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	duel* pduel = pgroup->pduel;
	group* new_group = pduel->new_group();
	uint32_t extraargs = lua_gettop(L) - 3;
	for(auto& pcard : cset) {
		if(pduel->lua->check_filter(L, pcard, 2, extraargs)) {
			new_group->container.insert(pcard);
		}
	}
	interpreter::group2value(L, new_group);
	return 1;
}
int32_t scriptlib::group_filter_count(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	card_set cset(pgroup->container);
	if(check_param(L, PARAM_TYPE_CARD, 3, TRUE)) {
		card* pexception = *(card**) lua_touserdata(L, 3);
		cset.erase(pexception);
	} else if(check_param(L, PARAM_TYPE_GROUP, 3, TRUE)) {
		group* pexgroup = *(group**) lua_touserdata(L, 3);
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	duel* pduel = pgroup->pduel;
	uint32_t extraargs = lua_gettop(L) - 3;
	uint32_t count = 0;
	for (auto& pcard : cset) {
		if(pduel->lua->check_filter(L, pcard, 2, extraargs))
			++count;
	}
	lua_pushinteger(L, count);
	return 1;
}
int32_t scriptlib::group_filter_select(lua_State *L) {
	check_action_permission(L);
	check_param_count(L, 6);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 3);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	card_set cset(pgroup->container);
	if(check_param(L, PARAM_TYPE_CARD, 6, TRUE)) {
		card* pexception = *(card**) lua_touserdata(L, 6);
		cset.erase(pexception);
	} else if(check_param(L, PARAM_TYPE_GROUP, 6, TRUE)) {
		group* pexgroup = *(group**) lua_touserdata(L, 6);
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	duel* pduel = pgroup->pduel;
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	uint32_t min = (uint32_t)lua_tointeger(L, 4);
	uint32_t max = (uint32_t)lua_tointeger(L, 5);
	uint32_t extraargs = lua_gettop(L) - 6;
	pduel->game_field->core.select_cards.clear();
	for (auto& pcard : cset) {
		if(pduel->lua->check_filter(L, pcard, 3, extraargs))
			pduel->game_field->core.select_cards.push_back(pcard);
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid, min + (max << 16));
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State *L, int32_t status, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		group* pgroup = pduel->new_group();
		for(int32_t i = 0; i < pduel->game_field->returns.bvalue[0]; ++i) {
			card* pcard = pduel->game_field->core.select_cards[pduel->game_field->returns.bvalue[i + 1]];
			pgroup->container.insert(pcard);
		}
		interpreter::group2value(L, pgroup);
		return 1;
	});
}
int32_t scriptlib::group_select(lua_State *L) {
	check_action_permission(L);
	check_param_count(L, 5);
	check_param(L, PARAM_TYPE_GROUP, 1);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	card_set cset(pgroup->container);
	if(check_param(L, PARAM_TYPE_CARD, 5, TRUE)) {
		card* pexception = *(card**) lua_touserdata(L, 5);
		cset.erase(pexception);
	} else if(check_param(L, PARAM_TYPE_GROUP, 5, TRUE)) {
		group* pexgroup = *(group**) lua_touserdata(L, 5);
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	duel* pduel = pgroup->pduel;
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	uint32_t min = (uint32_t)lua_tointeger(L, 3);
	uint32_t max = (uint32_t)lua_tointeger(L, 4);
	pduel->game_field->core.select_cards.clear();
	for (auto& pcard : cset) {
		pduel->game_field->core.select_cards.push_back(pcard);
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid, min + (max << 16));
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State *L, int32_t status, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		group* pgroup = pduel->new_group();
		for(int32_t i = 0; i < pduel->game_field->returns.bvalue[0]; ++i) {
			card* pcard = pduel->game_field->core.select_cards[pduel->game_field->returns.bvalue[i + 1]];
			pgroup->container.insert(pcard);
		}
		interpreter::group2value(L, pgroup);
		return 1;
	});
}
int32_t scriptlib::group_select_unselect(lua_State *L) {
	check_action_permission(L);
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_GROUP, 1);
	group* select_group = *(group**)lua_touserdata(L, 1);
	group* unselect_group = nullptr;
	if(check_param(L, PARAM_TYPE_GROUP, 2, TRUE))
		unselect_group = *(group**)lua_touserdata(L, 2);
	duel* pduel = select_group->pduel;
	int32_t playerid = (int32_t)lua_tointeger(L, 3);
	if(playerid != 0 && playerid != 1)
		return 0;
	if(select_group->container.size() == 0 && (!unselect_group || unselect_group->container.size() == 0))
		return 0;
	if(unselect_group) {
		for(auto it = unselect_group->container.begin(); it != unselect_group->container.end(); ++it) {
			card* pcard = *it;
			for(auto it2 = select_group->container.begin(); it2 != select_group->container.end(); ++it2) {
				if((*it2) == pcard) {
					return 0;
				}
			}
		}
	}
	uint32_t finishable = FALSE;
	if(lua_gettop(L) > 3) {
		finishable = lua_toboolean(L, 4);
	}
	uint32_t cancelable = FALSE;
	if(lua_gettop(L) > 4) {
		cancelable = lua_toboolean(L, 5);
	}
	uint32_t min = 1;
	if(lua_gettop(L) > 5) {
		min = (uint32_t)lua_tointeger(L, 6);
	}
	uint32_t max = 1;
	if(lua_gettop(L) > 6) {
		max = (uint32_t)lua_tointeger(L, 7);
	}
	if(min > max)
		min = max;
	pduel->game_field->core.select_cards.clear();
	pduel->game_field->core.unselect_cards.clear();
	for(auto it = select_group->container.begin(); it != select_group->container.end(); ++it) {
		pduel->game_field->core.select_cards.push_back(*it);
	}
	if(unselect_group) {
		for(auto it = unselect_group->container.begin(); it != unselect_group->container.end(); ++it) {
			pduel->game_field->core.unselect_cards.push_back(*it);
		}
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_UNSELECT_CARD, 0, 0, 0, playerid + (cancelable << 16), min + (max << 16), finishable);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State *L, int32_t status, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		if(pduel->game_field->returns.ivalue[0] == -1) {
			lua_pushnil(L);
		} else {
			card* pcard;
			const int32_t index = pduel->game_field->returns.bvalue[1];
			if(index < (int32_t)pduel->game_field->core.select_cards.size())
				pcard = pduel->game_field->core.select_cards[index];
			else
				pcard = pduel->game_field->core.unselect_cards[index - pduel->game_field->core.select_cards.size()];
			interpreter::card2value(L, pcard);
		}
		return 1;
	});
}
int32_t scriptlib::group_random_select(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_GROUP, 1);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	int32_t count = (int32_t)lua_tointeger(L, 3);
	duel* pduel = pgroup->pduel;
	group* newgroup = pduel->new_group();
	if (count > (int32_t)pgroup->container.size())
		count = (int32_t)pgroup->container.size();
	if(count == 0) {
		interpreter::group2value(L, newgroup);
		return 1;
	}
	if(count == pgroup->container.size())
		newgroup->container = pgroup->container;
	else {
		card_vector cv(pgroup->container.begin(), pgroup->container.end());
		int32_t back = (int32_t)cv.size() - 1;
		for (int32_t i = 0; i < count; ++i) {
			int32_t r = pduel->get_next_integer(i, back);
			std::swap(cv[i], cv[r]);
		}
		newgroup->container.insert(cv.begin(), cv.begin() + count);
	}
	pduel->write_buffer8(MSG_RANDOM_SELECTED);
	pduel->write_buffer8(playerid);
	pduel->write_buffer8(count);
	for(auto& pcard : newgroup->container) {
		pduel->write_buffer32(pcard->get_info_location());
	}
	interpreter::group2value(L, newgroup);
	return 1;
}
int32_t scriptlib::group_cancelable_select(lua_State *L) {
	check_action_permission(L);
	check_param_count(L, 5);
	check_param(L, PARAM_TYPE_GROUP, 1);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	card_set cset(pgroup->container);
	if(check_param(L, PARAM_TYPE_CARD, 5, TRUE)) {
		card* pexception = *(card**) lua_touserdata(L, 5);
		cset.erase(pexception);
	} else if(check_param(L, PARAM_TYPE_GROUP, 5, TRUE)) {
		group* pexgroup = *(group**) lua_touserdata(L, 5);
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	duel* pduel = pgroup->pduel;
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	uint32_t min = (uint32_t)lua_tointeger(L, 3);
	uint32_t max = (uint32_t)lua_tointeger(L, 4);
	pduel->game_field->core.select_cards.clear();
	for (auto& pcard : cset) {
		pduel->game_field->core.select_cards.push_back(pcard);
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + 0x10000, min + (max << 16));
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State *L, int32_t status, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		if(pduel->game_field->returns.ivalue[0] == -1) {
			lua_pushnil(L);
		} else {
			group* pgroup = pduel->new_group();
			for(int32_t i = 0; i < pduel->game_field->returns.bvalue[0]; ++i) {
				card* pcard = pduel->game_field->core.select_cards[pduel->game_field->returns.bvalue[i + 1]];
				pgroup->container.insert(pcard);
			}
			interpreter::group2value(L, pgroup);
		}
		return 1;
	});
}
int32_t scriptlib::group_is_exists(lua_State *L) {
	check_param_count(L, 4);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	card_set cset(pgroup->container);
	if(check_param(L, PARAM_TYPE_CARD, 4, TRUE)) {
		card* pexception = *(card**) lua_touserdata(L, 4);
		cset.erase(pexception);
	} else if(check_param(L, PARAM_TYPE_GROUP, 4, TRUE)) {
		group* pexgroup = *(group**) lua_touserdata(L, 4);
		for(auto& pcard : pexgroup->container)
			cset.erase(pcard);
	}
	duel* pduel = pgroup->pduel;
	uint32_t count = (uint32_t)lua_tointeger(L, 3);
	uint32_t extraargs = lua_gettop(L) - 4;
	uint32_t fcount = 0;
	uint32_t result = FALSE;
	if (count > pgroup->container.size()) {
		lua_pushboolean(L, 0);
		return 1;
	}
	for (auto& pcard : cset) {
		if(pduel->lua->check_filter(L, pcard, 2, extraargs)) {
			++fcount;
			if(fcount >= count) {
				result = TRUE;
				break;
			}
		}
	}
	lua_pushboolean(L, result);
	return 1;
}
int32_t scriptlib::group_check_with_sum_equal(lua_State *L) {
	check_param_count(L, 5);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	int32_t acc = (int32_t)lua_tointeger(L, 3);
	int32_t min = (int32_t)lua_tointeger(L, 4);
	int32_t max = (int32_t)lua_tointeger(L, 5);
	if(min < 0)
		min = 0;
	if(max < min)
		max = min;
	int32_t extraargs = lua_gettop(L) - 5;
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = (int32_t)cv.size();
	for(auto& pcard : pgroup->container) {
		auto it = std::find(pduel->game_field->core.must_select_cards.begin(), pduel->game_field->core.must_select_cards.end(), pcard);
		if(it == pduel->game_field->core.must_select_cards.end())
			cv.push_back(pcard);
	}
	pduel->game_field->core.must_select_cards.clear();
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, 2, extraargs);
	lua_pushboolean(L, field::check_with_sum_limit_m(cv, acc, 0, min, max, 0xffff, mcount));
	return 1;
}
int32_t scriptlib::group_select_with_sum_equal(lua_State *L) {
	check_action_permission(L);
	check_param_count(L, 6);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 3);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	int32_t acc = (int32_t)lua_tointeger(L, 4);
	int32_t min = (int32_t)lua_tointeger(L, 5);
	int32_t max = (int32_t)lua_tointeger(L, 6);
	if(min < 0)
		min = 0;
	if(max < min)
		max = min;
	if(max > UINT8_MAX)
		return luaL_error(L, "Parameter \"max\" exceeded 255.");
	int32_t extraargs = lua_gettop(L) - 6;
	pduel->game_field->core.select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	for(auto& pcard : pduel->game_field->core.must_select_cards) {
		auto it = std::remove(pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end(), pcard);
		pduel->game_field->core.select_cards.erase(it, pduel->game_field->core.select_cards.end());
	}
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = (int32_t)cv.size();
	cv.insert(cv.end(), pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end());
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, 3, extraargs);
	if(!field::check_with_sum_limit_m(cv, acc, 0, min, max, 0xffff, mcount)) {
		pduel->game_field->core.must_select_cards.clear();
		group* empty_group = pduel->new_group();
		interpreter::group2value(L, empty_group);
		return 1;
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_SUM, 0, 0, 0, acc, playerid, min, max);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State *L, int32_t status, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		group* pgroup = pduel->new_group();
		int32_t mcount = (int32_t)pduel->game_field->core.must_select_cards.size();
		for(int32_t i = mcount; i < pduel->game_field->returns.bvalue[0]; ++i) {
			card* pcard = pduel->game_field->core.select_cards[pduel->game_field->returns.bvalue[i + 1]];
			pgroup->container.insert(pcard);
		}
		pduel->game_field->core.must_select_cards.clear();
		interpreter::group2value(L, pgroup);
		return 1;
	});
}
int32_t scriptlib::group_check_with_sum_greater(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	int32_t acc = (int32_t)lua_tointeger(L, 3);
	int32_t extraargs = lua_gettop(L) - 3;
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = (int32_t)cv.size();
	for(auto& pcard : pgroup->container) {
		auto it = std::find(pduel->game_field->core.must_select_cards.begin(), pduel->game_field->core.must_select_cards.end(), pcard);
		if(it == pduel->game_field->core.must_select_cards.end())
			cv.push_back(pcard);
	}
	pduel->game_field->core.must_select_cards.clear();
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, 2, extraargs);
	lua_pushboolean(L, field::check_with_sum_greater_limit_m(cv, acc, 0, 0xffff, mcount));
	return 1;
}
int32_t scriptlib::group_select_with_sum_greater(lua_State *L) {
	check_action_permission(L);
	check_param_count(L, 4);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 3);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	if(playerid != 0 && playerid != 1)
		return 0;
	int32_t acc = (int32_t)lua_tointeger(L, 4);
	int32_t extraargs = lua_gettop(L) - 4;
	pduel->game_field->core.select_cards.assign(pgroup->container.begin(), pgroup->container.end());
	for(auto& pcard : pduel->game_field->core.must_select_cards) {
		auto it = std::remove(pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end(), pcard);
		pduel->game_field->core.select_cards.erase(it, pduel->game_field->core.select_cards.end());
	}
	card_vector cv(pduel->game_field->core.must_select_cards);
	int32_t mcount = (int32_t)cv.size();
	cv.insert(cv.end(), pduel->game_field->core.select_cards.begin(), pduel->game_field->core.select_cards.end());
	for(auto& pcard : cv)
		pcard->sum_param = pduel->lua->get_operation_value(pcard, 3, extraargs);
	if(!field::check_with_sum_greater_limit_m(cv, acc, 0, 0xffff, mcount)) {
		pduel->game_field->core.must_select_cards.clear();
		group* empty_group = pduel->new_group();
		interpreter::group2value(L, empty_group);
		return 1;
	}
	pduel->game_field->add_process(PROCESSOR_SELECT_SUM, 0, 0, 0, acc, playerid);
	return lua_yieldk(L, 0, (lua_KContext)pduel, [](lua_State *L, int32_t status, lua_KContext ctx) {
		duel* pduel = (duel*)ctx;
		group* pgroup = pduel->new_group();
		int32_t mcount = (int32_t)pduel->game_field->core.must_select_cards.size();
		for(int32_t i = mcount; i < pduel->game_field->returns.bvalue[0]; ++i) {
			card* pcard = pduel->game_field->core.select_cards[pduel->game_field->returns.bvalue[i + 1]];
			pgroup->container.insert(pcard);
		}
		pduel->game_field->core.must_select_cards.clear();
		interpreter::group2value(L, pgroup);
		return 1;
	});
}
int32_t scriptlib::group_get_min_group(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	if(pgroup->container.size() == 0)
		return 0;
	group* newgroup = pduel->new_group();
	int32_t min, op;
	int32_t extraargs = lua_gettop(L) - 2;
	auto cit = pgroup->container.begin();
	min = pduel->lua->get_operation_value(*cit, 2, extraargs);
	newgroup->container.insert(*cit);
	++cit;
	for(; cit != pgroup->container.end(); ++cit) {
		op = pduel->lua->get_operation_value(*cit, 2, extraargs);
		if(op == min)
			newgroup->container.insert(*cit);
		else if(op < min) {
			newgroup->container.clear();
			newgroup->container.insert(*cit);
			min = op;
		}
	}
	interpreter::group2value(L, newgroup);
	lua_pushinteger(L, min);
	return 2;
}
int32_t scriptlib::group_get_max_group(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	if(pgroup->container.size() == 0)
		return 0;
	group* newgroup = pduel->new_group();
	int32_t max, op;
	int32_t extraargs = lua_gettop(L) - 2;
	auto cit = pgroup->container.begin();
	max = pduel->lua->get_operation_value(*cit, 2, extraargs);
	newgroup->container.insert(*cit);
	++cit;
	for(; cit != pgroup->container.end(); ++cit) {
		op = pduel->lua->get_operation_value(*cit, 2, extraargs);
		if(op == max)
			newgroup->container.insert(*cit);
		else if(op > max) {
			newgroup->container.clear();
			newgroup->container.insert(*cit);
			max = op;
		}
	}
	interpreter::group2value(L, newgroup);
	lua_pushinteger(L, max);
	return 2;
}
int32_t scriptlib::group_get_sum(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	int32_t extraargs = lua_gettop(L) - 2;
	int32_t sum = 0;
	for(auto& pcard : pgroup->container) {
		sum += pduel->lua->get_operation_value(pcard, 2, extraargs);
	}
	lua_pushinteger(L, sum);
	return 1;
}
int32_t scriptlib::group_get_class_count(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	int32_t extraargs = lua_gettop(L) - 2;
	std::set<uint32_t> er;
	for(auto& pcard : pgroup->container) {
		er.insert(pduel->lua->get_operation_value(pcard, 2, extraargs));
	}
	lua_pushinteger(L, er.size());
	return 1;
}
int32_t scriptlib::group_remove(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	card* pexception = nullptr;
	if(!lua_isnil(L, 3)) {
		check_param(L, PARAM_TYPE_CARD, 3);
		pexception = *(card**) lua_touserdata(L, 3);
	}
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	uint32_t extraargs = lua_gettop(L) - 3;
	if(pgroup->is_readonly == GTYPE_READ_ONLY)
		return 0;
	pgroup->is_iterator_dirty = true;
	for (auto cit = pgroup->container.begin(); cit != pgroup->container.end();) {
		if((*cit) != pexception && pduel->lua->check_filter(L, *cit, 2, extraargs)) {
			cit = pgroup->container.erase(cit);
		}
		else
			++cit;
	}
	return 0;
}
int32_t scriptlib::group_merge(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_GROUP, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	group* mgroup = *(group**) lua_touserdata(L, 2);
	if(pgroup->is_readonly == GTYPE_READ_ONLY)
		return 0;
	pgroup->is_iterator_dirty = true;
	pgroup->container.insert(mgroup->container.begin(), mgroup->container.end());
	return 0;
}
int32_t scriptlib::group_sub(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_GROUP, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	group* sgroup = *(group**) lua_touserdata(L, 2);
	if(pgroup->is_readonly == GTYPE_READ_ONLY)
		return 0;
	pgroup->is_iterator_dirty = true;
	for (auto& pcard : sgroup->container) {
		pgroup->container.erase(pcard);
	}
	return 0;
}
int32_t scriptlib::group_equal(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_GROUP, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	group* sgroup = *(group**) lua_touserdata(L, 2);
	if(pgroup->container.size() != sgroup->container.size()) {
		lua_pushboolean(L, 0);
		return 1;
	}
	auto pit = pgroup->container.begin();
	auto sit = sgroup->container.begin();
	for (; pit != pgroup->container.end(); ++pit, ++sit) {
		if((*pit) != (*sit)) {
			lua_pushboolean(L, 0);
			return 1;
		}
	}
	lua_pushboolean(L, 1);
	return 1;
}
int32_t scriptlib::group_is_contains(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_CARD, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	card* pcard = *(card**) lua_touserdata(L, 2);
	if(pgroup->container.find(pcard) != pgroup->container.end())
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::group_search_card(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	uint32_t extraargs = lua_gettop(L) - 2;
	for (auto& pcard : pgroup->container) {
		if (pduel->lua->check_filter(L, pcard, 2, extraargs)) {
			interpreter::card2value(L, pcard);
			return 1;
		}
	}
	return 0;
}
int32_t scriptlib::group_get_bin_class_count(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_GROUP, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	group* pgroup = *(group**) lua_touserdata(L, 1);
	duel* pduel = pgroup->pduel;
	int32_t extraargs = lua_gettop(L) - 2;
	int32_t er = 0;
	for(auto& pcard : pgroup->container) {
		er |= pduel->lua->get_operation_value(pcard, 2, extraargs);
	}
	int32_t ans = 0;
	while(er) {
		er &= er - 1;
		++ans;
	}
	lua_pushinteger(L, ans);
	return 1;
}
int32_t scriptlib::group_meta_add(lua_State* L) {
	check_param_count(L, 2);
	if(!check_param(L, PARAM_TYPE_CARD, 1, TRUE) && !check_param(L, PARAM_TYPE_GROUP, 1, TRUE))
		return luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", 1);
	if(!check_param(L, PARAM_TYPE_CARD, 2, TRUE) && !check_param(L, PARAM_TYPE_GROUP, 2, TRUE))
		return luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", 2);
	duel* pduel = interpreter::get_duel_info(L);
	group* pgroup = pduel->new_group();
	if(check_param(L, PARAM_TYPE_CARD, 1, TRUE)) {
		card* ccard = *(card**) lua_touserdata(L, 1);
		pgroup->container.insert(ccard);
	} else if(check_param(L, PARAM_TYPE_GROUP, 1, TRUE)) {
		group* cgroup = *(group**) lua_touserdata(L, 1);
		for(auto cit = cgroup->container.begin(); cit != cgroup->container.end(); ++cit)
			pgroup->container.insert(*cit);
	}
	if(check_param(L, PARAM_TYPE_CARD, 2, TRUE)) {
		card* ccard = *(card**) lua_touserdata(L, 2);
		pgroup->container.insert(ccard);
	} else if(check_param(L, PARAM_TYPE_GROUP, 2, TRUE)) {
		group* cgroup = *(group**) lua_touserdata(L, 2);
		for(auto cit = cgroup->container.begin(); cit != cgroup->container.end(); ++cit)
			pgroup->container.insert(*cit);
	}
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::group_meta_sub(lua_State* L) {
	check_param_count(L, 2);
	if(!check_param(L, PARAM_TYPE_CARD, 1, TRUE) && !check_param(L, PARAM_TYPE_GROUP, 1, TRUE))
		return luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", 1);
	if(!check_param(L, PARAM_TYPE_CARD, 2, TRUE) && !check_param(L, PARAM_TYPE_GROUP, 2, TRUE))
		return luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", 2);
	duel* pduel = interpreter::get_duel_info(L);
	group* pgroup = pduel->new_group();
	if(check_param(L, PARAM_TYPE_CARD, 1, TRUE)) {
		card* ccard = *(card**) lua_touserdata(L, 1);
		pgroup->container.insert(ccard);
	} else if(check_param(L, PARAM_TYPE_GROUP, 1, TRUE)) {
		group* cgroup = *(group**) lua_touserdata(L, 1);
		for(auto cit = cgroup->container.begin(); cit != cgroup->container.end(); ++cit)
			pgroup->container.insert(*cit);
	}
	if(check_param(L, PARAM_TYPE_CARD, 2, TRUE)) {
		card* ccard = *(card**) lua_touserdata(L, 2);
		pgroup->container.erase(ccard);
	} else if(check_param(L, PARAM_TYPE_GROUP, 2, TRUE)) {
		group* cgroup = *(group**) lua_touserdata(L, 2);
		for(auto cit = cgroup->container.begin(); cit != cgroup->container.end(); ++cit)
			pgroup->container.erase(*cit);
	}
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::group_meta_band(lua_State* L) {
	check_param_count(L, 2);
	if(!check_param(L, PARAM_TYPE_CARD, 1, TRUE) && !check_param(L, PARAM_TYPE_GROUP, 1, TRUE))
		return luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", 1);
	if(!check_param(L, PARAM_TYPE_CARD, 2, TRUE) && !check_param(L, PARAM_TYPE_GROUP, 2, TRUE))
		return luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", 2);
	duel* pduel = interpreter::get_duel_info(L);
	group* pgroup = pduel->new_group();
	card_set check_set;
	if(check_param(L, PARAM_TYPE_CARD, 1, TRUE)) {
		card* ccard = *(card**) lua_touserdata(L, 1);
		check_set.insert(ccard);
	} else if(check_param(L, PARAM_TYPE_GROUP, 1, TRUE)) {
		group* cgroup = *(group**) lua_touserdata(L, 1);
		check_set = cgroup->container;
	}
	if(check_param(L, PARAM_TYPE_CARD, 2, TRUE)) {
		card* ccard = *(card**) lua_touserdata(L, 2);
		if(check_set.find(ccard) != check_set.end())
			pgroup->container.insert(ccard);
	} else if(check_param(L, PARAM_TYPE_GROUP, 2, TRUE)) {
		group* cgroup = *(group**) lua_touserdata(L, 2);
		for(auto cit = cgroup->container.begin(); cit != cgroup->container.end(); ++cit)
			if(check_set.find(*cit) != check_set.end())
				pgroup->container.insert(*cit);
	}
	interpreter::group2value(L, pgroup);
	return 1;
}
int32_t scriptlib::group_meta_bxor(lua_State* L) {
	check_param_count(L, 2);
	if(!check_param(L, PARAM_TYPE_CARD, 1, TRUE) && !check_param(L, PARAM_TYPE_GROUP, 1, TRUE))
		return luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", 1);
	if(!check_param(L, PARAM_TYPE_CARD, 2, TRUE) && !check_param(L, PARAM_TYPE_GROUP, 2, TRUE))
		return luaL_error(L, "Parameter %d should be \"Card\" or \"Group\".", 2);
	duel* pduel = interpreter::get_duel_info(L);
	group* pgroup = pduel->new_group();
	if(check_param(L, PARAM_TYPE_CARD, 1, TRUE)) {
		card* ccard = *(card**) lua_touserdata(L, 1);
		pgroup->container.insert(ccard);
	} else if(check_param(L, PARAM_TYPE_GROUP, 1, TRUE)) {
		group* cgroup = *(group**) lua_touserdata(L, 1);
		for(auto cit = cgroup->container.begin(); cit != cgroup->container.end(); ++cit)
			pgroup->container.insert(*cit);
	}
	if(check_param(L, PARAM_TYPE_CARD, 2, TRUE)) {
		card* ccard = *(card**) lua_touserdata(L, 2);
		if(pgroup->container.find(ccard) != pgroup->container.end())
			pgroup->container.erase(ccard);
		else
			pgroup->container.insert(ccard);
	} else if(check_param(L, PARAM_TYPE_GROUP, 2, TRUE)) {
		group* cgroup = *(group**) lua_touserdata(L, 2);
		for(auto cit = cgroup->container.begin(); cit != cgroup->container.end(); ++cit) {
			if(pgroup->container.find(*cit) != pgroup->container.end())
				pgroup->container.erase(*cit);
			else
				pgroup->container.insert(*cit);
		}
	}
	interpreter::group2value(L, pgroup);
	return 1;
}

static const struct luaL_Reg grouplib[] = {
	{ "CreateGroup", scriptlib::group_new },
	{ "KeepAlive", scriptlib::group_keep_alive },
	{ "DeleteGroup", scriptlib::group_delete },
	{ "Clone", scriptlib::group_clone },
	{ "FromCards", scriptlib::group_from_cards },
	{ "Clear", scriptlib::group_clear },
	{ "AddCard", scriptlib::group_add_card },
	{ "RemoveCard", scriptlib::group_remove_card },
	{ "GetNext", scriptlib::group_get_next },
	{ "GetFirst", scriptlib::group_get_first },
	{ "GetCount", scriptlib::group_get_count },
	{ "__len", scriptlib::group_get_count },
	{ "Filter", scriptlib::group_filter },
	{ "FilterCount", scriptlib::group_filter_count },
	{ "FilterSelect", scriptlib::group_filter_select },
	{ "Select", scriptlib::group_select },
	{ "SelectUnselect", scriptlib::group_select_unselect },
	{ "RandomSelect", scriptlib::group_random_select },
	{ "CancelableSelect", scriptlib::group_cancelable_select },
	{ "IsExists", scriptlib::group_is_exists },
	{ "CheckWithSumEqual", scriptlib::group_check_with_sum_equal },
	{ "SelectWithSumEqual", scriptlib::group_select_with_sum_equal },
	{ "CheckWithSumGreater", scriptlib::group_check_with_sum_greater },
	{ "SelectWithSumGreater", scriptlib::group_select_with_sum_greater },
	{ "GetMinGroup", scriptlib::group_get_min_group },
	{ "GetMaxGroup", scriptlib::group_get_max_group },
	{ "GetSum", scriptlib::group_get_sum },
	{ "GetClassCount", scriptlib::group_get_class_count },
	{ "Remove", scriptlib::group_remove },
	{ "Merge", scriptlib::group_merge },
	{ "Sub", scriptlib::group_sub },
	{ "Equal", scriptlib::group_equal },
	{ "IsContains", scriptlib::group_is_contains },
	{ "SearchCard", scriptlib::group_search_card },
	{ "GetBinClassCount", scriptlib::group_get_bin_class_count },
	{ "__add", scriptlib::group_meta_add },
	{ "__bor", scriptlib::group_meta_add },
	{ "__sub", scriptlib::group_meta_sub },
	{ "__band", scriptlib::group_meta_band },
	{ "__bxor", scriptlib::group_meta_bxor },
	{ nullptr, nullptr }
};
void scriptlib::open_grouplib(lua_State *L) {
	luaL_newlib(L, grouplib);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Group");
}
