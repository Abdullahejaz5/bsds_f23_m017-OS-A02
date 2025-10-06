/*
 ============================================================================
 Name        : ls-v1.3.0.c
 Description : Feature 4 - Horizontal Column Display (-x)
               Default  : down then across
               -C option: same as default
               -x option: horizontal (across then down)
               -l option: long listing
 ============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>

#define COL_PADDING 2
#define DEFAULT_TERM_WIDTH 80
#define MAX_FILES 4096

/* -------- Get terminal width -------- */
int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0)
        return DEFAULT_TERM_WIDTH;
    return (int)w.ws_col;
}

/* -------- Compare for sorting -------- */
int cmp_names(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

/* -------- Read all visible filenames -------- */
int read_filenames(const char *path, char ***out) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        *out = NULL;
        return 0;
    }

    char **names = malloc(sizeof(char *) * MAX_FILES);
    if (!names) {
        perror("malloc");
        closedir(dir);
        *out = NULL;
        return 0;
    }

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < MAX_FILES) {
        if (entry->d_name[0] == '.') continue;
        names[count++] = strdup(entry->d_name);
    }
    closedir(dir);

    qsort(names, count, sizeof(char *), cmp_names);
    *out = names;
    return count;
}

/* -------- Free filenames -------- */
void free_names(char **names, int n) {
    for (int i = 0; i < n; i++) free(names[i]);
    free(names);
}

/* -------- Long listing (-l) -------- */
void print_long_listing(const char *path, char **names, int n) {
    struct stat st;
    char full[1024];

    for (int i = 0; i < n; i++) {
        snprintf(full, sizeof(full), "%s/%s", path, names[i]);
        if (lstat(full, &st) == -1) {
            perror(names[i]);
            continue;
        }

        // File type
        printf((S_ISDIR(st.st_mode)) ? "d" : "-");
        printf((st.st_mode & S_IRUSR) ? "r" : "-");
        printf((st.st_mode & S_IWUSR) ? "w" : "-");
        printf((st.st_mode & S_IXUSR) ? "x" : "-");
        printf((st.st_mode & S_IRGRP) ? "r" : "-");
        printf((st.st_mode & S_IWGRP) ? "w" : "-");
        printf((st.st_mode & S_IXGRP) ? "x" : "-");
        printf((st.st_mode & S_IROTH) ? "r" : "-");
        printf((st.st_mode & S_IWOTH) ? "w" : "-");
        printf((st.st_mode & S_IXOTH) ? "x" : "-");

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st.st_mtime));

        printf(" %2ld %-8s %-8s %8ld %s %s\n",
               (long)st.st_nlink,
               pw ? pw->pw_name : "?",
               gr ? gr->gr_name : "?",
               (long)st.st_size,
               timebuf,
               names[i]);
    }
}

/* -------- Default Down-then-Across (Vertical Columns) -------- */
void print_down_then_across(char **names, int n) {
    if (n == 0) return;
    int term_width = get_terminal_width();

    // find the longest filename
    size_t maxlen = 0;
    for (int i = 0; i < n; i++)
        if (strlen(names[i]) > maxlen)
            maxlen = strlen(names[i]);

    int col_width = (int)maxlen + COL_PADDING;
    if (col_width < 1) col_width = 1;

    int cols = term_width / col_width;
    if (cols < 1) cols = 1;
    if (cols > n) cols = n;

    // ensure at least 2 rows if many files
    if (cols >= n && n > 3)
        cols = (n + 1) / 2;

    int rows = (n + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = r + c * rows;  // vertical index
            if (idx < n)
                printf("%-*s", (int)maxlen, names[idx]);
            if (c < cols - 1)
                for (int s = 0; s < COL_PADDING; s++) putchar(' ');
        }
        putchar('\n');
    }
}

/* -------- NEW: Horizontal Across-then-Down (-x) -------- */
void print_across_then_down(char **names, int n) {
    if (n == 0) return;
    int term_width = get_terminal_width();

    size_t maxlen = 0;
    for (int i = 0; i < n; i++)
        if (strlen(names[i]) > maxlen)
            maxlen = strlen(names[i]);

    int col_width = (int)maxlen + COL_PADDING;
    if (col_width < 1) col_width = 1;

    int cols = term_width / col_width;
    if (cols < 1) cols = 1;

    int cur_col = 0;
    for (int i = 0; i < n; i++) {
        printf("%-*s", (int)maxlen, names[i]);
        cur_col++;
        if (cur_col >= cols || i == n - 1) {
            putchar('\n');
            cur_col = 0;
        } else {
            for (int s = 0; s < COL_PADDING; s++) putchar(' ');
        }
    }
}

/* -------- main -------- */
int main(int argc, char *argv[]) {
    int opt;
    int flag_l = 0, flag_C = 0, flag_x = 0;
    const char *path = ".";

    while ((opt = getopt(argc, argv, "lCx")) != -1) {
        switch (opt) {
            case 'l': flag_l = 1; break;
            case 'C': flag_C = 1; break;
            case 'x': flag_x = 1; break;
            default: break;
        }
    }

    if (optind < argc) path = argv[optind];

    char **names = NULL;
    int n = read_filenames(path, &names);
    if (n <= 0) return 0;

    if (flag_l)
        print_long_listing(path, names, n);
    else if (flag_x)
        print_across_then_down(names, n);
    else
        print_down_then_across(names, n);  // default & -C

    free_names(names, n);
    return 0;
}
