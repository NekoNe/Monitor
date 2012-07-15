#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <zmq.h>
#include "zhelpers.h"

#include "config.h"
#include "monitor.h"
#include "topic.h"
#include "resource.h"

#define MAX_RESPONSE_SIZE 10000


void *monitor_client_service(void *arg)
{
    int ret;
    struct Monitor *monitor;

    void *context;
    void *responder;
    char *request;
    char *response;
    size_t msg_size;

    monitor = (struct Monitor *)arg;

    context = zmq_init(1);
    responder = zmq_socket(context, ZMQ_REP);
    zmq_bind(responder, "tcp://*:5555");

    printf(">>> [client_service]: Server started. Waiting for incomming messages...\n");

    while (true) {
        request = s_recv(responder, &msg_size);
        printf(">>> Request: %s\n", request);

        ret = monitor->request_handler(monitor, request, &response);
        free(request);
        s_send(responder, response, strlen(response) + 1);
        free(response);
    }

    zmq_close(responder);
    zmq_term(context);
    pthread_exit(NULL);
    return ;
}

void *monitor_task_ventilator(void *arg)
{
    int ret;
    struct Monitor *monitor;
    char *msg;

    void *context;
    void *sender;

    monitor = (struct Monitor *)arg;

    context = zmq_init(1);
    sender = zmq_socket(context, ZMQ_PUSH);
    zmq_bind(sender, "tcp://*:5569");

    printf(">>> [task ventilator]: Press Enter when the agents are ready: ");
    getchar();
    printf(">>> [task ventilator]: Task ventilator is ready. Sending tasks for agents...\n");
    /* while (true) { */
    monitor->distribute_tasks(monitor, sender);

        /* } */

    zmq_close(sender);
    zmq_term(context);
    pthread_exit(NULL);
    return ;
}

void *monitor_sink(void *arg)
{
    int ret;
    struct Monitor *monitor;

    void *context;
    void *receiver;

    char *result;
    size_t msg_size;

    monitor = (struct Monitor *)arg;

    context = zmq_init(1);
    receiver = zmq_socket(context, ZMQ_PULL);
    zmq_bind(receiver, "tcp://*:5558");

    printf(">>> [sink]: Sink is ready. Waiting results from agents...\n");

    while (true) {
        result = s_recv(receiver, &msg_size);
        ret = monitor->receive_result(monitor, result);

        free(result);
    }

    zmq_close(receiver);
    zmq_term(context);
    pthread_exit(NULL);

    return ;
}

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

    pthread_t task_ventilator; /* distributes tasks over agents */
    pthread_t client_service; /* handles messages from clients */
    pthread_t sink_service;

    /** init monitor **/

    /* creating monitor */
    ret = Monitor_new(&monitor);
    if (ret != OK) return ret;

    /* loading data */
    ret = monitor->load_topics_from_file(monitor, "topics.xml");
    if (ret != OK) return ret;

    /* init resources */

    monitor->resources = malloc(1 * sizeof(struct Resource *));
    Resource_new(&monitor->resources[0], "businesspravo", "http://www.businesspravo.ru/Docum", 0);
    monitor->resources_number = 1;


    ret = pthread_create(&client_service, NULL, monitor_client_service, (void *)monitor);
    if (ret) {
        printf(">>> [init monitor]: ERROR: pthread_create returned %d\n", ret);
        return OK;
    }
    ret = pthread_create(&task_ventilator, NULL, monitor_task_ventilator, (void *)monitor);
    if (ret) {
        printf(">>> [init monitor]: ERROR: pthread_create returned %d\n", ret);
        return OK;
    }

    ret = pthread_create(&sink_service, NULL, monitor_sink, (void *)monitor);
    if (ret) {
        printf(">>> [init monitor]: ERROR: pthread_create returned %d\n", ret);
        return OK;
    }

    pthread_join(client_service, NULL);
    pthread_join(task_ventilator, NULL);
    pthread_join(sink_service, NULL);

    return OK;
}
