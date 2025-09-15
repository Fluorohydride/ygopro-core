/*
 * scriptlib.cpp
 *
 *  Created on: 2010-7-29
 *      Author: Argon
 */
#include "scriptlib.h"
#include "duel.h"

static int32_t check_data_type(lua_State* L, int32_t index, const char* tname) {
	luaL_checkstack(L, 2, nullptr);
	int32_t result = FALSE;
	if(lua_getmetatable(L, index)) {
		lua_getglobal(L, tname);
		if(lua_rawequal(L, -1, -2))
			result = TRUE;
		lua_pop(L, 2);
	}
	return result;
}
int32_t scriptlib::check_param(lua_State* L, int32_t param_type, int32_t index, int32_t retfalse) {
	switch (param_type) {
	case PARAM_TYPE_CARD: {
		if(lua_isuserdata(L, index) && check_data_type(L, index, "Card"))
			return TRUE;
		if(retfalse)
			return FALSE;
		return luaL_error(L, "Parameter %d should be \"Card\".", index);
		break;
	}
	case PARAM_TYPE_GROUP: {
		if(lua_isuserdata(L, index) && check_data_type(L, index, "Group"))
			return TRUE;
		if(retfalse)
			return FALSE;
		return luaL_error(L, "Parameter %d should be \"Group\".", index);
		break;
	}
	case PARAM_TYPE_EFFECT: {
		if(lua_isuserdata(L, index) && check_data_type(L, index, "Effect"))
			return TRUE;
		if(retfalse)
			return FALSE;
		return luaL_error(L, "Parameter %d should be \"Effect\".", index);
		break;
	}
	case PARAM_TYPE_FUNCTION: {
		if(lua_isfunction(L, index))
			return TRUE;
		if(retfalse)
			return FALSE;
		return luaL_error(L, "Parameter %d should be \"Function\".", index);
		break;
	}
	case PARAM_TYPE_STRING: {
		if(lua_isstring(L, index))
			return TRUE;
		if(retfalse)
			return FALSE;
		return luaL_error(L, "Parameter %d should be \"String\".", index);
		break;
	}
	}
	return FALSE;
}

int32_t scriptlib::check_param_count(lua_State* L, int32_t count) {
	if (lua_gettop(L) < count)
		return luaL_error(L, "%d Parameters are needed.", count);
	return TRUE;
}
int32_t scriptlib::check_action_permission(lua_State* L) {
	duel* pduel = interpreter::get_duel_info(L);
	if(pduel->lua->no_action)
		return luaL_error(L, "Action is not allowed here.");
	return TRUE;
}
