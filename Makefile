## Makefile for dadder ##

CFLAGS=-c -Wall -O0 -DDEBUG

LDFLAGS=-lresolv -lpthread
SOURCES=main.c blacklist.c config.c hashtbl.c ini.c  common.c badip.c forwarder.c query_record.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=dadder

prefix=/usr/local/$(EXECUTABLE)

all: $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) -o $@ $(OBJECTS)  $(LDFLAGS)

main.o: main.c common.h config.h blacklist.h hashtbl.h badip.h forwarder.h query_record.h
	$(CC) $(CFLAGS) $< -o $@  

config.o: config.c config.h ini.c ini.h
	$(CC) $(CFLAGS) $< -o $@  

blacklist.o: blacklist.c blacklist.h common.h 
	$(CC) $(CFLAGS) $< -o $@  

badip.o: badip.c badip.h common.h hashtbl.h  
	$(CC) $(CFLAGS) $< -o $@  

forwarder.o: forwarder.c common.h config.h blacklist.h hashtbl.h badip.h forwarder.h query_record.h
	$(CC) $(CFLAGS) $< -o $@  
    
%.o: %.c %.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) 
	rm -f $(EXECUTABLE) 
    
install:
	./install.sh  $(prefix)  $(EXECUTABLE)
