#coding: utf-8

import zmq

usage =\
"""\
Запросы:
  Добавить категорию:

    <request type = "add_topic">
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
    </request>

  Добавить документ:

    <request type = "add_doc">
        <doc url = "[url]">
            <topic id ] = "[id]"/>
            ...
        </docum>
    </request>"""

addres = "tcp://localhost:5554"

add_doc_wiki = """\
<request type = "add_doc">
    <doc url = "http://ru.wikipedia.org">
        <topic id = "000.000.000"/>
    </doc>
</request>
"""
show_generic_children = """\
<request type = "show_children">
    <topic id = "000.000.000"/>
</request>
"""
show_010_children = """\
<request type = "show_children">
    <topic id = "010.000.000"/>
</request>
"""
show_010_010_children = """\
<request type = "show_children">
    <topic id = "010.010.000"/>
</request>
"""
show_010_010_010_children = """\
<request type = "show_children">
    <topic id = "010.010.010"/>
</request>
"""
show_generic_docs = """\
<request type = "show_docs">
    <topic id = "000.000.000"/>
</request>
"""

tests = [add_doc_wiki, show_generic_children, show_010_children, show_010_010_children, show_010_010_010_children, show_generic_docs]

def client_start():
    context = zmq.Context()
    print "usage\n", usage
    print ">>> Connectiong to Topic Storage..."

    socket = context.socket(zmq.REQ)
    socket.connect(addres)

    while(True):
        for test in tests:
            raw_input("Нажмите Enter, чтобы отправить новый запрос:")
            print ">>> Sending request:\n ", test, "\n..."
            socket.send(test)

            reply = socket.recv()
            print ">>> Recived reply:\n", reply



def main():
    client_start()


if (__name__ == "__main__"):
    main()

