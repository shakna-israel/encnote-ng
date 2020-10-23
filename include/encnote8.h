#ifndef LIB_ENCNOTE8
#define LIB_ENCNOTE8

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <sodium.h>
#include <sodium/crypto_box.h>

#include <unistd.h>
#include <sys/stat.h>

struct Field {
	size_t length;
	char* content;
};

/*
	Field.content = NULL on failure.
	Caller must free.
*/
struct Field get_field(lua_State* LuaState, const char* key, size_t key_len);

/*
	false on memory failure.
*/
bool set_field(lua_State* LuaState, const char* key, size_t key_len, const char* value, size_t value_len);

/*
	NULL on memory failure.
	Caller must free.
*/
struct Field dump_data(lua_State* LuaState);

/*
	Write `length` random bytes to field `name`.
*/
bool generate(lua_State* LuaState, const char* name, size_t length, char* pattern);

/*
	Initialise a Lua state from a given keyfile and datafile.
	Failure to decrypt, such as because non-existent files, will be ignored.
*/
void init_data(lua_State* L, const char* keyfilename, const char* datafilename);

/*
	Encrypt current data to file.
	Generates a _new_ keyfile.

	False on failure. (Memory, encryption, etc.)
*/
bool encrypt_data(lua_State* LuaState, const char* keyfile, const char* datafile);

/*
	Decrypt data from datafile to our Lua table.

	False on failure. (Memory, encryption, etc.)
*/
bool decrypt_data(lua_State* LuaState, const char* keyfile, const char* datafile);

/*
	Get the $XDG_DATA_DIR/encnote-ng/ path, or nearest.
	NULL on failure.
*/
char* get_data_dir();


// You probably don't want to touch these...
int LuaRandom(lua_State* L);
int LuaDump(lua_State* L);
int LuaGenerateString(lua_State* L);

#endif
