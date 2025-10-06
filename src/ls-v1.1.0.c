/*
 ============================================================================
 Name        : ls-v1.1.0.c
 Description : Feature 2 - Long Listing (-l)
 ============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void print_long_listing(const char *filename);

int main(int argc, char *argv[]) {
    int long_listing = 0;

    // Check if -l option is given
    if (argc > 1 && strcmp(argv[1], "-l") == 0)
        long_listing = 1;

    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue; // skip hidden files

        if (long_listing)
            print_long_listing(entry->d_name);
        else
            printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return 0;
}

void print_long_listing(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == -1) {
        perror("stat");
        return;
    }

    // File type
    printf((S_ISDIR(st.st_mode)) ? "d" : "-");

    // Permissions
    printf((st.st_mode & S_IRUSR) ? "r" : "-");
    printf((st.st_mode & S_IWUSR) ? "w" : "-");
    printf((st.st_mode & S_IXUSR) ? "x" : "-");
    printf((st.st_mode & S_IRGRP) ? "r" : "-");
    printf((st.st_mode & S_IWGRP) ? "w" : "-");
    printf((st.st_mode & S_IXGRP) ? "x" : "-");
    printf((st.st_mode & S_IROTH) ? "r" : "-");
    printf((st.st_mode & S_IWOTH) ? "w" : "-");
    printf((st.st_mode & S_IXOTH) ? "x" : "-");

    // Link count, owner, group, size, date, name
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
           filename);
}
