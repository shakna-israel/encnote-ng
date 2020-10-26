#include <encnote8.h>

#include <pattern.h>

bool set_field(lua_State* LuaState, const char* key, size_t key_len, const char* value, size_t value_len) {
	// Put our data onto the stack...
	lua_getglobal(LuaState, "ENCNOTE_DATA");

	// Push our key/values, which may contain binary...
	lua_pushlstring(LuaState, key, key_len);
	lua_pushlstring(LuaState, value, value_len);

	// Set the key/value in the table...
	lua_settable(LuaState, -3);

	return true;
}

struct Field get_field(lua_State* LuaState, const char* key, size_t key_len) {
	// Put our data onto the stack...
	lua_getglobal(LuaState, "ENCNOTE_DATA");

	// The key may contain binary, so we need to push
	// it first...
	lua_pushlstring(LuaState, key, key_len);
	lua_setglobal(LuaState, "key");

	// Place the value at the top of the stack...
	luaL_dostring(LuaState, "return ENCNOTE_DATA[key]");

	// Get the value and its length
	size_t length = 0;
	const char* str = lua_tolstring(LuaState, -1, &length);

	// Construct our return field
	struct Field r;
	r.length = length;
	r.content = strndup(str, length);
	if(r.content == NULL) {
		r.length = 0;
	}

	// Cleanup
	lua_pop(LuaState, 1);
	luaL_dostring(LuaState, "key = nil");

	return r;
}

int LuaDump(lua_State* L) {
	
	luaL_dostring(L, "local x = 'ENCNOTE_DATA = {';"
	"for k, v in pairs(ENCNOTE_DATA) do\n"
	"x = x .. string.format(\"[%q] = %q,\", k, v);\n"
	"end\n"
	"x = x .. '}'\n"
	"return x");

	return 1;
}

struct Field dump_data(lua_State* LuaState) {
	int status = luaL_dostring(LuaState, "local x = 'ENCNOTE_DATA = {';"
	"for k, v in pairs(ENCNOTE_DATA) do\n"
	"x = x .. string.format(\"[%q] = %q,\", k, v);\n"
	"end\n"
	"x = x .. '}'\n"
	"return x");

	struct Field r;

	if(status != 0) {
		// Lua error!
		r.length = 0;
		r.content = NULL;
		return r;
	}

	size_t length = 0;
	const char *key = lua_tolstring(LuaState, -1, &length);
	char* ret = strndup(key, length);

	lua_pop(LuaState, 1);

	r.length = length;
	r.content = ret;
	if(r.content == NULL) {
		r.length = 0;
	}

	return r;
}

// TODO: This should probably return an enum of success/error states... Instead of printing.
bool encrypt_data(lua_State* LuaState, const char* keyfile, const char* datafile) {
	struct Field f = dump_data(LuaState);

	size_t CIPHERTEXT_LEN = (crypto_secretbox_MACBYTES + f.length);

	unsigned char key[crypto_secretbox_KEYBYTES];
	unsigned char nonce[crypto_secretbox_NONCEBYTES];

	unsigned char* ciphertext = calloc(CIPHERTEXT_LEN, sizeof(unsigned char));
	if(ciphertext == NULL) {
		// Safety
		fprintf(stderr, "%s\n", "ERROR: Memory allocation error when creating ciphertext buffer.");
		return false;
	}

	crypto_secretbox_keygen(key);
	randombytes_buf(nonce, sizeof nonce);
	crypto_secretbox_easy(ciphertext, f.content, f.length, nonce, key);

	// Construct keyfile
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	lua_createtable(L, 0, 0);
	lua_setglobal(L, "keyfile");

	// Push our key
	lua_getglobal(L, "keyfile");
	lua_pushstring(L, "key");
	lua_pushlstring(L, key, crypto_secretbox_KEYBYTES);
	lua_settable(L, -3);

	// Push our nonce
	lua_getglobal(L, "keyfile");
	lua_pushstring(L, "nonce");
	lua_pushlstring(L, nonce, crypto_secretbox_NONCEBYTES);
	lua_settable(L, -3);

	// Generate the payload
	int status = luaL_dostring(L, "local x = 'keyfile = {';"
	"for k, v in pairs(keyfile) do\n"
	"x = x .. string.format(\"[%q] = %q,\", k, v);\n"
	"end\n"
	"x = x .. '}'\n"
	"return x");

	if(status != 0) {
		// Lua error!
		fprintf(stderr, "%s\n", "ERROR: Lua unable to dump keyfile.");
		sodium_memzero(ciphertext, CIPHERTEXT_LEN);
		free(ciphertext);
		free(f.content);
		return false;
	}

	size_t length = 0;
	const char* keyfile_data = lua_tolstring(L, -1, &length);

	// Write the keyfile...
	FILE* keyfile_f = fopen(keyfile, "wb");
	if(keyfile_f == NULL) {
		// Unable to get a lock...
		fprintf(stderr, "%s\n", "ERROR: Unable to get keyfile lock.");
		return false;
	}
	size_t written = fwrite(keyfile_data, sizeof(char), length, keyfile_f);
	if(written != length) {
		// Wrote the wrong number of bytes!
		fprintf(stderr, "%s\n", "ERROR: Wrong number of bytes written for keyfile!");
		return false;
	}

	fclose(keyfile_f);

	// Cleanup
	lua_close(L);

	// Construct datafile
	FILE* datafile_f = fopen(datafile, "wb");
	if(datafile_f == NULL) {
		// Unable to get a lock...
		fprintf(stderr, "%s\n", "ERROR: Unable to get datafile lock.");
		return false;
	}
	written = fwrite(ciphertext, sizeof(unsigned char), CIPHERTEXT_LEN, datafile_f);
	if(written != CIPHERTEXT_LEN) {
		// Wrote the wrong number of bytes!
		fprintf(stderr, "%s\n", "ERROR: Wrong number of bytes written for datafile!");
		return false;
	}
	fclose(datafile_f);

	sodium_memzero(ciphertext, CIPHERTEXT_LEN);
	free(ciphertext);
	free(f.content);
	return true;
}

// TODO: This should probably return an enum of success/error states... Instead of printing.
bool decrypt_data(lua_State* LuaState, const char* keyfile, const char* datafile) {
	// Load key
	lua_State* L = luaL_newstate();
	if(luaL_dofile(L, keyfile)) {
		// Failed!
		return false;
	}

	lua_getglobal(L, "keyfile");
	int t = lua_type(L, -1);
	if(t != LUA_TTABLE) {
		// Malformed keyfile
		return false;
	}

	lua_getfield(L, -1, "key");
	// Check is string
	t = lua_type(L, -1);
	if(t != LUA_TSTRING) {
		// Malformed keyfile
		return false;
	}

	size_t key_length = 0;
	const char* key_data = lua_tolstring(L, -1, &key_length);
	if(key_length != crypto_secretbox_KEYBYTES) {
		// Wrong length!
		lua_close(L);
		return false;
	}

	// Load nonce
	lua_getglobal(L, "keyfile");
	lua_getfield(L, -1, "nonce");
	// Check is string
	t = lua_type(L, -1);
	if(t != LUA_TSTRING) {
		// Malformed keyfile
		return false;
	}

	size_t nonce_length = 0;
	const char* nonce = lua_tolstring(L, -1, &nonce_length);
	if(nonce_length != crypto_secretbox_NONCEBYTES) {
		// Wrong length!
		lua_close(L);
		return false;
	}

	// Load data...
	FILE* f = fopen(datafile, "rb");
	fseek(f, 0, SEEK_END);
	size_t data_length = ftell(f);
	rewind(f);

	unsigned char* ciphertext = calloc(data_length + 1, sizeof(unsigned char));
	// Safety
	if(ciphertext == NULL) {
		fprintf(stderr, "%s\n", "ERROR: Memory allocation error when creating ciphertext buffer.");
		return false;
	}

	// Read the encrypted data in...
	int c;
	int pos = 0;
	do {
		c = fgetc(f);
		if(c != EOF) {
			ciphertext[pos] = c;
			pos++;
		}
	} while(c != EOF);
	fclose(f);

	// Decrypt data
	size_t MESSAGE_LEN = data_length - crypto_secretbox_MACBYTES;
	unsigned char* decrypted = calloc(MESSAGE_LEN + 1, sizeof(unsigned char));
	// Safety
	if(decrypted == NULL) {
		fprintf(stderr, "%s\n", "ERROR: Memory allocation error when creating decrypted buffer.");
		return false;
	}

	if (crypto_secretbox_open_easy(decrypted, ciphertext, data_length, nonce, key_data) != 0) {
		// Message forged!
		fprintf(stderr, "%s\n", "ERROR: Message forged.");
		lua_close(L);
		sodium_memzero(ciphertext, data_length + 1);
		sodium_memzero(decrypted, MESSAGE_LEN + 1);
		free(decrypted);
		free(ciphertext);
		return false;
	}
	lua_close(L);

	// Load data
	if(luaL_dostring(LuaState, decrypted)) {
		// Check for errors...
		fprintf(stderr, "%s\n", "ERROR: Malformed data after decryption.");

		sodium_memzero(ciphertext, data_length + 1);
		sodium_memzero(decrypted, MESSAGE_LEN + 1);
		free(decrypted);
		free(ciphertext);
		return false;
	}

	sodium_memzero(ciphertext, data_length + 1);
	sodium_memzero(decrypted, MESSAGE_LEN + 1);
	free(decrypted);
	free(ciphertext);
	return true;
}

int LuaEncrypt(lua_State* L) {
	// TODO: Type check...
	const char* keyfile = lua_tostring(L, 1);
	// TODO: Type check...
	const char* datafile = lua_tostring(L, 2);

	if(encrypt_data(L, keyfile, datafile)) {
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}

	return 1;
}

int LuaDecrypt(lua_State* L) {
	// TODO: Type check...
	const char* keyfile = lua_tostring(L, 1);
	// TODO: Type check...
	const char* datafile = lua_tostring(L, 2);

	if(decrypt_data(L, keyfile, datafile)) {
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}

	return 1;
}

int LuaRandom(lua_State* L) {
	// TODO: Type check...
	int upper_bound = lua_tointeger(L, 1);

	uint32_t result = randombytes_uniform(upper_bound);

	lua_pushinteger(L, result);
	return 1;
}

int LuaGenerateString(lua_State* L) {
	// TODO: Type check...
	const char* pattern = lua_tostring(L, 1);
	// TODO: Type check...
	const size_t length = lua_tointeger(L, 2);

	lua_State* L2 = luaL_newstate();
	luaL_openlibs(L2);
	lua_register(L2, "BetterRandom", LuaRandom);

	luaL_dostring(L2, src_pattern_lua);

	lua_pushstring(L2, pattern);
	lua_setglobal(L2, "pattern");

	lua_pushinteger(L2, length);
	lua_setglobal(L2, "length");

	luaL_dostring(L2, "return generate_string(length, pattern)");
	// TODO: Error check

	size_t pat_length = 0;
	const char* patstr = lua_tolstring(L2, -1, &pat_length);

	int t = lua_type(L2, -1);
	if(t != LUA_TSTRING) {
		// Error happened in Lua land.
		lua_close(L2);
		return 0;
	}

	// Return the value
	lua_pushlstring(L, patstr, pat_length);

	lua_close(L2);
	return 1;
}

bool generate(lua_State* LuaState, const char* name, size_t length, char* pattern) {
	// Default pattern
	if(pattern == NULL) {
		pattern = ":print:";
	}

	// Expand the pattern...
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	lua_register(L, "BetterRandom", LuaRandom);

	luaL_dostring(L, src_pattern_lua);

	lua_pushstring(L, pattern);
	lua_setglobal(L, "pattern");

	lua_pushinteger(L, length);
	lua_setglobal(L, "length");

	luaL_dostring(L, "return generate_string(length, pattern)");

	int t = lua_type(L, -1);
	if(t != LUA_TSTRING) {
		// Error happened in Lua land.
		lua_close(L);
		return false;
	}

	// Get the value and its length
	size_t pat_length = 0;
	const char* patstr = lua_tolstring(L, -1, &pat_length);

	char* data = calloc(pat_length + 1, sizeof(char));
	if(data == NULL) {
		// Safety
		lua_close(L);
		return false;
	}
	
	for(size_t i = 0; i < pat_length; i++) {
		data[i] = patstr[i];
	}
	lua_close(L);

	set_field(LuaState, name, strlen(name), data, pat_length);

	sodium_memzero(data, pat_length);
	free(data);

	return true;
}

void init_data(lua_State* L, const char* keyfilename, const char* datafilename) {
	luaL_openlibs(L);

	// Create our base table
	lua_createtable(L, 0, 0);
	lua_setglobal(L, "ENCNOTE_DATA");

	// Try and load any existing data, ignoring errors...
	decrypt_data(L, keyfilename, datafilename);
}

char* get_data_dir() {
	// $XDG_DATA_HOME/encnote8/
	// $HOME/.local/share/encnote8/

	char* path;

	const char *env;

	// Try and get from XDG_DATA_HOME...
	if((env = getenv("XDG_DATA_HOME")) && env[0] == '/') {
		char* tail = "/encnote8/";

		path = calloc(strlen(env) + strlen(tail) + 1, sizeof(char));
		if(path == NULL) {
			// Allocation failure...
			return NULL;
		}

		memcpy(path, env, strlen(env));
		memcpy(path + strlen(env), tail, strlen(tail));
		path[strlen(env) + strlen(tail)] = 0;

		return path;
	}

	// Fallback to $HOME/.local/share...
	if((env = getenv("HOME")) && env[0] == '/') {
		char* tail = "/.local/share/encnote8/";

		path = calloc(strlen(env) + strlen(tail) + 1, sizeof(char));
		if(path == NULL) {
			// Allocation failure...
			return NULL;
		}

		memcpy(path, env, strlen(env));
		memcpy(path + strlen(env), tail, strlen(tail));
		path[strlen(env) + strlen(tail)] = 0;

		return path;
	}

	// Complete failure!
	return NULL;
}
