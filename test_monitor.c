#include <stdio.h>
#include <stdlib.h>

#include "monitor.h"
#include "topic.h"
#include "config.h"

int main(int argc, char **argv)
{
    int ret;
    struct Monitor *monitor;

    ret = Monitor_new(&monitor);
    if (ret != OK) return ret;
    /*
    monitor->str(monitor);
    */
    ret = monitor->load_topics_from_file(monitor, "topics.xml");
    if (ret != OK) return ret;

    printf(" %i \n", monitor->generic_topic->children_number);


    return OK;
}
