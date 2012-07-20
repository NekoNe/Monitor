#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>
#include <zmq.h>

#include "zhelpers.h"

#include "config.h"

#define FIELD_SIZE 100

int main(const int argc,
         const char ** const argv)
{
    xmlDocPtr doc, request;
    xmlNodePtr root, topic_level, topic, request_node;
    xmlChar *out;
    size_t len;

    int ret;
    const char *path;

    void *context;
    void *topic_storage;
    char *income;
    size_t msg_size;

    char endpoint[FIELD_SIZE];
    char field[FIELD_SIZE];
    FILE *fp;

    /**** ****/
    if (argc != 2) {
        printf("usage: topic_loader file.xml\n");
        return OK;
    }
    printf(">>> [TopicLoader]: selected file: %s\n", argv[1]);
    path = argv[1];
    /**** ****/

    fp = fopen("topic_loader.ini", "r");
    if (fp == NULL) {
        printf(">>> [TopicLoader]: IO ERROR. can't open \"topic_loader.ini\"\n");
        return 1;
    }

    fscanf(fp,"%s %*s %s", field, endpoint);
    printf(">>> [TopicLoader]: field: %s; value: %s;\n", field, endpoint);

    fclose(fp);

    context = zmq_init(1);

    topic_storage = zmq_socket(context, ZMQ_REQ);
    zmq_connect(topic_storage, endpoint);

    ret = OK;
    doc = NULL; root = NULL;
    topic_level = NULL; request = NULL;

    doc = xmlReadFile(path, NULL, 0);
    if (!doc) {
        printf(">>> [TopicLoader]: libxml couldn't open the file\n");
        return FAIL;
    }

    root = xmlDocGetRootElement(doc);
    if (!root) {
        printf(">>> [TopicLoader]: libxml couldn't get root element\n");
        return FAIL;
    }

    topic_level = root->children;

    while (topic_level) {
        if (strcmp(topic_level->name, "topic")) {
            topic_level = topic_level->next;
            continue;
        }
        topic = NULL;
        request = NULL;
        out = NULL;
        request = NULL;
        request_node = NULL;

        topic = malloc(sizeof(xmlNode));
        memcpy(topic, topic_level, sizeof(xmlNode));
        topic->next = NULL;

        request = xmlNewDoc("1.0");
        request_node = xmlNewNode(NULL, "request");
        xmlNewProp(request_node, "type", "add_topic");
        request_node->children = topic;

        xmlDocSetRootElement(request, request_node);

        xmlDocDumpFormatMemoryEnc(request, &out, &len, "UTF-8", 1);

        printf("%s\n", (char *)out);

        s_send(topic_storage, (char *)out, strlen((char *)out) + 1);
        income = s_recv(topic_storage, &msg_size);

        printf(">>> [TopicLoader]: TS responsed: %s\n", income);


        if (request_node) xmlFree(request_node);
        if (topic) free(topic);
        if (out) xmlFree(out);
        topic_level = topic_level->next;
    }
    /* if (doc) xmlFreeDoc(doc); */

    return OK;
}
