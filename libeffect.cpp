/*
 * libeffect.cpp
 *
 *  Created on: 2010-7-20
 *      Author: Argon
 */

#include "scriptlib.h"
#include "duel.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"

int32_t scriptlib::get_effect_property(lua_State* L, effect_member type) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**)lua_touserdata(L, 1);
	lua_Integer value{};
	if (peffect) {
		switch (type) {
		case MEMBER_CATEGORY:
			value = (lua_Integer)peffect->category;
			break;
		case MEMBER_CODE:
			value = peffect->code;
			break;
		case MEMBER_DESCRIPTION:
			value = peffect->description;
			break;
		case MEMBER_ID:
			value = peffect->id;
			break;
		case MEMBER_RANGE:
			value = peffect->range;
			break;
		case MEMBER_TYPE:
			value = peffect->type;
			break;
		}
	}
	lua_pushinteger(L, value);
	return 1;
}
int32_t scriptlib::is_effect_has_property(lua_State* L, effect_member type) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**)lua_touserdata(L, 1);
	uint64_t value{};
	if (peffect) {
		switch (type) {
		case MEMBER_CATEGORY:
			value = peffect->category;
			break;
		case MEMBER_RANGE:
			value = peffect->range;
			break;
		case MEMBER_TYPE:
			value = peffect->type;
			break;
		}
	}
	uint64_t x = lua_tointeger(L, 2);
	if (value & x)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::effect_new(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_CARD, 1);
	card* pcard = *(card**) lua_touserdata(L, 1);
	duel* pduel = pcard->pduel;
	effect* peffect = pduel->new_effect();
	peffect->effect_owner = pduel->game_field->core.reason_player;
	peffect->owner = pcard;
	interpreter::effect2value(L, peffect);
	return 1;
}
int32_t scriptlib::effect_newex(lua_State *L) {
	duel* pduel = interpreter::get_duel_info(L);
	effect* peffect = pduel->new_effect();
	peffect->effect_owner = 0;
	peffect->owner = pduel->game_field->temp_card;
	interpreter::effect2value(L, peffect);
	return 1;
}
int32_t scriptlib::effect_clone(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	effect* ceffect = peffect->clone();
	interpreter::effect2value(L, ceffect);
	return 1;
}
int32_t scriptlib::effect_reset(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->owner == 0 || peffect->handler == 0)
		return 0;
	if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY))
		peffect->pduel->game_field->remove_effect(peffect);
	else
		peffect->handler->remove_effect(peffect);
	return 0;
}
int32_t scriptlib::effect_get_field_id(lua_State *L) {
	return get_effect_property(L, MEMBER_ID);
}
int32_t scriptlib::effect_set_description(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32_t v = (uint32_t)lua_tointeger(L, 2);
	peffect->description = v;
	return 0;
}
int32_t scriptlib::effect_set_code(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32_t v = (uint32_t)lua_tointeger(L, 2);
	peffect->code = v;
	return 0;
}
int32_t scriptlib::effect_set_range(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32_t v = (uint32_t)lua_tointeger(L, 2);
	peffect->range = v;
	return 0;
}
int32_t scriptlib::effect_set_target_range(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32_t s = (uint32_t)lua_tointeger(L, 2);
	uint32_t o = (uint32_t)lua_tointeger(L, 3);
	peffect->s_range = s;
	peffect->o_range = o;
	peffect->flag[0] &= ~EFFECT_FLAG_ABSOLUTE_TARGET;
	return 0;
}
int32_t scriptlib::effect_set_absolute_range(lua_State *L) {
	check_param_count(L, 4);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	uint32_t s = (uint32_t)lua_tointeger(L, 3);
	uint32_t o = (uint32_t)lua_tointeger(L, 4);
	if(playerid == 0) {
		peffect->s_range = s;
		peffect->o_range = o;
	} else {
		peffect->s_range = o;
		peffect->o_range = s;
	}
	peffect->flag[0] |= EFFECT_FLAG_ABSOLUTE_TARGET;
	return 0;
}
int32_t scriptlib::effect_set_count_limit(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32_t v = (uint32_t)lua_tointeger(L, 2);
	uint32_t code = 0;
	if(lua_gettop(L) >= 3)
		code = (uint32_t)lua_tointeger(L, 3);
	if(v == 0)
		v = 1;
	if(code == EFFECT_COUNT_CODE_CHAIN)
		code = EFFECT_COUNT_CODE_CHAIN | EFFECT_COUNT_CODE_SINGLE;
	peffect->flag[0] |= EFFECT_FLAG_COUNT_LIMIT;
	peffect->count_limit = v;
	peffect->count_limit_max = v;
	peffect->count_code = code;
	return 0;
}
int32_t scriptlib::effect_set_reset(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32_t v = (uint32_t)lua_tointeger(L, 2);
	int32_t c = (int32_t)lua_tointeger(L, 3);
	if(c == 0)
		c = 1;
	if(v & (RESET_PHASE) && !(v & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		v |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	peffect->reset_flag = v;
	peffect->reset_count = c;
	return 0;
}
int32_t scriptlib::effect_set_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32_t v = (uint32_t)lua_tointeger(L, 2);
	if (v & EFFECT_TYPE_ACTIVATE) {
		v = EFFECT_TYPE_FIELD | EFFECT_TYPE_ACTIVATE;
		peffect->range = LOCATION_SZONE + LOCATION_FZONE + LOCATION_HAND;
	}
	else if(v & EFFECT_TYPE_FLIP) {
		peffect->code = EVENT_FLIP;
		if (v & EFFECT_TYPE_TRIGGER_O) {
			v = EFFECT_TYPE_SINGLE | EFFECT_TYPE_FLIP | EFFECT_TYPE_TRIGGER_O;
			peffect->flag[0] |= EFFECT_FLAG_DELAY;
		}
		else {
			v = EFFECT_TYPE_SINGLE | EFFECT_TYPE_FLIP | EFFECT_TYPE_TRIGGER_F;
		}
	}
	else if (v & (EFFECT_TYPE_IGNITION | EFFECT_TYPE_QUICK_O | EFFECT_TYPE_QUICK_F)) {
			v |= EFFECT_TYPE_FIELD;
	}
	if (v & (EFFECT_TYPES_CHAIN_LINK | EFFECT_TYPE_CONTINUOUS))
		v |= EFFECT_TYPE_ACTIONS;
	else
		v &= ~EFFECT_TYPE_ACTIONS;
	peffect->type = v;
	return 0;
}
int32_t scriptlib::effect_set_property(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint64_t v1 = lua_tointeger(L, 2);
	uint64_t v2 = lua_tointeger(L, 3);
	peffect->flag[0] = (peffect->flag[0] & INTERNAL_FLAGS) | (v1 & ~INTERNAL_FLAGS);
	peffect->flag[1] = v2;
	return 0;
}
int32_t scriptlib::effect_set_label(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	peffect->label.clear();
	for(int32_t i = 2; i <= lua_gettop(L); ++i) {
		peffect->label.push_back(lua_tointeger(L, i));
	}
	return 0;
}
int32_t scriptlib::effect_set_label_object(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(lua_isnil(L, 2)) {
		peffect->label_object = 0;
		peffect->object_type = PARAM_TYPE_INT;
		return 0;
	}
	if(check_param(L, PARAM_TYPE_CARD, 2, TRUE)) {
		card* p = *(card**)lua_touserdata(L, 2);
		peffect->label_object = p->ref_handle;
		peffect->object_type = PARAM_TYPE_CARD;
	} else if(check_param(L, PARAM_TYPE_EFFECT, 2, TRUE)) {
		effect* p = *(effect**)lua_touserdata(L, 2);
		peffect->label_object = p->ref_handle;
		peffect->object_type = PARAM_TYPE_EFFECT;
	} else if(check_param(L, PARAM_TYPE_GROUP, 2, TRUE)) {
		group* p = *(group**)lua_touserdata(L, 2);
		peffect->label_object = p->ref_handle;
		peffect->object_type = PARAM_TYPE_GROUP;
	} else
		return luaL_error(L, "Parameter 2 should be \"Card\" or \"Effect\" or \"Group\".");
	return 0;
}
int32_t scriptlib::effect_set_category(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint64_t v = (uint64_t)lua_tointeger(L, 2);
	peffect->category = v;
	return 0;
}
int32_t scriptlib::effect_set_hint_timing(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32_t vs = (uint32_t)lua_tointeger(L, 2);
	uint32_t vo = vs;
	if(lua_gettop(L) >= 3)
		vo = (uint32_t)lua_tointeger(L, 3);
	peffect->hint_timing[0] = vs;
	peffect->hint_timing[1] = vo;
	return 0;
}
int32_t scriptlib::effect_set_condition(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->condition)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->condition);
	peffect->condition = interpreter::get_function_handle(L, 2);
	return 0;
}
int32_t scriptlib::effect_set_target(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->target)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->target);
	peffect->target = interpreter::get_function_handle(L, 2);
	return 0;
}
int32_t scriptlib::effect_set_cost(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->cost)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->cost);
	peffect->cost = interpreter::get_function_handle(L, 2);
	return 0;
}
int32_t scriptlib::effect_set_value(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->value && peffect->is_flag(EFFECT_FLAG_FUNC_VALUE))
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->value);
	if (lua_isfunction(L, 2)) {
		peffect->value = interpreter::get_function_handle(L, 2);
		peffect->flag[0] |= EFFECT_FLAG_FUNC_VALUE;
	} else {
		peffect->flag[0] &= ~EFFECT_FLAG_FUNC_VALUE;
		if(lua_isboolean(L, 2))
			peffect->value = lua_toboolean(L, 2);
		else if(lua_isinteger(L, 2))
			peffect->value = (int32_t)lua_tointeger(L, 2);
		else
			peffect->value = (int32_t)lua_tonumber(L, 2);
	}
	return 0;
}
int32_t scriptlib::effect_set_operation(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->operation)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->operation);
	if(!lua_isnil(L, 2)) {
		check_param(L, PARAM_TYPE_FUNCTION, 2);
		peffect->operation = interpreter::get_function_handle(L, 2);
	} else
		peffect->operation = 0;
	return 0;
}
int32_t scriptlib::effect_set_owner_player(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	int32_t p = (int32_t)lua_tointeger(L, 2);
	if(p != 0 && p != 1)
		return 0;
	peffect->effect_owner = p;
	return 0;
}
int32_t scriptlib::effect_get_description(lua_State *L) {
	return get_effect_property(L, MEMBER_DESCRIPTION);
}
int32_t scriptlib::effect_get_code(lua_State *L) {
	return get_effect_property(L, MEMBER_CODE);
}
int32_t scriptlib::effect_get_type(lua_State *L) {
	return get_effect_property(L, MEMBER_TYPE);
}
int32_t scriptlib::effect_get_property(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if (peffect) {
		lua_pushinteger(L, peffect->flag[0]);
		lua_pushinteger(L, peffect->flag[1]);
		return 2;
	}
	return 0;
}
int32_t scriptlib::effect_get_label(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if (peffect) {
		if(peffect->label.empty()) {
			lua_pushinteger(L, 0);
			return 1;
		}
		for(const auto& lab : peffect->label)
			lua_pushinteger(L, lab);
		return (int32_t)peffect->label.size();
	}
	return 0;
}
int32_t scriptlib::effect_get_label_object(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if (!peffect->label_object) {
		lua_pushnil(L);
		return 1;
	}
	lua_rawgeti(L, LUA_REGISTRYINDEX, peffect->label_object);
	if(lua_isuserdata(L, -1))
		return 1;
	else {
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}
}
int32_t scriptlib::effect_get_category(lua_State *L) {
	return get_effect_property(L, MEMBER_CATEGORY);
}
int32_t scriptlib::effect_get_range(lua_State* L) {
	return get_effect_property(L, MEMBER_RANGE);
}
int32_t scriptlib::effect_get_owner(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::card2value(L, peffect->get_owner());
	return 1;
}
int32_t scriptlib::effect_get_handler(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::card2value(L, peffect->get_handler());
	return 1;
}
int32_t scriptlib::effect_get_owner_player(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushinteger(L, peffect->get_owner_player());
	return 1;
}
int32_t scriptlib::effect_get_handler_player(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushinteger(L, peffect->get_handler_player());
	return 1;
}
int32_t scriptlib::effect_get_condition(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::function2value(L, peffect->condition);
	return 1;
}
int32_t scriptlib::effect_get_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::function2value(L, peffect->target);
	return 1;
}
int32_t scriptlib::effect_get_cost(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::function2value(L, peffect->cost);
	return 1;
}
int32_t scriptlib::effect_get_value(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->is_flag(EFFECT_FLAG_FUNC_VALUE))
		interpreter::function2value(L, peffect->value);
	else
		lua_pushinteger(L, (int32_t)peffect->value);
	return 1;
}
int32_t scriptlib::effect_get_operation(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::function2value(L, peffect->operation);
	return 1;
}
int32_t scriptlib::effect_get_active_type(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushinteger(L, peffect->get_active_type());
	return 1;
}
int32_t scriptlib::effect_is_active_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32_t type = (uint32_t)lua_tointeger(L, 2);
	if(peffect->get_active_type() & type)
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::effect_is_has_property(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint64_t tflag1 = lua_tointeger(L, 2);
	uint64_t tflag2 = lua_tointeger(L, 3);
	if (peffect && (!tflag1 || (peffect->flag[0] & tflag1)) && (!tflag2 || (peffect->flag[1] & tflag2)))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32_t scriptlib::effect_is_has_category(lua_State *L) {
	return is_effect_has_property(L, MEMBER_CATEGORY);
}
int32_t scriptlib::effect_is_has_type(lua_State *L) {
	return is_effect_has_property(L, MEMBER_TYPE);
}
int32_t scriptlib::effect_is_has_range(lua_State* L) {
	return is_effect_has_property(L, MEMBER_RANGE);
}
int32_t scriptlib::effect_is_activatable(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	int32_t playerid = (int32_t)lua_tointeger(L, 2);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32_t neglect_loc = 0;
	uint32_t neglect_target = 0;
	if(lua_gettop(L) > 2) {
		neglect_loc = lua_toboolean(L, 3);
		if (lua_gettop(L) > 3)
			neglect_target = lua_toboolean(L, 4);
	}
	lua_pushboolean(L, peffect->is_activateable(playerid, peffect->pduel->game_field->nil_event, 0, 0, neglect_target, neglect_loc));
	return 1;
}
int32_t scriptlib::effect_is_activated(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushboolean(L, (peffect->type & EFFECT_TYPES_CHAIN_LINK));
	return 1;
}
int32_t scriptlib::effect_is_cost_checked(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**)lua_touserdata(L, 1);
	lua_pushboolean(L, peffect->cost_checked);
	return 1;
}
int32_t scriptlib::effect_set_cost_check(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint8_t cost_check = lua_toboolean(L, 2);
	peffect->cost_checked = cost_check;
	return 0;
}
int32_t scriptlib::effect_get_activate_location(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushinteger(L, peffect->active_location);
	return 1;
}
int32_t scriptlib::effect_get_activate_sequence(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushinteger(L, peffect->active_sequence);
	return 1;
}
int32_t scriptlib::effect_check_count_limit(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	int32_t p = (int32_t)lua_tointeger(L, 2);
	lua_pushboolean(L, peffect->check_count_limit(p));
	return 1;
}
int32_t scriptlib::effect_use_count_limit(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	int32_t p = (int32_t)lua_tointeger(L, 2);
	int32_t count = 1;
	int32_t oath_only = 0;
	uint32_t code = peffect->count_code;
	if(lua_gettop(L) > 2) {
		count = (int32_t)lua_tointeger(L, 3);
		if (lua_gettop(L) > 3)
			oath_only = lua_toboolean(L, 4);
	}
	if (!oath_only || code & EFFECT_COUNT_CODE_OATH)
		while(count) {
			peffect->dec_count(p);
			--count;
		}
	return 0;
}

static const struct luaL_Reg effectlib[] = {
	{ "CreateEffect", scriptlib::effect_new },
	{ "GlobalEffect", scriptlib::effect_newex },
	{ "Clone", scriptlib::effect_clone },
	{ "Reset", scriptlib::effect_reset },
	{ "GetFieldID", scriptlib::effect_get_field_id },
	{ "SetDescription", scriptlib::effect_set_description },
	{ "SetCode", scriptlib::effect_set_code },
	{ "SetRange", scriptlib::effect_set_range },
	{ "SetTargetRange", scriptlib::effect_set_target_range },
	{ "SetAbsoluteRange", scriptlib::effect_set_absolute_range },
	{ "SetCountLimit", scriptlib::effect_set_count_limit },
	{ "SetReset", scriptlib::effect_set_reset },
	{ "SetType", scriptlib::effect_set_type },
	{ "SetProperty", scriptlib::effect_set_property },
	{ "SetLabel", scriptlib::effect_set_label },
	{ "SetLabelObject", scriptlib::effect_set_label_object },
	{ "SetCategory", scriptlib::effect_set_category },
	{ "SetHintTiming", scriptlib::effect_set_hint_timing },
	{ "SetCondition", scriptlib::effect_set_condition },
	{ "SetTarget", scriptlib::effect_set_target },
	{ "SetCost", scriptlib::effect_set_cost },
	{ "SetValue", scriptlib::effect_set_value },
	{ "SetOperation", scriptlib::effect_set_operation },
	{ "SetOwnerPlayer", scriptlib::effect_set_owner_player },
	{ "GetDescription", scriptlib::effect_get_description },
	{ "GetCode", scriptlib::effect_get_code },
	{ "GetType", scriptlib::effect_get_type },
	{ "GetProperty", scriptlib::effect_get_property },
	{ "GetLabel", scriptlib::effect_get_label },
	{ "GetLabelObject", scriptlib::effect_get_label_object },
	{ "GetCategory", scriptlib::effect_get_category },
	{ "GetRange", scriptlib::effect_get_range },
	{ "GetOwner", scriptlib::effect_get_owner },
	{ "GetHandler", scriptlib::effect_get_handler },
	{ "GetCondition", scriptlib::effect_get_condition },
	{ "GetTarget", scriptlib::effect_get_target },
	{ "GetCost", scriptlib::effect_get_cost },
	{ "GetValue", scriptlib::effect_get_value },
	{ "GetOperation", scriptlib::effect_get_operation },
	{ "GetActiveType", scriptlib::effect_get_active_type },
	{ "IsActiveType", scriptlib::effect_is_active_type },
	{ "GetOwnerPlayer", scriptlib::effect_get_owner_player },
	{ "GetHandlerPlayer", scriptlib::effect_get_handler_player },
	{ "IsHasProperty", scriptlib::effect_is_has_property },
	{ "IsHasCategory", scriptlib::effect_is_has_category },
	{ "IsHasType", scriptlib::effect_is_has_type },
	{ "IsHasRange", scriptlib::effect_is_has_range },
	{ "IsActivatable", scriptlib::effect_is_activatable },
	{ "IsActivated", scriptlib::effect_is_activated },
	{ "IsCostChecked", scriptlib::effect_is_cost_checked },
	{ "SetCostCheck", scriptlib::effect_set_cost_check },
	{ "GetActivateLocation", scriptlib::effect_get_activate_location },
	{ "GetActivateSequence", scriptlib::effect_get_activate_sequence },
	{ "CheckCountLimit", scriptlib::effect_check_count_limit },
	{ "UseCountLimit", scriptlib::effect_use_count_limit },
	{ nullptr, nullptr }
};
void scriptlib::open_effectlib(lua_State *L) {
	luaL_newlib(L, effectlib);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_setglobal(L, "Effect");
}
