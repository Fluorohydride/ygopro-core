/*
 * interface.h
 *
 *  Created on: 2010-4-28
 *      Author: Argon
 */

#ifndef OCGAPI_H_
#define OCGAPI_H_

#include "common.h"

#ifdef WIN32
#define DECL_DLLEXPORT __declspec(dllexport)
#else
#define DECL_DLLEXPORT
#endif

#define LEN_FAIL	0
#define LEN_EMPTY	4
#define LEN_HEADER	8
#define TEMP_CARD_ID	0

class card;
struct card_data;
class group;
class effect;
class interpreter;

typedef byte* (*script_reader)(const char*, int*);
typedef uint32_t (*card_reader)(uint32_t, card_data*);
typedef uint32_t (*message_handler)(intptr_t, uint32_t);

extern "C" DECL_DLLEXPORT void set_script_reader(script_reader f);
extern "C" DECL_DLLEXPORT void set_card_reader(card_reader f);
extern "C" DECL_DLLEXPORT void set_message_handler(message_handler f);

byte* read_script(const char* script_name, int* len);
uint32_t read_card(uint32_t code, card_data* data);
uint32_t handle_message(void* pduel, uint32_t message_type);

extern "C" DECL_DLLEXPORT intptr_t create_duel(uint_fast32_t seed);
extern "C" DECL_DLLEXPORT void start_duel(intptr_t pduel, uint32_t options);
extern "C" DECL_DLLEXPORT void end_duel(intptr_t pduel);
extern "C" DECL_DLLEXPORT void set_player_info(intptr_t pduel, int32_t playerid, int32_t lp, int32_t startcount, int32_t drawcount);
extern "C" DECL_DLLEXPORT void get_log_message(intptr_t pduel, char* buf);
extern "C" DECL_DLLEXPORT int32_t get_message(intptr_t pduel, byte* buf);
extern "C" DECL_DLLEXPORT uint32_t process(intptr_t pduel);
extern "C" DECL_DLLEXPORT void new_card(intptr_t pduel, uint32_t code, uint8_t owner, uint8_t playerid, uint8_t location, uint8_t sequence, uint8_t position);
extern "C" DECL_DLLEXPORT void new_tag_card(intptr_t pduel, uint32_t code, uint8_t owner, uint8_t location);
extern "C" DECL_DLLEXPORT int32_t query_card(intptr_t pduel, uint8_t playerid, uint8_t location, uint8_t sequence, int32_t query_flag, byte* buf, int32_t use_cache);
extern "C" DECL_DLLEXPORT int32_t query_field_count(intptr_t pduel, uint8_t playerid, uint8_t location);
extern "C" DECL_DLLEXPORT int32_t query_field_card(intptr_t pduel, uint8_t playerid, uint8_t location, uint32_t query_flag, byte* buf, int32_t use_cache);
extern "C" DECL_DLLEXPORT int32_t query_field_info(intptr_t pduel, byte* buf);
extern "C" DECL_DLLEXPORT void set_responsei(intptr_t pduel, int32_t value);
extern "C" DECL_DLLEXPORT void set_responseb(intptr_t pduel, byte* buf);
extern "C" DECL_DLLEXPORT int32_t preload_script(intptr_t pduel, const char* script_name);
byte* default_script_reader(const char* script_name, int* len);
uint32_t default_card_reader(uint32_t code, card_data* data);
uint32_t default_message_handler(intptr_t pduel, uint32_t msg_type);

#endif /* OCGAPI_H_ */
