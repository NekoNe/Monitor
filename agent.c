#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>

#include "agent.h"
#include "config.h"

static int
Agent_crawl_resource(struct Agent *self,
                    char *title,
                    char *url)
{

    return OK;
}

static int
Agent_request_handler(struct Agent *self,
                      char *request)
{
    xmlDocPtr doc;
    xmlNodePtr task_level, resource_level;
    xmlChar *tmp;

    char *title;
    char *url;

    int ret;


    doc = xmlReadMemory(request, strlen(request), "request.xml", NULL, 0);

    if (AGENT_DEBUG_LEVEL_2)
        printf(">>> [agent]: Parsing xml request...\n");

    task_level = xmlDocGetRootElement(doc);

    while (task_level) {
        if (strcmp(task_level->name, "task")) {
            task_level = task_level->next;
            continue;
        }
        resource_level = task_level->children;

        while (resource_level) {
            if (strcmp(resource_level->name, "resource")) {
                resource_level = resource_level->next;
                continue;
            }
            if (!xmlHasProp(resource_level, "title")) {
                resource_level = resource_level->next;
                continue;
            }
            if (!xmlHasProp(resource_level, "url")) {
                resource_level = resource_level->next;
                continue;
            }
            tmp = xmlGetProp(resource_level, "title");
            title = malloc(strlen((char *)tmp));
            if (!title) return NOMEM; /* goto error; to free mem */
            strcpy(title, (char *)tmp);
            xmlFree(tmp);

            tmp = xmlGetProp(resource_level, "url");
            url = malloc(strlen((char *)tmp));
            if (!url) return NOMEM; /* goto... */
            strcpy(url, (char *)tmp);
            xmlFree(tmp);

            /* func call */
            if (AGENT_DEBUG_LEVEL_2)
                printf(">>> [agent]: Arguments has gotten: resource title = %s; url = %s\n",\
                        title, url);

            ret = Agent_crawl_resource(self, title, url);
            if (ret != OK) return ret; /* goto... */

            if (title) free(title);
            if (url) free(url);
            break;
        }
        break;
    }

    return OK;
}

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
    self->init              = Agent_init;
    self->del               = Agent_del;
    self->str               = Agent_str;
    self->request_handler   = Agent_request_handler;

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
