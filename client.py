#coding: utf-8

import zmq

usage =\
"""\
Запросы:
  Добавить категорию:

    <request type = "add_doc">
        <topic id = "[id]">
            <title>[title]</title>
            <concepts>
                <concept name = "[name]"\>
                ...
            </concpts>
        </topic>
    </request>

  Показать подкатегории:

    <request type = "show_children">
        <topic id = "[id]"/>
    </request>

  Показать документы категории:

    <request type = "show_docs">
        <topic id = "[id]"/>
    </request>

  Количество документов в категории:

    <request type = "doc_num">
        <topic id = "[id]"/>
    </request>"""

addres = "tcp://localhost:5554"

show_generic_children = """\
<request type = "show_children">
    <topic id = "000.000.000"/>
</request>
"""
tests = [show_generic_children]

def client_start():
    context = zmq.Context()
    print "usage\n", usage
    print ">>> Connectiong to Topic Storage..."

    socket = context.socket(zmq.REQ)
    socket.connect(addres)

    for test in tests:
        print ">>> Sending request:\n ", test, "\n..."
        socket.send(test)

        reply = socket.recv()
        print ">>> Recived reply:\n", reply



def main():
    client_start()


if (__name__ == "__main__"):
    main()

