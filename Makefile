LIBSODIUM=`pkg-config --libs --cflags libsodium`
LIBLUA=`pkg-config --libs --cflags lua5.3`
INCLUDES=-Iinclude
OUTNAME=encnote8
PREFIX=/usr/local/bin/

all: cli.h pattern.h precommand.h postcommand.h preencrypthook.h setpaths.h utilities.h customcommand.h
	$(CC) -g $(LIBSODIUM) $(LIBLUA) -lrt $(INCLUDES) src/encnote8.c src/main.c -o $(OUTNAME)

.PHONY: install
install: all
	install $(OUTNAME) $(PREFIX)

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

.PHONY: utilities.h
utilities.h:
	xxd -i src/utilities.lua > include/utilities.h

.PHONY: customcommand.h
customcommand.h:
	xxd -i src/custom_command.lua > include/customcommand.h

.PHONY: test
test:
	sh test/cli_test.sh

clean:
	-rm include/cli.h
	-rm include/pattern.h
	-rm include/precommand.h
	-rm include/postcommand.h
	-rm include/preencrypthook.h
	-rm include/setpaths.h
	-rm include/utilities.h
	-rm include/customcommand.h
	-rm $(OUTNAME)
