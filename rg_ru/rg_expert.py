#coding: utf-8

import urllib
import urllib2
import zmq
import StringIO
import os
import time
import re
import zmq

from lxml import etree

RESOURCE = "http://www.rg.ru"

TOPICS =\
["tema/doc-fedzakon", "tema/doc-postanov",
"tema/doc-ukaz/", "/tema/doc-prikaz",
"tema/doc-soobshenie", "tema/doc-raspr",
"tema/doc-zakonoproekt"]

ENCODING = 'windows-1251'

ARCHIVE = "json/archive_list"
FORMAT = ".json"

PRINTABLE = "printable"

PATH = "docums"

CHUNK_SIZE = 10

def pack(url):
    result = """
<request type = "add_doc">
    <doc url = "%s">
        <topic id = "000.000.000"/>
    </doc>
</request>""" % url

    print result
    return result

def save_doc(topic, url, rel_url, refer, sender):

    time.sleep(0.01)

    error = False
    req = urllib2.Request(url)
    req.add_header('Referer', refer)

    try:
        r = urllib2.urlopen(req)
    except urllib2.HTTPError, e:
        try:
            r = urllib2.urlopen(refer)
            error = True
        except:
            return
    except urllib2.URLError, e:
        print ">>> FAILED TO REACH SERVER <<<"
        print ">>> Reason:", e.reason
        return

    html = r.read()

    html = html.decode(ENCODING) # lol, why?

    #print html
    if (not error):
        content_regex = re.compile(u"<td bgcolor=\"#cccccc\".+?>(.+?)<td bgcolor", re.IGNORECASE | re.DOTALL | re.UNICODE)
        content = content_regex.search(html)

        if (content == None):
            print ">>> !!! Unusual markup !!!"
            content = html
        else:
            content = content.group(1)
    else:
        content = html

    #print content
    begin = content.find("Опубликовано") # сдвинь маркер и дата будет копироваться нормально
    end = content.find("г.")

    published = content[begin:end]

    sub_path = rel_url[:rel_url.rfind("/")]
    path = PATH + "/" + topic + sub_path

    #print sub_path
    #print path

    page = "<doc><published>%s</published><source>%s</source><content>%s</content></doc>" % (published, url, content)

    sender.send(pack(url));

     # saving docs
#    if (not os.path.exists(path)): os.makedirs(path)

#    file_name = path + rel_url[rel_url.rfind("/"):]

#    fp = open(file_name, 'w')
#    fp.write(page)
#    fp.close()

#    print ">>> doc: \"%s\" stored at \"%s\"" % (url, file_name)


def parse_chunk(resource, topic, chank_size, doc_id, sender):

    url = resource + "/" + topic + "/" + ARCHIVE + "/" + str(chank_size) + "/" + doc_id + FORMAT
    print url

    time.sleep(0.01)
    try:
        socket = urllib.urlopen(url)
        html = socket.read()
    except:
        print ">>> SOCKET ERROR <<<"
        return

    socket.close()

    #html = html.decode(ENCODING) # lol, why?

    html = html[2:-2]
    #print html

    parser = etree.HTMLParser()
    try:
        doc = etree.parse(StringIO.StringIO(html), parser)
    except:
        raw_input(topic)
        return
    a_list_builder = etree.XPath("//a[@href]")
    a_list = a_list_builder(doc)

    for tag in a_list:
        rel_url = tag.items()[0][1][2:-2]
        refer = RESOURCE + '/' + rel_url
        url = RESOURCE + '/' + PRINTABLE + rel_url
        print url
        save_doc(topic, url, rel_url, refer, sender)


def open_topic(resource, topic, sender):

    url = resource + "/" + topic
    #print url
    time.sleep(0.01)
    try:
        socket = urllib.urlopen(url)
        html = socket.read()
    except:
        print ">>> SOCKET ERROR <<<"
        return

    socket.close()

    html = html.decode(ENCODING)

    #print html

    ids_regex = re.compile("ids: \[(.+?)\]", re.IGNORECASE | re.DOTALL | re.UNICODE)

    ids = ids_regex.search(html).group(1)
    ids = ids.split(',')

    # running through chunks

    i = 0
    while (i < len(ids)):
        parse_chunk(RESOURCE, topic, CHUNK_SIZE, ids[i][1:], sender)
        print ids[i][1:]
        i += CHUNK_SIZE

    #print ids


def full_load(sender):
    print "full_load"
    for topic in TOPICS:
        open_topic(RESOURCE, topic, sender)

def update(sender):
    print "update"
    pass

def handler(request,sender):

    parser = etree.XMLParser()
    root = etree.XML(request, parser)

    tag = root.tag

    if (tag != "request"):
        print ">>> [ng_expert]: unknown request"
        return
    if (root.items() == None):
        print ">>> [ng_expert]: wrong parametrs"
        return

    attr = root.items()[0][0]
    val = root.items()[0][1]

    if ((attr == None) or (val == None)):
        print ">>> [ng_expert]: wrong parametrs"
        return

    if (attr != "type"):
        print ">>> [ng_expert]: unknown parametrs"
        return

    if (val == "update"):
        update(sender)
        return
    if (val == "full_load"):
        full_load(sender)
        return



def main():
    context = zmq.Context()

    sender = context.socket(zmq.PUSH)
    sender.connect("tcp://localhost:5569")

    receiver = context.socket(zmq.PULL)
    receiver.connect("tcp://localhost:5559")

    print "test"
    while (True):
        request = receiver.recv()
        print ">>> [ng_expert]: request:\n%s" % request
        handler(request, sender)


main()
