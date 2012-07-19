#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "topic_storage.h"

int main(int const argc,
         const char  ** const argv)
{
    struct TopicStorage *storage;
    char *endpoint = "tcp://*:5554";

    int ret;


    ret = TopicStorage_new(&storage, endpoint);
    if (ret != OK) return FAIL;

    storage->serve_forever(storage);

    /* we never get here */
    return OK;
}
