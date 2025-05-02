#include <cebeq.h>
#include <cson.h>
#include <cwalk.h>

int main(void)
{
    (void) cson_current_arena;
    cwk_path_set_style(CWK_STYLE_UNIX);
    make_backup("testing", "d:/testing/new_backups", NULL);
    cson_free();
    return 0;
}