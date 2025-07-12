#ifndef LUA_MEM_TRACKER_H
#define LUA_MEM_TRACKER_H

#include <cstdlib>
#include <cstddef>
#include <lua.h>

#ifdef YGOPRO_LOG_LUA_MEMORY_SIZE
#include <ctime>     // time_t
#include <cstdio>    // FILE*, fopen, fprintf
#endif

class LuaMemTracker {
public:
	LuaMemTracker(size_t mem_limit = 0);
	~LuaMemTracker();
	static void* AllocThunk(void* ud, void* ptr, size_t osize, size_t nsize);
	void* Alloc(void* ptr, size_t osize, size_t nsize);

	size_t get_total() const { return total_allocated; }
	size_t get_limit() const { return limit; }

private:
#ifdef YGOPRO_LOG_LUA_MEMORY_SIZE
	FILE* log_file = nullptr;
	void write_log();
#endif

	lua_Alloc real_alloc;
	void* real_ud;
	size_t total_allocated = 0;
	size_t limit = 0;
#ifdef YGOPRO_LOG_LUA_MEMORY_SIZE
	size_t max_used = 0; // for logging purposes, to track peak memory usage
#endif
};

#endif // LUA_MEM_TRACKER_H
