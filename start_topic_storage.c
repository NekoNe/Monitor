#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "topic_storage.h"

#define FIELD_SIZE 100

int main(int const argc,
         const char  ** const argv)
{
    struct TopicStorage *storage;
    char endpoint[FIELD_SIZE];
    char field[FIELD_SIZE];

    FILE *fp;

    int ret;


    fp = fopen("topic_storage.ini", "r");
    if (fp == NULL) {
        printf(">>> [Start_Topic_Storage]: IO ERROR. can't open \"topic_storage.ini\"\n");
        return 1;
    }
    fscanf(fp,"%s %*s %s", field, endpoint);
    printf(">>> [Start_Topic_Storage]: field: %s; value: %s;\n", field, endpoint);
    fclose(fp);

    ret = TopicStorage_new(&storage, endpoint);
    if (ret != OK) return FAIL;

    storage->serve_forever(storage);

    /* we never get here */
    return OK;
}
