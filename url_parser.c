/*
 * NOTE: this is temporary unsafe solution
 * with magic numbers
 *
 * TODO: do cool and safe url parsing library
 *
 * unsafe lies in the fact that I expect
 * there is always enough memory. User must
 * control it.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "url_parser.h"

int url_is_absolute(char *url)
{
    char *pointer;

    pointer = strstr(url, "://");
    if (pointer) return true;
    return false;
}

int url_get_folder(char *url_in, char *url_out)
{
    char *pointer;

    url_out = strcpy(url_out, url_in);

    pointer = strrchr(url_out, '/');
    if (!strchr(pointer, '.')) {
        pointer = url_out + strlen(url_out) - 1;
        if (*pointer == '/') *pointer = '\0';
        return OK;
    }
    *pointer = '\0';

    return OK;
}

int url_is_file(char *url)
{
    char *pointer;

    pointer = strrchr(url, '/');

    if (!strchr(pointer, '.')) return false;
    return true;
}

char *url_strnchr(char *str, char symb, size_t n)
{
    while ((*str != '\0') && (n > 0)) {
        if (*str == symb) n--;
        str++;
    }
    if (n != 0) return NULL;
    str--;
    return str;
}

int url_get_full_path(char *folder, char *rel_path, char *result)
{
    char *pointer;
    char *rel_start;

    strcpy(result, folder);
    if (rel_path[0] == '/') {
        pointer = url_strnchr(result, '/', 2); /*magic const. num of // after scheme*/
        if (!pointer) return FAIL;
        pointer++;
        while ((*pointer != '\0') && (*pointer != '/')) pointer++;

        strcpy(pointer, rel_path);
        return OK;
    }
    pointer = result + strlen(folder);
    *pointer = '/';

    while (1) {
        rel_start = strstr(rel_path, "../");
        if (!rel_start) break;

        rel_path += 3; /* magic const again. len of ../ */
        pointer--;
        while ((pointer > result) && (*pointer != '/')) pointer--;
    }
    strcpy(pointer + 1, rel_path);
    return OK;
}


