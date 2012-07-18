#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <libxml/parser.h>
#include <zmq.h>

#include "zhelpers.h"

#include "oodict.h"
#include "ooarray.h"
#include "oolist.h"

#include "resource.h"
#include "monitor.h"
#include "config.h"


#define DICT_INIT_SIZE 1000

static int
Monitor_push_update(struct Monitor *self,
                    xmlNodePtr request_level,
                    void *rg_ru)
{
    xmlDocPtr doc;
    xmlNodePtr root;
    xmlChar *out;

    char *msg;
    size_t len;
    int ret;

    doc = NULL;
    root = NULL;
    out = NULL;
    ret = OK;

    doc = xmlNewDoc("1.0");
    if (!doc) return FAIL;

    root = xmlNewNode(NULL, "request");
    xmlNewProp(root, "type", "update");
    xmlDocSetRootElement(doc, root);

    xmlDocDumpFormatMemoryEnc(doc, &out, &len, "UTF-8", 1);
    if (!out) { ret = FAIL; goto exit; }

    s_send(rg_ru, (char *) out, strlen(((char *)out + 1)) * sizeof(char));

    if (MONITOR_DEBUG_LEVEL_2)
        printf(">>> [Monitor]: send update task\n");
exit:
    if (out) xmlFree(out);
    if (doc) xmlFreeDoc(doc);
    return ret;
}

/* rg_ru is temp solution */
static int
Monitor_clock_handler(struct Monitor *self,
                      char *request,
                      void *rg_ru)
{
    xmlDocPtr doc;
    xmlNodePtr request_level;
    xmlChar *tmp;

    int ret;

    ret = OK;
    doc = NULL;
    request_level = NULL;
    tmp = NULL;

    doc = xmlReadMemory(request, strlen(request) + 1, "request.xml", NULL, 0);
    if (!doc) return FAIL;

    if (MONITOR_DEBUG_LEVEL_2)
        printf(">>> [Monitor_clock]: Parsing xml request...\n");

    request_level = xmlDocGetRootElement(doc);
    if (!request_level) {
        ret = FAIL;
        if (MONITOR_DEBUG_LEVEL_2)
            printf(">>> [Monitor_clock]: Can't get request root.\n");
        goto exit;
    }

    if (strcmp(request_level->name, "request") == 0) {
        if (!xmlHasProp(request_level, "type")) {
            ret = FAIL;
            if (MONITOR_DEBUG_LEVEL_2)
                printf(">>> [Monitor_clock]: Field \"type\" not found.\n");
            goto exit;
        }

        tmp = xmlGetProp(request_level, "type");

        if (strcmp((char *)tmp, "update") == 0) {
            ret = Monitor_push_update(self, request_level, rg_ru);
        }

        ret = FAIL;
        goto exit;
    }
    ret = FAIL;
    if (MONITOR_DEBUG_LEVEL_2)
        printf(">>> [Monitor_clock]: Unknown command.\n");

exit:
    if (tmp) xmlFree(tmp);
    if (doc) xmlFreeDoc(doc);
    return ret;
}
/**********  PUBLIC METHODS  **********/
void *
Monitor_sink_forever(void *monitor)
{
    struct Monitor *self;
    void *context;

    void *topic_storage;
    void *sink;

    char *income;
    size_t msg_size;
    int ret;


    self = (struct Monitor *)monitor;
    context = zmq_init(1);

    topic_storage = zmq_socket(context, ZMQ_REQ);
    zmq_connect(topic_storage, self->topic_storage_endpoint);

    sink = zmq_socket(context, ZMQ_PULL);
    zmq_bind(sink, self->sink_endpoint);

    printf(">>> [Monitor_sink]: Sink-server is ready.\n");

    while (true) {
        income = NULL;

        income = s_recv(sink, &msg_size);
        printf(">>> [Monitor_sink]: new result: \n%s\n>>> sending to TS...\n", income);

        if (!income) continue;

        s_send(topic_storage, income, (strlen(income) + 1) * sizeof(char));
        if (income) free(income);

        income = s_recv(topic_storage, &msg_size);
        printf(">>> [Monitor_sink]: topic_server resposed: \n%s\n", income);

        if (income) free(income);
    }

    return ;
}
void *
Monitor_clock_forever(void *monitor)
{
    struct Monitor *self;
    void *context;

    void *clock;
    void *rg_ru;

    char *income;
    size_t msg_size;
    int ret;


    income = NULL;
    self = (struct Monitor *)monitor;
    context = zmq_init(1);

    clock = zmq_socket(context, ZMQ_PULL);
    zmq_bind(clock, self->clock_endpoint);

    rg_ru = zmq_socket(context, ZMQ_PUSH);
    zmq_bind(rg_ru, self->rg_ru_endpoint);

    printf(">>> [Monitor]: Clock-server is ready.\n");

    while (true) {
        income = s_recv(clock, &msg_size);

        ret = Monitor_clock_handler(self, income, rg_ru);
        if (income) free(income);
    }
    return ;
}

static int
Monitor_serve_forever(struct Monitor *self)
{
    int ret;

    pthread_t clock;
    pthread_t sink;


    ret = pthread_create(&clock, NULL, self->clock_forever, (void *)self);
    if (ret) {
        printf(">>> [Monitor]: can't start clock pthread\n");
        return OK;
    }

    ret = pthread_create(&sink, NULL, self->sink_forever, (void *)self);
    if (ret) {
        printf(">>> [Monitor]: can't start sink pthread\n");
        return OK;
    }

    pthread_join(clock, NULL);
    pthread_join(sink, NULL);

    return OK;
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
    self->clock_forever             = Monitor_clock_forever;
    self->sink_forever              = Monitor_sink_forever;
    self->serve_forever             = Monitor_serve_forever;

    return OK;
}

/* constructor */
extern int
Monitor_new(struct Monitor **monitor)
{
    int ret;
    struct Monitor *self;

    self = malloc(sizeof(struct Monitor));
    if (!self) return NOMEM;

    memset(self, 0, sizeof(struct Monitor));

    ret = Monitor_init(self);
    if (ret != OK) goto error;

    *monitor = self;
    return OK;

error:
    Monitor_del(self);
    return ret;
}

