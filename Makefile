CFLAGS=-I/usr/include/libxml2

MONITOR_OBJ = monitor.o\
			topic.o\
			ooarray.o\
			oodict.o\
			oolist.o\
			monitor_server.o

program: $(MONITOR_OBJ)
	cc -o monitor $(MONITOR_OBJ) -lzmq -Wall -lxml2 -pedantic -Ansi

clean:
	rm $(MONITOR_OBJ)
