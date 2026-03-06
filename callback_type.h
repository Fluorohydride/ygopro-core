#ifndef CALLBACK_TYPE_H_
#define CALLBACK_TYPE_H_

#include "common.h"

struct card_data;

typedef byte* (*script_reader)(const char* script_name, int* len);
typedef uint32_t (*card_reader)(uint32_t code, card_data* data);
typedef uint32_t (*message_handler)(intptr_t pduel, uint32_t msg_type);

#endif /* CALLBACK_TYPE_H_ */
