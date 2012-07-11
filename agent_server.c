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

    void *context;
    void *receiver; /* messages from monitor */
    void *sender; /* messages to monitor */

    char *incomming_message;
    size_t msg_size;

    context = zmq_init(1);
    receiver = zmq_socket(context, ZMQ_PULL);
    zmq_connect(receiver, "tcp://localhost:5569");

    while (1) {
        incomming_message = s_recv(receiver, &msg_size);
        printf(">>> Incomming message:\n %s\n", incomming_message);

        /* handling message */

        /* task complited */
    }

    return OK;
}
