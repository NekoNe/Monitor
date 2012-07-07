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
 * TODO: do not allocate mame for
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
        } else
            topic->del(topic);
        topic = NULL;
        topic_level = topic_level->next;
    }

    if (MONITOR_DEBUG_LEVEL_1) printf(">>> Loading topics from xml finished.\n");
    return OK;
}

static int
Monitor_add_topics_from_file(struct Monitor *self,
                   const char *path,
                   struct ooDict *topics)
{
    xmlDocPtr doc;

    doc = xmlReadFile(path, NULL, 0);
    if (!doc) return FAIL;
    /*
    Monitor_add_topics_xmlDocPtr(self, doc, topics);
    */
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
    /*
    iterator = topics_list->head;
    topic = iterator->data;
    printf("id %s\n", topic->id);

    iterator = topics_list->next_item(topics_list, iterator);
    topic = iterator->data;
    printf("id %s\n", topic->id);
    */

    iterator = topics_list->head;

    while (iterator) {
        topic = iterator->data;

        ret = topic->parent_id(topic, id);
        if (ret != OK) return FAIL;

        if (MONITOR_DEBUG_LEVEL_2)
            printf(">>> Topic's id: %s => parent's id: %s\n", topic->id, id);

        parent = topics_dict->get(topics_dict, id);
        if (!parent) continue;

        topic->parent = parent;
        ret = parent->add_child(parent, topic);
        if (ret != OK) return FAIL;


        iterator = topics_list->next_item(topics_list, iterator);
    }

    /*
    for (i = 0; i < topics_array->size; i++) {
        topic = topics_array->get_item(topics_array, i);
        ret = topic->parent_id(topic, id);
        if (ret != OK) return FAIL;

        parent = topics_dict->get(topics_dict, id);
        if (!parent) continue;

        topic->parent = parent;
        ret = parent->add_child(parent, topic);
        if (!ret) return FAIL;
    }
    */
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

    /*
    generic_topic->id = malloc(TOPIC_ID_SIZE * sizeof(char));
    if (!generic_topic->id) {
        ret = NOMEM;
        goto error;
    }
    */
    strcpy(generic_topic->id, GENERIC_TOPIC_ID);


    ret = Monitor_init(self);
    if (ret != OK) goto error;

    *monitor = self;
    return OK;

error:
    Monitor_del(self);
    return ret;
}

