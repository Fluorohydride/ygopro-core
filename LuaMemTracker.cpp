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
	auto shrink = [&](size_t dec) {
		// Clamp subtraction to avoid size_t underflow.
		if (dec > total_allocated) {
			total_allocated = 0;
		} else {
			total_allocated -= dec;
		}
	};

	if (nsize == 0) {  // free
		if (ptr) {
			shrink(osize);
		}
		return real_alloc(real_ud, ptr, osize, nsize);
	}

	// Unified path:
	// - grow > 0: check limit using safe inequality, then alloc; on success add 'grow'
	// - grow = 0: just alloc; accounting unchanged
	// - shrink > 0: shrink first, alloc; on failure, rollback the shrink
	auto do_alloc = [&](size_t grow_amount, size_t shrink_amount) -> void* {
		if (grow_amount > 0 && limit) {
			if (total_allocated >= limit || grow_amount > (limit - total_allocated)) {
				return nullptr;
			}
		}

		auto result = real_alloc(real_ud, ptr, osize, nsize);
		if (result) {
			if (shrink_amount > 0)
				shrink(shrink_amount);
			if (grow_amount > 0)
				total_allocated += grow_amount;
#ifdef YGOPRO_LOG_LUA_MEMORY_SIZE
			write_log();
#endif
		}

		return result;
	};

	if (ptr) {
		// realloc path: osize is the real old size
		if (nsize > osize) {
			// growth
			size_t grow = nsize - osize;
			return do_alloc(grow, 0);
		} else if (nsize < osize) {
			// shrink
			size_t dec = osize - nsize;
			return do_alloc(0, dec);
		} else {
			// no net change
			return do_alloc(0, 0);
		}
	} else {
		// malloc path: osize is a type tag; only count nsize
		return do_alloc(nsize, 0);
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
