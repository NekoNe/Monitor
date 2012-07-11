
struct Agent
{
    int (*init)(struct Agent *self);
    int (*del)(struct Agent *self);
    int (*str)(struct Agent *self);
};

extern int Agent(struct Agent **agent);
