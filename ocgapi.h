/*
 * interface.h
 *
 *  Created on: 2010-4-28
 *      Author: Argon
 */

#ifndef OCGAPI_H_
#define OCGAPI_H_

#include "common.h"

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#ifdef _WIN32
#define OCGAPI EXTERN_C __declspec(dllexport)
#else
#define OCGAPI EXTERN_C __attribute__ ((visibility ("default")))
#endif

#define LEN_FAIL	0
#define LEN_EMPTY	4
#define LEN_HEADER	8
#define TEMP_CARD_ID	0

struct card_data;

typedef byte* (*script_reader)(const char* script_name, int* len);
typedef uint32_t (*card_reader)(uint32_t code, card_data* data);
typedef uint32_t (*message_handler)(intptr_t pduel, uint32_t msg_type);

OCGAPI void set_script_reader(script_reader f);
OCGAPI void set_card_reader(card_reader f);
OCGAPI void set_message_handler(message_handler f);

byte* read_script(const char* script_name, int* len);
uint32_t read_card(uint32_t code, card_data* data);
uint32_t handle_message(void* pduel, uint32_t message_type);

OCGAPI intptr_t create_duel(uint_fast32_t seed);
OCGAPI void start_duel(intptr_t pduel, uint32_t options);
OCGAPI void end_duel(intptr_t pduel);
OCGAPI void set_player_info(intptr_t pduel, int32_t playerid, int32_t lp, int32_t startcount, int32_t drawcount);
OCGAPI void get_log_message(intptr_t pduel, char* buf);
OCGAPI int32_t get_message(intptr_t pduel, byte* buf);
OCGAPI uint32_t process(intptr_t pduel);
OCGAPI void new_card(intptr_t pduel, uint32_t code, uint8_t owner, uint8_t playerid, uint8_t location, uint8_t sequence, uint8_t position);
OCGAPI void new_tag_card(intptr_t pduel, uint32_t code, uint8_t owner, uint8_t location);
OCGAPI int32_t query_card(intptr_t pduel, uint8_t playerid, uint8_t location, uint8_t sequence, int32_t query_flag, byte* buf, int32_t use_cache);
OCGAPI int32_t query_field_count(intptr_t pduel, uint8_t playerid, uint8_t location);
OCGAPI int32_t query_field_card(intptr_t pduel, uint8_t playerid, uint8_t location, uint32_t query_flag, byte* buf, int32_t use_cache);
OCGAPI int32_t query_field_info(intptr_t pduel, byte* buf);
OCGAPI void set_responsei(intptr_t pduel, int32_t value);
OCGAPI void set_responseb(intptr_t pduel, byte* buf);
OCGAPI int32_t preload_script(intptr_t pduel, const char* script_name);
OCGAPI byte* default_script_reader(const char* script_name, int* len);

#endif /* OCGAPI_H_ */
