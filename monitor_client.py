#coding: utf-8


import zmq

# set of tests

tests = []

#tests: list_topic

test =\
"<request>\n\
    <list_topic>\n\
        <topic title = \"\"/>\n\
    </list_topic>\n\
</request>"
tests.append(test)

test =\
"<request>\n\
    <list_topic>\n\
        <topic title = \"КОНСТИТУЦИОННЫЙ СТРОЙ\"/>\n\
    </list_topic>\n\
</request>"
tests.append(test)

test =\
"<request>\n\
    <list_topic>\n\
        <topic title = \"КОНСТИТУЦИОННЫЙ СТРОЙ\">\n\
            <topic title = \"Конституция Российской Федерации. Конституции, уставы субъектов Российской Федерации\"/>\n\
        </topic>\n\
    </list_topic>\n\
</request>"
tests.append(test)



def client_start():
    context = zmq.Context()

    print ">>> Connectiong to hello world server..."

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
