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

int32 scriptlib::effect_new(lua_State *L) {
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
int32 scriptlib::effect_newex(lua_State *L) {
	duel* pduel = interpreter::get_duel_info(L);
	effect* peffect = pduel->new_effect();
	peffect->effect_owner = 0;
	peffect->owner = pduel->game_field->temp_card;
	interpreter::effect2value(L, peffect);
	return 1;
}
int32 scriptlib::effect_clone(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	effect* ceffect = peffect->clone();
	interpreter::effect2value(L, ceffect);
	return 1;
}
int32 scriptlib::effect_reset(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->owner == 0)
		return 0;
	if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY))
		peffect->pduel->game_field->remove_effect(peffect);
	else
		peffect->handler->remove_effect(peffect);
	return 0;
}
int32 scriptlib::effect_get_field_id(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushinteger(L, peffect->id);
	return 1;
}
int32 scriptlib::effect_set_description(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 v = lua_tointeger(L, 2);
	peffect->description = v;
	return 0;
}
int32 scriptlib::effect_set_code(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 v = lua_tointeger(L, 2);
	peffect->code = v;
	return 0;
}
int32 scriptlib::effect_set_range(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 v = lua_tointeger(L, 2);
	peffect->range = v;
	return 0;
}
int32 scriptlib::effect_set_target_range(lua_State *L) {
	check_param_count(L, 3);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 s = lua_tointeger(L, 2);
	uint32 o = lua_tointeger(L, 3);
	peffect->s_range = s;
	peffect->o_range = o;
	peffect->flag[0] &= ~EFFECT_FLAG_ABSOLUTE_TARGET;
	return 0;
}
int32 scriptlib::effect_set_absolute_range(lua_State *L) {
	check_param_count(L, 4);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 playerid = lua_tointeger(L, 2);
	uint32 s = lua_tointeger(L, 3);
	uint32 o = lua_tointeger(L, 4);
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
int32 scriptlib::effect_set_count_limit(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 v = lua_tointeger(L, 2);
	uint32 code = 0;
	if(lua_gettop(L) >= 3)
		code = lua_tointeger(L, 3);
	if(v == 0)
		v = 1;
	peffect->flag[0] |= EFFECT_FLAG_COUNT_LIMIT;
	peffect->count_limit = v;
	peffect->count_limit_max = v;
	peffect->count_code = code;
	return 0;
}
int32 scriptlib::effect_set_reset(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 v = lua_tointeger(L, 2);
	uint32 c = lua_tointeger(L, 3);
	if(c == 0)
		c = 1;
	if(v & (RESET_PHASE) && !(v & (RESET_SELF_TURN | RESET_OPPO_TURN)))
		v |= (RESET_SELF_TURN | RESET_OPPO_TURN);
	peffect->reset_flag = v;
	peffect->reset_count = c;
	return 0;
}
int32 scriptlib::effect_set_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 v = lua_tointeger(L, 2);
	if (v & 0x0ff0)
		v |= EFFECT_TYPE_ACTIONS;
	else
		v &= ~EFFECT_TYPE_ACTIONS;
	if(v & (EFFECT_TYPE_ACTIVATE | EFFECT_TYPE_IGNITION | EFFECT_TYPE_QUICK_O | EFFECT_TYPE_QUICK_F))
		v |= EFFECT_TYPE_FIELD;
	if(v & EFFECT_TYPE_ACTIVATE)
		peffect->range = LOCATION_SZONE + LOCATION_FZONE + LOCATION_HAND;
	if(v & EFFECT_TYPE_FLIP) {
		peffect->code = EVENT_FLIP;
		if(!(v & EFFECT_TYPE_TRIGGER_O))
			v |= EFFECT_TYPE_TRIGGER_F;
	}
	peffect->type = v;
	return 0;
}
int32 scriptlib::effect_set_property(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 v1 = lua_tointeger(L, 2);
	uint32 v2 = lua_tointeger(L, 3);
	peffect->flag[0] = (peffect->flag[0] & 0x4f) | (v1 & ~0x4f);
	peffect->flag[1] = v2;
	return 0;
}
int32 scriptlib::effect_set_label(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 v = lua_tointeger(L, 2);
	peffect->label = v;
	return 0;
}
int32 scriptlib::effect_set_label_object(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(lua_isnil(L, 2)) {
		peffect->label_object = 0;
		return 0;
	}
	if(!lua_isuserdata(L, 2))
		luaL_error(L, "Parameter 2 should be \"Card\" or \"Effect\" or \"Group\".");
	void* p = *(void**)lua_touserdata(L, 2);
	peffect->label_object = p;
	return 0;
}
int32 scriptlib::effect_set_category(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 v = lua_tointeger(L, 2);
	peffect->category = v;
	return 0;
}
int32 scriptlib::effect_set_hint_timing(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 vs = lua_tointeger(L, 2);
	uint32 vo = vs;
	if(lua_gettop(L) >= 3)
		vo = lua_tointeger(L, 3);
	peffect->hint_timing[0] = vs;
	peffect->hint_timing[1] = vo;
	return 0;
}
int32 scriptlib::effect_set_condition(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->condition)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->condition);
	peffect->condition = interpreter::get_function_handle(L, 2);
	return 0;
}
int32 scriptlib::effect_set_target(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->target)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->target);
	peffect->target = interpreter::get_function_handle(L, 2);
	return 0;
}
int32 scriptlib::effect_set_cost(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	check_param(L, PARAM_TYPE_FUNCTION, 2);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->cost)
		luaL_unref(L, LUA_REGISTRYINDEX, peffect->cost);
	peffect->cost = interpreter::get_function_handle(L, 2);
	return 0;
}
int32 scriptlib::effect_set_value(lua_State *L) {
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
		else
			peffect->value = round(lua_tonumber(L, 2));
	}
	return 0;
}
int32 scriptlib::effect_set_operation(lua_State *L) {
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
int32 scriptlib::effect_set_owner_player(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 p = lua_tointeger(L, 2);
	if(p != 0 && p != 1)
		return 0;
	peffect->effect_owner = p;
	return 0;
}
int32 scriptlib::effect_get_description(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if (peffect) {
		lua_pushinteger(L, peffect->description);
		return 1;
	}
	return 0;
}
int32 scriptlib::effect_get_code(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if (peffect) {
		lua_pushinteger(L, peffect->code);
		return 1;
	}
	return 0;
}
int32 scriptlib::effect_get_type(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if (peffect) {
		lua_pushinteger(L, peffect->type);
		return 1;
	}
	return 0;
}
int32 scriptlib::effect_get_property(lua_State *L) {
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
int32 scriptlib::effect_get_label(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if (peffect) {
		lua_pushinteger(L, peffect->label);
		return 1;
	}
	return 0;
}
int32 scriptlib::effect_get_label_object(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if (!peffect->label_object) {
		lua_pushnil(L);
		return 1;
	}
	int32 type = *(int32*)peffect->label_object;
	if(type == 1)
		interpreter::card2value(L, (card*)peffect->label_object);
	else if(type == 2)
		interpreter::group2value(L, (group*)peffect->label_object);
	else if(type == 3)
		interpreter::effect2value(L, (effect*)peffect->label_object);
	else lua_pushnil(L);
	return 1;
}
int32 scriptlib::effect_get_category(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if (peffect) {
		lua_pushinteger(L, peffect->category);
		return 1;
	}
	return 0;
}
int32 scriptlib::effect_get_owner(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::card2value(L, peffect->owner);
	return 1;
}
int32 scriptlib::effect_get_handler(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::card2value(L, peffect->get_handler());
	return 1;
}
int32 scriptlib::effect_get_owner_player(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushinteger(L, peffect->get_owner_player());
	return 1;
}
int32 scriptlib::effect_get_handler_player(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushinteger(L, peffect->get_handler_player());
	return 1;
}
int32 scriptlib::effect_get_condition(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::function2value(L, peffect->condition);
	return 1;
}
int32 scriptlib::effect_get_target(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::function2value(L, peffect->target);
	return 1;
}
int32 scriptlib::effect_get_cost(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::function2value(L, peffect->cost);
	return 1;
}
int32 scriptlib::effect_get_value(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	if(peffect->is_flag(EFFECT_FLAG_FUNC_VALUE))
		interpreter::function2value(L, peffect->value);
	else
		lua_pushinteger(L, (int32)peffect->value);
	return 1;
}
int32 scriptlib::effect_get_operation(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	interpreter::function2value(L, peffect->operation);
	return 1;
}
// active_type is set in add_chain()
int32 scriptlib::effect_get_active_type(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 atype;
	if(peffect->type & 0x7f0) {
		if(peffect->active_type)
			atype = peffect->active_type;
		else if((peffect->type & EFFECT_TYPE_ACTIVATE) && (peffect->get_handler()->data.type & TYPE_PENDULUM))
			atype = TYPE_PENDULUM + TYPE_SPELL;
		else
			atype = peffect->get_handler()->get_type();
	} else
		atype = peffect->owner->get_type();
	lua_pushinteger(L, atype);
	return 1;
}
int32 scriptlib::effect_is_active_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 tpe = lua_tointeger(L, 2);
	uint32 atype;
	if(peffect->type & 0x7f0) {
		if(peffect->active_type)
			atype = peffect->active_type;
		else if((peffect->type & EFFECT_TYPE_ACTIVATE) && (peffect->get_handler()->data.type & TYPE_PENDULUM))
			atype = TYPE_PENDULUM + TYPE_SPELL;
		else
			atype = peffect->get_handler()->get_type();
	} else
		atype = peffect->owner->get_type();
	lua_pushboolean(L, atype & tpe);
	return 1;
}
int32 scriptlib::effect_is_has_property(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 tflag1 = lua_tointeger(L, 2);
	uint32 tflag2 = lua_tointeger(L, 3);
	if (peffect && (!tflag1 || (peffect->flag[0] & tflag1)) && (!tflag2 || (peffect->flag[1] & tflag2)))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32 scriptlib::effect_is_has_category(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 tcate = lua_tointeger(L, 2);
	if (peffect && (peffect->category & tcate))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32 scriptlib::effect_is_has_type(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 ttype = lua_tointeger(L, 2);
	if (peffect && (peffect->type & ttype))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}
int32 scriptlib::effect_is_activatable(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	uint32 playerid = lua_tointeger(L, 2);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 neglect_loc = 0;
	uint32 neglect_target = 0;
	if(lua_gettop(L) > 2) {
		neglect_loc = lua_toboolean(L, 3);
		if (lua_gettop(L) > 3)
			neglect_target = lua_toboolean(L, 4);
	}
	lua_pushboolean(L, peffect->is_activateable(playerid, peffect->pduel->game_field->nil_event, 0, 0, neglect_target, neglect_loc));
	return 1;
}
int32 scriptlib::effect_is_activated(lua_State * L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushboolean(L, (peffect->type & 0x7f0));
	return 1;
}
int32 scriptlib::effect_get_activate_location(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushinteger(L, peffect->active_location);
	return 1;
}
int32 scriptlib::effect_get_activate_sequence(lua_State *L) {
	check_param_count(L, 1);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	lua_pushinteger(L, peffect->active_sequence);
	return 1;
}
int32 scriptlib::effect_check_count_limit(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 p = lua_tointeger(L, 2);
	lua_pushboolean(L, peffect->check_count_limit(p));
	return 1;
}
int32 scriptlib::effect_use_count_limit(lua_State *L) {
	check_param_count(L, 2);
	check_param(L, PARAM_TYPE_EFFECT, 1);
	effect* peffect = *(effect**) lua_touserdata(L, 1);
	uint32 p = lua_tointeger(L, 2);
	uint32 count = 1;
	uint32 oath_only = 0;
	uint32 code = peffect->count_code;
	if(lua_gettop(L) > 2) {
		count = lua_tointeger(L, 3);
		if (lua_gettop(L) > 3)
			oath_only = lua_toboolean(L, 4);
	}
	if (!oath_only || code & EFFECT_COUNT_CODE_OATH)
		while(count) {
			peffect->dec_count(p);
			count--;
		}
	return 0;
}
