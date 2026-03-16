#ifndef METADATA_H_
#define METADATA_H_

#include "common.h"
#include <unordered_map>
#include <string>
#include <lua.h>

class metadata_entry {
	public:
		metadata_entry();
		~metadata_entry();

		metadata_entry(const metadata_entry& other);
		metadata_entry& operator=(const metadata_entry& other);

		metadata_entry(metadata_entry&& other) noexcept;
		metadata_entry& operator=(metadata_entry&& other) noexcept;

		int32_t luaop_get(lua_State *L);
		int32_t luaop_set(lua_State *L, int32_t index);

	private:
		void reset();
		void copy_from(const metadata_entry& other);
		LuaParamType type;
		intptr_t value;
};

using metadata_entry_map = std::unordered_map<std::string, metadata_entry>;

class metadata {
	public:
		int32_t luaop_clear();
		int32_t luaop_get(lua_State *L, int32_t offset = 0);
		int32_t luaop_set(lua_State *L, int32_t offset = 0);
		int32_t luaop_has(lua_State *L, int32_t offset = 0);
		int32_t luaop_keys(lua_State *L);

	private:
		metadata_entry_map entries;
};

#endif /* METADATA_H_ */
