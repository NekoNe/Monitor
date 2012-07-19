#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libxml/parser.h>

#include "topic.h"
#include "oodict.h"
#include "config.h"

#define DICT_INIT_SIZE 1000

static int
Topic_verify_id(struct Topic *self,
                char *id)
{
    size_t len, i;

    if (!id) return FAIL;

    len = strlen(id);
    if (len != TOPIC_ID_SIZE - 1) {  return FAIL; }

    for (i = 0; i < len; i++) {
        if (i % (ID_DIGIT_SIZE + ID_SEPAR_SIZE) == ID_DIGIT_SIZE) {
            if (id[i] != '.') { printf("lol");return FAIL; }
        }
        else {
            if (!isdigit(id[i])) return FAIL;
        }
    }

    return OK;
}

/* TODO: set_title and set id are very similar
 * it may be generalized */
static int
Topic_set_title(struct Topic *self,
                char *title)
{
    char *new_title;


    if (self->title) {
        if (strlen(self->title) >= strlen(title)) {
            strcpy(self->title, title);
            return OK;
        }
        new_title = malloc((strlen(title) + 1) * sizeof(char));
        if (!new_title) return FAIL;
        strcpy(new_title, title);

        free(self->title);
        title = new_title;
    }

    self->title = malloc((strlen(title) + 1) * sizeof(char));
    if (!self->title) return FAIL;
    strcpy(self->title, title);

    return OK;
}

static int
Topic_set_id(struct Topic *self,
             char *id)
{
     char *new_id;

    if (self->verify_id(self, id) != OK) return FAIL;

    if (self->id) {
        if (strlen(self->id) >= strlen(id)) {
            strcpy(self->id, id);
            return OK;
        }
        new_id = malloc((strlen(id) + 1) * sizeof(char));
        if (!new_id) return FAIL;
        strcpy(new_id, id);

        free(self->id);
        id = new_id;
    }

    self->id = malloc((strlen(id) + 1) * sizeof(char));
    if (!self->id) return FAIL;
    strcpy(self->id, id);

    return OK;
}

static int
Topic_get_child(struct Topic *self,
                 char *id,
                 struct Topic **child)
{
    /* TODO */
    return OK;
}

/* TODO: write it right way */
static int
Topic_predict_parent_id(struct Topic *self,
                        char *id)
{
    size_t digits;
    size_t window;

    size_t pointer;
    char zero_digit[] = "000";

    digits = ID_DIGIT_NUMBER;

    window = ID_DIGIT_SIZE + ID_SEPAR_SIZE;
    strcpy(id, self->id);

    pointer = (digits - 1) * window;

    while (1) {
        if (!strncmp(id + pointer, zero_digit, ID_DIGIT_NUMBER)) {
            pointer -= window;
            continue;
        }
        strncpy(id + pointer, zero_digit, ID_DIGIT_NUMBER);
        return OK;
    }
    return FAIL;
}

static int
Topic_add_child(struct Topic *self,
                struct Topic *child)
{
    char **new_children;

    /* TODO: check child exists */
    new_children = realloc(self->children_id, (self->children_number + 1) * sizeof(char *));
    if (!new_children) return NOMEM;

    self->children_id = new_children;
    self->children_id[self->children_number] = child->id;
    self->children_number++;

    return OK;
}

/* TODO: make recoils at NOMEM cases */
static int
Topic_add_concept(struct Topic *self,
                  char *name)
{
    char **new_concepts;


    new_concepts = realloc(self->concepts, (self->concepts_number + 1) * sizeof(char *));
    if (!new_concepts) return NOMEM;
    self->concepts = new_concepts;

    self->concepts_number++;

    self->concepts[self->concepts_number - 1] = malloc(MAX_NAME_LENGTH * sizeof(char));
    if (!self->concepts[self->concepts_number - 1]) return NOMEM;

    strcpy(self->concepts[self->concepts_number - 1], name);

    return OK;
}

static int
Topic_str(struct Topic *self)
{
    printf("<struct Topic at %p> title: %s id: %s\n", self, self->title, self->id);
    return OK;
}

/* destructor */
static int
Topic_del(struct Topic *self)
{
    int ret;
    size_t i;

    /*
    if (!self) return OK;
    if (self->id) free(self->id);
    if (self->title) free(self->title);

    if (self->children_id) {
        for (i = 0; i < self->children_number; i++) {
            if (self->children_id[i]) free(self->children_id[i]);
        }
        free(self->children_id);
    }
    ret = self->documents->del(self->documents);
    free(self);
    */
    return ret;
}

static int
Topic_init(struct Topic *self)
{
    self->init                  = Topic_init;
    self->del                   = Topic_del;
    self->str                   = Topic_str;

    self->verify_id             = Topic_verify_id;
    self->set_title             = Topic_set_title;
    self->set_id                = Topic_set_id;
    self->get_child             = Topic_get_child;
    self->add_concept           = Topic_add_concept;
    self->add_child             = Topic_add_child;
    self->predict_parent_id     = Topic_predict_parent_id;

    return OK;
}

/* constructor */
extern int
Topic_new(struct Topic **topic)
{
    int ret;
    struct Topic *self;


    self = malloc(sizeof(struct Topic));
    if (!self) return NOMEM;

    memset(self, 0, sizeof(struct Topic));

    self->id = malloc(TOPIC_ID_SIZE * sizeof(char));
    if(!self->id) {
        ret = NOMEM;
        goto error;
    }

    ret = ooDict_new(&self->documents, DICT_INIT_SIZE);
    if (ret != OK) goto error;

    ret = Topic_init(self);
    if (ret != OK) goto error;

    *topic = self;
    return OK;

error:
    Topic_del(self);
    return ret;
}

