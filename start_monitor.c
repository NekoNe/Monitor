#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "config.h"
#include "monitor.h"

int main(int argc,
         const char ** const argv)
{
    struct Monitor *monitor;
    char *clock_endpoint = "tcp://*:5549";
    char *rg_ru_endpoint = "tcp://*:5559";
    char *topic_storage_endpoint = "tcp://localhost:5554";
    char *sink_endpoint = "tcp://*:5569";

    int ret;


    ret = Monitor_new(&monitor);
    if (ret != OK) return FAIL;

    monitor->rg_ru_endpoint = rg_ru_endpoint;
    monitor->clock_endpoint = clock_endpoint;
    monitor->topic_storage_endpoint = topic_storage_endpoint;
    monitor->sink_endpoint = sink_endpoint;

    monitor->serve_forever(monitor);

    return OK;
}
