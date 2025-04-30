#include <cebeq.h>
#define CSON_PARSE
#define CSON_WRITE
#include <cson.h>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

int delete_directory_recursive(const char *path) {
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (!dir) return 0;

    while ((entry = readdir(dir)) != NULL) {
        char fullPath[PATH_MAX];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        struct stat statbuf;
        if (stat(fullPath, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                delete_directory_recursive(fullPath);
            } else {
                unlink(fullPath);
            }
        }
    }

    closedir(dir);
    return rmdir(path);
}

int main(void)
{
    const char *path = "D:/Testing/new_backups/testing_4";
    delete_directory_recursive(path);
    return 0;
}