FILE:='./default.txt'

SignalProxy: main.c
	@gcc main.c -o SignalProxy -pthread
	@[ -f $(FILE) ] || (touch $(FILE) && echo "start\n" > $(FILE))

clean:
	@rm -f $(FILE)
	@rm -f SignalProxy

.PHONY: clean