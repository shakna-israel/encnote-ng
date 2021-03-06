# encnote8

An embeddable encrypted memory store.

---

## About

This is an ***BETA*** level attempt at creating an encrypted data store to replace my use cases for [pass](https://www.passwordstore.org/).

It works by binding together [libsodium](http://libsodium.org/)'s cryptobox with [Lua 5.3](lua.org/).

This provides a fairly secure platform for encrypting the data, and so long as your keyfile doesn't leak, then you should be fairly safe.

As most actions also _roll_ your keyfile, losing it may not be the end of the world.

***WARNING***: Other processes may also access memory that is handled by a GC, and leak that way.

This is not a state-of-the-art encryption tool, this is a "good enough" encryption tool, _for my own purposes_. It may not fit yours.

### Library or CLI Tool?

Both.

See Building->Embedding for using it as a library, for extending your own applications.

For the CLI Tool, after building, there is extensive documentation available by running:

	./encnote8 --help

For further specific help, see:

	./encnote8 --helpinfo "$THING"

The CLI tool is fairly advanced, and supports things like Lua hooks & custom commands, allowing you to modify behaviour and so on, and the Lua API has feature-parity with anything that can be done by running a command.

---

## Building

### Testing

[![builds.sr.ht status](https://builds.sr.ht/~shakna/encnote-ng.svg)](https://builds.sr.ht/~shakna/encnote-ng?)

The current testing architecture is woefully inadequate.

### Dependencies:

+ Linux (Probably. May work on other UNIX-y systems.)

+ C Compiler

+ `make`

+ `pkg-config`

+ `xxd`

+ [libsodium](http://libsodium.org/), including development headers.

+ [Lua 5.3](lua.org/), including development headers.

All of these should be in your package manager.

### Build Process

To create the example CLI tool, simply run:

	make clean
	make

The `make clean` step may have some ignored errors. They are ignored, because they can be safely ignored.

### Embedding

To embed, you'll need:

* `src/encnote8.c`

* `include/encnote8.h`

* `include/pattern.h` (generate by running `make pattern.h`)


You can ignore the other files, they're used to create our CLI tool.

You'll also need to link again Lua 5.3, and libsodium:

	pkg-config --libs --cflags libsodium
	pkg-config --libs --cflags lua5.3

Then the basic approach might be:

	#include <encnote8.h>

	const char* keyfilename = "/some/key";
	const char* datafilename = "/some/data";

	lua_State* L = luaL_newstate();
	init_data(L, keyfilename, datafilename);

	// Data is now decrypted into the ENCNOTE_DATA global table in `L`.

	// ... Do stuff here...

	// Re-encrypt to disk...
	bool e = encrypt_data(L, keyfilename, datafilename);
	if(e == false) {
		fprintf(stderr, "%s\n", "ERROR: Something went wrong whilst encrypting.");
	}

	// Cleanup
	lua_close(L);

There are several other methods found in `include/encnote8.h` that you can make use of.

---

## Hooks

Lua-based scripts called 'hooks' can be created for use with the CLI tool.

These hooks have available a number of functions to make them practical.

As a demonstration of what you might use a hook for, consider this post-command hook:

	local t = os.time()

	local key = string.format("%sbackup_%s.key", vararg['datadir'], t)
	local data = string.format("%sbackup_%s.data", vararg['datadir'], t)

	if vararg['mode'] ~= 'view' and vararg['mode'] ~= 'ls' and vararg['mode'] ~= 'dump' then
		if not Encrypt(key, data) then
			print("Failed to write backup...")
		end
	end

Every time a command is run that is not one of `view`, `ls` and `dump`, once the command has completed, a copy of the database and its keys are saved to our data directory.

You would write this file somewhere like: `"$(./encnote8 --datadir)"hooks/post.lua`.

---

## Custom Commands

As well as the provided modes, you can easily implement your own, with custom CLI flags and so on.

Here's an example "backup" mode script. You would place it into `"$(./encnote8 --datadir)"commands/backup.lua`

	local t

	CliArgs()
	if cli_args['time'] ~= nil then
	  t = cli_args['time']
	else
	  t = os.time()
	end

	local key = string.format("%sbackup_%s.key", vararg['datadir'], t)
	local data = string.format("%sbackup_%s.data", vararg['datadir'], t)

	if not Encrypt(key, data) then
		print("Failed to write backup...")
	end

And then you can call it like:

	./encnote8 --mode backup

Or, to supply a given suffix:

	./encnote8 --mode backup --time 20201011

---

# License

See the `LICENSE.md` file for the legal text, and remember that you're also linking against Lua and libsodium, and their licenses.
