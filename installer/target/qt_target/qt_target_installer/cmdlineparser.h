extern "C"
{
#include <stdio.h>
int parse_option(const char *line, const char *option, char *value, size_t size);
void get_cmdline_option(const char *option, char *value, size_t size);
}
