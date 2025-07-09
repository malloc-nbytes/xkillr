#include <stdio.h>
#include <stdlib.h>

#include "flags.h"
#include "copying.h"
#include "config.h"

void
usage(void)
{
        printf("xkillr version " VERSION ", Copyright (C) 2025 malloc-nbytes.\n\n");
        printf("xkillr comes with ABSOLUTELY NO WARRANTY.\n");
        printf("This is free software, and you are welcome to redistribute it\n");
        printf("under certain conditions; see command `copying`.\n\n");

        printf("Compilation Information:\n");
        printf("| cc: " COMPILER_NAME "\n");
        printf("| path: " COMPILER_PATH "\n");
        printf("| ver.: " COMPILER_VERSION "\n");
        printf("| flags: " COMPILER_FLAGS "\n\n");

        printf("Github repository: https://www.github.com/malloc-nbytes/xkillr.git/\n\n");
        printf("Send bug reports to:\n");
        printf("  - https://github.com/malloc-nbytes/xkillr/issues\n");
        printf("  - or " PACKAGE_BUGREPORT "\n\n");

        printf("Usage: xkillr [options...] [pid|name]\n");
        printf("Options:\n");
        printf("    -%c, --%s       show this menu\n", FLAG_1HY_HELP, FLAG_2HY_HELP);
        printf("    -%c, --%s       show running procs\n", FLAG_1HY_LIST, FLAG_2HY_LIST);
        printf("        --%s    show copying information\n", FLAG_2HY_COPYING);
        exit(0);
}

void
copying(void)
{
        printf(COPYING1);
        printf(COPYING2);
        printf(COPYING3);
        printf(COPYING4);
        printf(COPYING5);
        printf(COPYING6);
        exit(0);
}
