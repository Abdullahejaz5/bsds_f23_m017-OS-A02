/*
 ============================================================================
 Name        : ls-v1.3.0.c
 Description : Feature 4 - Horizontal Column Display (-x)
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
#define DEFAULT_TERM_WIDTH 80
#define COL_PADDING 2

// ---------- Helper to get terminal width ----------
int get_terminal_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0)
        return DEFAULT_TERM_WIDTH;
    return (int)w.ws_col;
}

// ---------- Read filenames ----------
int read_filenames(const char *path, char ***names_out) {
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
        if (entry->d_name[0] == '.') continue; // skip hidden files
        names[count] = strdup(entry->d_name);
        if (!names[count]) {
            perror("strdup");
            break;
        }
        count++;
    }
    closedir(dir);
    *names_out = names;
    return count;
}

void free_names(char **names, int count) {
    if (!names) return;
    for (int i = 0; i < count; ++i)
        free(names[i]);
    free(names);
}

// ---------- Simple one-per-line ----------
void print_simple_listing_from_names(char **names, int count) {
    for (int i = 0; i < count; ++i)
        printf("%s\n", names[i]);
}

// ---------- Long listing ----------
void print_long_listing_from_names(const char *path, char **names, int count) {
    struct stat st;
    char fullpath[1024];

    for (int i = 0; i < count; ++i) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, names[i]);
        if (lstat(fullpath, &st) == -1) {
            fprintf(stderr, "Error: cannot access %s: %s\n", fullpath, strerror(errno));
            continue;
        }

        // File type
        char type = '-';
        if (S_ISDIR(st.st_mode)) type = 'd';
        else if (S_ISLNK(st.st_mode)) type = 'l';
        else if (S_ISCHR(st.st_mode)) type = 'c';
        else if (S_ISBLK(st.st_mode)) type = 'b';
        else if (S_ISFIFO(st.st_mode)) type = 'p';
        else if (S_ISSOCK(st.st_mode)) type = 's';

        // Permissions
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

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        const char *uname = pw ? pw->pw_name : "unknown";
        const char *gname = gr ? gr->gr_name : "unknown";

        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&st.st_mtime));

        printf("%s %2ld %-8s %-8s %8ld %s %s\n",
               perms, (long)st.st_nlink, uname, gname,
               (long)st.st_size, timebuf, names[i]);
    }
}

// ---------- Vertical (down then across) ----------
void print_vertical_columns_from_names(char **names, int count) {
    if (count == 0) return;
    int term_width = get_terminal_width();

    size_t maxlen = 0;
    for (int i = 0; i < count; ++i)
        if (strlen(names[i]) > maxlen) maxlen = s
