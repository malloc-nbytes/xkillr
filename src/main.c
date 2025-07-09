#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
#include <regex.h>

#include <ncurses.h>

#include "dyn_array.h"

#define CTRL(x) ((x) & 0x1F)
#define BACKSPACE 263
#define ESCAPE 27
#define ENTER 10
#define SPACE 23

typedef struct {
        char *user;
        char *pid;
        char *cmd;
} proc;

DYN_ARRAY_TYPE(proc *, proc_ptr_array);

typedef struct {
        struct {
                int w;
                int h;
        } win;
        int scroll_offset;
        proc_ptr_array procs;
} context;

void
cleanup(void)
{
        endwin();
}

void
init_ncurses(context *ctx)
{
        initscr();
        raw();
        keypad(stdscr, TRUE);
        noecho();
        curs_set(0);
        timeout(100);

        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        int half_width = max_x / 2;

        ctx->win.w = max_x;
        ctx->win.h = max_y;

        scrollok(stdscr, TRUE);
}

proc *
get_process_info(const char *pid_str)
{
        proc *proc = malloc(sizeof(proc));
        if (!proc) {
                perror("malloc");
                return NULL;
        }

        char path[256] = {0}, line[256] = {0};
        FILE *status;
        char cmd[256] = "N/A";
        struct passwd *pw;
        uid_t uid;
        char *user = strdup("unknown");
        char *pid = strdup(pid_str);

        // Open /proc/[pid]/status
        snprintf(path, sizeof(path), "/proc/%s/status", pid_str);
        status = fopen(path, "r");
        if (!status) {
                free(user);
                free(pid);
                proc->user = NULL;
                return NULL;
        }

        // Read process information
        while (fgets(line, sizeof(line), status)) {
                if (strncmp(line, "Name:", 5) == 0) {
                        sscanf(line, "Name: %s", cmd);
                } else if (strncmp(line, "Uid:", 4) == 0) {
                        sscanf(line, "Uid: %u", &uid);
                        pw = getpwuid(uid);
                        if (pw) {
                                free(user);
                                user = strdup(pw->pw_name);
                        }
                }
        }
        fclose(status);

        proc->user = user;
        proc->pid = pid;
        proc->cmd = strdup(cmd);

        return proc;
}

int
regex(const char *pattern,
      const char *s)
{
        regex_t regex;
        int reti;

        reti = regcomp(&regex, pattern, REG_ICASE);
        if (reti) {
                perror("regex");
                return 0;
        }

        reti = regexec(&regex, s, 0, NULL, 0);

        regfree(&regex);

        if (!reti) return 1;
        else return 0;
}

int
iota(int forward)
{
        assert(forward == -1 || forward >= 0);
        static int __f = 0;
        int res = __f;
        if (forward == -1) {
                __f = 1;
        } else {
                __f += forward;
        }
        return res;
}

void
dump_procs(const context *ctx)
{
        printw("%-8s %-8s %s\n", "USER", "PID", "COMMAND");
        iota(-1), iota(1);
        for (size_t i = 1; i < ctx->procs.len; ++i) {
                const proc *p = ctx->procs.data[i];
                mvwprintw(stdscr, iota(1), 1, "%-8s %-8s %-8s", p->user, p->pid, p->cmd);
        }
        wrefresh(stdscr);
}

void
input_loop(const context *ctx)
{
        while (1) {
                dump_procs(ctx);

                char ch = getch();

                switch (ch) {
                case CTRL('q'): {
                        return;
                } break;
                }

                printw("%c", ch);
        }
}

int
main(void)
{
        context ctx = (context) {
                .win = {
                        .w = 0,
                        .h = 0,
                },
                .scroll_offset = 0,
                .procs = dyn_array_empty(proc_ptr_array),
        };

        init_ncurses(&ctx);
        --ctx.win.h; // for input bar
        atexit(cleanup);

        DIR *proc_dir;
        struct dirent *entry;

        if (!(proc_dir = opendir("/proc"))) {
                perror("opendir");
                return 1;
        }

        /* printf("%-8s %-8s %s\n", "USER", "PID", "COMMAND"); */
        while ((entry = readdir(proc_dir))) {
                if (entry->d_type == DT_DIR
                    && strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
                        dyn_array_append(ctx.procs, get_process_info(entry->d_name));
                }
        }

        input_loop(&ctx);

        for (size_t i = 0; i < ctx.procs.len; ++i) {
                free(ctx.procs.data[i]->user);
                free(ctx.procs.data[i]->pid);
                free(ctx.procs.data[i]->cmd);
                free(ctx.procs.data[i]);
        }
        dyn_array_free(ctx.procs);

        return 0;
}
