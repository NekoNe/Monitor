#coding: utf-8

import zmq
import sys
import getopt

update = """
<request type = "update"/>"""

full_load = """
<request type = "full_load"/>"""

def main():

    args = sys.argv
    if (len(args) != 2):
        print "Not enough arguments"
        return

    context = zmq.Context()
    sender = context.socket(zmq.PUSH)
    sender.connect("tcp://localhost:5549")

    if (args[1] == "update"):
        sender.send(update)
        return

    if (args[1] == "full_load"):
        sender.send(full_load)
        return

    print "unknown argument"

if (__name__ == "__main__"):
    main()

