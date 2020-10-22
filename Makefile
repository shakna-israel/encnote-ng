LIBSODIUM=`pkg-config --libs --cflags libsodium`
LIBLUA=`pkg-config --libs --cflags lua5.3`
INCLUDES=-Iinclude
OUTNAME=encnote8

all: cli.h pattern.h
	$(CC) -g $(LIBSODIUM) $(LIBLUA) -lrt $(INCLUDES) src/encnote8.c src/main.c -o $(OUTNAME)

.PHONY: cli.h
cli.h:
	xxd -i src/cli.lua > include/cli.h

.PHONY: pattern.h
pattern.h:
	xxd -i src/pattern.lua > include/pattern.h
