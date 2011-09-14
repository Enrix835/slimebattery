CC=gcc
MAIN=slimebattery

LIBS=`pkg-config --cflags --libs gtk+-2.0`

all: 
	$(CC) $(MAIN).c $(LIBS) -g -o $(MAIN)

install: 
	mv $(MAIN) /usr/bin

uninstall:
	rm /usr/bin/$(MAIN)
	
.PHONY: clean

clean:
	rm $(MAIN)
