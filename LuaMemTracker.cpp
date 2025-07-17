#include "LuaMemTracker.h"
#include <lauxlib.h>

LuaMemTracker::LuaMemTracker(size_t mem_limit)
		: limit(mem_limit) {
	lua_State* tmp_L = luaL_newstate();  // get default alloc
	real_alloc = lua_getallocf(tmp_L, &real_ud);
	lua_close(tmp_L);

#ifdef YGOPRO_LOG_LUA_MEMORY_SIZE
	time_t now = time(nullptr);
	char filename[64];
	std::snprintf(filename, sizeof(filename), "memtrace-%ld.log", static_cast<long>(now));

	log_file = std::fopen(filename, "a");
	if (log_file) {
		std::fprintf(log_file, "---- Lua memory tracking started ----\n");
	}
#endif
}

LuaMemTracker::~LuaMemTracker() {
#ifdef YGOPRO_LOG_LUA_MEMORY_SIZE
	if (log_file) {
		std::fprintf(log_file, "---- Lua memory tracking ended ----\n");
		std::fclose(log_file);
		log_file = nullptr;
	}
#endif
}

void* LuaMemTracker::AllocThunk(void* ud, void* ptr, size_t osize, size_t nsize) {
	return static_cast<LuaMemTracker*>(ud)->Alloc(ptr, osize, nsize);
}

void* LuaMemTracker::Alloc(void* ptr, size_t osize, size_t nsize) {
	if (nsize == 0) {
		if (ptr) {
			total_allocated -= osize;
		}
		return real_alloc(real_ud, ptr, osize, nsize);
	} else {
		size_t projected = total_allocated - osize + nsize;
		if (limit && projected > limit) {
			return nullptr;  // over limit
		}
		void* newptr = real_alloc(real_ud, ptr, osize, nsize);
		if (newptr) {
			total_allocated = projected;
#ifdef YGOPRO_LOG_LUA_MEMORY_SIZE
			write_log();
#endif
		}
		return newptr;
	}
}

#ifdef YGOPRO_LOG_LUA_MEMORY_SIZE
void LuaMemTracker::write_log() {
	if (!log_file) return;

	time_t now = time(nullptr);
	struct tm* tm_info = localtime(&now);

	char time_buf[32];
	std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

	if (total_allocated > max_used)
		max_used = total_allocated;

	if (limit)
		std::fprintf(log_file, "%s | used = %zu bytes | max_used = %zu bytes | limit = %zu\n",
			time_buf, total_allocated, max_used, limit);
	else
		std::fprintf(log_file, "%s | used = %zu bytes | max_used = %zu bytes | limit = unlimited\n",
			time_buf, total_allocated, max_used);

	std::fflush(log_file);  // make it write instantly
}
#endif
