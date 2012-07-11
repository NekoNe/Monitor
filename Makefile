CFLAGS=-I/usr/include/libxml2

MONITOR_OBJ = monitor.o\
			topic.o\
			ooarray.o\
			oodict.o\
			oolist.o\
			resource.o\
			monitor_server.o

AGENT_OBJ = agent.o\
			agent_server.o

program: $(MONITOR_OBJ) $(AGENT_OBJ)
	cc -o monitor $(MONITOR_OBJ) -lzmq -Wall -lxml2 -pedantic -Ansi
	cc -o agent $(AGENT_OBJ) -lzmq -Wall -lxml2 -pedantic -Ansi

clean:
	rm -rf *.o
