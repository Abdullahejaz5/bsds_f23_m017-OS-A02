/*
 ============================================================================
 Name        : ls-v1.3.0.c
 Author      : You
 Description : Feature-4 - Horizontal Column Display (-x)
               - default: vertical (down-then-across) columns
               - -x : horizontal (row-major) wrapping based on terminal width
               - -l : long listing (detailed)
               - -C : explicit vertical columns
 ============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MAX_FILES 4096
#define MAX_NAME_LEN 1024
#define DEFAULT_TERM_WIDTH 80
#define COL_PADDING 2 /* spaces between columns */

/* ---------- helper: terminal width ---------- */
static int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0) {
        return DEFAULT_TERM_WIDTH;
    }
    return (int)w.ws_col;
}

/* ---------- read filenames into array (skip dotfiles) ---------- */
/* returns count, sets *names_out to malloc'd array (caller frees) */
static int read_filenames(const char *path, char ***names_out) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        *names_out = NULL;
        return 0;
    }

    char **names = malloc(sizeof(char*) * MAX_FILES);
    if (!names) {
        perror("malloc");
        closedir(dir);
        *names_out = NULL;
        return 0;
    }

    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL && count < MAX_FILES) {
        if (entry->d_name[0] == '.') continue; /* skip hidden files */
        names[count] = strdup(entry->d_name);
        if (!names[count]) { perror("strdup"); break; }
        count++;
    }
    closedir(dir);

    *names_out = names;
    return count;
}

static void free_names(char **names, int count) {
    if (!names) return;
    for (int i = 0; i < count; ++i) free(names[i]);
    free(names);
}

/* ---------- long listing: iterate names[] and lstat each ---------- */
static void print_long_listing_from_names(const char *path, char **names, int count) {
    struct stat st;
    char fullpath[MAX_NAME_LEN];

    for (int i = 0; i < count; ++i) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, names[i]);
        if (lstat(fullpath, &st) == -1) {
            fprintf(stderr, "Error: cannot access %s: %s\n", fullpath, strerror(errno));
            continue;
        }

        /* file type */
        char type = '-';
        if (S_ISDIR(st.st_mode)) type = 'd';
        else if (S_ISLNK(st.st_mode)) type = 'l';
        else if (S_ISCHR(st.st_mode)) type = 'c';
        else if (S_ISBLK(st.st_mode)) type = 'b';
        else if (S_ISFIFO(st.st_mode)) type = 'p';
        else if (S_ISSOCK(st.st_mode)) type = 's';

        /* permissions */
        char perms[11];
        perms[0] = type;
        perms[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        perms[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        perms[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
        perms[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        perms[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        perms[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
        perms[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
        perms[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        perms[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
        perms[10] = '\0';

        long nlink = (long) st.st_nlink;
        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        const char *uname = pw ? pw->pw_name : "unknown";
        const char *gname = gr ? gr->gr_name : "unknown";
        long size = (long) st.st_size;

        char timebuf[64];
        struct tm *tm_info = localtime(&st.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);

        printf("%s %2ld %-8s %-8s %8ld %s %s\n",
               perms, nlink, uname, gname, size, timebuf, names[i]);
    }
}

/* ---------- vertical columns: down then across ---------- */
static void print_vertical_columns_from_names(char **names, int count) {
    if (count == 0) return;
    int term_width = get_terminal_width();

    /* find max name length */
    size_t maxlen = 0;
    for (int i = 0; i < count; ++i) {
        size_t L = strlen(names[i]);
        if (L > maxlen) maxlen = L;
    }

    int col_width = (int)maxlen + COL_PADDING;
    if (col_width <= 0) col_width = 1;

    int cols = term_width / col_width;
    if (cols < 1) cols = 1;

    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = c * rows + r; /* down then across */
            if (idx < count) {
                printf("%-*s", (int)maxlen, names[idx]);
                if (c < cols - 1) {
                    for (int p = 0; p < COL_PADDING; ++p) putchar(' ');
                }
            }
        }
        putchar('\n');
    }
}

/* ---------- horizontal across: left-to-right, wrap by terminal width ---------- */
static void print_horizontal_across_from_names(char **names, int count) {
    if (count == 0) return;
    int term_width = get_terminal_width();

    /* find max name length */
    size_t maxlen = 0;
    for (int i = 0; i < count; ++i) {
        size_t L = strlen(names[i]);
        if (L > maxlen) maxlen = L;
    }

    int col_width = (int)maxlen + COL_PADDING;
    if (col_width <= 0) col_width = 1;

    int cur_width = 0;
    for (int i = 0; i < count; ++i) {
        int need = col_width;
        if (cur_width == 0) {
            /* start of line */
            printf("%-*s", (int)maxlen, names[i]);
            cur_width += need;
        } else {
            if (cur_width + need > term_width) {
                putchar('\n');
                printf("%-*s", (int)maxlen, names[i]);
                cur_width = need;
            } else {
                /* print with a single separating space, but align to maxlen anyway */
                putchar(' ');
                printf("%-*s", (int)maxlen, names[i]);
                cur_width += need + 1;
            }
        }
    }
    putchar('\n');
}

/* ---------- main: parse -l, -x, -C and optional path ---------- */
int main(int argc, char *argv[]) {
    int opt;
    int flag_long = 0;
    int flag_horizontal = 0; /* -x */
    int flag_C = 0;          /* explicit vertical columns */
    const char *path = ".";

    while ((opt = getopt(argc, argv, "lxC")) != -1) {
        switch (opt) {
            case 'l': flag_long = 1; break;
            case 'x': flag_horizontal = 1; break;
            case 'C': flag_C = 1; break;
            default: break;
        }
    }

    if (optind < argc) path = argv[optind];

    char **names = NULL;
    int count = read_filenames(path, &names);

    if (count == 0) {
        free_names(names, count);
        return 0;
    }

    if (flag_long) {
        print_long_listing_from_names(path, names, count);
    } else if (flag_horizontal) {
        print_horizontal_across_from_names(names, count);
    } else {
        /* default and -C both use vertical down-then-across */
        print_vertical_columns_from_names(names, count);
    }

    free_names(names, count);
    return 0;
}
