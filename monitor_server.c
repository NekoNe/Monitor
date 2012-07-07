#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zmq.h>
#include "zhelpers.h"

#include "config.h"
#include "monitor.h"


#define MAX_RESPONSE_SIZE 500

int main(int const argc,
         const char ** const argv)
{
    int ret;
    struct Monitor *monitor;

    void *context;
    void *responder;
    char *request;
    char response[MAX_RESPONSE_SIZE];
    size_t msg_size;

    /* creating monitor */
    ret = Monitor_new(&monitor);
    if (ret != OK) return ret;

    /* loading data */
    ret = monitor->load_topics_from_file(monitor, "topics.xml");
    if (ret != OK) return ret;

    /* start server */
    context = zmq_init(1);
    responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, "tcp://*:5555");

    printf(">>> Server started. Waiting for incomming messages...\n");

    strcpy(response, "OK");

    while (true) {

        request = s_recv(responder, &msg_size);
        printf(">>> Request: %s\n", request);

        /* monitor->request_handler(monitor, request, response, MAX_RESPONSE_SIZE); */

        s_send(responder, response, strlen(response));

    }

    zmq_close(responder);
    zmq_term(context);

    return OK;
}
