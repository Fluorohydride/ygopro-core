# YGOPro script engine.

## Introduction
The core logic and lua script processor of YGOPro. This library can be made external of the project and used to power server technologies. It maintains a state engine that is manipulated by Lua scripts using manipulation functions it exposes.

## Compiling
### 1.) Download Fluorohydride/ygopro
Start by downloading the most parent of the source code. The team developing this project are the de facto edge and experts in our community. The most up-to-date `ocgcore` is a compiled dll version of the `Fluorohydride/ygopro/ocgcore` folders project.

### 2.) Install Premake5 and Visual Studio 2022 (or later).
Download premake5.exe, put it in `c:\windows` or a similar folder that is globally accessible via `cmd` or PowerShell. Install Visual Studio 2022, it is the system used for the guide because other parts of the project use C# and most the development team are Windows users.

### 3.) Download dependencies
Dependencies are absent from the main project. There is information on how to build them but the easiest thing to do is to download the following folders from a [soarqin/ygopro](https://github.com/soarqin/ygopro) fork and simply copy them into the `Fluorohydride/ygopro` folder.

* event
* freetype
* irrlicht
* lua
* sqlite3

### 4.) Create the project files
Run the following commands from the command line in the `Fluorohydride/ygopro` folder.

` premake5 vs2022 `

If you are not using Visual Studio 2022 or higher, make necessary adjustments. In the file system open `Fluorohydride/ygopro/build` folder open the `ygo` project.

### 5.) Build the system
Make sure the code actually compiles. Compile them in the following order one by one:

* lua
* sqlite3
* ocgcore

This should provide you with `ocgcore.lib` in the build output folder. `YGOCore` requires a `*.dll`; in `ocgcore` project properties change it to a dynamically linked library. Recompile, it should fail with an error indicating missing dependencies. Right click the project, add an existing file. Add `lua.lib` from the build folder to the project. It should now compile.

## Exposed Functions

These three function need to be provided to the core so it can get card and database information.
- `void set_script_reader(script_reader f);` : Interface provided returns scripts based on number that corresponds to a lua file, send in a string.

- `void set_card_reader(card_reader f);` : Interface provided function that provides database information from the `data` table of `cards.cdb`.

- `void set_message_handler(message_handler f);` : Interface provided function that handles errors

These functions create the game itself and then manipulate it.
- `ptr create_duel(uint32 seed);` : Create a the instance of the duel using a random number.
- `void start_duel(ptr pduel, int32 options);` : Starts the duel
- `void end_duel(ptr pduel);` : ends the duel
- `void set_player_info(ptr pduel, int32 playerid, int32 lp, int32 startcount, int32 drawcount);` sets the duel up
- `void get_log_message(ptr pduel, byte* buf);`
- `int32 get_message(ptr pduel, byte* buf);`
- `int32 process(ptr pduel);` : do a game tick
- `void new_card(ptr pduel, uint32 code, uint8 owner, uint8 playerid, uint8 location, uint8 sequence, uint8 position);` : add a card to the duel state.
- `void new_tag_card(ptr pduel, uint32 code, uint8 owner, uint8 location);` : add a new card to the tag pool.
- `int32 query_card(ptr pduel, uint8 playerid, uint8 location, uint8 sequence, int32 query_flag, byte* buf, int32 use_cache);` : find out about a card in a specific spot.
- `int32 query_field_count(ptr pduel, uint8 playerid, uint8 location);` : Get the number of cards in a specific field/zone.
- `int32 query_field_card(ptr pduel, uint8 playerid, uint8 location, int32 query_flag, byte* buf, int32 use_cache);`
- `int32 query_field_info(ptr pduel, byte* buf);`
- `void set_responsei(ptr pduel, int32 value);`
- `void set_responseb(ptr pduel, byte* buf);`
- `int32 preload_script(ptr pduel, char* script, int32 len);`

# Lua functions
`interpreter.cpp`
