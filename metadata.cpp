#include "metadata.h"
#include "interpreter.h"
#include "scriptlib.h"
#include <cstring>

metadata_entry::metadata_entry() {
	type = PARAM_TYPE_INT;
	value = 0;
}

metadata_entry::~metadata_entry() {
	reset();
}

metadata_entry::metadata_entry(metadata_entry&& other) noexcept {
	type = other.type;
	value = other.value;
	other.type = PARAM_TYPE_INT;
	other.value = 0;
}

metadata_entry& metadata_entry::operator=(metadata_entry&& other) noexcept {
	if (this != &other) {
		reset();
		type = other.type;
		value = other.value;
		other.type = PARAM_TYPE_INT;
		other.value = 0;
	}
	return *this;
}

int32_t metadata_entry::luaop_get(lua_State *L) {
	if (type == PARAM_TYPE_INT) {
		lua_pushinteger(L, static_cast<int32_t>(value));
		return 1;
	}
	if (type == PARAM_TYPE_INDEX) {
		lua_pushnumber(L, *reinterpret_cast<double*>(value));
		return 1;
	}
	if (type == PARAM_TYPE_STRING) {
		lua_pushstring(L, reinterpret_cast<const char*>(value));
		return 1;
	}
	if (type == PARAM_TYPE_BOOLEAN) {
		lua_pushboolean(L, value != 0);
		return 1;
	}
	if (type == PARAM_TYPE_CARD) {
		interpreter::card2value(L, reinterpret_cast<card*>(value));
		return 1;
	}
	if (type == PARAM_TYPE_GROUP) {
		interpreter::group2value(L, reinterpret_cast<group*>(value));
		return 1;
	}
	if (type == PARAM_TYPE_EFFECT) {
		interpreter::effect2value(L, reinterpret_cast<effect*>(value));
		return 1;
	}
	if (type == PARAM_TYPE_FUNCTION) {
		interpreter::function2value(L, value);
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

int32_t metadata_entry::luaop_set(lua_State *L, int32_t index) {
	reset();
	if (lua_isnil(L, index)) {
		// should never reach here because nil would remove the entry
		value = 0;
		type = PARAM_TYPE_INT;
		return 0;
	}
	if (lua_isboolean(L, index)) {
		value = lua_toboolean(L, index);
		type = PARAM_TYPE_BOOLEAN;
		return 0;
	}
	if (lua_isinteger(L, index)) {
		value = lua_tointeger(L, index);
		type = PARAM_TYPE_INT;
		return 0;
	}
	if (lua_isnumber(L, index)) {
		auto num = lua_tonumber(L, index);
		auto saved_num = new double(num);
		value = reinterpret_cast<intptr_t>(saved_num);
		type = PARAM_TYPE_INDEX;
		return 0;
	}
	if (lua_isstring(L, index)) {
		auto str = lua_tostring(L, index);
		auto saved_str = new char[strlen(str) + 1];
		strcpy(saved_str, str);
		value = reinterpret_cast<intptr_t>(saved_str);
		type = PARAM_TYPE_STRING;
		return 0;
	}
	if(lua_isfunction(L, index)) {
		value = interpreter::get_function_handle(L, index);
		type = PARAM_TYPE_FUNCTION;
		return 0;
	}
	if (lua_isuserdata(L, index)) {
		void* raw_ud = lua_touserdata(L, index);
		void* obj = *(void**)raw_ud;

		LuaParamType type_list[] = {
			PARAM_TYPE_CARD,
			PARAM_TYPE_GROUP,
			PARAM_TYPE_EFFECT
		};

		for (auto t : type_list) {
			if (scriptlib::check_param(L, t, index, TRUE)) {
				value = reinterpret_cast<intptr_t>(obj);
				type = t;
				return 0;
			}
		}
		return luaL_error(L, "Unsupported userdata for metadata entry.");
	}
	
	return luaL_error(L, "Unsupported type for metadata entry.");
}

void metadata_entry::copy_from(const metadata_entry& other) {
	type = other.type;
	switch (type) {
	case PARAM_TYPE_STRING: {
		const char* str = reinterpret_cast<const char*>(other.value);
		char* dup = new char[strlen(str) + 1];
		strcpy(dup, str);
		value = reinterpret_cast<intptr_t>(dup);
		break;
	}
	case PARAM_TYPE_INDEX: {
		const double* pd = reinterpret_cast<const double*>(other.value);
		value = reinterpret_cast<intptr_t>(new double(*pd));
		break;
	}
	default:
		// for int, bool, function handle, and userdata pointer types
		value = other.value;
		break;
	}
}

metadata_entry::metadata_entry(const metadata_entry& other) {
	copy_from(other);
}

metadata_entry& metadata_entry::operator=(const metadata_entry& other) {
	if (this != &other) {
		reset();
		copy_from(other);
	}
	return *this;
}

void metadata_entry::reset() {
	if (type == PARAM_TYPE_STRING) {
		delete[] reinterpret_cast<char*>(value);
	} else if (type == PARAM_TYPE_INDEX) {
		delete reinterpret_cast<double*>(value);
	}
	value = 0;
	type = PARAM_TYPE_INT;
}

int32_t metadata::luaop_clear() {
	entries.clear();
	return 0;
}

int32_t metadata::luaop_get(lua_State *L, int32_t offset) {
	scriptlib::check_param_count(L, 1 + offset);
	scriptlib::check_param(L, PARAM_TYPE_STRING, 1 + offset);
	auto key = lua_tostring(L, 1 + offset);
	auto it = entries.find(key);
	if (it == entries.end()) {
		lua_pushnil(L);
		return 1;
	}
	return it->second.luaop_get(L);
}

int32_t metadata::luaop_set(lua_State *L, int32_t offset) {
	scriptlib::check_param_count(L, 2 + offset);
	scriptlib::check_param(L, PARAM_TYPE_STRING, 1 + offset);
	auto key = lua_tostring(L, 1 + offset);

	if(lua_isnil(L, 2 + offset)) {
		entries.erase(key);
		return 0;
	}

	auto existing = entries.find(key);
	if (existing != entries.end()) {
		return existing->second.luaop_set(L, 2 + offset);
	}

	entries[key] = metadata_entry();
	return entries[key].luaop_set(L, 2 + offset);
}

int32_t metadata::luaop_has(lua_State *L, int32_t offset) {
	scriptlib::check_param_count(L, 1 + offset);
	scriptlib::check_param(L, PARAM_TYPE_STRING, 1 + offset);
	auto key = lua_tostring(L, 1 + offset);
	lua_pushboolean(L, entries.find(key) != entries.end());
	return 1;
}

int32_t metadata::luaop_keys(lua_State *L) {
	if(entries.empty()) {
		lua_pushnil(L);
		return 1;
	}

	int32_t count = 0;
	for (const auto& pair : entries) {
		lua_pushstring(L, pair.first.c_str());
		++count;
	}
	return count;
}
