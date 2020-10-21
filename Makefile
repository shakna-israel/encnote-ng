LIBSODIUM=`pkg-config --libs --cflags libsodium`
LIBLUA=`pkg-config --libs --cflags lua5.3`
INCLUDES=-Iinclude
OUTNAME=encnote8

all:
	$(CC) -g $(LIBSODIUM) $(LIBLUA) $(INCLUDES) src/main.c -o $(OUTNAME)
