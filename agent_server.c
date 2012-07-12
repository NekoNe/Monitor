#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zhelpers.h"

#include "agent.h"
#include "config.h"

int main(int const argc,
         const char ** const argv)
{
    int ret;

    struct Agent *agent;

    void *context;
    void *receiver; /* messages from monitor */
    void *sender; /* messages to monitor */

    char *incomming_message;
    size_t msg_size;


    ret = Agent_new(&agent);
    if (ret != OK) return ret;

    context = zmq_init(1);
    receiver = zmq_socket(context, ZMQ_PULL);
    zmq_connect(receiver, "tcp://localhost:5569");

    while (1) {
        if (AGENT_DEBUG_LEVEL_1)
            printf(">>> [agent]: Waiting for incomming message...\n");

        incomming_message = s_recv(receiver, &msg_size);

        if (AGENT_DEBUG_LEVEL_1)
            printf(">>> [agent]: Incomming message:\n %s\n", incomming_message);

        /* handling message */

        agent->request_handler(agent, incomming_message);

        /* task complited */
        free(incomming_message);
    }

    return OK;
}
