#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <cebeq.h>
#include <cwalk.h>
#include <cson.h>
#include <nob.h>
#include <flib.h>
#include <threading.h>
#include <message_queue.h>

#include <raylib.h>
#define CLAY_IMPLEMENTATION
#include <clay.h>
#include <clay_renderer_raylib.c>

#define NEW_BRANCH_MAX_LEN 32

#define NO_COLOR ((Clay_Color) {0})
#define BLUR_COLOR ((Clay_Color) {255, 255, 255, 51})
#define TEXT_PADDING (Clay_Padding){8, 8, 4, 4}
#define clay_string(str) (Clay_String) {false, strlen(str), str}

#define info(msg, ...) (fprintf(stderr, "[INFO] " msg "\n", ##__VA_ARGS__)) 
#define error(msg, ...) (fprintf(stderr, "[ERROR] " msg "\n", ##__VA_ARGS__)) 

typedef enum{
    SCENE_MAIN,
    SCENE_SETTINGS,
    SCENE_NEW,
    SCENE_HISTORY,
    SCENE_FILE,
    SCENE_ABOUT,
    SCENE_CONFIRM,
    SCENE_BACKUP,
    SCENE_RUN
} Scene;

typedef enum{
    DEFAULT,
    MONO_12,
    MONO_16,
    _FontId_Count
} FontIds;

typedef enum{
    SYMBOL_EXIT_16,
    SYMBOL_REFRESH_16,
    SYMBOL_BACK_16,
    SYMBOL_FWD_16,
    SYMBOL_DELETE_12,
    SYMBOL_CHECK_12,
    _TextureId_Count
} TextureIds;

typedef struct {
    Clay_Color background;
    Clay_Color text;
    Clay_Color accent;
    Clay_Color border;
    Clay_Color hover;
    Clay_Color secondary;
    Clay_Color danger;
    Clay_Color success;
} Theme;

typedef void (*FuncButtonData)(void);

typedef struct{
    void (*func) (void*);
    void *arg;
} ArgFuncButtonData;

#define arg_func(func, arg) (ArgFuncButtonData){(func), (void*) (arg)}

const Theme charcoal_teal = {
    { 34,  43,  46,  255 },  // background
    { 217, 225, 232, 255 },  // text
    { 0,   191, 166, 255 },  // accent
    { 55,  64,  69,  255 },  // border
    { 2,   158, 136, 255 },  // hover
    { 124, 138, 148, 255 },  // secondary
    { 220,  53,  69,  255 }, // danger 
    { 40,  167,  69,  255 }, // success
};

typedef struct{
    const char *name;
    Clay_String text;
} Branch;

typedef struct{
    Branch *items;
    size_t count;
    size_t capacity;
} Branches;

typedef struct{
    char path[FILENAME_MAX];
    char name[FILENAME_MAX];
} File;

typedef struct{
    File *items;
    size_t count;
    size_t capacity;
} Files;

typedef struct{
    char **items;
    size_t count; 
    size_t capacity;
} Log;

typedef struct{
    char dir_path[FILENAME_MAX];
    File file;
    Files items;
    int item_index;
    bool first_frame;
    void (*on_set)(void);
} FileDialog;

typedef struct{
    Files dirs;
} NewBranchDialog;

typedef struct{
    bool result;
    void (*on_exit)(void);
    Clay_String question;
} ConfirmDialog;

typedef struct{
    // output
    char *branch_name;
    char dest[FILENAME_MAX];
    char prev[FILENAME_MAX];
    // internal
    int dest_len;
    int selected_backup;
    Branches backups;
    bool prev_enable;
    bool is_backup;
} BackupDialog;

typedef struct{
    thread_fn fn;
    thread_args_t args;
    thread_t worker;
    char msg[MAX_MSG_LEN];
    Log log;
    bool running;
} RunDialog;

typedef struct{
    Theme theme;
    Branches branches;
    int selected_branch;
    Texture2D textures[_TextureId_Count];
    Scene scene;
    Scene prev_scene;
    char info_path[FILENAME_MAX];
    Cson *cson_info;
    Cson *cson_branches;
    int frame_counter;
    char new_branch_name[NEW_BRANCH_MAX_LEN+1];
    int new_branch_len;
    char temp_buffer[256];
    char *version;
    Nob_String_Builder sb;
    FileDialog file_dialog;
    NewBranchDialog new_branch_dialog;
    ConfirmDialog confirm_dialog;
    BackupDialog backup_dialog;
    RunDialog run_dialog;
} State;

static State state = {
    .theme = charcoal_teal,
    .scene = SCENE_MAIN,
    .selected_branch = -1,
    .file_dialog = {
        .item_index = -1,
        .first_frame = true,
    },
    .confirm_dialog = {
        .question = CLAY_STRING("Question goes here.."),
    },
    .backup_dialog = {
        .prev_enable = true
    }
};

static Clay_String license_text = CLAY_STRING("\
Permission is hereby granted, free of charge, to any person obtaining a copy \n\
of this software and associated documentation files (the \"Software\"), to deal \n\
in the Software without restriction, including without limitation the rights \n\
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n\
copies of the Software, and to permit persons to whom the Software is \n\
furnished to do so, subject to the following conditions: \n\
 \n\
The above copyright notice and this permission notice shall be included in all \n\
copies or substantial portions of the Software. \n\
 \n\
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n\
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n\
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE \n\
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n\
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n\
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE \n\
SOFTWARE.");

void func_toggle_scene(void *arg)
{
    Scene scene = (Scene) arg;
    state.prev_scene = state.scene;
    state.scene = scene;
}

void func_new_branch_delete(size_t index)
{
    Files *dirs = &state.new_branch_dialog.dirs;
    if (index >= dirs->count) return;
    if (index < dirs->count-1){
        memmove(&dirs->items[index], &dirs->items[index+1], (dirs->count-index-1)*sizeof(*dirs->items));
    }
    dirs->count--;
}

void func_new_branch_set(void)
{
    int index = state.file_dialog.item_index;
    File file = {0};
    if (index < 0){
        memcpy(file.path, state.file_dialog.dir_path, FILENAME_MAX);
    } else{
        memcpy(file.path, state.file_dialog.items.items[index].path, FILENAME_MAX);
    }
    nob_da_append(&state.new_branch_dialog.dirs, file);
}

void func_new_branch_add(void)
{
    state.file_dialog.on_set = func_new_branch_set;
    func_toggle_scene((void*) SCENE_FILE);
}

bool set_branches(void)
{
    state.branches.count = 0;
    Cson *branch_names = cson_map_keys(state.cson_branches);
    if (!cson_is_array(branch_names)){
        error("Invalid branch info state!");
        return false;
    }
    size_t branch_count = cson_len(branch_names);
    for (size_t i=0; i<branch_count; ++i){
        char *name = cson_get_cstring(branch_names, index(i));
        Cson *branch = cson_get(state.cson_branches, key(name));
        if (!cson_is_map(branch)){
            error("branch '%s' is not in a valid state!", name);
            return false;
        }
        Cson *dirs = cson_get(branch, key("dirs"));
        if (!cson_is_array(dirs)){
            error("branch '%s' has no <dirs> field!", name);
            return 1;
        }
        size_t dir_count = cson_len(dirs);
        state.sb.count = 0;
        nob_sb_append_cstr(&state.sb, name);
        nob_sb_append_cstr(&state.sb, ": [");
        for (size_t i=0; i<dir_count; ++i){
            if (i > 0){
                nob_sb_append_cstr(&state.sb, ", ");
            }
            nob_sb_append_cstr(&state.sb, "'");
            nob_sb_append_cstr(&state.sb, cson_get_cstring(dirs, index(i)));
            nob_sb_append_cstr(&state.sb, "'");
        }
        nob_sb_append_cstr(&state.sb, "]");
        nob_sb_append_null(&state.sb);
        Branch b = {
            .text = (Clay_String) {false, state.sb.count-1, strdup(state.sb.items)},
            .name = name
        };
        nob_da_append(&state.branches, b);
    }
    return true;
}

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

Clay_Color darken_color(Clay_Color color)
{
    const float factor = 0.8f;
    return (Clay_Color){color.r*factor, color.g*factor, color.b*factor, color.a};
}

// button functions
void func_refresh(void)
{
    state.cson_info = cson_read(state.info_path);
    if (state.cson_info == NULL){
        eprintf("Could not find internal info file!\n");
        return;
    }
    state.cson_branches = cson_get(state.cson_info, key("branches"));
    if (!cson_is_map(state.cson_branches)){
        eprintf("Could not find <state.branches> field in info file!");
        return;
    }
    (void) set_branches();
}

void func_branch_exit(void)
{
    state.new_branch_dialog.dirs.count = 0;
    memset(state.new_branch_name, 0, sizeof(state.new_branch_name));
    state.new_branch_len = 0;
    func_toggle_scene((void*) SCENE_MAIN);
}

void func_branch_new(void)
{
    func_toggle_scene((void*) SCENE_NEW);
}

void func_branch_history(void)
{
    func_toggle_scene((void*) SCENE_HISTORY);
}

void func_add_branch(void)
{
    if (state.new_branch_len > 0){        
        Cson *dirs = cson_array_new();
        for (size_t i=0; i<state.new_branch_dialog.dirs.count; ++i){
            char *dir = state.new_branch_dialog.dirs.items[i].path;
            cson_array_push(dirs, cson_new_cstring(dir));
        }
        
        Cson *branch = cson_map_new();
        cson_map_insert(branch, cson_str("last_id"), cson_new_int(0));
        cson_map_insert(branch, cson_str("dirs"), dirs);
        cson_map_insert(branch, cson_str("backups"), cson_array_new());
        
        cson_map_insert(state.cson_branches, cson_str(state.new_branch_name), branch);
        cson_write(state.cson_info, state.info_path);
        
        func_toggle_scene((void*) SCENE_MAIN);
        func_refresh();
        state.new_branch_dialog.dirs.count = 0;
        func_branch_exit();
    }
}

void func_branch_remove(void)
{
    if (state.selected_branch >= 0 && state.selected_branch < (int) state.branches.count){
        char *branch_name = (char*) state.branches.items[state.selected_branch].name;
        if (cson_map_remove(state.cson_branches, cson_str(branch_name)) != CsonError_Success){
            eprintf("Could not remove brandch '%s'!", branch_name);
        }
        set_branches();
        cson_write(state.cson_info, state.info_path);
    }
}

void func_branch_remove_after_confirm(void)
{
    func_toggle_scene((void*) SCENE_MAIN);
    if (state.confirm_dialog.result){
        func_branch_remove();
    }
}

void func_branch_remove_before_confirm(void)
{
    if (state.selected_branch >= 0 && state.selected_branch < (int) state.branches.count){
        state.confirm_dialog.question = CLAY_STRING("Do you really want to delete this branch?");
        state.confirm_dialog.on_exit = &func_branch_remove_after_confirm;
        func_toggle_scene((void*) SCENE_CONFIRM);
    }
}

void func_task_loc_open(void)
{
    char command[FILENAME_MAX + 32] = {0};
  #ifdef _WIN32
    const char *fman = "explorer";
  #else
    const char *fman = "xdg-open";
  #endif // _WIN32
    snprintf(command, sizeof(command), "%s %s", fman, exe_dir);
    system(command);
}

void func_dir_dialog_exit(void)
{
    state.file_dialog.first_frame = true;
    state.file_dialog.items.count = 0;
    func_toggle_scene((void*) state.prev_scene);
}

void func_dir_dialog_back(void)
{
    (void) get_parent_dir(state.file_dialog.dir_path, state.file_dialog.dir_path, FILENAME_MAX);
    state.file_dialog.first_frame = true;
    state.file_dialog.items.count = 0;
    state.file_dialog.item_index = -1;
}

void func_dir_dialog_forward(void)
{
    int index = state.file_dialog.item_index;
    if (index >= 0){
        memcpy(state.file_dialog.dir_path, state.file_dialog.items.items[index].path, FILENAME_MAX);
        state.file_dialog.first_frame = true;
        state.file_dialog.items.count = 0;
        state.file_dialog.item_index = -1;
    }
}

void func_dir_dialog_set(void)
{   
    if (state.file_dialog.on_set != NULL) state.file_dialog.on_set();
    func_dir_dialog_exit();
}

void func_confirm_yes(void)
{
    state.confirm_dialog.result = true;
    if (state.confirm_dialog.on_exit != NULL){
        state.confirm_dialog.on_exit();
    } else{
        func_toggle_scene((void*) state.prev_scene);
    }
}

void func_confirm_no(void)
{
    state.confirm_dialog.result = false;
    if (state.confirm_dialog.on_exit != NULL){
        state.confirm_dialog.on_exit();
    } else{
        func_toggle_scene((void*) state.prev_scene);
    }
}

void func_backup_dialog_init(void)
{
    if (state.selected_branch >= 0){
        BackupDialog *bd = &state.backup_dialog;
        bd->selected_backup = -1;
        bd->branch_name = (char*) state.branches.items[state.selected_branch].name;
        bd->prev_enable = false;
        
        Cson *backups = cson_get(state.cson_branches, key(bd->branch_name), key("backups"));
        if (!cson_is_array(backups)){
            eprintf("Invalid branch structure of branch '%s'!", bd->branch_name);
            return;
        }
        size_t len = cson_len(backups);
        for (size_t i=0; i<len; ++i){
            char *backup_path = cson_get_cstring(backups, index(i));
            Branch backup;
            backup.name = backup_path;
            backup.text = clay_string(backup_path);
            nob_da_append(&bd->backups, backup);
        }
        func_toggle_scene((void*) SCENE_BACKUP);
    }
}

void func_backup_dialog_set_path(void)
{
    int index = state.file_dialog.item_index;
    if (index < 0){
        cwk_path_normalize(state.file_dialog.dir_path, state.backup_dialog.dest, sizeof(state.backup_dialog.dest));
    } else{
        cwk_path_normalize(state.file_dialog.items.items[index].path, state.backup_dialog.dest, sizeof(state.backup_dialog.dest));
    }
    state.backup_dialog.dest_len = strlen(state.backup_dialog.dest);
}

void func_backup_dialog_exit(void)
{
    state.backup_dialog.backups.count = 0;
    state.backup_dialog.dest_len = 0;
    func_toggle_scene((void*) SCENE_MAIN);
}

void func_backup_dialog_select(void)
{
    state.file_dialog.on_set = func_backup_dialog_set_path;
    func_toggle_scene((void*) SCENE_FILE);
}

void func_backup_dialog_toggle(void)
{
    state.backup_dialog.prev_enable = !state.backup_dialog.prev_enable;
}

void func_backup_dialog_parent_select(int index)
{
    if (index < (int) state.backup_dialog.backups.count){
        state.backup_dialog.selected_backup = index;
        cwk_path_normalize(state.backup_dialog.backups.items[index].name, state.backup_dialog.prev, sizeof(state.backup_dialog.prev));
    }
}

void func_backup_dialog_backup(void)
{
    state.backup_dialog.is_backup = true;
    func_backup_dialog_init();
}

void func_backup_dialog_merge(void)
{
    state.backup_dialog.is_backup = false;
    func_backup_dialog_init();
}

void func_run_dialog_init(void)
{
    RunDialog *rn = &state.run_dialog;
    msgq_init();
    thread_create(&rn->worker, rn->fn, &rn->args);
    state.run_dialog.running = true;
    worker_done = false;
}

void func_run_dialog_exit(void)
{
    RunDialog *rn = &state.run_dialog;
    for (size_t i=0; i<rn->log.count; ++i){
        free(rn->log.items[i]);
    }
    rn->log.count = 0;
    func_refresh();
    func_toggle_scene((void*) SCENE_MAIN);
}

void func_backup_dialog_create(void)
{
    BackupDialog *bd = &state.backup_dialog;
    RunDialog *rn = &state.run_dialog;
    if (!flib_isdir(bd->dest)) return;
    thread_args_t args;
    if (bd->is_backup){
        args.args[0] = bd->branch_name;
        args.args[1] = bd->dest;
        args.args[2] = bd->prev_enable && flib_isdir(bd->prev) ? bd->prev : NULL;
        rn->fn = tbackup;
        rn->args = args;
    }else{
        args.args[0] = bd->prev;
        args.args[1] = bd->dest;
        rn->fn = tmerge;
        rn->args = args;
    }
    bd->backups.count = 0;
    func_run_dialog_init();
    func_toggle_scene((void*) SCENE_RUN);
}

void HandleFuncButtonInteraction(Clay_ElementId id, Clay_PointerData pointer_data, intptr_t user_data)
{
    (void) id;
    FuncButtonData button_data = (FuncButtonData)user_data;
    if (pointer_data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME){
        if (button_data != NULL) button_data();
    }
}

void HandleArgFuncButtonInteraction(Clay_ElementId id, Clay_PointerData pointer_data, intptr_t user_data)
{
    (void) id;
    ArgFuncButtonData *button_data = (ArgFuncButtonData*) user_data;
    if (pointer_data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME){
        if (button_data != NULL && button_data->func != NULL){
            button_data->func(button_data->arg);
        }
    }
}

void HandleBranchButtonInteraction(Clay_ElementId id, Clay_PointerData pointer_data, intptr_t user_data)
{
    (void) id;
    int index = (int) user_data;
    if (pointer_data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
        state.selected_branch = index;
    }
}

void HandleFileButtonInteraction(Clay_ElementId id, Clay_PointerData pointer_data, intptr_t user_data)
{
    (void) id;
    int index = (int) user_data;
    if (pointer_data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
        state.file_dialog.item_index = index;
    }
}

void HandleSceneButtonInteraction(Clay_ElementId id, Clay_PointerData pointer_data, intptr_t user_data)
{
    (void) id;
    if (pointer_data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME){
        func_toggle_scene((void*) user_data);
    }
}

void HandleFileDeleteButtonInteraction(Clay_ElementId id, Clay_PointerData pointer_data, intptr_t user_data)
{
    (void) id;
    if (pointer_data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME){
        func_new_branch_delete((size_t) user_data);
    }
}

void HandleParentSelectInteraction(Clay_ElementId id, Clay_PointerData pointer_data, intptr_t user_data)
{
    (void) id;
    if (pointer_data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
        func_backup_dialog_parent_select((int) user_data);
    }
}

// render functions

void text_layout(Clay_String string, uint16_t font_id, uint16_t font_size, uint16_t spacing)
{
    CLAY_TEXT(string, CLAY_TEXT_CONFIG({
        .textColor = state.theme.text,
        .fontId = font_id,
        .fontSize = font_size,
        .letterSpacing = spacing
    }));
}

void settings_menu_layout(void)
{
    CLAY({
        .backgroundColor = BLUR_COLOR,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {.element=CLAY_ATTACH_POINT_CENTER_CENTER, .parent=CLAY_ATTACH_POINT_CENTER_CENTER}
        },
        .layout = {
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
            .childAlignment={CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
        },
    }){
        CLAY({
            .id = CLAY_ID("settings_frame"),
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }){
            CLAY({
                .backgroundColor = state.theme.secondary,
                .layout = {
                    .sizing = {.width=CLAY_SIZING_GROW()},
                    .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                }
            }){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .padding = {.left=4}
                    }
                }){
                    CLAY_TEXT(CLAY_STRING("Settings"), CLAY_TEXT_CONFIG({
                        .textColor = state.theme.text,
                        .fontId = DEFAULT, 
                        .fontSize = 12,
                        .letterSpacing = 2
                    }));
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? darken_color(state.theme.danger) : state.theme.danger,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    },
                }){
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                        },
                        .image = {&state.textures[SYMBOL_EXIT_16]}
                    }){
                        Clay_OnHover(HandleSceneButtonInteraction, (intptr_t) SCENE_MAIN);
                    }
                }
            }
            CLAY({
                .backgroundColor = state.theme.background,
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(196), CLAY_SIZING_FIXED(196)}
                }
            }){}
        }
    }
}

void input_menu_layout(void)
{
    CLAY({
        .backgroundColor = BLUR_COLOR,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {.element=CLAY_ATTACH_POINT_CENTER_CENTER, .parent=CLAY_ATTACH_POINT_CENTER_CENTER}
        },
        .layout = {
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
            .childAlignment={CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
        },
    }){
        CLAY({
            .id = CLAY_ID("input_frame"),
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }){
            CLAY({
                .backgroundColor = state.theme.secondary,
                .layout = {
                    .sizing = {.width=CLAY_SIZING_GROW()},
                    .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                }
            }){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .padding = {.left=4}
                    }
                }){
                    text_layout(CLAY_STRING("Create Branch"), DEFAULT, 12, 2);
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? darken_color(state.theme.danger) : state.theme.danger,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    },
                }){
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                        },
                        .image = {&state.textures[SYMBOL_EXIT_16]}
                    }){
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_branch_exit);
                    }
                }
            }
            CLAY({
                .backgroundColor = state.theme.background,
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    .childGap = 4
                }
            }){
                CLAY({
                    
                }){
                    snprintf(state.temp_buffer, sizeof(state.temp_buffer), "(%d/%d)", state.new_branch_len, NEW_BRANCH_MAX_LEN);
                    text_layout(CLAY_STRING("Insert the name for the branch: "), DEFAULT, 12, 1);
                    text_layout(clay_string(state.temp_buffer), DEFAULT, 12, 1);
                }
                CLAY({
                    .backgroundColor = state.theme.secondary,
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIXED(256), .height=CLAY_SIZING_FIXED(20)},
                        .padding = CLAY_PADDING_ALL(4),
                        .childAlignment = {.y=CLAY_ALIGN_Y_CENTER},
                    },
                    .border = {
                        .color = darken_color(state.theme.secondary),
                        .width = CLAY_BORDER_OUTSIDE(2)
                    },
                }){
                    if (Clay_Hovered()){
                        SetMouseCursor(MOUSE_CURSOR_IBEAM);

                        int key = GetCharPressed();

                        while (key > 0){
                            if ((key >= 32) && (key <= 125) && (state.new_branch_len < NEW_BRANCH_MAX_LEN)){
                                state.new_branch_name[state.new_branch_len] = (char)key;
                                state.new_branch_name[state.new_branch_len+1] = '\0'; 
                                state.new_branch_len++;
                            }
                            key = GetCharPressed(); 
                        }
                        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)){
                            state.new_branch_len--;
                            if (state.new_branch_len < 0) state.new_branch_len = 0;
                            state.new_branch_name[state.new_branch_len] = '\0';
                        }
                        state.frame_counter++;
                    }else{
                        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
                    }
                    text_layout((Clay_String){false, state.new_branch_len, state.new_branch_name}, MONO_12, 12, 1);
                    if (Clay_Hovered() && ((state.frame_counter/20)%2) == 0){
                        text_layout(CLAY_STRING("_"), MONO_12, 12, 1);
                    }
                }
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                    }
                }){
                    CLAY({
                        .layout = {
                            .sizing = {.width=CLAY_SIZING_GROW()}
                        }
                    }){
                        text_layout(CLAY_STRING("Dirs:"), DEFAULT, 12, 1);
                    }
                    CLAY({
                        .backgroundColor = Clay_Hovered()? state.theme.hover : state.theme.accent,
                        .layout = {
                            .padding = TEXT_PADDING
                        }
                    }){
                        if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t)func_new_branch_add);
                        text_layout(CLAY_STRING("Add"), DEFAULT, 12, 1);
                    }
                }
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    }
                }){
                    Files dirs = state.new_branch_dialog.dirs;
                    for (size_t i=0; i<dirs.count; ++i){
                        CLAY({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW()},
                                .childGap = 2,
                                .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                            }
                        }){
                            CLAY({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_GROW()},
                                    .childGap = 2
                                }
                            }){
                                text_layout(CLAY_STRING("-"), MONO_12, 12, 0);
                                text_layout(clay_string(dirs.items[i].path), MONO_12, 12, 0);
                            }
                            CLAY({
                                .backgroundColor = Clay_Hovered()? darken_color(state.theme.danger) : state.theme.danger,
                                .layout = {
                                    .sizing = {CLAY_SIZING_FIXED(12), CLAY_SIZING_FIXED(12)}
                                },
                                .image = {&state.textures[SYMBOL_DELETE_12]}
                            }){
                                Clay_OnHover(HandleFileDeleteButtonInteraction, (intptr_t) i);
                            }
                        }
                    }
                }
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    }
                }){
                    CLAY({
                        .backgroundColor = Clay_Hovered()? state.theme.hover : state.theme.accent,
                        .layout = {
                            .padding = {.left=8, .right=8, .top=4, .bottom=4},
                        }
                    }){
                        if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t)func_add_branch);
                        text_layout(CLAY_STRING("Create"), DEFAULT, 12, 1);
                    }
                }
            }
        }
    }
}

void history_menu_layout(void)
{
    CLAY({
        .backgroundColor = BLUR_COLOR,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {.element=CLAY_ATTACH_POINT_CENTER_CENTER, .parent=CLAY_ATTACH_POINT_CENTER_CENTER}
        },
        .layout = {
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
            .childAlignment={CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
        },
    }){
        CLAY({
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }){
            CLAY({
                .backgroundColor = state.theme.secondary,
                .layout = {
                    .sizing = {.width=CLAY_SIZING_GROW()},
                    .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                }
            }){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .padding = {.left=4}
                    }
                }){
                    CLAY_TEXT(CLAY_STRING("History"), CLAY_TEXT_CONFIG({
                        .textColor = state.theme.text,
                        .fontId = DEFAULT, 
                        .fontSize = 12,
                        .letterSpacing = 2
                    }));
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? darken_color(state.theme.danger) : state.theme.danger,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    },
                }){
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                        },
                        .image = {&state.textures[SYMBOL_EXIT_16]}
                    }){}
                    Clay_OnHover(HandleSceneButtonInteraction, (intptr_t) SCENE_MAIN);
                }
            }
            CLAY({
                .backgroundColor = state.theme.background,
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = CLAY_PADDING_ALL(4),
                    .childGap = 8
                }
            }){
                
            }
        }
    }
}
void about_menu_layout(void)
{
    CLAY({
        .backgroundColor = BLUR_COLOR,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {.element=CLAY_ATTACH_POINT_CENTER_CENTER, .parent=CLAY_ATTACH_POINT_CENTER_CENTER}
        },
        .layout = {
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
            .childAlignment={CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
        },
    }){
        CLAY({
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }){
            CLAY({
                .backgroundColor = state.theme.secondary,
                .layout = {
                    .sizing = {.width=CLAY_SIZING_GROW()},
                    .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                }
            }){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .padding = {.left=4}
                    }
                }){
                    CLAY_TEXT(CLAY_STRING("About"), CLAY_TEXT_CONFIG({
                        .textColor = state.theme.text,
                        .fontId = DEFAULT, 
                        .fontSize = 12,
                        .letterSpacing = 2
                    }));
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? darken_color(state.theme.danger) : state.theme.danger,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    },
                }){
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                        },
                        .image = {&state.textures[SYMBOL_EXIT_16]}
                    }){
                        Clay_OnHover(HandleSceneButtonInteraction, (intptr_t) SCENE_MAIN);
                    }
                }
            }
            CLAY({
                .backgroundColor = state.theme.background,
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = CLAY_PADDING_ALL(4),
                    .childGap = 8
                }
            }){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                        .padding = TEXT_PADDING
                    }
                }){
                    text_layout(CLAY_STRING("Cebeq v"), MONO_16, 16, 0);
                    text_layout(clay_string(state.version), MONO_16, 16, 0);
                }
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW(0, 312), .height=CLAY_SIZING_FIXED(128)},
                        .padding = CLAY_PADDING_ALL(4)
                    },
                    .border = {
                        .color = state.theme.hover,
                        .width = CLAY_BORDER_OUTSIDE(2)
                    }
                }){
                    CLAY({
                        .backgroundColor = state.theme.background,
                        .layout = {
                            .padding = {.left=4, .right=2, .top=3},
                        },
                        .floating = {
                            .attachTo = CLAY_ATTACH_TO_PARENT,
                            .attachPoints = {.parent = CLAY_ATTACH_POINT_CENTER_TOP, .element = CLAY_ATTACH_POINT_CENTER_CENTER},
                        },
                    }){
                        text_layout(CLAY_STRING("MIT License"), MONO_12, 12, 1);
                    }
                    CLAY({
                        .layout = {
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            .padding = {.left=8, .right=8, .top=10, .bottom=8},
                            .childGap = 8
                        },
                        .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
                    }){
                        CLAY({
                            .layout = {
                                .sizing = {.width=CLAY_SIZING_GROW()},
                                .childAlignment = {.x=CLAY_ALIGN_X_CENTER},
                            }
                        }){
                            CLAY_TEXT(CLAY_STRING("Copyright (c) 2025 Constantijn de Meer"), CLAY_TEXT_CONFIG({
                                .textColor = state.theme.text,
                                .fontId = MONO_12,
                                .fontSize = 12,
                                .textAlignment = CLAY_TEXT_ALIGN_CENTER
                            }));
                        }
                        text_layout(license_text, MONO_12, 12, 0);
                    }
                }
            }
        }
    }
}

void dir_input_menu_layout()
{
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    FileDialog *fd = &state.file_dialog;
    if (fd->first_frame){
        DIR *dir = opendir(fd->dir_path);
        if (dir == NULL){
            func_toggle_scene((void*) SCENE_MAIN);
            eprintf("Invalid initial dir_path: '%s'!", fd->dir_path);
            return;
        }
        flib_entry entry;
        fd->items.count = 0;
        while (flib_get_entry(dir, fd->dir_path, &entry)){
            if (entry.type == FLIB_DIR){
                File file;
                memcpy(file.path, entry.path, FILENAME_MAX);
                memcpy(file.name, entry.name, FILENAME_MAX);
                nob_da_append(&fd->items, file);
            }
        }
        closedir(dir);
        fd->first_frame = false;
    }
    CLAY({
        .backgroundColor = BLUR_COLOR,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {.element=CLAY_ATTACH_POINT_CENTER_CENTER, .parent=CLAY_ATTACH_POINT_CENTER_CENTER}
        },
        .layout = {
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
            .childAlignment={CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
        },
    }){
        CLAY({
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }){
            CLAY({
                .backgroundColor = state.theme.secondary,
                .layout = {
                    .sizing = {.width=CLAY_SIZING_GROW()},
                    .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                }
            }){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .padding = {.left=4}
                    }
                }){
                    CLAY_TEXT(CLAY_STRING("Select Directory"), CLAY_TEXT_CONFIG({
                        .textColor = state.theme.text,
                        .fontId = DEFAULT, 
                        .fontSize = 12,
                        .letterSpacing = 2
                    }));
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? darken_color(state.theme.danger) : state.theme.danger,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    },
                }){
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                        },
                        .image = {&state.textures[SYMBOL_EXIT_16]}
                    }){
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_dir_dialog_exit);
                    }
                }
            }
            CLAY({
                .backgroundColor = state.theme.background,
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .padding = CLAY_PADDING_ALL(4),
                    .childGap = 4
                }
            }){
                CLAY({
                    // nav bar
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .childAlignment = {.y=CLAY_ALIGN_Y_CENTER},
                        .childGap = 4
                    }
                }){
                    CLAY({
                        .backgroundColor = state.theme.secondary,
                        .layout = {
                            .sizing = {.width=CLAY_SIZING_GROW(312)},
                            .padding = {.left=6, .right=6, .top=4, .bottom=4}
                        },
                        .border = {
                            .color = darken_color(state.theme.secondary),
                            .width = CLAY_BORDER_OUTSIDE(2)
                        },
                        .clip = {.horizontal=true, .childOffset = Clay_GetScrollOffset()}
                    }){
                        text_layout(clay_string(state.file_dialog.dir_path), MONO_12, 12, 0);
                    }
                    CLAY({
                        .layout = {
                            .childGap = 4
                        }
                    }){
                        CLAY({
                            .backgroundColor = Clay_Hovered()? state.theme.secondary : state.theme.text,
                            .layout = {
                                .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                            },
                            .image = {&state.textures[SYMBOL_BACK_16]}
                        }){
                            Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_dir_dialog_back);
                        }
                        CLAY({
                            .backgroundColor = Clay_Hovered()? state.theme.secondary : state.theme.text,
                            .layout = {
                                .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                            },
                            .image = {&state.textures[SYMBOL_FWD_16]}
                        }){
                            Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_dir_dialog_forward);
                        }
                    }
                }
                CLAY({
                    // file display
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW(), .height=CLAY_SIZING_FIXED(128)},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .padding = {.top=2}
                    },
                    .border = {
                        .color = state.theme.hover,
                        .width = CLAY_BORDER_OUTSIDE(2)
                    },
                    .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
                }){
                    for (size_t i=0; i<fd->items.count; ++i){
                        File *file = &fd->items.items[i];
                        CLAY({
                            .backgroundColor = fd->item_index == (int) i? state.theme.accent: Clay_Hovered()? state.theme.secondary : NO_COLOR,
                            .layout = {
                                .sizing = {.width=CLAY_SIZING_GROW()},
                                .padding = {.left=4, .right=4, .top=2, .bottom=2}
                            }
                        }){
                            Clay_OnHover(HandleFileButtonInteraction, (intptr_t)i);
                            text_layout(clay_string(file->name), MONO_12, 12, 0);
                        }
                    }
                    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)){
                        if (fd->item_index < (int) fd->items.count-1){
                            fd->item_index++;
                        }
                    }
                    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)){
                        if (fd->item_index > 0){
                            fd->item_index--;
                        }
                    }
                    if (IsKeyPressed(KEY_RIGHT) && fd->item_index >= 0){
                        func_dir_dialog_forward();
                    }
                    if (IsKeyPressed(KEY_LEFT)){
                        func_dir_dialog_back();
                    }
                    if (IsKeyPressed(KEY_ENTER)){
                        func_dir_dialog_set();
                    }
                }
                CLAY({
                    // status bar
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .childGap = 4
                    }
                }){
                    CLAY({
                        .backgroundColor = state.theme.secondary,
                        .layout = {
                            .sizing = {.width=CLAY_SIZING_GROW(), .height=CLAY_SIZING_GROW()},
                            .padding = {.left=6, .right=6},
                            .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                        },
                        .border = {
                            .color = darken_color(state.theme.secondary),
                            .width = CLAY_BORDER_OUTSIDE(2)
                        }
                    }){
                        if (fd->item_index >= 0){
                            text_layout(clay_string(fd->items.items[fd->item_index].name), MONO_12, 12, 0);
                        }
                    }
                    CLAY({
                        .backgroundColor = Clay_Hovered()? state.theme.hover : state.theme.accent,
                        .layout = {
                            .padding = TEXT_PADDING
                        }
                    }){
                        if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_dir_dialog_set);
                        text_layout(CLAY_STRING("Ok"), DEFAULT, 16, 0);
                    }
                }
            }
        }
    }
}

void confirm_menu_layout(void)
{
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    CLAY({
        .backgroundColor = BLUR_COLOR,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {.element=CLAY_ATTACH_POINT_CENTER_CENTER, .parent=CLAY_ATTACH_POINT_CENTER_CENTER}
        },
        .layout = {
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
            .childAlignment={CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
        },
    }){
        CLAY({
            .id = CLAY_ID("input_frame"),
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }){
            CLAY({
                .backgroundColor = state.theme.secondary,
                .layout = {
                    .sizing = {.width=CLAY_SIZING_GROW()},
                    .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                }
            }){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .padding = {.left=4}
                    }
                }){
                    text_layout(CLAY_STRING("Confirm"), DEFAULT, 12, 2);
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? darken_color(state.theme.danger) : state.theme.danger,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    },
                }){
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                        },
                        .image = {&state.textures[SYMBOL_EXIT_16]}
                    }){
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_confirm_no);
                    }
                }
            }
            CLAY({
                .backgroundColor = state.theme.background,
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    .childGap = 4
                }
            }){
                text_layout(state.confirm_dialog.question, DEFAULT, 12, 1);
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .childAlignment = {.x=CLAY_ALIGN_X_CENTER},
                        .childGap = 16
                    }
                }){
                    CLAY({
                        .backgroundColor = Clay_Hovered()? state.theme.hover : state.theme.accent,
                        .layout = {
                            .padding = TEXT_PADDING,
                        }
                    }){
                        if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_confirm_yes);
                        text_layout(CLAY_STRING("Yes"), DEFAULT, 12, 1);
                    }
                    CLAY({
                        .backgroundColor = Clay_Hovered()? darken_color(state.theme.danger) : state.theme.danger,
                        .layout = {
                            .padding = TEXT_PADDING,
                        }
                    }){
                        if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_confirm_no);
                        text_layout(CLAY_STRING("No"), DEFAULT, 12, 1);
                    }
                }
            }
        }
    }
}

void backup_dialog_layout(void)
{
    BackupDialog *bd = &state.backup_dialog;
    CLAY({
        .backgroundColor = BLUR_COLOR,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {.element=CLAY_ATTACH_POINT_CENTER_CENTER, .parent=CLAY_ATTACH_POINT_CENTER_CENTER}
        },
        .layout = {
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
            .childAlignment={CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
        },
    }){
        CLAY({
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }){
            CLAY({
                .backgroundColor = state.theme.secondary,
                .layout = {
                    .sizing = {.width=CLAY_SIZING_GROW()},
                    .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                }
            }){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .padding = {.left=4}
                    }
                }){
                    CLAY_TEXT(clay_string(bd->is_backup? "Backup" : "Merge"), CLAY_TEXT_CONFIG({
                        .textColor = state.theme.text,
                        .fontId = DEFAULT, 
                        .fontSize = 12,
                        .letterSpacing = 2
                    }));
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? darken_color(state.theme.danger) : state.theme.danger,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    },
                }){
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                        },
                        .image = {&state.textures[SYMBOL_EXIT_16]}
                    }){
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_backup_dialog_exit);
                    }
                }
            }
            CLAY({
                .backgroundColor = state.theme.background,
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = CLAY_PADDING_ALL(4),
                    .childGap = 4
                }
            }){
                text_layout(CLAY_STRING("Enter a destination:"), DEFAULT, 12, 1);
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .padding = CLAY_PADDING_ALL(6),
                        .childGap = 4
                    },
                    .border = {.color=state.theme.hover, .width=CLAY_BORDER_OUTSIDE(2)},
                }){
                    CLAY({
                        .backgroundColor = state.theme.secondary,
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(256), .height=CLAY_SIZING_FIXED(20)},
                            .padding = CLAY_PADDING_ALL(6),
                            .childAlignment = {.y=CLAY_ALIGN_Y_CENTER},
                        },
                        .border = {
                            .color = darken_color(state.theme.secondary),
                            .width = CLAY_BORDER_OUTSIDE(2)
                        },
                    }){
                        if (Clay_Hovered()){
                            SetMouseCursor(MOUSE_CURSOR_IBEAM);

                            int key = GetCharPressed();

                            while (key > 0){
                                if ((key >= 32) && (key <= 125) && (bd->dest_len < FILENAME_MAX)){
                                    bd->dest[bd->dest_len] = (char)key;
                                    bd->dest[bd->dest_len+1] = '\0'; 
                                    bd->dest_len++;
                                }
                                key = GetCharPressed(); 
                            }
                            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)){
                                bd->dest_len--;
                                if (bd->dest_len < 0) bd->dest_len = 0;
                                bd->dest[bd->dest_len] = '\0';
                            }
                            state.frame_counter++;
                        }else{
                            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
                        }
                        text_layout((Clay_String){false, bd->dest_len, bd->dest}, MONO_12, 12, 1);
                        if (Clay_Hovered() && ((state.frame_counter/20)%2) == 0){
                            text_layout(CLAY_STRING("_"), MONO_12, 12, 1);
                        }
                    }
                    CLAY({
                        .backgroundColor = Clay_Hovered()? state.theme.hover : state.theme.accent,
                        .layout = {
                            .padding = TEXT_PADDING
                        }
                    }){
                        if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                        text_layout(CLAY_STRING("Select"), DEFAULT, 12, 1);
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_backup_dialog_select);
                    }
                }
                CLAY({
                    .layout = {
                        .childGap = 4,
                    }
                }){
                    if (bd->is_backup){
                        text_layout(CLAY_STRING("Create with parent:"), DEFAULT, 12, 1);
                        CLAY({
                            .backgroundColor = state.theme.secondary,
                            .layout = {
                                .sizing = {CLAY_SIZING_FIXED(12), CLAY_SIZING_FIXED(12)},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                            },
                            .border = {.color=darken_color(state.theme.secondary), .width=CLAY_BORDER_OUTSIDE(2)}
                        }){
                            Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_backup_dialog_toggle);
                            if (state.backup_dialog.prev_enable){
                                CLAY({
                                    .backgroundColor = state.theme.text,
                                    .layout = {
                                        .sizing = {CLAY_SIZING_FIXED(12), CLAY_SIZING_FIXED(12)}
                                    },
                                    .image = {&state.textures[SYMBOL_CHECK_12]}
                                }){}
                            }
                        }
                    } else{
                        text_layout(CLAY_STRING("Select a backup:"), DEFAULT, 12, 1);
                    }
                }
                CLAY({
                    .layout = {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .sizing = {.width=CLAY_SIZING_GROW(), .height=CLAY_SIZING_FIXED(20*4.5)},
                    },
                    .border = {
                        .color = state.theme.hover,
                        .width = CLAY_BORDER_OUTSIDE(2)
                    },
                    .clip = {.vertical = true, .childOffset = Clay_GetScrollOffset()},
                }){
                    for (size_t i=0; i<state.backup_dialog.backups.count; ++i){
                        Branch backup = state.backup_dialog.backups.items[i];
                        CLAY({
                            .backgroundColor = (int) i == state.backup_dialog.selected_backup? state.theme.hover : Clay_Hovered()? state.theme.secondary : NO_COLOR,
                            .layout = {
                                .sizing = {.width=CLAY_SIZING_GROW()},
                                .padding = TEXT_PADDING
                            }
                        }){
                            Clay_OnHover(HandleParentSelectInteraction, (intptr_t) i);
                            text_layout(backup.text, MONO_12, 12, 0);
                        }
                    }
                    if (bd->is_backup && !state.backup_dialog.prev_enable){
                        CLAY({
                            .backgroundColor = {255, 255, 255, 128},
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()}
                            },
                            .floating = {
                                .attachTo = CLAY_ATTACH_TO_PARENT,
                            }
                        }){}
                    }
                }
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .childAlignment = {.x=CLAY_ALIGN_X_CENTER}
                    }
                }){
                    CLAY({
                        .backgroundColor = Clay_Hovered()? state.theme.hover : state.theme.accent,
                        .layout = {
                            .padding = TEXT_PADDING
                        }
                    }){
                        if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_backup_dialog_create);
                        text_layout(CLAY_STRING("Create"), DEFAULT, 16, 1);
                    }
                }
            }
        }
    }
}

void run_dialog_layout(void)
{
    RunDialog *rn = &state.run_dialog;
    if (rn->running){
        char msg[MAX_MSG_LEN];
        while (msgq_pop(msg, sizeof(msg))){
            nob_da_append(&rn->log, strdup(msg));
        }
    }
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
    CLAY({
        .backgroundColor = BLUR_COLOR,
        .floating = {
            .attachTo = CLAY_ATTACH_TO_ROOT,
            .attachPoints = {.element=CLAY_ATTACH_POINT_CENTER_CENTER, .parent=CLAY_ATTACH_POINT_CENTER_CENTER}
        },
        .layout = {
            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
            .childAlignment={CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }){
        CLAY({
            .backgroundColor = state.theme.background,
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .padding = CLAY_PADDING_ALL(4)
            }
        }){
            CLAY({
                .backgroundColor = state.theme.secondary,
                .layout = {
                    .sizing = {.width=CLAY_SIZING_GROW()},
                    .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                }
            }){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .padding = {.left=4}
                    }
                }){
                    CLAY_TEXT(CLAY_STRING("Run"), CLAY_TEXT_CONFIG({
                        .textColor = state.theme.text,
                        .fontId = DEFAULT, 
                        .fontSize = 12,
                        .letterSpacing = 2
                    }));
                }
            }
            CLAY({
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = CLAY_PADDING_ALL(4),
                    .childGap = 4
                }
            }){
                CLAY({
                    .backgroundColor = state.theme.background,
                    .layout = {
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .sizing = {.width=CLAY_SIZING_FIXED(512), .height=CLAY_SIZING_FIXED(156)},
                        .padding = CLAY_PADDING_ALL(6),
                        .childGap = 4
                    },
                    .border = {.color=state.theme.hover, .width=CLAY_BORDER_OUTSIDE(2)},
                    .clip = {.vertical=true, .childOffset=Clay_GetScrollOffset()}
                }){
                    for (size_t i=0; i<rn->log.count; ++i){
                        Clay_String string = clay_string(rn->log.items[i]);
                        CLAY({
                            .backgroundColor = NO_COLOR,
                            .layout = {
                                .sizing = {.width=CLAY_SIZING_GROW()}
                            }
                        }){
                            text_layout(string, MONO_12, 12, 0);
                        }
                    }
                }
            }
            if (worker_done && rn->running){
                msgq_destroy();
                rn->running = false;
            }
            if (!rn->running){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .childAlignment = {.x=CLAY_ALIGN_X_CENTER}
                    }
                }){
                    CLAY({
                        .backgroundColor = Clay_Hovered()? state.theme.hover : state.theme.accent,
                        .layout = {
                            .padding = TEXT_PADDING,
                        }
                    }){
                        if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) func_run_dialog_exit);
                        text_layout(CLAY_STRING("Exit"), DEFAULT, 12, 1);
                    }
                }
            }
        }
    }
}

void branch_action_button_layout(Clay_String string, Clay_Color color, FuncButtonData data)
{
    CLAY({
        .backgroundColor = Clay_Hovered()? darken_color(color) : color,
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW()},
            .padding = {.left=8, .right=8, .top=4, .bottom=4},
            .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
        },
    }){
        CLAY_TEXT(string, CLAY_TEXT_CONFIG({
            .textColor = state.theme.text,
            .fontId = DEFAULT,
            .fontSize = 16,
            .letterSpacing = 1
        }));
        if (Clay_Hovered()){
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) data);
    }
}

void action_button_layout(Clay_String string, FuncButtonData data)
{
    CLAY({
        .backgroundColor = Clay_Hovered()? state.theme.hover : state.theme.accent,
        .layout = {
            .padding = CLAY_PADDING_ALL(8),
        },
    }){
        CLAY_TEXT(string, CLAY_TEXT_CONFIG({
            .textColor = state.theme.text,
            .fontId = DEFAULT, 
            .fontSize = 18,
            .letterSpacing = 2,
        }));        
        if (Clay_Hovered()){
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) data);
    }
}

void render_task_menu_button(Clay_String string, FuncButtonData data)
{
    CLAY({
        .backgroundColor = Clay_Hovered()? state.theme.hover : state.theme.secondary,
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW()},
            .padding = {.left=8, .right=8, .top=4, .bottom=4},
        },
    }){
        CLAY_TEXT(string, CLAY_TEXT_CONFIG({
            .textColor = state.theme.text,
            .fontId = DEFAULT,
            .fontSize = 12,
            .letterSpacing = 2
        }));
        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t)data);
    }
}

// Layouts

Clay_RenderCommandArray main_layout()
{
    Clay_BeginLayout();
    {
        CLAY({
            .backgroundColor = state.theme.background,
            .layout = {
                .sizing = {.height=CLAY_SIZING_GROW(), .width=CLAY_SIZING_GROW()},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }){
            if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            CLAY({
                .id = CLAY_ID("task_bar"),
                .backgroundColor = state.theme.secondary,
                .layout = {
                    .sizing = {.width=CLAY_SIZING_GROW()},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .padding = CLAY_PADDING_ALL(4),
                }
            }){
                CLAY({
                    .id = CLAY_ID("task_file_button"),
                    .backgroundColor = Clay_Hovered()? state.theme.hover : NO_COLOR,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                    },
                }){
                    CLAY_TEXT(CLAY_STRING("File"), CLAY_TEXT_CONFIG({
                        .textColor = state.theme.text,
                        .fontId = DEFAULT,
                        .fontSize = 16
                    }));
                    bool hovered = Clay_PointerOver(Clay_GetElementId(CLAY_STRING("task_file_button"))) || Clay_PointerOver(Clay_GetElementId(CLAY_STRING("task_file_menu")));
                    if (hovered){
                        CLAY({
                            .id = CLAY_ID("task_file_menu"),
                            .floating = {
                                .attachTo = CLAY_ATTACH_TO_PARENT,
                                .attachPoints = {.parent = CLAY_ATTACH_POINT_LEFT_BOTTOM},
                            },
                            .layout = {
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                            .border = {
                                .color = darken_color(state.theme.secondary),
                                .width = CLAY_BORDER_OUTSIDE(2)
                            }
                        }){
                            render_task_menu_button(CLAY_STRING("Open File Location"), func_task_loc_open);
                            render_task_menu_button(CLAY_STRING("Open Config File"), NULL);
                            render_task_menu_button(CLAY_STRING("Copy Path"), NULL);
                        }
                    }
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? state.theme.hover : NO_COLOR,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                    },
                }){
                    CLAY_TEXT(CLAY_STRING("Settings"), CLAY_TEXT_CONFIG({
                        .textColor = state.theme.text,
                        .fontId = DEFAULT,
                        .fontSize = 16
                    }));
                    Clay_OnHover(HandleSceneButtonInteraction, (intptr_t) SCENE_SETTINGS);
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? state.theme.hover : NO_COLOR,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                    }
                }){
                    CLAY_TEXT(CLAY_STRING("About"), CLAY_TEXT_CONFIG({
                        .textColor = state.theme.text,
                        .fontId = DEFAULT,
                        .fontSize = 16
                    }));
                    Clay_OnHover(HandleSceneButtonInteraction, (intptr_t) SCENE_ABOUT);
                }
            }
            
            CLAY({
                .id = CLAY_ID("content_frame"),
                .layout = {
                    .sizing = {.height=CLAY_SIZING_GROW(), .width=CLAY_SIZING_GROW()},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = CLAY_PADDING_ALL(8),
                    .childGap = 8
                }
            }){
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW(), .height=CLAY_SIZING_GROW()},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .childGap = 8
                    }
                }){
                    CLAY({
                        .layout = {
                            .sizing = {.width=CLAY_SIZING_GROW(), .height=CLAY_SIZING_GROW()},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            .childGap = 8
                        }
                    }){
                        CLAY({
                            .layout = {
                                .sizing = {.width=CLAY_SIZING_GROW()},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                                .childAlignment = {.y=CLAY_ALIGN_Y_CENTER}
                            }
                        }){
                            CLAY({
                                .layout = {
                                    .sizing = {.width=CLAY_SIZING_GROW()}
                                }
                            }){
                                CLAY_TEXT(CLAY_STRING("Select a branch:"), CLAY_TEXT_CONFIG({
                                    .textColor = state.theme.text,
                                    .fontId = DEFAULT, 
                                    .fontSize = 16,
                                    .letterSpacing = 1
                                }));
                            }
                            CLAY({
                                .backgroundColor = Clay_Hovered()? state.theme.hover : state.theme.accent,
                                .layout = {
                                    .padding = {.left=8, .right=8, .top=4, .bottom=4},
                                    .childGap = 8
                                }
                            }){
                                if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                                Clay_OnHover(HandleFuncButtonInteraction, (intptr_t)func_refresh);
                                
                                CLAY_TEXT(CLAY_STRING("Refresh"), CLAY_TEXT_CONFIG({
                                    .textColor = state.theme.text,
                                    .fontId = DEFAULT, 
                                    .fontSize = 16,
                                    .letterSpacing = 1
                                }));
                                CLAY({
                                    .layout = {
                                        .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)},
                                    },
                                    .image = {&state.textures[SYMBOL_REFRESH_16]}
                                }){}
                            }
                        }
                        CLAY({
                            .id = CLAY_ID("branch_selector"),
                            .layout = {
                                .sizing = {.height=CLAY_SIZING_GROW(), .width=CLAY_SIZING_GROW()},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                .padding = CLAY_PADDING_ALL(4)
                            },
                            .border = {
                                .color = state.theme.hover,
                                .width = CLAY_BORDER_OUTSIDE(2)
                            },
                            .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
                        }){
                            for (size_t i=0; i<state.branches.count; ++i){
                                Branch branch = state.branches.items[i];
                                CLAY({
                                    .backgroundColor = state.selected_branch == (int)i? state.theme.accent : Clay_Hovered()? state.theme.secondary : NO_COLOR,
                                    .layout = {
                                        .sizing = {.width=CLAY_SIZING_GROW()},
                                        .padding = CLAY_PADDING_ALL(2),
                                    }
                                }){
                                    CLAY_TEXT(branch.text, CLAY_TEXT_CONFIG({
                                        .textColor = state.theme.text,
                                        .fontId = MONO_16, 
                                        .fontSize = 16,
                                    }));
                                    Clay_OnHover(HandleBranchButtonInteraction, (int) i);
                                }
                            }
                        }
                        CLAY({
                            .id = CLAY_ID("action_frame"),
                            .layout = {
                                .sizing = {.width=CLAY_SIZING_GROW()},
                                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                                .childGap = 64
                            }
                        }){
                            action_button_layout(CLAY_STRING("Make Backup"), func_backup_dialog_backup);
                            action_button_layout(CLAY_STRING("Merge Backup"), func_backup_dialog_merge);
                        }
                    }
                    CLAY({
                        .id = CLAY_ID("branch_action_frame"),
                        .layout = {
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .childGap = 4,
                            .padding = {.top=32}
                        }
                    }){
                        branch_action_button_layout(CLAY_STRING("New"), state.theme.accent, func_branch_new);
                        branch_action_button_layout(CLAY_STRING("History"), state.theme.accent, func_branch_history);
                        branch_action_button_layout(CLAY_STRING("Remove"), state.theme.danger, func_branch_remove_before_confirm);
                    }
                }
            }   
        }
        switch (state.scene){
            case SCENE_MAIN: break;
            case SCENE_NEW: {
                input_menu_layout();
            } break;
            case SCENE_HISTORY:{
                history_menu_layout();
            } break;
            case SCENE_FILE: {
                dir_input_menu_layout();
            } break;
            case SCENE_SETTINGS: {
                settings_menu_layout();
            } break;
            case SCENE_ABOUT:{
                about_menu_layout();
            } break;
            case SCENE_CONFIRM:{
                confirm_menu_layout();
            } break;
            case SCENE_BACKUP:{
                backup_dialog_layout();
            } break;
            case SCENE_RUN:{
                run_dialog_layout();
            } break;
            default:{
                eprintf("Invalid scene state!");
                CloseWindow();
            }
        }
    }
    return Clay_EndLayout();
}

int main(void) {
    if (!setup()) return 1;
    
    int value = 0;    
    cwk_path_join(program_dir, BACKUPS_JSON, state.info_path, sizeof(state.info_path));
    
    state.cson_info = cson_read(state.info_path);
    if (state.cson_info == NULL){
        error("Could not find internal info file!\n");
        return_defer(1);
    }
    
    Cson *c_version = cson_get(state.cson_info, key("version"));
    if (!cson_is_string(c_version)){
        eprintf("Could not find <version> field in info file!");
        return_defer(1);
    }
    state.version = cson_get_cstring(c_version);
    
    state.cson_branches = cson_get(state.cson_info, key("branches"));
    if (!cson_is_map(state.cson_branches)){
        error("Could not find <branches> field in info file!");
        return_defer(1);
    }
    if (!set_branches()) return_defer(1);
    
    memcpy(state.file_dialog.dir_path, program_dir, sizeof(state.file_dialog.dir_path));
    
    Clay_Raylib_Initialize(768, 432, "Cebeq Gui", FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);

    uint64_t clayRequiredMemory = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(clayRequiredMemory, malloc(clayRequiredMemory));
    Clay_Initialize(clayMemory, (Clay_Dimensions) {
       .width = GetScreenWidth(),
       .height = GetScreenHeight()
    }, (Clay_ErrorHandler) { HandleClayErrors, NULL}); 
    
    char font_path[FILENAME_MAX] = {0};    
    Font fonts[_FontId_Count];
    
    fonts[DEFAULT] = GetFontDefault();
    cwk_path_join(program_dir, "resources/JuliaMono-Regular.ttf", font_path, sizeof(font_path));
    fonts[MONO_16] = LoadFontEx(font_path, 16, NULL, 400);
    SetTextureFilter(fonts[MONO_16].texture, TEXTURE_FILTER_BILINEAR);
    fonts[MONO_12] = LoadFontEx(font_path, 12, NULL, 400);
    SetTextureFilter(fonts[MONO_12].texture, TEXTURE_FILTER_BILINEAR);
    
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);
    
    cwk_path_join(program_dir, "resources/MaterialIcons/close_16.png", font_path, sizeof(font_path));
    state.textures[SYMBOL_EXIT_16] = LoadTexture(font_path);
    cwk_path_join(program_dir, "resources/MaterialIcons/refresh_16.png", font_path, sizeof(font_path));
    state.textures[SYMBOL_REFRESH_16] = LoadTexture(font_path);
    cwk_path_join(program_dir, "resources/MaterialIcons/arrow_back_16.png", font_path, sizeof(font_path));
    state.textures[SYMBOL_BACK_16] = LoadTexture(font_path);
    cwk_path_join(program_dir, "resources/MaterialIcons/arrow_forward_16.png", font_path, sizeof(font_path));
    state.textures[SYMBOL_FWD_16] = LoadTexture(font_path);
    cwk_path_join(program_dir, "resources/MaterialIcons/delete_12.png", font_path, sizeof(font_path));
    state.textures[SYMBOL_DELETE_12] = LoadTexture(font_path);
    cwk_path_join(program_dir, "resources/MaterialIcons/check_12.png", font_path, sizeof(font_path));
    state.textures[SYMBOL_CHECK_12] = LoadTexture(font_path);
    
    cwk_path_join(program_dir, "resources/icon.png", font_path, sizeof(font_path));
    Image icon = LoadImage(font_path);
    SetWindowIcon(icon);
    
    while (!WindowShouldClose()) {
        Clay_SetLayoutDimensions((Clay_Dimensions) {
            .width = GetScreenWidth(),
            .height = GetScreenHeight()
        });

        Vector2 mousePosition = GetMousePosition();
        Vector2 scrollDelta = GetMouseWheelMoveV();
        Clay_SetPointerState(
            (Clay_Vector2) { mousePosition.x, mousePosition.y },
            IsMouseButtonDown(0)
        );
        Clay_UpdateScrollContainers(
            true,
            (Clay_Vector2) { scrollDelta.x, scrollDelta.y },
            GetFrameTime()
        );

        Clay_RenderCommandArray renderCommands = main_layout();
        
        BeginDrawing();
        Clay_Raylib_Render(renderCommands, fonts);
        EndDrawing();
    }
  defer:
    for (size_t i=0; i<_FontId_Count; ++i){
        UnloadFont(fonts[i]);
    }
    for (size_t i=0; i<_TextureId_Count; ++i){
        UnloadTexture(state.textures[i]);
    }
    UnloadImage(icon);

    free(state.file_dialog.items.items);
    free(state.new_branch_dialog.dirs.items);
    free(state.backup_dialog.backups.items);
    free(state.run_dialog.log.items);
    
    Clay_Raylib_Close();
    cleanup();
    return value;
}
