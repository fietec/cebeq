#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <cebeq.h>
#include <cwalk.h>
#include <cson.h>
#include <message_queue.h>

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

char program_dir[FILENAME_MAX];
char exe_dir[FILENAME_MAX];
volatile int worker_done = 0;

bool setup(void)
{
    (void) cson_current_arena;
    
    if (!get_exe_path(program_dir, sizeof(program_dir))){
        eprintf("Failed to retrieve executable path!");
        return false;
    }
    (void) get_parent_dir(program_dir, exe_dir, sizeof(exe_dir));
    (void) get_parent_dir(exe_dir, program_dir, sizeof(program_dir));
    return true;
}

void cleanup(void)
{
    free(long_path_buf);
    long_path_buf = NULL;
    cson_free();
}

void escape_string(const char *string, char *buffer, size_t buffer_size)
{
    if (string == NULL || buffer == NULL || buffer_size == 0) return;
    size_t ri = 0;
    size_t wi = 0;
    while (string[ri] != '\0' && wi < buffer_size - 1){
        char c = string[ri++];
        if (c == '\\') {
            if (wi + 2 > buffer_size) break;
            buffer[wi++] = '\\';
            buffer[wi++] = '\\';
        } else {
            buffer[wi++] = c;
        }
    }
    buffer[wi] = '\0';
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
    cwk_path_normalize(buffer, buffer, buffer_size);
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
