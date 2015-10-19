/*
 * Copyright © 2005-2014 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "getopt.h"

char *optarg;
int optind = 1, opterr = 1, optopt;
static int optpos;

int getopt(int argc, char * const argv[], const char *optstring)
{
    if (optind < 1) {
        optpos = 0;
        optind = 1;
    }

    if (optind >= argc || !argv[optind])
        return -1;

    if (argv[optind][0] != '-') {
        if (optstring[0] == '-') {
            optarg = argv[optind++];
            return 1;
        }
        return -1;
    }

    if (!argv[optind][1])
        return -1;

    if (argv[optind][1] == '-' && !argv[optind][2]) {
        optind++;
        return -1;
    }

    if (!optpos)
        optpos++;
    optopt = argv[optind][optpos++];

    if (!argv[optind][optpos]) {
        optind++;
        optpos = 0;
    }

    if (optstring[0] == '-' || optstring[0] == '+')
        optstring++;

    char *p = strchr(optstring, optopt);
    if (!p) {
        if (optstring[0] != ':' && opterr)
            fprintf(stderr, "%s: unrecognized option: %c\n", argv[0], optopt);
        return '?';
    }

    if (p[1] == ':') {
        if (p[2] == ':') {
            optarg = NULL;
        } else if (optind >= argc) {
            if (optstring[0] == ':')
                return ':';
            if (opterr)
                fprintf(stderr, "%s: option requires an argument: %c\n", argv[0], optopt);
            return '?';
        }
        if (p[2] != ':' || optpos) {
            optarg = argv[optind++] + optpos;
            optpos = 0;
        }
    }

    return optopt;
}
