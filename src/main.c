#include <encnote8.h>
#include <cli.h>

#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

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

void run_view_mode(lua_State* L, const char* argfile) {
	if(argfile != NULL) {
		lua_pushstring(L, argfile);
		lua_setglobal(L, "argfile");

		luaL_dostring(L, "print(ENCNOTE_DATA[argfile] or ''); argfile=nil;");
	} else {
		fprintf(stderr, "%s\n", "No --file supplied.");
	}
}

void run_edit_mode(lua_State* L, const char* argfile) {
	if(argfile == NULL) {
		return;
	}

	// Create an anonymous file...

	// shm_open requires a file name...
	char anon_name[15 + 1];
	for(size_t i = 0; i < 15; i++) {
		char c = 0;
		while(c == 0 || c > 90 && c < 97) {
			c = (char)('A' + randombytes_uniform('z') % ('z' - 'A' + 1));
		}
		anon_name[i] = c;
	}
	anon_name[0] = '/';
	anon_name[1] = 't';
	anon_name[2] = 'm';
	anon_name[3] = 'p';
	anon_name[4] = '.';
    anon_name[15] = 0;

    // Try and open our temporary file!
	mode_t mode = 0600;
	int fd = shm_open(anon_name, O_RDWR | O_CREAT | O_EXCL, mode);

	// If we got an already existing file, find a new one...
	while(fd < 0 && errno == EEXIST) {
		for(size_t i = 0; i < 15; i++) {
			char c = 0;
			while(c == 0 || c > 90 && c < 97) {
				c = (char)('A' + randombytes_uniform('z') % ('z' - 'A' + 1));
			}
			anon_name[i] = c;
		}
		anon_name[0] = '/';
		anon_name[1] = 't';
		anon_name[2] = 'm';
		anon_name[3] = 'p';
		anon_name[4] = '.';
	    anon_name[15] = 0;

		fd = shm_open(anon_name, O_RDWR | O_CREAT | O_EXCL, mode);
	}

	// TODO: Handle safety...
	if(fd < 0) {
		switch(errno) {
			case EACCES:
				printf("%s\n", "Permission denied.");
				break;
			case EEXIST:
				printf("%s\n", "Already exists.");
				break;
			case EINVAL:
				printf("%s\n", "Invalid name.");
				break;
			case EMFILE:
				printf("%s\n", "Perprocess limit exceeded.");
				break;
			case ENAMETOOLONG:
				printf("%s\n", "Name too long.");
				break;
			case ENFILE:
				printf("%s\n", "System file limit reached.");
				break;
		}
		exit(1);
	}

	// Write the data...
	FILE* f = fdopen(fd, "wb");

	lua_pushstring(L, argfile);
	lua_setglobal(L, "argfile");

	luaL_dostring(L, "return ENCNOTE_DATA[argfile] or ''");

	size_t length = 0;
	const char* str = lua_tolstring(L, -1, &length);

	size_t written = fwrite(str, sizeof(char), length, f);
	if(written != length) {
		// TODO: Incomplete data...
	}

	luaL_dostring(L, "argfile=nil");

	// Find the actual filepath!
	int MAXSIZE = 0xFFF;
    char proclnk[0xFFF];
    char filename[0xFFF];
    ssize_t r;

    sprintf(proclnk, "/proc/self/fd/%d", fd);
    r = readlink(proclnk, filename, MAXSIZE);
    if(r < 0) {
    	// TODO: Fail
    }
    filename[r] = '\0';

    // Finish writing
	fclose(f);

	// Call $EDITOR on the anonymous file
	luaL_dostring(L, "return os.getenv(\"EDITOR\") or 'nano -R'");
	const char* editor = lua_tostring(L, -1);

	size_t com_bytes = snprintf(NULL, 0, "%s %s", editor, filename);
	char* com_buf = calloc(com_bytes + 2, sizeof(char));
	if(com_buf == NULL) {
		// TODO: Safety
	}
	snprintf(com_buf, com_bytes + 1, "%s %s", editor, filename);
	// TODO: Check the right number wrote...

	system(com_buf);
	sodium_memzero(com_buf, com_bytes + 1);
	free(com_buf);

	// Read file data back.
	lua_pushstring(L, filename);
	lua_setglobal(L, "filepath");
	lua_pushstring(L, argfile);
	lua_setglobal(L, "filename");

	luaL_dostring(L, "f = io.open(filepath); if f ~= nil then ENCNOTE_DATA[filename] = f:read(\"*all\");f:close();end");
	luaL_dostring(L, "filepath=nil;filename=nil");

	// Delete the file securely

	f = fopen(filename, "rb");
	if(f == NULL) {
		// The file has disappeared!
		return;
	}
	// 0 the file out...
	fseek(f, 0L, SEEK_END);
	size_t flen = ftell(f);
	fclose(f);
	truncate(filename, 0);
	truncate(filename, flen);

	// 150 rounds of overwriting with random bytes...
	f = fopen(filename, "wb");
	if(f == NULL) {
		// The file has disappeared!
		return;
	}

	for(size_t i = 0; i < 150; i++) {
		fseek(f, 0L, SEEK_SET);
		for(size_t ix = 0; ix < flen; ix++) {
			fputc(randombytes_uniform(255), f);
		}
		fflush(f);
	}
	fclose(f);

	// Truncate and kill
	truncate(filename, 0);
	shm_unlink(anon_name);
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
			printf("\t+ view\n");
			printf("\t\tPrint the contents of a file in the repository.\n");
			printf("\t\tSee --helpinfo 'mode view' for more.\n");
			printf("\t+ ls\n");
			printf("\t\tList the size and content of the repository.\n");
			printf("\t\tSee --helpinfo 'mode ls' for more.\n");
			printf("\t+ edit\n");
			printf("\t\tEdit a file in $EDITOR.\n");
			printf("\t+ generate\n");
			printf("\t\tCreate/overwrite a field with a randomly generated piece of data.\n");
			printf("\t\tSee --helpinfo 'mode generate' for more.\n");
			printf("\t+ dump\n");
			printf("\t\tDump a Lua-compatible piece of code containing the entire repository to the console.\n");
			printf("\t\tSee --helpinfo 'mode dump' for more.\n");
		} else

		// mode edit
		if(strcmp(helpstring, "mode edit") == 0) {
			printf("--mode edit\n");
			printf("\tEdit a file in $EDITOR.\n");
			printf("\tThe field to read/write to is set by `--file`.\n");
			printf("\tIf $EDTIOR is not set, nano will be called in restricted mode.\n");
		} else

		// mode view
		if(strcmp(helpstring, "mode view") == 0) {
			printf("--mode view\n");
			printf("\tPrint the contents of a file in the repository to the standard output device.\n");
			printf("\tThe field to read to is set by `--file`.\n");
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

			printf("\tDefined patterns include:\n");
			printf("\t\t+ :graph:\n");
			printf("\t\t\tAll normal printable characters, excluding space.\n");
			printf("\t\t+ :print:\n");
			printf("\t\t\tAll normal printable characters.\n");
			printf("\t\t+ :alnum:\n");
			printf("\t\t\tAll Latin alphabet characters and Arabic numbers.\n");
			printf("\t\t+ :alpha:\n");
			printf("\t\t\tAll Latin alphabet characters.\n");
			printf("\t\t+ :space:\n");
			printf("\t\t\tAll horizontal and vertical whitespace.\n");
			printf("\t\t+ :blank:\n");
			printf("\t\t\tAll horizontal whitespace.\n");
			printf("\t\t+ :cntrl:\n");
			printf("\t\t\tControl sequences.\n");
			printf("\t\t+ :xdigit:\n");
			printf("\t\t\tAll hexadecimal characters.\n");
			printf("\t\t+ :digit:\n");
			printf("\t\t\tAll Arabic numbers.\n");
			printf("\t\t+ :lower:\n");
			printf("\t\t\tAll lowercase Latin alphabet characters.\n");
			printf("\t\t+ :upper:\n");
			printf("\t\t\tAll uppercase Latin alphabet characters.\n");
			printf("\t\t+ :punct:\n");
			printf("\t\t\tAll punctuation characters.\n");
			printf("\t\t+ :greek:\n");
			printf("\t\t\tAll Greek alphabet characters.\n");
			printf("\t\t+ :greekupper:\n");
			printf("\t\t\tAll Greek uppercase alphabet characters.\n");
			printf("\t\t+ :greeklower:\n");
			printf("\t\t\tAll Greek lowercase alphabet characters.\n");

			// TODO: List all punctuation sets...
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
	VIEW_MODE,
	EDIT_MODE,
	INVALID_MODE,
};

// TODO: clipboard (X11 or SDL...)
// TODO: copy to file
// TODO: copy existing file
// TODO: rename existing file
// TODO: delete file
// TODO: grep-like search file
// TODO: find-like search for file

// TODO: Lua-based user commands
// - Need to expose our run commands as an API...
// - And expose BetterRandom...

// TODO: Lua-based user hooks
// - Needs access to Lua-based user commands
// - pre-command
// - post-command
// - pre-encrypt

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
		} else
		if(strcmp(mode_string, "view") == 0) {
			current_mode = VIEW_MODE;
		} else
		if(strcmp(mode_string, "edit") == 0) {
			current_mode = EDIT_MODE;
		} else
		{
			fprintf(stderr, "ERROR: %s\n", "Invalid or no mode set.");
			run_help(progname, "mode");
			free(datadir);
			free(progname);
			lua_close(CLI_LUA);
			return 1;	
		}
	} else {
		fprintf(stderr, "ERROR: %s\n", "Invalid or no mode set.");
		run_help(progname, "mode");
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

	// TODO: Expose our stuff...

	// Expose a cryptographic random...
	lua_register(L, "BetterRandom", LuaRandom);

	// Expose a raw arg table...
	lua_createtable(L, 0, 0);
	lua_setglobal(L, "arg");
	for(size_t i = 0; i < argc; i++) {
		lua_getglobal(L, "arg");
		lua_pushnumber(L, i);
		lua_pushstring(L, argv[i]);

		// Set the key/value in the table...
		lua_settable(L, -3);
	}

	// TODO: Reconstruct a table of parsed args...

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
    	case VIEW_MODE:
    		run_view_mode(L, argfile);
    		break;
    	case EDIT_MODE:
    		run_edit_mode(L, argfile);
    	default:
    		// TODO: invalid state. Somehow.
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
