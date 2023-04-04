CC=gcc
CFLAGS=-Wall -Wextra -pedantic
LIBS=
INSTALL_DIR=~/bin

PROGRAM=netcat_server
INSTALL_NAMES=myprogram1 myprogram2 myprogram3

all: $(PROGRAM)

$(PROGRAM): $(PROGRAM).c
	$(CC) $(CFLAGS) -o $(PROGRAM) $(PROGRAM).c $(LIBS)

install: $(PROGRAM)
	@if [ ! -d $(INSTALL_DIR) ]; then mkdir -p $(INSTALL_DIR); fi
	@for name in $(INSTALL_NAMES); do \
		echo "Installing $(PROGRAM) as $$name"; \
		install $(PROGRAM) $(INSTALL_DIR)/$$name; \
	done

.PHONY: clean
clean:
	rm -f $(PROGRAM)
