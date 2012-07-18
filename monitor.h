#include "oodict.h"

struct Monitor
{

    /* zmq vars */
    void *context;

    /* REQ connection with topic_storage */
    void *topic_storage;
    char *topic_storage_endpoint;

    /* PUSH connection with universal agents */
    void *ventilator;
    char *ventilator_endpoint;

    /* PULL connection with clock */
    void *clock;
    char *clock_endpoint;

    /* PUSH connections with experts */
    void **experts;
    char **experts_endpoints;
    char *rg_ru_endpoint; /* tmp */
    size_t experts_num;


    /* PULL connection with agents and experts */
    void *sink;
    char *sink_endpoint;

    struct Resource **resources;
    size_t resources_number;

    /** interface methods **/

    /* clock thread */
    void *(*clock_forever)(void *monitor);
    /* sink thread */
    void *(*sink_forever)(void *monitor);

    /* main method that starts service */
    int (*serve_forever)(struct Monitor *self);

    /* takes xml-request. handles it. ans makes xml-reply */
    int (*request_handler)(struct Monitor *self,
                          const char *request,
                          char **response);


    int (*init)(struct Monitor *selfi);
    /* destructor */
    int (*del)(struct Monitor *self);
    int (*str)(struct Monitor *self);
};

/* constructor */
extern int Monitor_new(struct Monitor **monitor);

