#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>

#include "topic.h"
#include "config.h"

/* TODO!!!: do it constants-free i.e.
 * no matter how many digits, digits length etc.
 * */
static int
Topic_parent_id(struct Topic *self,
                char *id)
{
    size_t digits;
    size_t window; /* DIGIT_SIZE + SEPAR_SIZE i.e. "complete" digit "000." */
    size_t pointer;
    char zero_digit[] = "000";

    digits = ID_DIGIT_NUMBER; /* TODO: here should be func call "digit_number" */
    window = ID_DIGIT_SIZE + ID_SEPAR_SIZE;
    strcpy(id, self->id);

    pointer = (digits - 1) * window;

    while (1) {
        if (!strncmp(id + pointer, zero_digit, ID_DIGIT_NUMBER)) {
            pointer -= window;
            /* TODO!!!: if id is corrupted e.g. "00.000.000"... look at size_t */
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
    struct Topic **new_children;

    if (TOPIC_DEBUG_LEVEL_2)
        printf(">>> Adding child <%s> at <%p> \n\tto <%s> at <%p>...\n", child->title, child, self->title, self);
    new_children = realloc(self->children, self->children_number + 1);
    if (!new_children) return NOMEM;

    self->children = new_children;
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
Topic_pack(struct Topic *self,
           char *buffer,
           size_t buffer_size)
{
    /* TODO */
    return OK;
}

static int
Topic_str(struct Topic *self)
{
    printf("<struct Topic at %p>\n", self);
    return OK;
}

/* destructor */
static int
Topic_del(struct Topic *self)
{
    /* TODO */
    return OK;
}

static int
Topic_init(struct Topic *self)
{
    self->init          = Topic_init;
    self->del           = Topic_del;
    self->str           = Topic_str;
    self->pack          = Topic_pack;
    self->add_concept   = Topic_add_concept;
    self->add_child     = Topic_add_child;
    self->parent_id     = Topic_parent_id;

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

    self->title = malloc(MAX_NAME_LENGTH * sizeof(char));
    if(!self->title) {
        ret = NOMEM;
        goto error;
    }

    ret = Topic_init(self);
    if (ret != OK) goto error;

    *topic = self;
    return OK;

error:
    Topic_del(self);
    return ret;
}

