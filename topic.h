
struct Topic
{
    char *id;

    char **concepts;
    size_t concepts_number;

    char *title;

    struct Topic *parent;

    struct Topic **children;
    size_t children_number;

    /** interface methods **/

    /* finding topics child by his name */
    int (*find_child)(struct Topic *self,
                      char *name,
                      struct Topic **child);

    /* topic to xml-string */
    int (*pack)(struct Topic *self,
                char *buffer,
                size_t buffer_size);

    int (*add_concept)(struct Topic *self,
                       char *name);

    int (*add_child)(struct Topic *self,
                     struct Topic *child);

    int (*parent_id)(struct Topic *self, char *id);

    int (*init)(struct Topic *self);
    int (*del)(struct Topic *self);
    int (*str)(struct Topic *self);
};

/* constructor */
extern int Topic_new(struct Topic **topic);

