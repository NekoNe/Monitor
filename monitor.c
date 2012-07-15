#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>
#include <zmq.h>

#include "zhelpers.h"

#include "oodict.h"
#include "ooarray.h"
#include "oolist.h"

#include "resource.h"
#include "monitor.h"
#include "topic.h"
#include "config.h"


#define DICT_INIT_SIZE 1000

/*  TODO: compare the monitor error code oomnik and globbie error codes
 *  TODO: check names length before assigment
 *  TODO: change ooArray by smth more convenient
 *
 *
 */

static int
Monitor_receive_result(struct Monitor *self,
                       const char *result)
{
    xmlDocPtr doc;
    xmlNodePtr report_level, topic_level;
    xmlChar *tmp;

    char *url;
    char *id;
    char *weight;

    struct Topic *topic;


    if (MONITOR_DEBUG_LEVEL_3)
        printf(">>> [Monitor]: Received result from [Agent]: \n%s\n", result);

    doc = xmlReadMemory(result, strlen(result), "result.xml", NULL, 0);

    report_level = xmlDocGetRootElement(doc);

    while (report_level) {
        if (strcmp(report_level->name, "report")) {
            report_level = report_level->next;
            continue;
        }
        if (!xmlHasProp(report_level, "url")) {
            report_level = report_level->next;
            continue;
        }
        tmp = xmlGetProp(report_level, "url");
        url = malloc((strlen((char *)tmp) + 1) * sizeof(char));
        if (!url) return NOMEM;
        strcpy(url, (char *)tmp);
        xmlFree(tmp);

        break;
    }
    topic_level = report_level->children;

    while (topic_level) {
        if (strcmp(topic_level->name, "topic")) {
            topic_level = topic_level->next;
            continue;
        }
        if (!xmlHasProp(topic_level, "id")) {
            topic_level = topic_level->next;
            continue;
        }
        if (!xmlHasProp(topic_level, "weight")) {
            topic_level = topic_level->next;
            continue;
        }
        tmp = xmlGetProp(topic_level, "weight");
        weight = malloc((strlen((char *)tmp) + 1) * sizeof(char));
        if (!weight) return NOMEM;
        strcpy(weight, (char *)tmp);
        xmlFree(tmp);

        if (strcmp(weight, "true")) {
            free(weight);
            topic_level = topic_level->next;
            continue;
        }

        tmp = xmlGetProp(topic_level, "id");
        id = malloc((strlen((char *)tmp) + 1) * sizeof(char));
        if (!id) return NOMEM;
        strcpy(id, (char *)tmp);
        xmlFree(tmp);

        if (!self->topics->key_exists(self->topics, id)) {
            if (MONITOR_DEBUG_LEVEL_3)
                printf(">>> [Monitor]: topic with id = %s doesn't exist.\n", id);
            free(weight);
            free(id);
            topic_level = topic_level->next;
            continue;
        }

        topic = self->topics->get(self->topics, id);
        if (!topic->documents->key_exists(topic->documents, url)) {
            topic->documents->set(topic->documents, url, NULL);
            topic->documents_number++;
        }

        topic_level = topic_level->next;
    }

    free(url);
    return OK;
}

/* TODO: topic = topics[i];*/
/* NOTE: this function allocates mem char **task */
static int
Monitor_pack_task(struct Monitor *self,
                  struct Resource *resource,
                  struct Topic **topics,
                  size_t topics_num,
                  char **task)
{
    xmlDocPtr doc;
    xmlNodePtr task_level, resource_level, topic_level, concept_level;
    xmlChar *out;
    size_t len;
    size_t i, j;

    doc = xmlNewDoc("1.0");

    task_level = xmlNewNode(NULL, "task");

    xmlDocSetRootElement(doc, task_level);

    resource_level = xmlNewChild(task_level, NULL, "resource", NULL);
    xmlNewProp(resource_level, "title", resource->title);
    xmlNewProp(resource_level, "url", resource->url);

    for (i = 0; i < topics_num; i++) {
        topic_level = xmlNewChild(task_level, NULL, "topic", NULL);
        xmlNewProp(topic_level, "id", topics[i]->id);
        xmlNewProp(topic_level, "title", topics[i]->title);

        for (j = 0; j < topics[i]->concepts_number; j++) {
            concept_level = xmlNewChild(topic_level, NULL, "concept", NULL);
            xmlNewProp(concept_level, "name", topics[i]->concepts[j]);
        }
    }

    xmlDocDumpFormatMemoryEnc(doc, &out, &len, "UTF-8", 1);

    *task = malloc((strlen((char *)out) + 1) * sizeof(char));
    if (!*task) return NOMEM; /* TODO: free in caller */

    strcpy(*task, (char *)out);

    xmlFree(out);
    xmlFreeDoc(doc);

    return OK;

}

static int
Monitor_distribute_tasks(struct Monitor *self,
                         void *sender)
{
    /* NOTE: tmp solution sends only 1 task */

    struct Topic **chain;
    char *task;
    int res;
    size_t num = 4; /* tmp const*/
    size_t i;


    chain = malloc(num * sizeof(struct Topic *));
    if (!chain) return NOMEM; /* TODO: free mem */

    chain[0] = self->topics->get(self->topics, "000.000.000");
    chain[1] = self->topics->get(self->topics, "010.000.000");
    chain[2] = self->topics->get(self->topics, "010.010.000");
    chain[3] = self->topics->get(self->topics, "010.010.010");

    printf("%s\n", chain[0]->title);
    printf("%s\n", chain[1]->title);
    printf("%s\n", chain[2]->title);
    printf("%s\n", chain[3]->title);

    Monitor_pack_task(self, self->resources[0], chain, num, &task);

    if (MONITOR_DEBUG_LEVEL_1)
        printf(">>> [task ventilator]: task had been packed:\n%s\n", task);

    s_send(sender, task, strlen(task));

    if (MONITOR_DEBUG_LEVEL_1)
        printf(">>> [task ventilator]: Task had been send.\n");

    return OK;
}

static int
Monitor_pack_number_xml(struct Monitor *self,
                      char *id,
                      char **response)
{
    struct Topic *topic;
    struct Topic *child;
    char num[MAX_URL_SIZE];
    size_t len, i;

    xmlDocPtr doc;
    xmlNodePtr number_level;
    xmlChar *out;

    int ret;

    if (!self->topics->key_exists(self->topics, id)) return FAIL;
    topic = self->topics->get(self->topics, id);
    topic->documents->rewind(topic->documents);

    doc = xmlNewDoc("1.0");
    sprintf(num, "%d", topic->documents_number);
    number_level = xmlNewNode(NULL, num);
    xmlDocSetRootElement(doc, number_level);

    xmlDocDumpFormatMemoryEnc(doc, &out, &len, "UTF-8", 1);

    *response = malloc((strlen((char *)out) + 1) * sizeof(char));
    if (!*response) return NOMEM; /* TODO: free in caller & free mem above */

    strcpy(*response, (char *)out);
    xmlFree(out);
    xmlFreeDoc(doc);

    return OK;
}
static int
Monitor_pack_children_xml(struct Monitor *self,
                      char *id,
                      char **response)
{
    struct Topic *topic;
    struct Topic *child;

    const char *key;
    void *val;
    size_t len, i;

    xmlDocPtr doc;
    xmlNodePtr topics_level, topic_level;
    xmlChar *out;

    int ret;

    if (!self->topics->key_exists(self->topics, id)) return FAIL;
    topic = self->topics->get(self->topics, id);
    topic->documents->rewind(topic->documents);

    doc = xmlNewDoc("1.0");
    topics_level = xmlNewNode(NULL, "topics");
    xmlDocSetRootElement(doc, topics_level);

    for (i = 0; i < topic->children_number; i++) {
        child = topic->children[i];

        topic_level = xmlNewChild(topics_level, NULL, "topic", NULL);
        xmlNewProp(topic_level, "id", child->id);
        xmlNewProp(topic_level, "title", child->title);

    }
    xmlDocDumpFormatMemoryEnc(doc, &out, &len, "UTF-8", 1);

    *response = malloc((strlen((char *)out) + 1) * sizeof(char));
    if (!*response) return NOMEM; /* TODO: free in caller & free mem above */

    strcpy(*response, (char *)out);
    xmlFree(out);
    xmlFreeDoc(doc);

    return OK;
}

static int
Monitor_pack_docs_xml(struct Monitor *self,
                      char *id,
                      char **response)
{
    struct Topic *topic;
    const char *key;
    void *val;
    size_t len;

    xmlDocPtr doc;
    xmlNodePtr docs_level, url_level;
    xmlChar *out;

    int ret;

    ret = OK;
    if (!self->topics->key_exists(self->topics, id)) return FAIL;
    topic = self->topics->get(self->topics, id);
    topic->documents->rewind(topic->documents);

    doc = xmlNewDoc("1.0");
    docs_level = xmlNewNode(NULL, "docs");
    xmlDocSetRootElement(doc, docs_level);

    while (true) {
        topic->documents->next_item(topic->documents, &key, &val);
        if (!key) break;

        url_level = xmlNewChild(docs_level, NULL, "a", NULL);
        xmlNewProp(url_level, "href", key);
        xmlNodeSetContent(url_level, key);
    }
    xmlDocDumpFormatMemoryEnc(doc, &out, &len, "UTF-8", 1);

    *response = malloc((strlen((char *)out) + 1) * sizeof(char));
    if (!*response) return NOMEM; /* TODO: free in caller & free mem above */

    strcpy(*response, (char *)out);
    xmlFree(out);
    xmlFreeDoc(doc);

    return OK;
}

static int
Monitor_show_children(struct Monitor *self,
                      xmlNodePtr request_level,
                      char **response)
{
    xmlChar *tmp;

    char *id;
    int ret;


    ret = OK;
    if (!xmlHasProp(request_level, "id"))
        return FAIL;

    tmp = xmlGetProp(request_level, "id");
    id = malloc((strlen((char *)tmp) + 1) * sizeof(char));
    if (!id) { ret = NOMEM; goto monitor_show_docs_exit; }

    strcpy(id, (char *)tmp);

    ret = Monitor_pack_children_xml(self, id, response);

monitor_show_docs_exit:
    if (tmp) xmlFree(tmp);
    if (id) free(id);
    return ret;
}

static int
Monitor_doc_number(struct Monitor *self,
                  xmlNodePtr request_level,
                  char **response)
{
    xmlChar *tmp;

    char *id;
    int ret;


    ret = OK;
    if (!xmlHasProp(request_level, "id"))
        return FAIL;

    tmp = xmlGetProp(request_level, "id");
    id = malloc((strlen((char *)tmp) + 1) * sizeof(char));
    if (!id) { ret = NOMEM; goto monitor_show_docs_exit; }

    strcpy(id, (char *)tmp);

    ret = Monitor_pack_number_xml(self, id, response);

monitor_show_docs_exit:
    if (tmp) xmlFree(tmp);
    if (id) free(id);
    return ret;
}

static int
Monitor_show_docs(struct Monitor *self,
                  xmlNodePtr request_level,
                  char **response)
{
    xmlChar *tmp;

    char *id;
    int ret;


    ret = OK;
    if (!xmlHasProp(request_level, "id"))
        return FAIL;

    tmp = xmlGetProp(request_level, "id");
    id = malloc((strlen((char *)tmp) + 1) * sizeof(char));
    if (!id) { ret = NOMEM; goto monitor_show_docs_exit; }

    strcpy(id, (char *)tmp);

    ret = Monitor_pack_docs_xml(self, id, response);

monitor_show_docs_exit:
    if (tmp) xmlFree(tmp);
    if (id) free(id);
    return ret;
}

static int
Monitor_request_handler(struct Monitor *self,
                        const char *request,
                        char **response)
{
    xmlDocPtr doc;
    xmlNodePtr request_level;

    int ret;


    ret = OK;
    doc = xmlReadMemory(request, strlen(request), "request.xml", NULL, 0);

    if (MONITOR_DEBUG_LEVEL_1)
        printf(">>> Parsing xml request...\n");

    request_level = xmlDocGetRootElement(doc);

    if (strcmp(request_level->name, "show_docs") == 0) {
        ret = Monitor_show_docs(self, request_level, response);
        goto monitor_request_handler_exit;
    }

    if (strcmp(request_level->name, "show_children") == 0) {
        ret = Monitor_show_children(self, request_level, response);
        goto monitor_request_handler_exit;
    }

    if (strcmp(request_level->name, "doc_num") == 0) {
        ret = Monitor_doc_number(self, request_level, response);
        goto monitor_request_handler_exit;
    }

    xmlFreeDoc(doc);

monitor_request_handler_exit:
    return ret;
}

static int
Monitor_add_topic(struct Monitor *self,
                  char *buffer)
{
    /* TODO */
    return OK;
}



/*
 * takes topics from xmlDocPtr and adds
 * them to unbound topics dictionary
 *
 * TODO: function decomposition
 * TODO: do not allocate mem for
 * names. libxml has already allocated it
 * TODO: check if gotten field has correct
 * syntax(?)
 *
 */
static int
Monitor_add_topics_xmlDocPtr(struct Monitor *self,
                             xmlDocPtr doc,
                             struct ooList *topics_list)
{
    int ret;
    struct Topic *topic;
    size_t i;
    size_t new_size;

    xmlNodePtr root, topic_level, title_level, concept_level;
    xmlAttrPtr attributes;
    xmlChar *tmp;

    if (MONITOR_DEBUG_LEVEL_1)
        printf(">>> Parsing topics in *.xml...\n");

    i = 0;

    root = xmlDocGetRootElement(doc);
    topic_level = root->children;
    /* TODO: check root has right name */

    while (topic_level) {
        if (strcmp(topic_level->name, "topic")) {
            topic_level = topic_level->next;
            continue;
        }
        if (!xmlHasProp(topic_level, "id")) {
            topic_level = topic_level->next;
            continue;
        }
        ret = Topic_new(&topic, true);
        if (ret != OK) return ret;

        tmp = xmlGetProp(topic_level, "id");
        strcpy(topic->id, (char *)tmp);
        xmlFree(tmp);

        if (MONITOR_DEBUG_LEVEL_2) {
            topic->str(topic);
            printf("    id = %s\n", topic->id);
        }
        title_level = topic_level->children;

        while (title_level) {
            if (strcmp(title_level->name, "title")) {
                title_level = title_level->next;
                continue;
            }
            tmp = xmlNodeGetContent(title_level);

            /*
            length = strlen((char *) tmp);
            if (length >= MAX_NAME_LENGTH) {
                printf("!\n");
                return FAIL;
            }
            */

            strcpy(topic->title, (char *)tmp);
            xmlFree(tmp);

            if (MONITOR_DEBUG_LEVEL_2)
                printf("    title = %s\n", topic->title);

            title_level = title_level->next;
            break;
        }
        while (title_level) {
            if (!strcmp(title_level->name, "concepts")) break;
            title_level = title_level->next;
        }
        if (!title_level) {
            topic_level = topic_level->next;
            continue;
        }
        concept_level = title_level->children;

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
            /*
            length = strlen((char *) tmp);
            if (length >= MAX_NAME_LENGTH) {
                printf("!\n");
                return FAIL;
            }
            */
            topic->add_concept(topic, (char *)tmp);
            xmlFree(tmp);

            if (MONITOR_DEBUG_LEVEL_2)
                printf("        [%s]\n", topic->concepts[topic->concepts_number - 1]);

            concept_level = concept_level->next;
        }

        if (!self->topics->key_exists(self->topics, topic->id)) {
            self->topics->set(self->topics, topic->id, topic);
            topics_list->add(topics_list, topic, NULL);
        } else {
            /* maybe merge topics with same ids? */
            topic->del(topic);
        }
        topic = NULL;
        topic_level = topic_level->next;
    }

    if (MONITOR_DEBUG_LEVEL_1) printf(">>> Loading topics from xml finished.\n");
    return OK;
}

static int
Monitor_establish_dependencies(struct Monitor *self,
                               struct ooList *topics_list)
{
    struct Topic *topic, *parent;
    struct ooListItem *iterator;

    char id[TOPIC_ID_SIZE];
    size_t i;
    int ret;

    if (MONITOR_DEBUG_LEVEL_1) {
        printf(">>> Establishing dependencies between topics...\n");
        printf(">>> Topics in array %i \n", topics_list->size);
    }

    iterator = topics_list->head;

    while (iterator) {
        topic = iterator->data;

        ret = topic->parent_id(topic, id);
        if (ret != OK) return FAIL;

        /* generic_topic is not it's parent */
        /* TODO: make special return code in topic->parent_id for this case */
        /*
        if (!strcmp(id, self->generic_topic->id)) {
            iterator = topics_list->next_item(topics_list, iterator);
            continue;
        }
        */
        if (MONITOR_DEBUG_LEVEL_2)
            printf(">>> Topic's id: %s => parent's id: %s\n", topic->id, id);

        parent = self->topics->get(self->topics, id);
        if (!parent) continue;

        topic->parent = parent;
        ret = parent->add_child(parent, topic);
        if (ret != OK) return FAIL;


        iterator = topics_list->next_item(topics_list, iterator);
    }

    /* TODO: free unused topics */
    if (MONITOR_DEBUG_LEVEL_1)
        printf(">>> Establishing dependencies between topics complited.\n");

    return OK;
}

/*
 *  load topics from *.xml
 *  and establishing relationships
 *
 * TODO: free mem in FAIL cases
 */

static int
Monitor_load_topics_from_file(struct Monitor *self,
                              const char *path)
{
    int ret;

    /* struct ooDict *topics_dict; */
    struct ooList *topics_list;
    size_t topics_array_size;

    xmlDocPtr doc;
    struct Topic *tmp;

    /*
    ret = ooDict_new(&topics_dict, DICT_INIT_SIZE);
    if (ret != OK) return FAIL;
    */

    ret = ooList_new(&topics_list);
    if (ret != OK) return FAIL;

    doc = xmlReadFile(path, NULL, 0);
    if (!doc) return FAIL;

    Monitor_add_topics_xmlDocPtr(self, doc, topics_list);

    self->topics->set(self->topics, self->generic_topic->id, self->generic_topic);
    topics_list->add(topics_list, self->generic_topic, NULL);

    Monitor_establish_dependencies(self, topics_list);

    return ret;
}

static int
Monitor_str(struct Monitor *self)
{
    printf("<struct Monitor at %p>\n", self);
    return OK;
}

/* destructor */
static int
Monitor_del(struct Monitor *self)
{
    /* TODO */
    return OK;
}

static int
Monitor_init(struct Monitor *self)
{
    self->init                      = Monitor_init;
    self->del                       = Monitor_del;
    self->str                       = Monitor_str;
    self->add_topic                 = Monitor_add_topic;
    self->load_topics_from_file     = Monitor_load_topics_from_file;
    self->request_handler           = Monitor_request_handler;
    self->distribute_tasks          = Monitor_distribute_tasks;
    self->receive_result            = Monitor_receive_result;

    self->generic_topic->title = "GENERIC";

    return OK;
}

/* constructor */
extern int
Monitor_new(struct Monitor **monitor)
{
    int ret;
    struct Monitor *self;
    struct Topic *generic_topic;

    self = malloc(sizeof(struct Monitor));
    if (!self) return NOMEM;

    memset(self, 0, sizeof(struct Monitor));

    ret = Topic_new(&generic_topic, true);
    if (ret != OK) goto error;

    self->generic_topic = generic_topic;

    strcpy(generic_topic->id, GENERIC_TOPIC_ID);

    ret = ooDict_new(&self->topics, DICT_INIT_SIZE);
    if (ret != OK) goto error;

    ret = Monitor_init(self);
    if (ret != OK) goto error;

    *monitor = self;
    return OK;

error:
    Monitor_del(self);
    return ret;
}

