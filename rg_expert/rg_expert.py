#coding: utf-8

import urllib
import urllib2
import zmq
import StringIO
import os
import time
import re
import zmq
import time

from datetime import date
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

    print ">>> [ng_expert]: Document url: ", url

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
            print ">>> [ng_expert]: SOCKET ERROR"
            return
    except urllib2.URLError, e:
        print ">>> [ng_expert]: FAILED TO REACH SERVER <<<"
        print ">>> [ng_expert]: Reason:", e.reason
        return

    html = r.read()
    html = html.decode(ENCODING)

    if (not error):
        content_regex = re.compile(u"<td bgcolor=\"#cccccc\".+?>(.+?)<td bgcolor", re.IGNORECASE | re.DOTALL | re.UNICODE)
        content = content_regex.search(html)
        if (content == None):
            print ">>> [ng_expert]: !!! Unusual markup !!!"
            content = html
        else:
            content = content.group(1)
    else:
        content = html

    begin = content.find("Опубликовано")
    end = content[begin:].find("г.") + begin

    published = content[begin + 12:end+2]
    published = published.replace("&nbsp;", " ")
    print "Дата публикации: ", published

    begin = content.find("Вступает в силу")
    end = content[begin:].find("г.") + begin

    actual = content[begin + 16:end+2]
    actual = actual.replace("&nbsp;", " ")
    print "Вступает в силу: ", actual

    # attach
    parser = etree.HTMLParser()
    try:
        doc = etree.parse(StringIO.StringIO(html), parser)
    except:
        print ">>> [rg_expert]: can not parse html."
        return

    a_list_builder = etree.XPath("//a[@href]")
    a_list = a_list_builder(doc)

    for tag in a_list:
        if (tag.text == None): continue
        text = tag.text
        if (text.find("При") != -1):
            for attr, val in tag.items():
                if (attr == "href"): print "file", val

    content_regex = re.compile(u"<title>(.+?)<", re.IGNORECASE | re.DOTALL | re.UNICODE)
    title = content_regex.search(html)

    if (title != None):
        title = title.group(1)
        print "Заголовок:", title


    sub_path = rel_url[:rel_url.rfind("/")]
    path = PATH + "/" + topic + sub_path

    #print sub_path
    #print path

    page = "<doc><published>%s</published><source>%s</source><content>%s</content></doc>" % (published, url, content)

    sender.send(pack(url));

# saving docs
    if (not os.path.exists(path)): os.makedirs(path)
    file_name = path + rel_url[rel_url.rfind("/"):]

    try:
        fp = open(file_name, 'w')
        fp.write(page)
        fp.close()
    except:
        return
    print ">>> doc: \"%s\" stored at \"%s\"" % (url, file_name)


def parse_chunk(resource, topic, chank_size, doc_id, sender):

    url = resource + "/" + topic + "/" + ARCHIVE + "/" + str(chank_size) + "/" + doc_id + FORMAT

    print ">>> [ng_expert]: Chunk url: ", url

    time.sleep(0.01)
    try:
        socket = urllib.urlopen(url)
        html = socket.read()
    except:
        print ">>> [ng_expert]: SOCKET ERROR"
        return

    socket.close()

    #html = html.decode(ENCODING)

    html = html[2:-2]

    parser = etree.HTMLParser()
    try:
        doc = etree.parse(StringIO.StringIO(html), parser)
    except:
        print ">>> [rg_expert]: can not parse html."
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

    print ">>> [ng_expert]: ng_topic url:", url

    time.sleep(0.01)

    try:
        socket = urllib.urlopen(url)
        html = socket.read()
    except:
        print ">>> [ng_expert]: SOCKET ERROR"
        return

    socket.close()

    html = html.decode(ENCODING)

    # getting ids
    ids_regex = re.compile("ids: \[(.+?)\]", re.IGNORECASE | re.DOTALL | re.UNICODE)

    ids = ids_regex.search(html).group(1)
    ids = ids.split(',')

    # getting first document

    parser = etree.HTMLParser()
    try:
        doc = etree.parse(StringIO.StringIO(html), parser)
    except:
        print ">>> [rg_expert]: can not parse html."
        return

    a_list_builder = etree.XPath(u"//div[@class='txt-np reddoc'][1]/a")
    a_list = a_list_builder(doc)

    for tag in a_list:
        for attr, val in tag.items():
            rel_url = val
            refer = RESOURCE + '/' + rel_url
            url = RESOURCE + '/' + PRINTABLE + rel_url
            save_doc(topic, url, rel_url, refer, sender)
    # running through chunks
    i = 0
    while (i < len(ids)):
        id = ids[i]
        id = id.replace(" ","")
        if (id == ""): break
        parse_chunk(RESOURCE, topic, CHUNK_SIZE, id, sender)
        print ids[i][1:]
        i += CHUNK_SIZE


def full_load(sender):
    print ">>> [ng_expert]: Starting full_load"
    for topic in TOPICS:
        open_topic(RESOURCE, topic, sender)

def update(sender):
    print ">>> [ng_expert]: Starting update"

    today = date.today()

    day = str(today.day)
    month = str(today.month)
    year = str(today.year)

    if (len(month) == 1): month = "0" + month
    if (len(day) == 1): day = "0" + day

    print ">>> [ng_expert]: Date today (yyyy.mm.dd): %s.%s.%s" % (year, month, day)
    additional_path = "/" + year + "/" + month + "/" + day

    for topic in TOPICS:
        open_topic(RESOURCE, topic + additional_path, sender)

def handler(request,sender):

    parser = etree.XMLParser()
    root = etree.XML(request, parser)

    tag = root.tag

    if (tag != "request"):
        print ">>> [ng_expert]: Unknown request"
        return
    if (root.items() == None):
        print ">>> [ng_expert]: Wrong parametrs"
        return

    attr = root.items()[0][0]
    val = root.items()[0][1]

    if ((attr == None) or (val == None)):
        print ">>> [ng_expert]: Wrong parametrs"
        return

    if (attr != "type"):
        print ">>> [ng_expert]: Unknown attribue"
        return

    if (val == "update"):
        update(sender)
        return
    if (val == "full_load"):
        full_load(sender)
        return

    print ">>> [ng_expert]: Unknown attribute value"

def main():
    context = zmq.Context()

    sender = context.socket(zmq.PUSH)
    sender.connect("tcp://localhost:5569")

    receiver = context.socket(zmq.PULL)
    receiver.connect("tcp://localhost:5559")

    print "test"
    while (True):
        print ">>> [ng_expert]: Waiting for request..."
        request = receiver.recv()
        print ">>> [ng_expert]: Request:\n%s" % request
        handler(request, sender)


if (__name__ == "__main__"): main()
