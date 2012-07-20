#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "config.h"
#include "monitor.h"

#define FIELD_SIZE 100

int main(int argc,
         const char ** const argv)
{
    struct Monitor *monitor;
    char clock_endpoint[FIELD_SIZE];
    char rg_ru_endpoint[FIELD_SIZE];
    char topic_storage_endpoint[FIELD_SIZE];
    char sink_endpoint[FIELD_SIZE];

    char field[FIELD_SIZE];

    FILE *fp;

    int ret;

    fp = fopen("monitor.ini", "r");
    if (fp == NULL) {
        printf(">>> [Start_Monitor]: IO ERROR. can't open \"monitor.ini\"\n");
        return 1;
    }

    fscanf(fp,"%s %*s %s", field, clock_endpoint);
    printf(">>> [Start_Monitor]: field: %s; value: %s;\n", field, clock_endpoint);

    fscanf(fp,"%s %*s %s", field, rg_ru_endpoint);
    printf(">>> [Start_Topic_Monitor]: field: %s; value: %s;\n", field, rg_ru_endpoint);

    fscanf(fp,"%s %*s %s", field, topic_storage_endpoint);
    printf(">>> [Start_Topic_Monitor]: field: %s; value: %s;\n", field, topic_storage_endpoint);

    fscanf(fp,"%s %*s %s", field, sink_endpoint);
    printf(">>> [Start_Topic_Monitor]: field: %s; value: %s;\n", field, sink_endpoint);

    fclose(fp);

    ret = Monitor_new(&monitor);
    if (ret != OK) return FAIL;

    monitor->rg_ru_endpoint = rg_ru_endpoint;
    monitor->clock_endpoint = clock_endpoint;
    monitor->topic_storage_endpoint = topic_storage_endpoint;
    monitor->sink_endpoint = sink_endpoint;

    monitor->serve_forever(monitor);

    return OK;
}
