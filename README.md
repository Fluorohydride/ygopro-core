# YGOPro script engine.

## Introduction
The core logic and lua script processor of YGOPro. This library can be made external of the project and used to power server technologies. It maintains a state engine that is manipulated by Lua scripts using manipulation functions it exposes.

## Compiling
In most cases, what you want to compile is the main program of YGOPro. You should refer to the main YGOPro project's [wiki](https://github.com/Fluorohydride/ygopro/wiki).
If you want to compile the dynamic link library of this repository (ocgcore.dll), you can refer to [this script](https://github.com/Fluorohydride/ygopro-core/blob/master/.github/workflows/build.yml).

## Exposed Functions

The 3 functions need to be provided to the core so it can get card and database information.
- `void set_script_reader(script_reader f);`  
Interface provided returns scripts based on number that corresponds to a lua file, send in a string.

- `void set_card_reader(card_reader f);`  
Interface provided function that provides database information from the `data` table of `cards.cdb`.

- `void set_message_handler(message_handler f);`  
Interface provided function that handles error messages.

These functions create the game itself and then manipulate it.
- `intptr_t create_duel(uint_fast32_t seed);`  
Create a the instance of the duel with a PRNG seed.

- `void start_duel(intptr_t pduel, uint32 options);`  
Start the duel.

- `void end_duel(intptr_t pduel);`  
End the duel.

- `void set_player_info(intptr_t pduel, int32 playerid, int32 lp, int32 startcount, int32 drawcount);`  
Set up the duel information.

- `void get_log_message(intptr_t pduel, char* buf);`

- `int32 get_message(intptr_t pduel, byte* buf);`

- `int32 process(intptr_t pduel);`  
Do a game tick.

- `void new_card(intptr_t pduel, uint32 code, uint8 owner, uint8 playerid, uint8 location, uint8 sequence, uint8 position);`  
Add a card to the duel state.

- `void new_tag_card(intptr_t pduel, uint32 code, uint8 owner, uint8 location);`  
Add a new card to the tag pool.

- `int32 query_field_card(intptr_t pduel, uint8 playerid, uint8 location, uint32 query_flag, byte* buf, int32 use_cache);`  
Get a card in a specific location.

- `int32 query_field_count(intptr_t pduel, uint8 playerid, uint8 location);`  
Get the number of cards in a specific location.

- `int32 query_field_card(intptr_t pduel, uint8 playerid, uint8 location, uint32 query_flag, byte* buf, int32 use_cache);`  
Get all cards in some location.

- `int32 query_field_info(intptr_t pduel, byte* buf);`

- `void set_responsei(intptr_t pduel, int32 value);`

- `void set_responseb(intptr_t pduel, byte* buf);`

- `int32 preload_script(intptr_t pduel, const char* script_name);`


# Lua functions
- `libcard.cpp`
- `libdebug.cpp`
- `libduel.cpp`
- `libeffect.cpp`
- `libgroup.cpp`

