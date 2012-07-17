#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zmq.h>
#include <libxml/parser.h>

#include "zhelpers.h"

#include "topic.h"
#include "topic_storage.h"
#include "config.h"

static int
TopicStorage_serve_forever(struct TopicStorage *self)
{
    void *context;
    void *responder;

    char *request;
    char *reply;
    size_t msg_size;

    context = zmq_init(1);
    responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, self->endpoint);

    request = NULL;
    reply = NULL;
    msg_size = 0;

    printf(">>> [TopicStorage]: Server started. Waiting for requests...\n");

    while (true) {
        request = s_recv(responder, &msg_size);

        printf(">>> [TopicStorage]: Request:\n%s\n", request);

        /** handle request **/

        self->request_handler(self, request, &reply);

        if (request) free(request);

        if (!reply) {
            s_send(responder, "</nothing>", strlen("</nothing>") + 1);
            printf(">>> [TopicStorage]: Reply: <nothing/>\n");
        }
        else {
            s_send(responder, reply, strlen(reply) + 1);
            printf(">>> [TopicStorage]: Reply:\n%s\n", reply);
            free(reply);
        }
    }
    zmq_close(responder);
    zmq_term(context);
    /* we never get here */
    return OK;
}

static int
TopicStorage_str(struct TopicStorage *self)
{
    printf(">>> [TopicStorage]: I'am at <%p>, binded at \"%s\"\n", self, self->endpoint);
    return OK;
}

/** destructor **/
static int
TopicStorage_del(struct TopicStorage *self)
{
    if (!self) return OK;
    free(self);
    return OK;
}

static int
TopicStorage_init(struct TopicStorage *self,
                  const char *endpoint)
{
    /** binding methods **/
    self->init              = TopicStorage_init;
    self->str               = TopicStorage_str;
    self->del               = TopicStorage_del;
    self->serve_forever     = TopicStorage_serve_forever;

    /** init fields **/

    if (!endpoint) return FAIL;

    /* TODO: it will be very good to parse endpoint
     * and check it correctnes
     * */

    self->endpoint = endpoint;
    return OK;
}

/** constructor **/
extern int
TopicStorage_new(struct TopicStorage **topic_storage,
                 const char *endpoint)
{
    struct TopicStorage *self;
    int ret;

    self = malloc(sizeof(struct TopicStorage));
    if (!self) return NOMEM;

    ret = TopicStorage_init(self, endpoint);
    if (ret != OK) goto error;

    *topic_storage = self;
    return OK;
error:
    TopicStorage_del(self);
    return ret;
}
