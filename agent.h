
struct Agent
{
    struct ooDict *unwatched;
    struct ooDict *watched;

    /** public methods **/

    int (*request_handler)(struct Agent *selfi,
                           char *request);

    int (*init)(struct Agent *self);
    int (*del)(struct Agent *self);
    int (*str)(struct Agent *self);
};

extern int Agent(struct Agent **agent);
