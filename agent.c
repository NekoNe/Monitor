#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/HTMLparser.h>
#include <curl/curl.h>
#include <zmq.h>

#include "agent.h"
#include "topic.h"
#include "config.h"
#include "oodict.h"

#include "zhelpers.h"

#define PAGE_SIZE 100000
#define DICT_INIT_SIZE 1000

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
Agent_pack_response(struct Agent *self,
                    const char *url,
                    struct Topic **topics,
                    size_t topics_num,
                    char **response)
{
    xmlDocPtr doc;
    xmlNodePtr report_level, topic_level;
    xmlChar *out;
    size_t len;

    size_t i;


    doc = xmlNewDoc("1.0");

    report_level = xmlNewNode(NULL, "report");
    xmlNewProp(report_level, "url", url);
    xmlDocSetRootElement(doc, report_level);

    for (i = 0; i < topics_num; i++) {
        topic_level = xmlNewChild(report_level, NULL, "topic", NULL);
        xmlNewProp(topic_level, "id", topics[i]->id);
        xmlNewProp(topic_level, "weight", "true");
    }

    xmlDocDumpFormatMemoryEnc(doc, &out, &len, "UTF-8", 1);

    *response = malloc((strlen((char *)out) + 1) * sizeof(char));
    if (!*response) return NOMEM; /* TODO: free in caller */

    strcpy(*response, (char *)out);

    xmlFree(out);
    xmlFreeDoc(doc);

    return OK;
}
static int
Agent_walk_through_htmlTree(struct Agent *self,
                            xmlNodePtr node,
                            const char *url,
                            char *branch,
                            void *sender)
{
    xmlNodePtr child;
    xmlChar *tmp;
    char *property;
    char folder[MAX_URL_SIZE]; /* TODO: DO NOT allocate this every call even on stack! */
    char new_url[MAX_URL_SIZE];
    int ret;

    if (!node) return OK;

    child = node->children;
    while (child) {
        ret = Agent_walk_through_htmlTree(self, child, url, branch, sender);
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

            /** getting new url **/
            /* printf("href = %s\n", property); */
            ret = url_is_absolute(property);

            if (!ret) {
                url_get_folder(url, folder);
                url_get_full_path(folder, property, new_url);
            } else {
                strcpy(new_url, property);
            }

            printf("href = %s | gotten new_url = %s ", property, new_url);
            if (strstr(new_url, branch)) {
                if (!self->watched->key_exists(self->watched, new_url)) {
                    printf(" [[ADDED]]\n");
                    self->unwatched->set(self->unwatched, new_url, NULL);
                }
            } else {
                printf(" [[NOT ADDED]]\n");
            }
            if (AGENT_USER_CONTROL_1) {
                printf(">>> [agent]: user control is on: press enter... ");
                getchar();
            }

            free(property);
        }
        child = child->next;
    }
    return OK;
}

static int
Agent_parse_page(struct Agent *self,
                 char *buffer,
                 const char *url,
                 char *branch,
                 void *sender,
                 struct Topic **topics,
                 size_t topics_num)
{
    htmlDocPtr doc;
    xmlNodePtr root;

    char *response;

    doc = htmlReadMemory(buffer, strlen(buffer), url, NULL, HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);

    if (doc == NULL) {
        if (AGENT_DEBUG_LEVEL_1) printf(">>> [agent]: html parse failed\n");
        return FAIL;
    }

    root = xmlDocGetRootElement(doc);
    Agent_walk_through_htmlTree(self, root, url, branch, sender);

    Agent_pack_response(self, url, topics, topics_num, &response);

    s_send(sender, response, strlen(response) + 1);
    if (AGENT_DEBUG_LEVEL_1)
        printf(">>> [Agent]: response has been send:\n%s\n", response);
    if (response) free(response);

    xmlFreeDoc(doc);
    return OK;
}

static int
Agent_open_page(struct Agent *self,
                const char *url,
                char *branch,
                void *sender,
                struct Topic **topics,
                size_t topics_num)
{
    CURL *curl_handler;
    CURLcode res;

    char error_buffer[CURL_ERROR_SIZE];

    struct MemoryChank chunk;


    chunk.buffer = malloc(1 * sizeof(char));
    chunk.size = 0;

    curl_handler = curl_easy_init();

    if (!curl_handler) return FAIL;

    res = curl_easy_setopt(curl_handler, CURLOPT_ERRORBUFFER, error_buffer);
    if (res != CURLE_OK) return FAIL;
    res = curl_easy_setopt(curl_handler, CURLOPT_URL, url);
    if (res != CURLE_OK) return FAIL;
    res = curl_easy_setopt(curl_handler, CURLOPT_HEADER, 1);
    if (res != CURLE_OK) return FAIL;
    res = curl_easy_setopt(curl_handler, CURLOPT_WRITEFUNCTION, Agent_write_memory);
    if (res != CURLE_OK) return FAIL;
    res = curl_easy_setopt(curl_handler, CURLOPT_WRITEDATA, (void *)&chunk);
    if (res != CURLE_OK) return FAIL;

    res = curl_easy_perform(curl_handler);
    curl_easy_cleanup(curl_handler);

    if (res != CURLE_OK) {
        if (AGENT_DEBUG_LEVEL_1) printf(">>> [agent]: curl error: %s\n", error_buffer);
        return FAIL; /* TODO: free mem */ /* TODO: make 1 fail handle */
    }

    Agent_parse_page(self, chunk.buffer, url, branch, sender, topics, topics_num);

    free(chunk.buffer);

    return OK;
}

static int
Agent_crawl_resource(struct Agent *self,
                     char *title,
                     char *url,
                     void *sender,
                     struct Topic **topics,
                     size_t topics_num)
{
    const char *key;
    void *val;
    char *branch;

    key = NULL; /* TODO: free ?*/
    val = NULL;

    branch = malloc((strlen(url) + 1) * sizeof(char)); /* TODO: free */
    strcpy(branch, url);

    self->unwatched->set(self->unwatched, url, NULL);
    if (AGENT_DEBUG_LEVEL_2)
        printf(">>> [agent]: URL <%s> has been added to unwatched\n", url);

    while (true) {
        self->unwatched->rewind(self->unwatched);
        self->unwatched->next_item(self->unwatched, &key, &val);
        if (!key) break;

        if (AGENT_DEBUG_LEVEL_2)
            printf(">>> [agent]: URL <%s> has been gotten from unwatched\n", key);

        /* parse page */
        Agent_open_page(self, key, branch, sender, topics, topics_num);

        self->watched->set(self->watched, key, NULL);
        self->unwatched->remove(self->unwatched, key);
    }
    if (AGENT_DEBUG_LEVEL_2)
        printf(">>> [agent]: list of unwatched has ended\n");
    return OK;
}

static int
Agent_request_handler(struct Agent *self,
                      char *request,
                      void *sender)
{
    xmlDocPtr doc;
    xmlNodePtr task_level, resource_level, topic_level;
    xmlChar *tmp;

    char *title;
    char *url;

    struct Topic **topics, **new_topics;
    size_t topics_num;
    struct Topic *topic;

    int ret;
    size_t i;


    new_topics = NULL; topics = NULL;
    topics_num = 0;

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

        /* TODO: write it simpler */
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
            url = malloc(strlen((char *)tmp) + 1);
            if (!url) return NOMEM; /* goto... */
            strcpy(url, (char *)tmp);
            xmlFree(tmp);
            break;
        }

        /* getting topics list */

        topic_level = resource_level->next;
        while (topic_level) {
            if(strcmp(topic_level->name, "topic")) {
                topic_level = topic_level->next;
                continue;
            }
            if (!xmlHasProp(topic_level, "id")) {
                topic_level = topic_level->next;
                continue;
            }
            if (!xmlHasProp(topic_level, "title")) {
                topic_level = topic_level->next;
                continue;
            }

            ret = Topic_new(&topic, false);
            if (ret != OK) return ret;

            tmp = xmlGetProp(topic_level, "id");
            strcpy(topic->id, (char *)tmp);
            xmlFree(tmp);

            tmp = xmlGetProp(topic_level, "title");
            strcpy(topic->title, (char *)tmp);
            xmlFree(tmp);



            new_topics = NULL;
            new_topics = realloc(topics, (topics_num + 1) * sizeof(struct Topic *));
            if (!new_topics) { return NOMEM; }

            topics = new_topics;
            topics[topics_num] = topic;
            topics_num++;

            topic_level = topic_level->next;
        }

        /* func call */
        if (AGENT_DEBUG_LEVEL_2) {
            printf(">>> [agent]: Arguments has gotten: resource title = %s; url = %s\n",\
                    title, url);
            for (i = 0; i < topics_num; i++) {
                printf(">>> [agent]: Topic's title = %s; id = %s\n",\
                        topics[i]->title, topics[i]->id);
            }
        }

        ret = Agent_crawl_resource(self, title, url, sender, topics, topics_num);
        if (ret != OK) return ret; /* goto... */

        if (title) free(title);
        if (url) free(url);

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
    struct ooDict *watched;
    struct ooDict *unwatched;

    self = malloc(sizeof(struct Agent));
    if (!self) return NOMEM;

    memset(self, 0, sizeof(struct Agent));

    ret = ooDict_new(&self->watched, DICT_INIT_SIZE);
    if (ret != OK) goto error;

    ret = ooDict_new(&self->unwatched, DICT_INIT_SIZE);
    if (ret != OK) goto error;

    ret = Agent_init(self);
    if (ret != OK) goto error;

    *agent = self;
    return OK;
error:
    Agent_del(self);
    return ret;
}
