
struct Resource
{
    char *title;
    char *url;

    size_t id;

    int (*init)(struct Resource *self);
    int (*del)(struct Resource *self);
    int (*str)(struct Resource *self);
};

extern int Resource(struct Resource **resource,
                    const char *title,
                    const char *url,
                    size_t id);
