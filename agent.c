#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "agent.h"
#include "config.h"

static int
Agent_str(struct Agent *self)
{
    printf("<struct Agent at %p>\n", self);
    return OK;
}

/* destructor */
static int
Agent_del(struct Agent *self)
{
    /* TODO */
    return OK;
}

static int
Agent_init(struct Agent *self)
{
    self->init          = Agent_init;
    self->del           = Agent_del;
    self->str           = Agent_str;

    return OK;
}

/* constructor */
extern int
Agent_new(struct Agent **agent)
{
    int ret;
    struct Agent *self;

    self = malloc(sizeof(struct Agent));
    if (!self) return NOMEM;

    memset(self, 0, sizeof(struct Agent));

    ret = Agent_init(self);
    if (ret != OK) goto error;

    *agent = self;
    return OK;
error:
    Agent_del(self);
    return ret;
}
