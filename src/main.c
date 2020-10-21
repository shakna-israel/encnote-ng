#include <encnote8.h>

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

bool encrypt_data(lua_State* LuaState, const char* keyfile, const char* datafile) {
	struct Field f = dump_data(LuaState);

	size_t CIPHERTEXT_LEN = (crypto_secretbox_MACBYTES + f.length);

	unsigned char key[crypto_secretbox_KEYBYTES];
	unsigned char nonce[crypto_secretbox_NONCEBYTES];

	// TODO: Consider sodium_malloc
	unsigned char* ciphertext = calloc(CIPHERTEXT_LEN, sizeof(unsigned char));

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
		return false;
	}
	size_t written = fwrite(keyfile_data, sizeof(char), length, keyfile_f);
	if(written != length) {
		// Wrote the wrong number of bytes!
		return false;
	}

	fclose(keyfile_f);

	// Cleanup
	lua_close(L);

	// Construct datafile
	FILE* datafile_f = fopen(datafile, "wb");
	if(datafile_f == NULL) {
		// Unable to get a lock...
		return false;
	}
	written = fwrite(ciphertext, sizeof(unsigned char), CIPHERTEXT_LEN, datafile_f);
	if(written != CIPHERTEXT_LEN) {
		// Wrote the wrong number of bytes!
		return false;
	}
	fclose(datafile_f);

	sodium_memzero(ciphertext, CIPHERTEXT_LEN);
	free(ciphertext);
	free(f.content);
	return true;
}

bool decrypt_data(lua_State* LuaState, const char* keyfile, const char* datafile) {
	// Load key
	lua_State* L = luaL_newstate();
	if(luaL_dofile(L, keyfile)) {
		// Failed!
		return false;
	}

	lua_getglobal(L, "keyfile");
	lua_getfield(L, -1, "key");
	// TODO: Check is string

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
	// TODO: Check is string

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
	// TODO: Safety

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
	// TODO: Safety

	if (crypto_secretbox_open_easy(decrypted, ciphertext, data_length, nonce, key_data) != 0) {
		// Message forged!
		lua_close(L);
		sodium_memzero(ciphertext, data_length + 1);
		sodium_memzero(decrypted, MESSAGE_LEN + 1);
		free(decrypted);
		free(ciphertext);
		return false;
	}
	lua_close(L);

	// Load data
	luaL_dostring(LuaState, decrypted);
	// TODO: Check for errors...

	sodium_memzero(ciphertext, data_length + 1);
	sodium_memzero(decrypted, MESSAGE_LEN + 1);
	free(decrypted);
	free(ciphertext);
	return true;
}

bool generate(lua_State* LuaState, const char* name, size_t length) {
	const uint32_t upper = 255;

	char* data = calloc(length, sizeof(char));
	// TODO: Safety

	for(size_t i = 0; i < length; i++) {
		// TODO: Pattern of allowed bytes!
		uint32_t c = randombytes_uniform(upper);
		data[i] = c;
	}

	set_field(LuaState, name, strlen(name), data, length);

	sodium_memzero(data, length);
	free(data);
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

void run_dump_mode(lua_State* L) {
	struct Field f = dump_data(L);
	printf("%s\n", f.content);
	free(f.content);
}

void run_ls_mode(lua_State* L) {
	luaL_dostring(L, "for k, v in pairs(ENCNOTE_DATA) do print(#v, k) end");
}

void run_generate_mode(lua_State* L, const char* argfile) {
	printf("%s\n", "Generating...");

	if(argfile == NULL) {
		return;
	}
	// TODO: Allow setting length...
	if(generate(L, argfile, 20) != true) {
		fprintf(stderr, "ERROR: Generate failed.\n");
	}
}

void run_help(const char* progname, const char* helpstring) {
	if(helpstring == NULL) {
		// General usage
		printf("Usage: %s [options...]\n", progname);
		fputc('\n', stdout);

		run_help(progname, "mode");
		fputc('\n', stdout);

		run_help(progname, "keyfile");
		fputc('\n', stdout);

		run_help(progname, "datafile");
		fputc('\n', stdout);

		run_help(progname, "datadir");
		fputc('\n', stdout);

		run_help(progname, "help");
		fputc('\n', stdout);
	} else {
		// Specific usage
		// TODO: mode
		// TODO: mode dump
		// TODO: mode ls

		// TODO: keyfile
		// TODO: datafile
		// TODO: datadir
		// TODO: help
	}
}

enum MODES {
	DUMP_MODE,
	LS_MODE,
	GENERATE_MODE,
	INVALID_MODE,
};

int main(int argc, char* argv[]) {
	// TODO: Parse CLI args here...
	char* keyfilename = NULL;
	char* datafilename = NULL;
	char* data_dir = get_data_dir();
	char* argfile = NULL;

	int c;
    int digit_optind = 0;
    int current_mode = INVALID_MODE;
    int should_exit = -1;

    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
            {"mode",        required_argument, 0,  'm'},
            {"keyfile",     required_argument, 0,  'k'},
            {"datafile",    required_argument, 0,  'd'},
            {"datadir",     no_argument,       0,  'D'},
            {"file",        required_argument, 0,  'f'},
            {"help",        optional_argument, 0,  'h'},
            {0,         0,                     0,   0 }
        };

       c = getopt_long(argc, argv, "m:k:d:f:h?D",
                 long_options, &option_index);
        if (c == -1)
            break;

       switch (c) {
       	case 0:
       	case 'm':
       		if(optarg) {
       			if(strcmp(optarg, "dump") == 0) {
       				current_mode = DUMP_MODE;
       			} else
       			if(strcmp(optarg, "ls") == 0) {
       				current_mode = LS_MODE;
       			} else
       			if(strcmp(optarg, "generate") == 0) {
       				current_mode = GENERATE_MODE;
       			} else {
       				fprintf(stderr, "ERROR: --%s supplied with invalid mode.\n", long_options[option_index].name);
       				should_exit = 1;
       			}
       		} else {
       			fprintf(stderr, "ERROR: --%s supplied without mode.\n", long_options[option_index].name);
       			should_exit = 1;
       		}
       		break;
       	case 1:
       	case 'k':
       		if(optarg) {
       			keyfilename = strdup(optarg);
       			// TODO: Safety
       		} else {
       			fprintf(stderr, "ERROR: --%s supplied without filename.\n", long_options[option_index].name);
       			should_exit = 1;
       		}
       		break;
       	case 2:
       	case 'd':
       		if(optarg) {
       			datafilename = strdup(optarg);
       			// TODO: Safety
       		} else {
       			fprintf(stderr, "ERROR: --%s supplied without filename.\n", long_options[option_index].name);
       			should_exit = 1;
       		}
       		break;
       	case 3:
       	case 'D':
       		if(data_dir == NULL) {
       			fprintf(stderr, "NO DATA DIRECTORY FOUND.\n");
       		} else {
       			printf("%s\n", data_dir);
       		}
       		should_exit = 0;
       	case 4:
       	case 'f':
       		if(optarg) {
       			argfile = strdup(optarg);
       			// TODO: Safety
       		} else {
       			fprintf(stderr, "ERROR: --%s supplied without name.\n", long_options[option_index].name);
       			should_exit = 1;
       		}
       	case 5:
       	case 'h':
       		if(optarg) {
       			run_help(argv[0], optarg);
       			should_exit = 0;
       		} else {
       			run_help(argv[0], NULL);
       			should_exit = 0;
       		}
        }
    }

   if(optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }

    if(current_mode == INVALID_MODE) {
    	fprintf(stderr, "ERROR: --mode not set.\n");
    	run_help(argv[0], NULL);
    	should_exit = 1;
    }

    if(should_exit != -1) {
    	return should_exit;
    }

    // BUG: If --file is set, flow control breaks...

    if(keyfilename == NULL) {
    	// Set one by XDG fallbacks...
		// $XDG_DATA_HOME/encnote8/key
		// $HOME/.local/share/encnote8/key

		if(data_dir == NULL) {
			// TODO: Allocation/search failure!
		} else {
			// Ensure data directory exists...
			struct stat st = {0};
			if(stat(data_dir, &st) == -1) {
    			mkdir(data_dir, 0755);
			}
		}

		char* tail = "key";

		char* path;

		path = calloc(strlen(data_dir) + strlen(tail) + 1, sizeof(char));
		if(path == NULL) {
			// TODO: Allocation failure...
		}

		memcpy(path, data_dir, strlen(data_dir));
		memcpy(path + strlen(data_dir), tail, strlen(tail));
		path[strlen(data_dir) + strlen(tail)] = 0;

		keyfilename = path;
    }

    if(datafilename == NULL) {
    	// Set one by XDG fallbacks...
		// $XDG_DATA_HOME/encnote8/data
		// $HOME/.local/share/encnote8/data

		if(data_dir == NULL) {
			// TODO: Allocation/search failure!
		} else {
			// Ensure data directory exists...
			struct stat st = {0};
			if(stat(data_dir, &st) == -1) {
    			mkdir(data_dir, 0755);
			}
		}

		char* tail = "data";

		char* path;

		path = calloc(strlen(data_dir) + strlen(tail) + 1, sizeof(char));
		if(path == NULL) {
			// TODO: Allocation failure...
		}

		memcpy(path, data_dir, strlen(data_dir));
		memcpy(path + strlen(data_dir), tail, strlen(tail));
		path[strlen(data_dir) + strlen(tail)] = 0;

		datafilename = path;
    }

    // FINISH setting CLI Args.

	// Initialise Lua
	lua_State* L = luaL_newstate();
	init_data(L, keyfilename, datafilename);

	switch(current_mode) {
		case DUMP_MODE:
			run_dump_mode(L);
			break;
		case LS_MODE:
			run_ls_mode(L);
			break;
		case(GENERATE_MODE):
			run_generate_mode(L, argfile);
			if(argfile != NULL) {
				free(argfile);
			}
			break;
		default:
			printf("%s\n", "ERROR: CRAP! Mode fell through...");
			break;
	}

	/*

	e = decrypt_data(L, "key", "data");
	if(e == false) {
		fprintf(stderr, "%s\n", "ERROR: Something went wrong whilst decrypting.");
	}

	struct Field f = dump_data(L);
	printf("%s\n", f.content);
	free(f.content);
	*/

	// Re-encrypt before exit...
	bool e = encrypt_data(L, keyfilename, datafilename);
	if(e == false) {
		fprintf(stderr, "%s\n", "ERROR: Something went wrong whilst encrypting.");
	}

	// Cleanup Lua
	free(keyfilename);
	free(datafilename);
	free(data_dir);
	lua_close(L);
	return 0;
}
