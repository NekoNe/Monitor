#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/HTMLparser.h>
#include <curl/curl.h>

#include "agent.h"
#include "config.h"

#define PAGE_SIZE 100000

struct MemoryChank
{
    char *buffer;
    size_t size;
};


/* allocator for curl */
static size_t
Agent_write_memory(void *contents,
                   size_t size,
                   size_t nmemb,
                   void *userp)
{
    size_t real_size;
    struct MemoryChank *mem;

    real_size = size * nmemb;
    mem = (struct MemoryChank *)userp;

    mem->buffer = realloc(mem->buffer, (mem->size + real_size + 1)\
            * sizeof(char));
    if (!mem->buffer) return 0;

    memcpy(&(mem->buffer[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->buffer[mem->size] = 0;

    return real_size;
}

static int
Agent_walk_through_htmlTree(struct Agent *self,
                      xmlNodePtr node)
{
    xmlNodePtr child;
    xmlChar *tmp;
    char *property;
    int ret;

    if (!node) return OK;

    child = node->children;
    while (child) {
        ret = Agent_walk_through_htmlTree(self, child);
        if (ret != OK) return ret;

        if (child->name == NULL) {
            child = child->next;
            continue;
        }
        if (strcmp(child->name, "a") == 0) {
            /* TODO: decompose */
            if (!xmlHasProp(child, "href")) {
                child = child->next;
                continue;
            }

            tmp = xmlGetProp(child, "href");
            if (tmp == NULL) {
                child = child->next;
                continue;
            }

            property = NULL;
            property = malloc((strlen((char *)tmp) + 1) * sizeof(char));
            if (!property) return FAIL;

            strcpy(property, (char *)tmp);
            xmlFree(tmp);
            printf("href = %s\n", property);
            free(property);

        }


        child = child->next;
    }
    return OK;
}

static int
Agent_parse_page(struct Agent *self,
                 char *buffer)
{
    htmlDocPtr doc;
    xmlNodePtr root;

    doc = htmlReadMemory(buffer, strlen(buffer), "buisnesspravo.ru", NULL, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);

    if (doc == NULL) {
        if (AGENT_DEBUG_LEVEL_1) printf(">>> [agent]: html parse failed\n");
        return FAIL;
    }

    root = xmlDocGetRootElement(doc);
    Agent_walk_through_htmlTree(self, root);
    printf("root's name: %s\n", root->name);

}

static int
Agent_crawl_resource(struct Agent *self,
                     char *title,
                     char *url)
{
    CURL *curl;
    CURLcode res;
    char *buffer;
    char error_buffer[CURL_ERROR_SIZE];
    struct MemoryChank chunk;

    chunk.buffer = malloc(1 * sizeof(char));
    chunk.size = 0;

    buffer = NULL;
    curl = curl_easy_init();

    if (!curl) return FAIL;

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Agent_write_memory);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);
    printf("error: %s\n", error_buffer);
    /* printf("ololol: %s\n", chunk.buffer); */



    Agent_parse_page(self, chunk.buffer);

    curl_easy_cleanup(curl);

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
