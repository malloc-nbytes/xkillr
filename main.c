#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
#include <regex.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>

#include <ncurses.h>

#include "flags.h"
#include "dyn_array.h"
#define CLAP_IMPL
#include "clap.h"

#define TODO(msg)                                       \
        do {                                            \
                fprintf(stderr, "TODO: %s\n", msg);     \
                exit(1);                                \
        } while (0)

#define CTRL(x) ((x) & 0x1F)
#define BACKSPACE 263
#define ESCAPE 27
#define ENTER 10
#define SPACE 23

DYN_ARRAY_TYPE(char, char_array);

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
        uint32_t flags;
        int selected;
        int scroll_offset;
        proc_ptr_array procs;
        proc_ptr_array filtered_procs;
        char_array input;
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
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE); // Highlight: black text, white background
        raw();
        keypad(stdscr, TRUE);
        noecho();
        curs_set(0);
        timeout(100);

        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        ctx->win.w = max_x;
        ctx->win.h = max_y - 1; // Reserve one line for input
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

        reti = regcomp(&regex, pattern, REG_ICASE | REG_NOSUB);
        if (reti) {
                fprintf(stderr, "Could not compile regex\n");
                return 0;
        }

        reti = regexec(&regex, s, 0, NULL, 0);
        regfree(&regex);

        return !reti;
}

void
update_filtered_procs(context *ctx)
{
        // Clear existing filtered list
        ctx->filtered_procs.len = 0;

        // If input is empty, include all processes
        if (ctx->input.len == 0) {
                for (size_t i = 0; i < ctx->procs.len; ++i) {
                        dyn_array_append(ctx->filtered_procs, ctx->procs.data[i]);
                }
                return;
        }

        // Create regex string
        char *pattern = malloc(ctx->input.len + 1);
        if (!pattern) return;
        memcpy(pattern, ctx->input.data, ctx->input.len);
        pattern[ctx->input.len] = '\0';

        // Filter processes based on cmd matching input
        for (size_t i = 0; i < ctx->procs.len; ++i) {
                const proc *p = ctx->procs.data[i];
                if (regex(pattern, p->cmd) || regex(pattern, p->user) || regex(pattern, p->pid)) {
                        dyn_array_append(ctx->filtered_procs, ctx->procs.data[i]);
                }
        }

        free(pattern);

        // Adjust selection and scroll offset
        if (ctx->filtered_procs.len == 0) {
                ctx->selected = 0;
                ctx->scroll_offset = 0;
        } else if (ctx->selected >= (int)ctx->filtered_procs.len) {
                ctx->selected = ctx->filtered_procs.len - 1;
                if (ctx->selected < ctx->scroll_offset) {
                        ctx->scroll_offset = ctx->selected;
                } else if (ctx->selected >= ctx->scroll_offset + ctx->win.h - 1) {
                        ctx->scroll_offset = ctx->selected - (ctx->win.h - 2);
                }
        }
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
dump_procs(context *ctx)
{
        int max_rows = ctx->win.h; // Available rows for processes

        mvprintw(0, 0, "%-8s %-8s %s", "USER", "PID", "COMMAND");

        // Clear process display area
        for (int row = 1; row < max_rows; row++) {
                move(row, 0);
                clrtoeol();
        }

        // Calculate visible processes
        size_t start = ctx->scroll_offset;
        size_t end = start + max_rows - 1; // -1 for header
        if (end > ctx->filtered_procs.len) end = ctx->filtered_procs.len;

        // Filtered processes
        for (size_t i = start; i < end; ++i) {
                const proc *p = ctx->filtered_procs.data[i];
                int row = i - start + 1; // +1 for header

                if ((int)i == ctx->selected) {
                        attron(COLOR_PAIR(1));
                        mvprintw(row, 0, "%-8s %-8s %s", p->user, p->pid, p->cmd);
                        attroff(COLOR_PAIR(1));
                } else {
                        mvprintw(row, 0, "%-8s %-8s %s", p->user, p->pid, p->cmd);
                }
        }

        // Clear and redraw
        move(ctx->win.h, 0);
        clrtoeol();
        mvprintw(ctx->win.h, 0, "> %.*s_", (int)ctx->input.len, ctx->input.data);

        wnoutrefresh(stdscr);
        doupdate();
}

void
kill_selected_proc(context *ctx)
{
        int ok = 1;

        clear();
        if (ctx->filtered_procs.len > 0 && ctx->selected < (int)ctx->filtered_procs.len) {
                const proc *p = ctx->filtered_procs.data[ctx->selected];
                pid_t pid = atoi(p->pid);
                if (kill(pid, SIGTERM) == 0) {
                        mvprintw(0, 0, "Successfully sent SIGTERM to process %s (%s)", p->pid, p->cmd);
                } else {
                        mvprintw(0, 0, "Failed to send SIGTERM to process %s (%s): %s",
                                 p->pid, p->cmd, strerror(errno));
                        ok = 0;
                }
        } else {
                mvprintw(0, 0, "No process selected");
        }
        wrefresh(stdscr);

        if (ok) {
                return;
        } else {
                printw("\nPress any key to continue...");
        }

        // Wait for a keypress to exit
        timeout(-1);
        getch();
}

void
input_loop(context *ctx)
{
        int last_selected = -1;
        int last_scroll_offset = -1;
        size_t last_input_len = 0;

        // Initial filter
        update_filtered_procs(ctx);

        while (1) {
                // Redraw if selection, scroll offset, or input changed
                if (last_selected != ctx->selected || last_scroll_offset != ctx->scroll_offset ||
                    last_input_len != ctx->input.len) {
                        dump_procs(ctx);
                        last_selected = ctx->selected;
                        last_scroll_offset = ctx->scroll_offset;
                        last_input_len = ctx->input.len;
                }

                int ch = getch();
                if (ch == ERR) continue;

                switch (ch) {
                case CTRL('q'): return;
                case KEY_UP: {
                        if (ctx->selected > 0) {
                                ctx->selected--;
                                if (ctx->selected < ctx->scroll_offset) {
                                        ctx->scroll_offset--;
                                }
                        }
                } break;
                case KEY_DOWN: {
                        if (ctx->selected < (int)ctx->filtered_procs.len - 1) {
                                ctx->selected++;
                                if (ctx->selected >= ctx->scroll_offset + ctx->win.h - 1) {
                                        ctx->scroll_offset++;
                                }
                        }
                } break;
                case BACKSPACE: {
                        if (ctx->input.len > 0) {
                                ctx->input.data[--ctx->input.len] = 0;
                                update_filtered_procs(ctx);
                        }
                } break;
                case ENTER: {
                        kill_selected_proc(ctx);
                        return;
                } break;
                default: {
                        if (ch >= 32 && ch <= 126) {
                                dyn_array_append(ctx->input, (char)ch);
                                update_filtered_procs(ctx);
                        }
                } break;
                }
        }
}

int
main(int argc, char **argv)
{
        context ctx = (context) {
                .win = {
                        .w = 0,
                        .h = 0,
                },
                .flags = 0x0000,
                .selected = 0,
                .scroll_offset = 0,
                .procs = dyn_array_empty(proc_ptr_array),
                .filtered_procs = dyn_array_empty(proc_ptr_array),
                .input = dyn_array_empty(char_array),
        };

        --argc, ++argv;
        clap_init(argc, argv);

        Clap_Arg arg = {0};
        while (clap_next(&arg)) {
                int one = arg.hyphc == 1;
                int two = arg.hyphc == 2;

                if (one && arg.start[0] == FLAG_1HY_HELP) {
                        usage();
                } else if (one && arg.start[0] == FLAG_1HY_LIST) {
                        ctx.flags |= FT_LIST;
                } else if (two && !strcmp(arg.start, FLAG_2HY_HELP)) {
                        usage();
                } else if (two && !strcmp(arg.start, FLAG_2HY_LIST)) {
                        ctx.flags |= FT_LIST;
                } else if (two && !strcmp(arg.start, FLAG_2HY_COPYING)) {
                        copying();
                }
                else {
                        TODO("handling processes on the CLI");
                }
        }

        DIR *proc_dir;
        struct dirent *entry;

        if (!(proc_dir = opendir("/proc"))) {
                perror("opendir");
                return 1;
        }

        while ((entry = readdir(proc_dir))) {
                if (entry->d_type == DT_DIR
                    && strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
                        proc *p = get_process_info(entry->d_name);
                        if (p && p->user) {
                                dyn_array_append(ctx.procs, p);
                        }
                }
        }

        if (ctx.flags & FT_LIST) {
                printf("%-8s %-8s %s\n", "USER", "PID", "COMMAND");
                for (size_t i = 0; i < ctx.procs.len; ++i) {
                        const proc *p = ctx.procs.data[i];
                        printf("%-8s %-8s %s\n", p->user, p->pid, p->cmd);
                }
        } else {
                init_ncurses(&ctx);
                atexit(cleanup);
                input_loop(&ctx);
        }

        for (size_t i = 0; i < ctx.procs.len; ++i) {
                free(ctx.procs.data[i]->user);
                free(ctx.procs.data[i]->pid);
                free(ctx.procs.data[i]->cmd);
                free(ctx.procs.data[i]);
        }
        dyn_array_free(ctx.procs);

        return 0;
}
