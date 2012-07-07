CFLAGS=-I/usr/include/libxml2

MONITOR_OBJ = monitor.o\
			topic.o\
			ooarray.o\
			oodict.o\
			oolist.o\
			test_monitor.o

program: $(MONITOR_OBJ)
	cc -o monitor $(MONITOR_OBJ) -lxml2 -pedantic -Ansi

clean:
	rm $(MONITOR_OBJ)
