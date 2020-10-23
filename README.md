# encnote8

An embeddable encrypted memory store.

---

## About

This is an ***ALPHA*** level attempt at creating an encrypted data store to replace my use cases for [pass](https://www.passwordstore.org/).

It works by binding together [libsodium](http://libsodium.org/)'s cryptobox with [Lua 5.3](lua.org/).

This provides a fairly secure platform for encrypting the data, and so long as your keyfile doesn't leak, then you should be fairly safe.

As most actions also _roll_ your keyfile, losing it may not be the end of the world.

***WARNING***: There are various security holes that are currently gaping like festering wounds. Other processes may also access memory that is handled by a GC, and leak that way.

This is not a state-of-the-art encryption tool, this is a "good enough" encryption tool, which _is not yet good enough_.

---

## Building

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

# License

See the `LICENSE.md` file for the legal text, and remember that you're also linking against Lua and libsodium, and their licenses.
