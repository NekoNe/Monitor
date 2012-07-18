#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zmq.h>
#include <libxml/parser.h>

#include "zhelpers.h"

#include "oodict.h"
#include "oolist.h"
#include "ooarray.h"

#include "topic.h"
#include "topic_storage.h"
#include "config.h"

#define DICT_INIT_SIZE 1000

static int
TopicStorage_doc_num(struct TopicStorage *self,
                     xmlNodePtr topic_level,
                     char **response)
{
    xmlDocPtr resp;
    xmlNodePtr root;
    xmlChar *tmp, *out;

    struct Topic *topic;
    size_t len;
    char num[MAX_URL_SIZE];

    int ret;


    ret = OK;
    resp = NULL; root = NULL;
    tmp = NULL; out = NULL;

    while (topic_level) {
        if (strcmp(topic_level->name, "topic")) {
            topic_level = topic_level->next;
            continue;
        }
        if (!xmlHasProp(topic_level, "id"))
            return FAIL;
        tmp = xmlGetProp(topic_level, "id");

        topic = self->topics->get(self->topics, (char *)tmp);
        if (!topic) {
            ret = FAIL;
            goto doc_num_exit;
        }

        resp = xmlNewDoc("1.0");
        if (!resp) { ret = FAIL; goto doc_num_exit; }
        root = xmlNewNode(NULL, "doc_number");
        if (!root) {ret = FAIL; goto doc_num_exit; }
        sprintf(num, "%d", topic->documents_number);
        xmlNewProp(root, "number", num);

        xmlDocSetRootElement(resp, root);

        xmlDocDumpFormatMemoryEnc(resp, &out, &len, "UTF-8", 1);
        *response = malloc((strlen((char *)out) + 1) * sizeof(char));
        if (!*response) { ret = NOMEM; goto doc_num_exit; }

        strcpy(*response, (char *)out);
        break;
    }

doc_num_exit:
    if (out) xmlFree(out);
    if (tmp) xmlFree(tmp);
    if (resp) xmlFreeDoc(resp);
    return ret;
}

static int
TopicStorage_show_docs(struct TopicStorage *self,
                       xmlNodePtr topic_level,
                       char **response)
{
    xmlDocPtr resp;
    xmlNodePtr root, url_level;
    xmlChar *tmp, *out;

    struct Topic *topic;
    const char *key;
    void *val;
    size_t len;
    char num[MAX_URL_SIZE];

    int ret;


    ret = OK;
    resp = NULL; root = NULL; url_level = NULL;
    tmp = NULL; out = NULL;
    key = NULL; val = NULL;

    while (topic_level) {
        if (strcmp(topic_level->name, "topic")) {
            topic_level = topic_level->next;
            continue;
        }
        if (!xmlHasProp(topic_level, "id"))
            return FAIL;
        tmp = xmlGetProp(topic_level, "id");

        topic = self->topics->get(self->topics, (char *)tmp);
        if (!topic) {
            ret = FAIL;
            goto show_docs_exit;
        }

        resp = xmlNewDoc("1.0");
        if (!resp) { ret = FAIL; goto show_docs_exit; }
        root = xmlNewNode(NULL, "docs");
        if (!root) {ret = FAIL; goto show_docs_exit; }
        sprintf(num, "%d", topic->documents_number);
        xmlNewProp(root, "number", num);

        xmlDocSetRootElement(resp, root);

        topic->documents->rewind(topic->documents);
        while (true) {
            topic->documents->next_item(topic->documents, &key, &val);
            if (!key) break;

            url_level = xmlNewChild(root, NULL, "a", NULL);
            xmlNewProp(url_level, "href", key);
            xmlNodeSetContent(url_level, key);
        }
        xmlDocDumpFormatMemoryEnc(resp, &out, &len, "UTF-8", 1);
        *response = malloc((strlen((char *)out) + 1) * sizeof(char));
        if (!*response) { ret = NOMEM; goto show_docs_exit; }

        strcpy(*response, (char *)out);
        break;
    }

show_docs_exit:
    if (out) xmlFree(out);
    if (tmp) xmlFree(tmp);
    if (resp) xmlFreeDoc(resp);
    return ret;
}

static int
TopicStorage_show_children(struct TopicStorage *self,
                           xmlNodePtr topic_level,
                           char **response)
{
    xmlChar *tmp, *out;
    xmlDocPtr resp;
    xmlNodePtr root, topics;

    struct Topic *topic, *child;
    char *id;

    int ret;
    size_t i, len;


    out = NULL;
    tmp = NULL;
    ret = OK;
    root = NULL;
    topics = NULL;

    while (topic_level) {
        if (strcmp(topic_level->name, "topic")) {
            topic_level = topic_level->next;
            continue;
        }

        if (!xmlHasProp(topic_level, "id"))
            return FAIL;
        tmp = xmlGetProp(topic_level, "id");

        topic = self->topics->get(self->topics, (char *)tmp);
        if (!topic) {
            ret = FAIL;
            goto show_children_exit;
        }

        resp = xmlNewDoc("1.0");
        if (!resp) return FAIL;
        root = xmlNewNode(NULL, "topics");
        if (!root) goto show_children_exit;

        xmlDocSetRootElement(resp, root);

        for (i = 0; i < topic->children_number; i++) {
            id = topic->children_id[i];

            child = self->topics->get(self->topics, id);
            if (!child) continue;
            topics = xmlNewChild(root, NULL, "topic", NULL);
            if (!topics) goto show_children_exit;
            xmlNewProp(topics, "id", id);
            xmlNewProp(topics, "title", child->title);
        }

        xmlDocDumpFormatMemoryEnc(resp, &out, &len, "UTF-8", 1);
        *response = malloc((strlen((char *)out) + 1) * sizeof(char));
        if (!*response) { ret = NOMEM; goto show_children_exit; }

        strcpy(*response, (char *)out);

        break;
    }

show_children_exit:
    if (tmp) xmlFree(tmp);
    if (out) xmlFree(out);
    if (resp) xmlFree(resp);
    return ret;
}

/* TODO: if function gets invalid data it returns FAIL now
 * let it do next iteration if some piece of data is invalid
 */
static int
TopicStorage_add_topic(struct TopicStorage *self,
                       xmlNodePtr topic_level,
                       char **reply)
{
    xmlNodePtr title_level, concepts_level, concept_level;
    xmlChar *tmp;
    struct Topic *topic;

    int ret;


    if (TOPIC_STORAGE_DEBUG_LEVEL_1)
        printf(">>> [TopicStorage]: adding topic(s)...\n");

    while (topic_level) {
        if (strcmp(topic_level->name, "topic")) {
            topic_level = topic_level->next;
            continue;
        }
        if (!xmlHasProp(topic_level, "id")) {
            topic_level = topic_level->next;
            continue;
        }

        topic = NULL;
        tmp = NULL;
        ret = OK;

        ret = Topic_new(&topic);
        if (ret != OK) return ret;

        tmp = xmlGetProp(topic_level, "id");
        ret = topic->set_id(topic, (char *)tmp);
        if (ret != OK) goto add_topic_exit;
        if (tmp) { xmlFree(tmp); tmp = NULL; }

        title_level = topic_level->children;

        while (title_level) {
            if (strcmp(title_level->name, "title")) {
                title_level = title_level->next;
                continue;
            }

            tmp = xmlNodeGetContent(title_level);
            ret = topic->set_title(topic, (char *)tmp);

            if (tmp) { xmlFree(tmp); tmp = NULL; }

            if (ret != OK) goto add_topic_exit;
            else break;

            title_level = title_level->next;
        }

        concepts_level = title_level->next;
        while (concepts_level) {
            if (strcmp(concepts_level->name, "concepts")) {
                concepts_level = concepts_level->next;
                continue;
            }
            concept_level = concepts_level->children;

            while (concept_level) {
                if (strcmp(concept_level->name, "concept")) {
                    concept_level = concept_level->next;
                    continue;
                }
                if (!xmlHasProp(concept_level, "name")) {
                    concept_level = concept_level->next;
                    continue;
                }

                tmp = xmlGetProp(concept_level, "name");
                ret = topic->add_concept(topic, (char *)tmp);
                if (ret != OK) goto add_topic_exit;
                if (tmp) { xmlFree(tmp); tmp = NULL; }

                concept_level = concept_level->next;
            }
            break;
        }

        /* add new topic */
        topic->str(topic);
        ret = self->topics->set(self->topics, topic->id, topic);
        ret = self->unlinked->set(self->unlinked, topic->id, topic);

        self->establish_dependencies(self);

        if (tmp) xmlFree(tmp);
        topic_level = topic_level->next;
    }

add_topic_exit:
    /* topic->del(topic); */
    if (ret == OK) {
        *reply = malloc((strlen("<ok/>") + 1) * sizeof(char *));
        if (!*reply) return FAIL;
        strcpy(*reply, "<ok/>");
    }
    return ret;
}

/**********  PUBLIC METHODS  **********/

static int
TopicStorage_establish_dependencies(struct TopicStorage *self)
{
    struct Topic *topic, *parent;

    char id[TOPIC_ID_SIZE];
    const char *key;
    void *val;
    int ret;


    if (TOPIC_STORAGE_DEBUG_LEVEL_2)
        printf(">>> [TopicStorage]: establishing dependencies...\n");
    self->unlinked->rewind(self->unlinked);

    while (true) {
        self->unlinked->next_item(self->unlinked, &key, &val);
        if ((!key) || (!val)) break;
        printf("%s\n", key);

        topic = (struct Topic *)val;
        if (strcmp(key, self->generic->id) == 0) continue;

        ret = topic->predict_parent_id(topic, id);
        if (ret != OK) continue;

        if (TOPIC_STORAGE_DEBUG_LEVEL_2)
            printf(">>> [TopicStorage]: Predicted parent's id of %s is %s\n", topic->id, id);

        parent = self->topics->get(self->topics, id);
        if (!parent) continue;

        topic->parent_id = parent->id;
        ret = parent->add_child(parent, topic);
        if (ret != OK) continue;

    }
    return OK;
}

static int
TopicStorage_request_handler(struct TopicStorage *self,
                             char *request,
                             char **reply)
{
    xmlDocPtr req;
    xmlNodePtr request_level;
    xmlChar *tmp;

    int ret;


    ret = OK;
    req = NULL;
    tmp = NULL;
    request_level = NULL;

    req = xmlReadMemory(request, strlen(request) + 1, "request.xml", NULL, 0);
    if (!req) return FAIL;

    if (TOPIC_STORAGE_DEBUG_LEVEL_1)
        printf(">>> [TopicStorage]: Parsing xml request...\n");

    request_level = xmlDocGetRootElement(req);
    if (!request_level) {
        ret = FAIL;
        if (TOPIC_STORAGE_DEBUG_LEVEL_1)
            printf(">>> [TopicStorage]: Can't get request root element.\n");
        goto request_handler_exit;
    }

    if (strcmp(request_level->name, "request") == 0) {
        if (!xmlHasProp(request_level, "type")) {
            ret = FAIL;
            if (TOPIC_STORAGE_DEBUG_LEVEL_1)
                printf(">>> [TopicStorage]: field \"type\" not found.\n");
            goto request_handler_exit;
        }

        tmp = xmlGetProp(request_level, "type");

        if (strcmp((char *)tmp, "add_topic") == 0) {
            ret = TopicStorage_add_topic(self, request_level->children, reply);
            goto request_handler_exit;
        }
        if (strcmp((char *)tmp, "show_children") == 0) {
            ret = TopicStorage_show_children(self, request_level->children, reply);
            goto request_handler_exit;
        }
        if (strcmp((char *)tmp, "show_docs") == 0) {
            goto request_handler_exit;
        }
        if (strcmp((char *)tmp, "doc_num") == 0) {
            goto request_handler_exit;
        }
        if (strcmp((char *)tmp, "show_childless") == 0) {
            goto request_handler_exit;
        }

        ret = FAIL;
        goto request_handler_exit;
    }

    ret = FAIL;
    if (TOPIC_STORAGE_DEBUG_LEVEL_1)
        printf(">>> [TopicStorage]: Unknown command.\n");

request_handler_exit:
    if (tmp) xmlFree(tmp);
    if (req) xmlFreeDoc(req);
    return ret;
}

static int
TopicStorage_serve_forever(struct TopicStorage *self)
{
    void *context;
    void *responder;

    char *request;
    char *reply;
    size_t msg_size;

    int ret;


    context = zmq_init(1);
    responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, self->endpoint);

    request = NULL;
    reply = NULL;
    msg_size = 0;


    while (true) {
        printf(">>> [TopicStorage]: Waiting for requests...\n");

        request = s_recv(responder, &msg_size);

        printf(">>> [TopicStorage]: Request:\n%s\n", request);

        /** handle request **/

        ret = self->request_handler(self, request, &reply);
        if (ret != OK) {
            if (reply) free(reply);
            reply = NULL;
        }

        if (request) free(request);

        if (!reply) {
            s_send(responder, "</error>", strlen("</error>") + 1);
            printf(">>> [TopicStorage]: Reply: <error/>\n");
        }
        else {
            s_send(responder, reply, strlen(reply) + 1);
            printf(">>> [TopicStorage]: Reply:\n%s\n", reply);
            free(reply);

        }
    }

    zmq_close(responder);
    zmq_term(context);

    return OK;
}

static int
TopicStorage_str(struct TopicStorage *self)
{
    printf(">>> [TopicStorage]: I'am at <%p>, binded at \"%s\"\n", self, self->endpoint);
    return OK;
}

/** destructor **/
static int
TopicStorage_del(struct TopicStorage *self)
{
    if (!self) return OK;
    free(self);
    return OK;
}

static int
TopicStorage_init(struct TopicStorage *self,
                  const char *endpoint)
{
    /** binding methods **/

    self->init                      = TopicStorage_init;
    self->str                       = TopicStorage_str;
    self->del                       = TopicStorage_del;
    self->serve_forever             = TopicStorage_serve_forever;
    self->request_handler           = TopicStorage_request_handler;
    self->establish_dependencies    = TopicStorage_establish_dependencies;

    /** init fields **/

    if (!endpoint) return FAIL;

    /* TODO: it will be very good to parse endpoint
     * and check it correctnes
     * */

    self->endpoint = endpoint;
    return OK;
}

/** constructor **/
extern int
TopicStorage_new(struct TopicStorage **topic_storage,
                 const char *endpoint)
{
    struct TopicStorage *self;
    struct Topic *generic;
    struct ooDict *topics, *unlinked, *childless;

    int ret;


    self = malloc(sizeof(struct TopicStorage));
    if (!self) return NOMEM;

    ret = TopicStorage_init(self, endpoint);
    if (ret != OK) goto error;

    ret = ooDict_new(&topics, DICT_INIT_SIZE);
    if (ret != OK) goto error;
    self->topics = topics;

    ret = ooDict_new(&unlinked, DICT_INIT_SIZE);
    if (ret != OK) goto error;
    self->unlinked = unlinked;

    ret = ooDict_new(&childless, DICT_INIT_SIZE);
    if (ret != OK) goto error;
    self->childless = childless;

    ret = Topic_new(&generic);
    if (ret != OK) goto error;
    ret = generic->set_id(generic, "000.000.000");
    if (ret != OK) goto error;
    ret = generic->set_title(generic, "GENERIC");
    if (ret != OK) goto error;
    self->generic = generic;

    ret = self->topics->set(self->topics, generic->id, generic);
    if (ret != OK) goto error;

    *topic_storage = self;
    return OK;
error:
    TopicStorage_del(self);
    return ret;
}
