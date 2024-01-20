# Spécification du compilateur et des options de compilation
CC=gcc
CFLAGS=-Wall -Wextra -pedantic

# Bibliothèques à lier, si nécessaire
LIBS=

# Répertoire d'installation des programmes
INSTALL_DIR=~/bin

# Nom du programme principal à compiler
PROGRAM=netcat_server

# Noms des programmes à installer.
# Actuellement, seul le PROGRAM principal est installé.
# Pour installer plusieurs programmes, décommentez et modifiez la ligne ci-dessous.
# INSTALL_NAMES=myprogram1 myprogram2 myprogram3
INSTALL_NAMES=$(PROGRAM)

# Règle pour construire le programme principal
all: $(PROGRAM)

# Règle pour compiler le programme
$(PROGRAM): $(PROGRAM).c
	$(CC) $(CFLAGS) -o $(PROGRAM) $(PROGRAM).c $(LIBS)

# Règle pour installer le ou les programmes
install: $(PROGRAM)
	@if [ ! -d $(INSTALL_DIR) ]; then mkdir -p $(INSTALL_DIR); fi
	@for name in $(INSTALL_NAMES); do \
		echo "Installing $(PROGRAM) as $$name"; \
		install $(PROGRAM) $(INSTALL_DIR)/$$name; \
	done

# Règle pour nettoyer les fichiers générés lors de la compilation
.PHONY: clean
clean:
	rm -f $(PROGRAM)
