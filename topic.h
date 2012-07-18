
struct Topic
{
    char *id;

    char **concepts;
    size_t concepts_number;

    char *title;

    char *parent_id;

    char **children_id;
    size_t children_number;

    struct ooDict *documents;
    /* TODO: it must be not size_t but long-arithm number or string */
    size_t documents_number;

    /** interface methods **/

    /* checks id's syntax */
    int (*verify_id)(struct Topic *self,
                     char *id);

    /* alloc mem and makes string copy */
    int (*set_title)(struct Topic *self,
                     char *title);

    /* alloc mem and makes string copy */
    int (*set_id)(struct Topic *self,
                     char *id);

    /* finding topics child by his id */
    int (*find_child)(struct Topic *self,
                      char *id,
                      struct Topic **child);

    /* add new keyword */
    int (*add_concept)(struct Topic *self,
                       char *name);

    int (*add_child)(struct Topic *self,
                     struct Topic *child);

    /* getting parent's id */
    int (*predict_parent_id)(struct Topic *self, char *id);

    int (*init)(struct Topic *self);
    /* destructor */
    int (*del)(struct Topic *self);
    int (*str)(struct Topic *self);
};

/* constructor */
extern int Topic_new(struct Topic **topic, int has_documents_dict);

