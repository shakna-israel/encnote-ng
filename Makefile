LIBSODIUM=`pkg-config --libs --cflags libsodium`
LIBLUA=`pkg-config --libs --cflags lua5.3`
INCLUDES=-Iinclude
OUTNAME=encnote8

all: cli.h pattern.h precommand.h postcommand.h preencrypthook.h setpaths.h
	$(CC) -g $(LIBSODIUM) $(LIBLUA) -lrt $(INCLUDES) src/encnote8.c src/main.c -o $(OUTNAME)

.PHONY: cli.h
cli.h:
	xxd -i src/cli.lua > include/cli.h

.PHONY: pattern.h
pattern.h:
	xxd -i src/pattern.lua > include/pattern.h

.PHONY: precommand.h
precommand.h:
	xxd -i src/pre_command_hook.lua > include/precommand.h

.PHONY: postcommand.h
postcommand.h:
	xxd -i src/post_command_hook.lua > include/postcommand.h

.PHONY: preencrypthook.h
preencrypthook.h:
	xxd -i src/pre_encrypt_command_hook.lua > include/preencrypthook.h

.PHONY: setpaths.h
setpaths.h:
	xxd -i src/set_paths.lua > include/setpaths.h

clean:
	-rm include/cli.h
	-rm include/pattern.h
	-rm include/precommand.h
	-rm include/postcommand.h
	-rm include/preencrypthook.h
	-rm include/setpaths.h
	-rm $(OUTNAME)
