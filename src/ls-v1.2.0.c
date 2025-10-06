/*
 ============================================================================
 Name        : ls-v1.2.0.c
 Description : Feature 3 – Column Display (Down then Across)
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
int get_terminal_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0)
        return DEFAULT_TERM_WIDTH;
    return (int)w.ws_col;
}

/* -------- Compare function for qsort -------- */
int cmp_names(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

/* -------- Read filenames -------- */
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

void free_names(char **names, int n) {
    for (int i = 0; i < n; i++) free(names[i]);
    free(names);
}

/* -------- Long Listing (-l) -------- */
void print_long_listing(const char *path, char **names, int n) {
    struct stat st;
    char full[1024];

    for (int i = 0; i < n; i++) {
        snprintf(full, sizeof(full), "%s/%s", path, names[i]);
        if (lstat(full, &st) == -1) {
            perror(names[i]);
            continue;
        }

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
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M",
                 localtime(&st.st_mtime));

        printf(" %2ld %-8s %-8s %8ld %s %s\n",
               (long)st.st_nlink,
               pw ? pw->pw_name : "?",
               gr ? gr->gr_name : "?",
               (long)st.st_size,
               timebuf,
               names[i]);
    }
}

/* -------- Column Display (Down then Across) -------- */
void print_down_then_across(char **names, int n) {
    if (n == 0) return;

    int term_width = get_terminal_width();

    // 1️⃣ Find longest filename
    size_t maxlen = 0;
    for (int i = 0; i < n; i++)
        if (strlen(names[i]) > maxlen)
            maxlen = strlen(names[i]);

    int col_width = (int)maxlen + COL_PADDING;
    if (col_width < 1) col_width = 1;

    // 2️⃣ How many columns fit
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;

    // 3️⃣ Compute rows (ceil)
    int rows = (n + cols - 1) / cols;

    // 4️⃣ Print DOWN then ACROSS
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = r + c * rows;
            if (idx < n)
                printf("%-*s", (int)maxlen, names[idx]);
            if (c < cols - 1)
                for (int s = 0; s < COL_PADDING; s++) putchar(' ');
        }
        putchar('\n');
    }
}

/* -------- main -------- */
int main(int argc, char *argv[]) {
    int flag_l = 0, flag_C = 0;
    int opt;

    while ((opt = getopt(argc, argv, "lC")) != -1) {
        switch (opt) {
            case 'l': flag_l = 1; break;
            case 'C': flag_C = 1; break;
            default: break;
        }
    }

    const char *path = ".";
    if (optind < argc) path = argv[optind];

    char **names = NULL;
    int n = read_filenames(path, &names);
    if (n <= 0) return 0;

    if (flag_l)
        print_long_listing(path, names, n);
    else
        print_down_then_across(names, n);

    free_names(names, n);
    return 0;
}
