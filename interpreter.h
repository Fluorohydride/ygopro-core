/*
 * interpreter.h
 *
 *  Created on: 2010-4-28
 *      Author: Argon
 */

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "common.h"
#include <unordered_map>
#include <list>
#include <vector>
#include <cstdio>

class card;
struct card_data;
class effect;
class group;
class duel;

enum LuaParamType : int32 {
	PARAM_TYPE_INT = 0x01,
	PARAM_TYPE_STRING = 0x02,
	PARAM_TYPE_CARD = 0x04,
	PARAM_TYPE_GROUP = 0x08,
	PARAM_TYPE_EFFECT = 0x10,
	PARAM_TYPE_FUNCTION = 0x20,
	PARAM_TYPE_BOOLEAN = 0x40,
	PARAM_TYPE_INDEX = 0x80,
};

class interpreter {
public:
	union lua_param {
		void* ptr;
		int32 integer;
	};
	using coroutine_map = std::unordered_map<int32, std::pair<lua_State*, int32>>;
	using param_list = std::list<std::pair<lua_param, LuaParamType>>;
	
	duel* pduel;
	char msgbuf[64];
	lua_State* lua_state;
	lua_State* current_state;
	param_list params;
	param_list resumes;
	coroutine_map coroutines;
	int32 no_action;
	int32 call_depth;

	explicit interpreter(duel* pd);
	~interpreter();

	int32 register_card(card* pcard);
	void register_effect(effect* peffect);
	void unregister_effect(effect* peffect);
	void register_group(group* pgroup);
	void unregister_group(group* pgroup);

	int32 load_script(const char* script_name);
	int32 load_card_script(uint32 code);
	void add_param(void* param, LuaParamType type, bool front = false);
	void add_param(int32 param, LuaParamType type, bool front = false);
	void push_param(lua_State* L, bool is_coroutine = false);
	int32 call_function(int32 f, uint32 param_count, int32 ret_count);
	int32 call_card_function(card* pcard, const char* f, uint32 param_count, int32 ret_count);
	int32 call_code_function(uint32 code, const char* f, uint32 param_count, int32 ret_count);
	int32 check_condition(int32 f, uint32 param_count);
	int32 check_matching(card* pcard, int32 findex, int32 extraargs);
	int32 get_operation_value(card* pcard, int32 findex, int32 extraargs);
	int32 get_function_value(int32 f, uint32 param_count);
	int32 get_function_value(int32 f, uint32 param_count, std::vector<int32>* result);
	int32 call_coroutine(int32 f, uint32 param_count, int32* yield_value, uint16 step);
	int32 clone_function_ref(int32 func_ref);
	void* get_ref_object(int32 ref_handler);

	static void card2value(lua_State* L, card* pcard);
	static void group2value(lua_State* L, group* pgroup);
	static void effect2value(lua_State* L, effect* peffect);
	static void function2value(lua_State* L, int32 func_ref);
	static int32 get_function_handle(lua_State* L, int32 index);
	static duel* get_duel_info(lua_State* L);
	static bool is_load_script(card_data data);

	template <size_t N, typename... TR>
	static int sprintf(char (&buffer)[N], const char* format, TR... args) {
		return std::snprintf(buffer, N, format, args...);
	}
};

#define COROUTINE_FINISH	1
#define COROUTINE_YIELD		2
#define COROUTINE_ERROR		3

#endif /* INTERPRETER_H_ */
