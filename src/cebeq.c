#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <cebeq.h>
#include <cwalk.h>

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#include <libgen.h>
#include <limits.h>
#elif __linux__
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#endif

void convert_seperators(const char *path, char *buffer, size_t buffer_size)
{
    if (path == NULL || buffer == NULL) return;
    size_t pos_out = 0;
    size_t pos_in = 0;
    char c;
    while (pos_out < buffer_size && (c = path[pos_in++]) != '\0'){
        if (c == '\\') {
            buffer[pos_out++] = '/';
        } else{
            buffer[pos_out++] = c;
        }
    }
    buffer[pos_out] = '\0';
}

void norm_path(const char *path, char *buffer, size_t buffer_size)
{
    convert_seperators(path, buffer, buffer_size);
    cwk_path_normalize(buffer, buffer, buffer_size);
}

bool get_exe_path(char *buffer, size_t buffer_size)
{
#ifdef _WIN32
    GetModuleFileNameA(NULL, buffer, buffer_size);
#elif __APPLE__
    if (_NSGetExecutablePath(buffer, &buffer_size) != 0) return false;
#elif __linux__
    ssize_t len = readlink("/proc/self/exe", buffer, buffer_size-1);
    if (len == -1) return false;
    buffer[len] = '\0';
#else
    fprintf(stderr, "[ERROR] %s:%d:%s: Unsupported platform!\n", __LINE__, __FILE__, __func__);
    return false;
#endif 
    norm_path(buffer, buffer, buffer_size);
    return true;
}

bool get_parent_dir(const char *path, char *buffer, size_t buffer_size)
{
    size_t parent_len;
    cwk_path_get_dirname(path, &parent_len);
    if (parent_len == 0 || parent_len >= buffer_size) return false;
    memmove(buffer, path, parent_len);
    buffer[parent_len] = '\0';
    cwk_path_normalize(buffer, buffer, buffer_size);
    return true;
}