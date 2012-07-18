CFLAGS=-I/usr/include/libxml2

MONITOR_OBJ = monitor.o\
				ooarray.o\
				oodict.o\
				oolist.o\
				resource.o\
				start_monitor.o

TOPIC_LOADER = topic_loader.o

#AGENT_OBJ = agent.o\
			url_parser.o\
			topic.o\
			oodict.o\
			oolist.o\
			ooarray.o\
			agent_server.o

TOPIC_STORAGE_OBJ = topic_storage.o\
					topic.o\
					start_topic_storage.o\
					ooarray.o\
					oolist.o\
					oodict.o

program: $(MONITOR_OBJ) $(TOPIC_STORAGE_OBJ) $(TOPIC_LOADER)
	cc -o monitor $(MONITOR_OBJ) -lzmq -Wall -lxml2 -pedantic -Ansi
#	cc -o agent $(AGENT_OBJ) -lzmq -Wall -lxml2 -pedantic -Ansi -lcurl
	cc -o storage $(TOPIC_STORAGE_OBJ) -lzmq -Wall -lxml2 -pedantic -Ansi
	cc -o topic_loader $(TOPIC_LOADER) -lzmq -Wall -lxml2 -pedantic -Ansi

clean:
	rm -rf *.o
