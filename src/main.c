#include <encnote8.h>
#include <cli.h>

void run_dump_mode(lua_State* L) {
	struct Field f = dump_data(L);
	printf("%s\n", f.content);
	free(f.content);
}

void run_ls_mode(lua_State* L) {
	luaL_dostring(L, "for k, v in pairs(ENCNOTE_DATA) do print(#v, k) end");
}

void run_generate_mode(lua_State* L, const char* argfile, size_t length, char* pattern) {
	if(argfile == NULL) {
		// TODO: Error...
		return;
	}
	
	if(generate(L, argfile, length, pattern) != true) {
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

		run_help(progname, "helpinfo");
		fputc('\n', stdout);
	} else {
		// Specific usage
		// --mode | -m
		if(strcmp(helpstring, "mode") == 0) {
			printf("--mode $MODE\n");
			printf("-m $MODE\n");
			printf("\tSet the mode to operate in.\n");
			printf("\tValid Options:\n");
			printf("\t+ ls\n");
			printf("\t\tList the size and content of the repository.\n");
			printf("\t\tSee --helpinfo 'mode ls' for more.\n");
			printf("\t+ generate\n");
			printf("\t\tCreate/overwrite a field with a randomly generated piece of data.\n");
			printf("\t\tSee --helpinfo 'mode generate' for more.\n");
			printf("\t+ dump\n");
			printf("\t\tDump a Lua-compatible piece of code containing the entire repository to the console.\n");
			printf("\t\tSee --helpinfo 'mode dump' for more.\n");
		} else

		// mode dump
		if(strcmp(helpstring, "mode dump") == 0) {
			printf("--mode dump\n");
			printf("\tDump a Lua-compatible piece of code containing the entire repository to the console.\n");
		} else

		// mode ls
		if(strcmp(helpstring, "mode ls") == 0) {
			printf("--mode ls\n");
			printf("\tList the size and content of the repository.\n");
			printf("\tFormat:\n");
			printf("\t$SIZE $FILENAME\n");
		} else

		// mode generate
		if(strcmp(helpstring, "mode generate") == 0) {
			printf("--mode generate\n");
			printf("\tCreate/overwrite a field with a randomly generated piece of data.\n");
			printf("\tThe field to write to is set by `--file`.\n");
			printf("\tThe length of randomness to write to is set by `--length` (Defaulting to 20 bytes).\n");
			printf("\tThe pattern to use for generation defaults to `:print:`, see `--pattern` for more options.\n");
		} else

		// --pattern | -p
		if(strcmp(helpstring, "pattern") == 0) {
			printf("--pattern $PATTERN\n");
			printf("-p $PATTERN\n");
			printf("\tSupply a character set, which may be expanded, usually for use with `--mode generate`.\n");
			printf("\tPattern expansion occurs between two colons:\n");
			printf("\te.g. :print: will expand to all printable characters.\n");
			printf("\tUse :colon: to get a literal colon.\n");
			// TODO: List all sets...
		} else

		// --file | -f
		if(strcmp(helpstring, "file") == 0) {
			printf("--file $NAME\n");
			printf("-f $NAME\n");
			printf("\tSets a filename, generally used by various modes to operate on a particular field in the encrypted repository.\n");
		} else
		
		// --length | -l
		if(strcmp(helpstring, "length") == 0) {
			printf("--length $NUM\n");
			printf("-l $NUM\n");
			printf("\tSet a length value, generally used by various modes.\n");
		} else

		// --keyfile | -k
		if(strcmp(helpstring, "keyfile") == 0) {
			printf("--keyfile $FILENAME\n");
			printf("-k $FILENAME\n");
			printf("\tSet the location of the keyfile to this path.\n");
			printf("\tIf this option is not used, uses the file 'key' inside the path provided by --datadir\n");
		} else

		// --datafile | -d
		if(strcmp(helpstring, "datafile") == 0) {
			printf("--datafile $FILENAME\n");
			printf("-d $FILENAME\n");
			printf("\tSet the location of the datafile to this path.\n");
			printf("\tIf this option is not used, uses the file 'data' inside the path provided by --datadir\n");
		} else

		// --datadir | -D
		if(strcmp(helpstring, "datadir") == 0) {
			printf("--datadir\n");
			printf("-D\n");
			printf("\tPrints the current data directory, if any, and then quits.\n");
			printf("\tPath is looked up as follows:\n");
			printf("\t+ $XDG_DATA_HOME/encnote8/\n");
			printf("\t+ $HOME/.local/share/encnote8/\n");
			printf("\tIf neither can be found, lookup fails, and the program will exit.\n");
		} else

		// --help | -h
		if(strcmp(helpstring, "help") == 0) {
			printf("--help\n");
			printf("-h\n");
			printf("\tPrints general help information, if any, and then quits.\n");
		} else

		// --helpinfo | -hh
		if(strcmp(helpstring, "helpinfo") == 0) {
			printf("--helpinfo $STRING\n");
			printf("-hh $STRING\n");
			printf("\tPrints specific help information, if any, and then quits.\n");
		} else

		{
			printf("Unknown help option: <%s>\n", helpstring);
		}
	}
}

enum MODES {
	DUMP_MODE,
	LS_MODE,
	GENERATE_MODE,
	INVALID_MODE,
};

// TODO: view file
// TODO: clipboard (X11 or SDL...)
// TODO: copy to file
// TODO: copy existing file
// TODO: rename existing file
// TODO: delete file
// TODO: grep-like search file
// TODO: find-like search for file
// TODO: text editor
// TODO: Lua-based user hooks
// TODO: Lua-based user commands
// TODO: arbitrary command
// TODO: Example git-hooks...

int main(int argc, char* argv[]) {
	lua_State* CLI_LUA = luaL_newstate();	
	luaL_openlibs(CLI_LUA);

	// Global cli stuff...
	char* datadir = get_data_dir();
	size_t current_mode = INVALID_MODE;
	char* keyfilename = NULL;
	char* datafilename = NULL;
	char* argfile = NULL;
	char* argpattern = NULL;
	size_t arglength = 0;

	lua_createtable(CLI_LUA, 0, 0);
	lua_setglobal(CLI_LUA, "arg");

	for(size_t i = 0; i < argc; i++) {
		lua_getglobal(CLI_LUA, "arg");
		lua_pushnumber(CLI_LUA, i);
		lua_pushstring(CLI_LUA, argv[i]);

		// Set the key/value in the table...
		lua_settable(CLI_LUA, -3);
	}

	// Parse arguments into cli_args table
	luaL_dostring(CLI_LUA, src_cli_lua);
	
	// TODO: Disable this line...
	//luaL_dostring(CLI_LUA, "for k, v in pairs(cli_args) do print(k, v) end");

	lua_getglobal(CLI_LUA, "cli_args");
	lua_getfield(CLI_LUA, -1, "progname");
	const char* progname_tmp = lua_tostring(CLI_LUA, -1);
	char* progname = strdup(progname_tmp);
	if(progname == NULL) {
		// TODO: Safety
	}

	// Check if help called
	lua_getglobal(CLI_LUA, "cli_args");
	lua_getfield(CLI_LUA, -1, "help");
	int t = lua_type(CLI_LUA, -1);
	if(t == LUA_TBOOLEAN) {
		lua_pop(CLI_LUA, -1);

		run_help(progname, NULL);

		// Cleanup and close.
		free(progname);
		free(datadir);
		lua_close(CLI_LUA);
		return 0;
	}

	// Check if helpinfo called
	lua_getglobal(CLI_LUA, "cli_args");
	lua_getfield(CLI_LUA, -1, "helpinfo");
	t = lua_type(CLI_LUA, -1);
	if(t == LUA_TSTRING) {
		const char* helpinfo = lua_tostring(CLI_LUA, -1);

		run_help(progname, helpinfo);

		// Cleanup and close.
		free(progname);
		free(datadir);
		lua_close(CLI_LUA);
		return 0;
	}

	// Check for datadir
	lua_getglobal(CLI_LUA, "cli_args");
	lua_getfield(CLI_LUA, -1, "datadir");
	t = lua_type(CLI_LUA, -1);
	if(t == LUA_TBOOLEAN) {
		lua_pop(CLI_LUA, -1);

		if(datadir != NULL) {
			printf("%s\n", datadir);
			free(datadir);
			free(progname);
		} else {
			fprintf(stderr, "%s\n", "NOT DATADIR FOUND!");
		}

		// Cleanup and close.
		lua_close(CLI_LUA);
		return 0;
	}

	// Check for mode
	lua_getglobal(CLI_LUA, "cli_args");
	lua_getfield(CLI_LUA, -1, "mode");
	t = lua_type(CLI_LUA, -1);
	if(t == LUA_TSTRING) {
		const char* mode_string = lua_tostring(CLI_LUA, -1);

		if(strcmp(mode_string, "dump") == 0) {
			current_mode = DUMP_MODE;
		} else
		if(strcmp(mode_string, "ls") == 0) {
			current_mode = LS_MODE;
		} else
		if(strcmp(mode_string, "generate") == 0) {
			current_mode = GENERATE_MODE;
		} else {
			fprintf(stderr, "ERROR: %s\n", "Invalid or no mode set.");
			free(datadir);
			free(progname);
			lua_close(CLI_LUA);
			return 1;	
		}
	} else {
		fprintf(stderr, "ERROR: %s\n", "Invalid or no mode set.");
		free(datadir);
		free(progname);
		lua_close(CLI_LUA);
		return 1;
	}

	// Check for argfile
	lua_getglobal(CLI_LUA, "cli_args");
	lua_getfield(CLI_LUA, -1, "file");
	t = lua_type(CLI_LUA, -1);
	if(t == LUA_TSTRING) {
		const char* arg_file_tmp = lua_tostring(CLI_LUA, -1);
		argfile = strdup(arg_file_tmp);
		if(argfile == NULL) {
			// TODO: Safety
		}
	}

	// Check for length
	lua_getglobal(CLI_LUA, "cli_args");
	lua_getfield(CLI_LUA, -1, "length");
	t = lua_type(CLI_LUA, -1);
	if(t == LUA_TNUMBER) {
		arglength = lua_tonumber(CLI_LUA, -1);
	} else {
		// Default argument
		arglength = 20;
	}

	// Check for keyfile
	lua_getglobal(CLI_LUA, "cli_args");
	lua_getfield(CLI_LUA, -1, "keyfile");
	t = lua_type(CLI_LUA, -1);
	if(t == LUA_TSTRING) {
		const char* tmp = lua_tostring(CLI_LUA, -1);
		keyfilename = strdup(tmp);
		if(keyfilename == NULL) {
			// TODO: Safety
		}
	} else {
		lua_pop(CLI_LUA, -1);
	}

	// Check for datafile
	lua_getglobal(CLI_LUA, "cli_args");
	lua_getfield(CLI_LUA, -1, "datafile");
	t = lua_type(CLI_LUA, -1);
	if(t == LUA_TSTRING) {
		const char* tmp = lua_tostring(CLI_LUA, -1);
		datafilename = strdup(tmp);
		if(datafilename == NULL) {
			// TODO: Safety
		}
	} else {
		lua_pop(CLI_LUA, -1);
	}

	// Check for pattern
	lua_getglobal(CLI_LUA, "cli_args");
	lua_getfield(CLI_LUA, -1, "pattern");
	t = lua_type(CLI_LUA, -1);
	if(t == LUA_TSTRING) {
		const char* tmp = lua_tostring(CLI_LUA, -1);
		argpattern = strdup(tmp);
		if(argpattern == NULL) {
			// TODO: Safety
		}
	} else {
		lua_pop(CLI_LUA, -1);
	}

	lua_close(CLI_LUA);

	if(keyfilename == NULL) {
    	// Set one by XDG fallbacks...
		// $XDG_DATA_HOME/encnote8/key
		// $HOME/.local/share/encnote8/key

		if(datadir == NULL) {
			// TODO: Allocation/search failure!
		} else {
			// Ensure data directory exists...
			struct stat st = {0};
			if(stat(datadir, &st) == -1) {
    			mkdir(datadir, 0755);
			}
		}

		char* tail = "key";

		char* path;

		path = calloc(strlen(datadir) + strlen(tail) + 1, sizeof(char));
		if(path == NULL) {
			// TODO: Allocation failure...
		}

		memcpy(path, datadir, strlen(datadir));
		memcpy(path + strlen(datadir), tail, strlen(tail));
		path[strlen(datadir) + strlen(tail)] = 0;

		keyfilename = path;
    }

    if(datafilename == NULL) {
    	// Set one by XDG fallbacks...
		// $XDG_DATA_HOME/encnote8/data
		// $HOME/.local/share/encnote8/data

		if(datadir == NULL) {
			// TODO: Allocation/search failure!
		} else {
			// Ensure data directory exists...
			struct stat st = {0};
			if(stat(datadir, &st) == -1) {
    			mkdir(datadir, 0755);
			}
		}

		char* tail = "data";

		char* path;

		path = calloc(strlen(datadir) + strlen(tail) + 1, sizeof(char));
		if(path == NULL) {
			// TODO: Allocation failure...
		}

		memcpy(path, datadir, strlen(datadir));
		memcpy(path + strlen(datadir), tail, strlen(tail));
		path[strlen(datadir) + strlen(tail)] = 0;

		datafilename = path;
    }

    // Initialise...
    lua_State* L = luaL_newstate();
	init_data(L, keyfilename, datafilename);

	// Check what we want to do...
    switch(current_mode) {
    	case DUMP_MODE:
    		run_dump_mode(L);
    		break;
    	case LS_MODE:
    		run_ls_mode(L);
    		break;
    	case GENERATE_MODE:
    		run_generate_mode(L, argfile, arglength, argpattern);
    		break;
    }

    // Re-encrypt!
    bool e = encrypt_data(L, keyfilename, datafilename);
	if(e == false) {
		fprintf(stderr, "%s\n", "ERROR: Something went wrong whilst encrypting.");
	}

	// Cleanup
	free(datafilename);
	free(keyfilename);
	free(progname);
	if(argfile != NULL) {
		free(argfile);
	}
	if(argpattern != NULL) {
		free(argpattern);
	}
	free(datadir);
	lua_close(L);
}
