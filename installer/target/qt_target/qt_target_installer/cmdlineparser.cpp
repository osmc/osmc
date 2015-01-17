/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
/* Thanks to http://stackoverflow.com/questions/10536261/how-to-know-where-i-am-booting-from */
extern "C"
{
#include "cmdlineparser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_option(const char *line, const char *option, char *value, size_t size)
{
    const char *p0, *p1;
    int len;

    p0 = strstr(line, option);
    if (!p0)
        return 0;
    p0 += strlen(option);
    p1  = strchr(p0, ' ');
    if (!p1)
       p1 = p0 + strlen(p0);
    len = p1 - p0;
    if (len > size - 1)
        len = size - 1;
    memcpy(value, p0, len);
    value[len] = '\0';
    return len;
}

void get_cmdline_option(const char *option, char *value, size_t size)
{
    FILE  *fp;
    char  *line = NULL;
    size_t len = 0;
    size_t read;

    if (!size)
        return;
    *value = '\0';
    fp = fopen("/proc/cmdline", "r");
    if (fp == NULL)
         return;
    while ((read = getline(&line, &len, fp)) != -1) {
        if (parse_option(line, option, value, size))
            break;
    }
    fclose(fp);
    if (line)
        free(line);
    return;
}
}
