FILE := 'default.txt'
EXE := 'main'
DIR := './defaultPath/'

all: main.c
	@[ -f $(FILE) ] || echo "?ERROR" && (mkdir $(DIR); mv $(FILE) $(DIR)/$(FILE); gcc main.c -o $(DIR)/$(EXE) -pthread;)

clear:
	@rm -f $(DIR)/$(EXE)

.PHONY: clear