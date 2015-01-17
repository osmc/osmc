/*
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
*/
extern "C"
{
#include <stdio.h>
int parse_option(const char *line, const char *option, char *value, size_t size);
void get_cmdline_option(const char *option, char *value, size_t size);
}
