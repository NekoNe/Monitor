#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.h"
#include "config.h"

static int
Resource_str(struct Resource *self)
{
    printf("<struct Resource at %p>\n", self);
    return OK;
}

/* destructor */
static int
Resource_del(struct Resource *self)
{
    /* TODO */
    return OK;
}

static int
Resource_init(struct Resource *self,
              const char *title,
              const char *url,
              size_t id)
{
    self->init          = Resource_init;
    self->del           = Resource_del;
    self->str           = Resource_str;

    memcpy(self->title, title, strlen(title) * sizeof(char));
    memcpy(self->url, url, strlen(url) * sizeof(char));
    self->id = id;

    return OK;
}

/* constructor */
extern int
Resource_new(struct Resource **resource,
             const char *title,
             const char *url,
             size_t id)
{
    int ret;
    struct Resource *self;

    self = malloc(sizeof(struct Resource));
    if (!self) return NOMEM;

    memset(self, 0, sizeof(struct Resource));

    self->title = malloc(strlen(title) * sizeof(char));
    if (!self->title) {
        ret = NOMEM;
        goto error;
    }
    self->url = malloc(strlen(url) * sizeof(char));
    if (!self->url) {
        ret = NOMEM;
        goto error;
    }

    ret = Resource_init(self, title, url, id);
    if (ret != OK) goto error;

    *resource = self;
    return OK;
error:
    Resource_del(self);
    return ret;
}
