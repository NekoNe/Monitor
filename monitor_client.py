#coding: utf-8


import zmq

# set of tests

tests = []

#tests: list_topic

usage = "    Запросить список подкатегорий: <show_children id = \"id категории\"/>\n\
    Запросить список документов относящихся к категории: <show_docs id =\"id категории\"/>"



def client_start():
    context = zmq.Context()
    print "usage\n", usage
    print ">>> Connectiong to monitor server..."

    socket = context.socket(zmq.REQ)
    socket.connect("tcp://localhost:5555")

    for req in tests:
        print ">>> Sending request:\n ", req, "\n..."
        socket.send(req)

        reply = socket.recv()
        print ">>> Recived reply: \n", reply

    while (True):
        req = raw_input("<<< Your request: ");
        socket.send(req)

        reply = socket.recv()
        print ">>> Recived reply: \n", reply


if (__name__ == "__main__"):
    client_start()
