
struct TopicStorage
{
    const char *endpoint;

    /* root of topic-tree */
    struct Topic *generic;

    /* all topics */
    struct ooDict *topics;

    /* topics that have no path to generic */
    struct ooDict *unlinked;

    /* leaves of topic tree */
    struct ooDict *childless;


    /** interface methods **/

    /* the main method that starts service */
    int (*serve_forever)(struct TopicStorage *self);

    /* takes xml-request. hanldes it. and make xml-reply */
    int (*request_handler)(struct TopicStorage *self,
                           char *request,
                           char **reply);

    int (*init)(struct TopicStorage *self,
                const char endpoint);

    int (*str)(struct TopicStorage *self);

    /* destructor */
    int (*del)(struct TopicStorage *self);
};

/* constructor */
extern int TopicStorage_new(struct TopicStorage **topic_storage,
                            const char *endpoint);
