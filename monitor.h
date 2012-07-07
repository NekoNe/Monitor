#include "oodict.h"

struct Monitor
{
    struct Topic *generic_topic;

    size_t agents_number;

    /** interface methods **/

    int (*add_topic)(struct Monitor *self,
                     char *buffer);
    /* add topics from file */
    int (*load_topics_from_file)(struct Monitor *self,
                                const char *path);


    int (*init)(struct Monitor *selfi);
    int (*del)(struct Monitor *self);
    int (*str)(struct Monitor *self);
};

/* constructor */
extern int Monitor_new(struct Monitor **monitor);

