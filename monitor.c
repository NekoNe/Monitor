#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>

#include "oodict.h"
#include "ooarray.h"
#include "oolist.h"

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
Monitor_distribute_tasks(struct Monitor *self,
                         void *sender)
{
    return OK;
}


static int
Monitor_request_handler_list_topic_xml(struct Monitor *self,
                                       struct Topic *topic,
                                       char *response,
                                       const size_t response_size)
{
    xmlDocPtr doc;
    xmlNodePtr topic_list_level, topic_level;
    xmlChar *out;
    size_t len, i;

    doc = xmlNewDoc("1.0"); /* */

    topic_list_level = xmlNewNode(NULL, "topic_list");
    xmlNewProp(topic_list_level, "parent", topic->title);

    xmlDocSetRootElement(doc, topic_list_level);

    for (i = 0; i < topic->children_number; i++) {
        topic_level = xmlNewChild(topic_list_level, NULL, "topic", NULL);
        xmlNewProp(topic_level, "title", topic->children[i]->title);
    }
    xmlDocDumpFormatMemoryEnc(doc, &out, &len, "UTF-8", 1);

    if (len > response_size) return NOMEM;
    strcpy(response, (char *)out);

    xmlFree(out);

    /* printf("response: %s\n", response); */
    return OK;
}

static int
Monitor_request_handler_list_topic(struct Monitor *self,
                                   xmlDocPtr doc,
                                   xmlNodePtr request_level,
                                   char *response,
                                   size_t response_size)
{

    xmlNodePtr topic_level, prev_topic;
    xmlChar *prop_val;

    struct Topic *topic, *topic_child;
    size_t prop_size;
    char *topic_name;

    size_t i;

    if (MONITOR_DEBUG_LEVEL_3)
        printf(">>> Handling list_topic request...\n");

    topic = self->generic_topic;
    topic_level = request_level->children;

    while (topic_level) {

        if (strcmp(topic_level->name, "topic")) {
            topic_level = topic_level->next;
            continue;
        }
        if (!xmlHasProp(topic_level, "title")) {
            return SYNTAX_FAIL;
        }

        prop_val = xmlGetProp(topic_level, "title");
        prop_size = strlen((char *)prop_val);
        topic_name = malloc(prop_size * sizeof(char));
        strcpy(topic_name, (char *)prop_val);
        xmlFree(prop_val);

        if (MONITOR_DEBUG_LEVEL_3)
            printf(">>> Finding [%s] in [%s] topic...\n", topic_name, topic->title);
        topic->find_child(topic, topic_name, &topic_child);
        if (!topic_child) { printf("NOT FOUND\n"); break; }/**/
        topic = topic_child;
        topic_level = topic_level->next;
    }

    if (topic) {
        return Monitor_request_handler_list_topic_xml(self, topic, response, response_size);
    }

    return OK;
}


/* TODO: there are some repeated parts of code
 * maybe I should combine it and make "goto"?*/
static int
Monitor_request_handler(struct Monitor *self,
                        const char *request,
                        char *response,
                        size_t response_size)
{
    char *func_name;
    size_t prop_size;
    int ret;

    xmlDocPtr doc;
    xmlChar *prop_val;
    xmlNodePtr request_level;

    doc = xmlReadMemory(request, strlen(request), "request.xml", NULL, 0);

    if (MONITOR_DEBUG_LEVEL_1)
        printf(">>> Parsing xml request...\n");

    request_level = xmlDocGetRootElement(doc);

    while (request_level) {
        if (strcmp(request_level->name, "request")) {
            request_level = request_level->next;
            continue;
        }
        if (!xmlHasProp(request_level, "name")) {
            request_level = request_level->next;
            continue;
        }

        prop_val = xmlGetProp(request_level, "name");
        prop_size = strlen((char *)prop_val);
        func_name = malloc(prop_size * sizeof(char));
        strcpy(func_name, (char *)prop_val);
        xmlFree(prop_val);

        if (!strcmp(func_name, LIST_TOPIC)) {

            if (MONITOR_DEBUG_LEVEL_2)
                printf(">>> Calling %s function...\n", LIST_TOPIC);

            /* calling target function */
            ret = Monitor_request_handler_list_topic(self, doc, request_level, response, response_size);
            if (ret != OK) { return ret; }

            if (func_name) free(func_name);
            request_level = request_level->next;
            continue;
        }

        if (MONITOR_DEBUG_LEVEL_2)
            printf(">>> Unknown command [%s]\n", func_name);

        if (func_name) free(func_name);
        request_level = request_level->next;
    }
    self->generic_topic->parent = NULL; /*TODO: do not add parent to generic*/
    return OK;
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
                             struct ooDict *topics_dict,
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
        ret = Topic_new(&topic);
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

        if (!topics_dict->key_exists(topics_dict, topic->id)) {
            topics_dict->set(topics_dict, topic->id, topic);
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
                               struct ooDict *topics_dict,
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

        parent = topics_dict->get(topics_dict, id);
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

    struct ooDict *topics_dict;
    struct ooList *topics_list;
    size_t topics_array_size;

    xmlDocPtr doc;
    struct Topic *tmp;


    ret = ooDict_new(&topics_dict, DICT_INIT_SIZE);
    if (ret != OK) return FAIL;

    ret = ooList_new(&topics_list);
    if (ret != OK) return FAIL;

    doc = xmlReadFile(path, NULL, 0);
    if (!doc) return FAIL;

    Monitor_add_topics_xmlDocPtr(self, doc, topics_dict, topics_list);

    topics_dict->set(topics_dict, self->generic_topic->id, self->generic_topic);
    topics_list->add(topics_list, self->generic_topic, NULL);

    Monitor_establish_dependencies(self, topics_dict, topics_list);

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

    ret = Topic_new(&generic_topic);
    if (ret != OK) return ret;

    self->generic_topic = generic_topic;

    strcpy(generic_topic->id, GENERIC_TOPIC_ID);

    ret = Monitor_init(self);
    if (ret != OK) goto error;

    *monitor = self;
    return OK;

error:
    Monitor_del(self);
    return ret;
}

