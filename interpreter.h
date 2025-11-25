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

enum LuaParamType : int32_t {
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
		int32_t integer;
	};
	using coroutine_map = std::unordered_map<int32_t, std::pair<lua_State*, int32_t>>;
	using param_list = std::list<std::pair<lua_param, LuaParamType>>;
	
	duel* pduel;
	char msgbuf[64]{};
	lua_State* lua_state;
	lua_State* current_state;
	param_list params;
	param_list resumes;
	coroutine_map coroutines;
	int32_t no_action{};
	int32_t call_depth{};
	bool enable_unsafe_feature{};

	explicit interpreter(duel* pd, bool enable_unsafe_libraries);
	~interpreter();

	void register_card(card* pcard);
	void register_effect(effect* peffect);
	void unregister_effect(effect* peffect);
	void register_group(group* pgroup);
	void unregister_group(group* pgroup);

	int32_t load_script(const char* script_name);
	int32_t load_card_script(uint32_t code);
	void add_param(void* param, LuaParamType type, bool front = false);
	void add_param(int32_t param, LuaParamType type, bool front = false);
	void push_param(lua_State* L, bool is_coroutine = false);
	int32_t call_function(int32_t f, uint32_t param_count, int32_t ret_count);
	int32_t call_card_function(card* pcard, const char* f, uint32_t param_count, int32_t ret_count);
	int32_t call_code_function(uint32_t code, const char* f, uint32_t param_count, int32_t ret_count);
	int32_t check_condition(int32_t f, uint32_t param_count);
	int32_t check_filter(lua_State* L, card* pcard, int32_t findex, int32_t extraargs);
	int32_t get_operation_value(card* pcard, int32_t findex, int32_t extraargs);
	int32_t get_function_value(int32_t f, uint32_t param_count);
	int32_t get_function_value(int32_t f, uint32_t param_count, std::vector<lua_Integer>& result);
	int32_t call_coroutine(int32_t f, uint32_t param_count, int32_t* yield_value, uint16_t step);
	int32_t clone_function_ref(int32_t func_ref);
	void* get_ref_object(int32_t ref_handler);

	static void card2value(lua_State* L, card* pcard);
	static void group2value(lua_State* L, group* pgroup);
	static void effect2value(lua_State* L, effect* peffect);
	static void function2value(lua_State* L, int32_t func_ref);
	static int32_t get_function_handle(lua_State* L, int32_t index);
	static duel* get_duel_info(lua_State* L);
	static bool is_load_script(const card_data& data);

	template <size_t N, typename... TR>
	static int sprintf(char (&buffer)[N], const char* format, TR... args) {
		return std::snprintf(buffer, N, format, args...);
	}
};

#define COROUTINE_FINISH	1
#define COROUTINE_YIELD		2
#define COROUTINE_ERROR		3

#endif /* INTERPRETER_H_ */
