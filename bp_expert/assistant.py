#coding: utf-8

from lxml import etree
from urlparse import urlparse, urljoin

import urllib
import zmq
import StringIO
import os
import time


class Topic(object):

    title = None
    id = None

    child = None
    parent = None


def parse_document(topic, url, path, sender):
    print ">>> [assistant->parse_document]: Topic [title=%s, id=%s] url = %s" % (topic.title, topic.id, url)
    time.sleep(0.01)
    try:
        socket = urllib.urlopen(url)
        html = socket.read()
    except:
        print ">>> !!! TIMED OUT !!! <<<"
        parse_document(topic, url, path, sender)

    socket.close()

    html = html.decode('windows-1251') #TODO: make it var

    report = "<report url=\"%s\">\n\
    <topic id=\"%s\" weight=\"true\"/>\n\
</report>" % (url, topic.id)
    sender.send(report.encode('utf-8'))

    topic_list = []
    while (topic):
        topic_list.append(topic.title)
        topic = topic.parent

    topic_list.reverse()
    dir = '/'.join(topic_list)

    if (not os.path.exists(dir)): os.makedirs(dir)

    index = url.find("DocumID")
    name = url[index:]

    path = dir + "/" + name

    fp = open(path, 'w')
    fp.write(html)
    fp.close()

#    print report

def thumb_page(topic, url, path, sender):
    if (topic == None): return
    print ">>> [assistant->thumb_page]: Topic [title=%s, id=%s] url = %s" % (topic.title, topic.id, url)
    time.sleep(0.01)

    try:
        socket = urllib.urlopen(url)
        html = socket.read()
    except:
        print ">>> !!! TIMED OUT !!! <<<"
        thumb_page(topic, url, path, sender)

    socket.close()

    html = html.decode('windows-1251') #TODO: make it var
    #print html

    parser = etree.HTMLParser()
    doc = etree.parse(StringIO.StringIO(html), parser)

    # listing all documents in current cat

    a_list_builder = etree.XPath(u"//a[@href]")
    a_list = a_list_builder(doc)

    for tag in a_list:
        for attr, val in tag.items():
            if ((attr == "href") and (val.count("DocumID"))):
                url = urljoin(path, val[2:])
                parse_document(topic, url, path, sender)

    for tag in a_list:
        if (tag.text != "След.»"):
            continue
        for attr, val in tag.items():
            if (attr == "href"):
                url = val
                thumb_page(topic.child, url, path, sender);


def parse_page(topic, url, path, sender):
    if (topic == None): return

    print ">>> [assistant->parse_page]: Topic [title=%s, id=%s] url = %s" % (topic.title, topic.id, url)
    time.sleep(0.01)

    try:
        socket = urllib.urlopen(url)
        html = socket.read()
    except:
        print ">>> !!! TIMED OUT !!! <<<"
        parse_page(topic, url, path, sender)
        return

    socket.close()

    html = html.decode('windows-1251') #TODO: make it var
    #print html

    parser = etree.HTMLParser()
    doc = etree.parse(StringIO.StringIO(html), parser)

    thumb_page(topic, url, path, sender)

    # finding next cat # devided for structuring

    if (topic.child == None): return

    topic_child_title = topic.child.title

    a_list_builder = etree.XPath(u"//a[@href]")
    a_list = a_list_builder(doc)

    for tag in a_list:
        if (tag.text != topic_child_title):
            continue
        for attr, val in tag.items():
            if (attr == "href"):
                url = urljoin(path, val[2:]) # 2 means 2 dots "../"
                parse_page(topic.child, url, path, sender);


def parse_request(request):

    url = None
    global RESOURCE
    prev_topic = None

    parser = etree.XMLParser()
    root = etree.XML(request, parser)

    topic_list = []

    for element in root.iter():
        if (element.tag == "resource"):
            for attr, val in element.items():
                if (attr == "url"): RESOURCE = val

        if (element.tag == "topic"):
            topic = Topic()
            topic_list.append(topic)

            if (prev_topic != None):
                prev_topic.child = topic
                topic.parent = prev_topic

            for attr, val in element.items():
                if (attr == "id"): topic.id = val
                elif (attr == "title"): topic.title = val

            prev_topic = topic

    return (topic_list[0], RESOURCE)

def main():

    context = zmq.Context()

    sender = context.socket(zmq.PUSH)
    sender.connect("tcp://localhost:5558");

    receiver = context.socket(zmq.PULL)
    receiver.connect("tcp://localhost:5569")

    while (True):
        request = receiver.recv()

        ret = parse_request(request)
        print "request\n", request
        resource = ret[1]
        topic = ret[0]

        parse_url = urlparse(resource)
        net_loc = parse_url.netloc
        scheme = parse_url.scheme + "://"
        path = scheme + net_loc

        print "path=%s" % path
        print "start url=%s" % resource

        parse_page(topic, resource, path, sender)


main()
