#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <message_queue.h>
#include <cebeq.h>
#include <cwalk.h>
#include <cson.h>
#include <nob.h>

#include <raylib.h>
#define CLAY_IMPLEMENTATION
#include <clay.h>
#include <clay_renderer_raylib.c>

#define NEW_BRANCH_MAX_LEN 32

#define NO_COLOR ((Clay_Color) {0})
#define TEXT_PADDING (Clay_Padding){8, 8, 4, 4}
#define clay_string(str) (Clay_String) {false, strlen(str), str}

#define error(msg, ...) (fprintf(stderr, "[ERROR] " msg "\n", ##__VA_ARGS__)) 

typedef enum{
    SCENE_MAIN,
    SCENE_SETTINGS,
    SCENE_INPUT,
    SCENE_ABOUT,
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

Theme window_theme = charcoal_teal;

typedef struct{
    const char *name;
    Clay_String text;
} Branch;

typedef struct{
    Branch *items;
    size_t count;
    size_t capacity;
} Branches;

static Scene current_scene = SCENE_MAIN;
static char info_path[FILENAME_MAX] = {0};
static Texture2D textures[_TextureId_Count];
static Branches branches = {0};
static int selected_branch = -1;
static Cson *c_info = NULL;
static Cson *c_branches = NULL;
static int frame_counter = 0;
char new_branch_name[NEW_BRANCH_MAX_LEN+1] = {0};
int new_branch_len = 0;
static char temp_buffer[256] = {0};
static char *version = NULL;

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

static Nob_String_Builder sb = {0};

void func_toggle_scene(void *arg)
{
    Scene scene = (Scene) arg;
    current_scene = scene;
}

bool set_branches(void)
{
    branches.count = 0;
    Cson *branch_names = cson_map_keys(c_branches);
    if (!cson_is_array(branch_names)){
        error("Invalid branch info state!");
        return false;
    }
    size_t branch_count = cson_len(branch_names);
    for (size_t i=0; i<branch_count; ++i){
        char *name = cson_get_cstring(branch_names, index(i));
        Cson *branch = cson_get(c_branches, key(name));
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
        sb.count = 0;
        nob_sb_append_cstr(&sb, name);
        nob_sb_append_cstr(&sb, ": [");
        for (size_t i=0; i<dir_count; ++i){
            if (i > 0){
                nob_sb_append_cstr(&sb, ", ");
            }
            nob_sb_append_cstr(&sb, "'");
            nob_sb_append_cstr(&sb, cson_get_cstring(dirs, index(i)));
            nob_sb_append_cstr(&sb, "'");
        }
        nob_sb_append_cstr(&sb, "]");
        nob_sb_append_null(&sb);
        Branch b = {
            .text = (Clay_String) {false, sb.count-1, strdup(sb.items)},
            .name = name
        };
        nob_da_append(&branches, b);
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

void HandleIndexButtonInteraction(Clay_ElementId id, Clay_PointerData pointer_data, intptr_t user_data)
{
    (void) id;
    int index = (int) user_data;
    if (pointer_data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
        selected_branch = index;
    }
}

void HandleSceneButtonInteraction(Clay_ElementId id, Clay_PointerData pointer_data, intptr_t user_data)
{
    (void) id;
    if (pointer_data.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME){
        func_toggle_scene((void*) user_data);
    }
}

// button functions
void func_refresh(void)
{
    c_info = cson_read(info_path);
    if (c_info == NULL){
        eprintf("Could not find internal info file!\n");
        return;
    }
    c_branches = cson_get(c_info, key("branches"));
    if (!cson_is_map(c_branches)){
        eprintf("Could not find <branches> field in info file!");
        return;
    }
    (void) set_branches();
    iprintf("Refreshed!");
}

void func_branch_new(void)
{
    func_toggle_scene((void*) SCENE_INPUT);
}

void func_add_branch(void)
{
    if (new_branch_len > 0){
        iprintf("adding branch '%s'", new_branch_name);
        func_toggle_scene((void*) SCENE_MAIN);
    }
}

void func_branch_remove(void)
{
    if (selected_branch >= 0 && selected_branch < (int) branches.count){
        char *branch_name = (char*) branches.items[selected_branch].name;
        if (cson_map_remove(c_branches, cson_str(branch_name)) != CsonError_Success){
            eprintf("Could not remove brandch '%s'!", branch_name);
        }
        set_branches();
        cson_write(c_info, info_path);
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

void func_backup(void)
{
    printf("Backup!\n");
}

void func_merge(void)
{
    printf("Merge!\n");
}

// render functions

void text_layout(Clay_String string, uint16_t font_id, uint16_t font_size, uint16_t spacing)
{
    CLAY_TEXT(string, CLAY_TEXT_CONFIG({
        .textColor = window_theme.text,
        .fontId = font_id,
        .fontSize = font_size,
        .letterSpacing = spacing
    }));
}

void settings_menu_layout(void)
{
    CLAY({
        .backgroundColor = {255, 255, 255, 51},
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
                .backgroundColor = window_theme.secondary,
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
                        .textColor = window_theme.text,
                        .fontId = DEFAULT, 
                        .fontSize = 12,
                        .letterSpacing = 2
                    }));
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? darken_color(window_theme.danger) : window_theme.danger,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    },
                }){
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                        },
                        .image = {&textures[SYMBOL_EXIT_16]}
                    }){
                        Clay_OnHover(HandleSceneButtonInteraction, (intptr_t) SCENE_MAIN);
                    }
                }
            }
            CLAY({
                .backgroundColor = window_theme.background,
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
        .backgroundColor = {255, 255, 255, 51},
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
                .backgroundColor = window_theme.secondary,
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
                    .backgroundColor = Clay_Hovered()? darken_color(window_theme.danger) : window_theme.danger,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    },
                }){
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                        },
                        .image = {&textures[SYMBOL_EXIT_16]}
                    }){
                        Clay_OnHover(HandleSceneButtonInteraction, (intptr_t) SCENE_MAIN);
                    }
                }
            }
            CLAY({
                .backgroundColor = window_theme.background,
                .layout = {
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    .childGap = 4
                }
            }){
                CLAY({
                    
                }){
                    snprintf(temp_buffer, sizeof(temp_buffer), "(%d/%d)", new_branch_len, NEW_BRANCH_MAX_LEN);
                    text_layout(CLAY_STRING("Insert the name for the branch: "), DEFAULT, 12, 1);
                    text_layout(clay_string(temp_buffer), DEFAULT, 12, 1);
                }
                CLAY({
                    .backgroundColor = window_theme.secondary,
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIXED(256), .height=CLAY_SIZING_FIXED(20)},
                        .padding = CLAY_PADDING_ALL(4),
                        .childAlignment = {.y=CLAY_ALIGN_Y_CENTER},
                    },
                    .border = {
                        .color = darken_color(window_theme.secondary),
                        .width = CLAY_BORDER_OUTSIDE(2)
                    },
                }){
                    if (Clay_Hovered()){
                        SetMouseCursor(MOUSE_CURSOR_IBEAM);

                        int key = GetCharPressed();

                        while (key > 0){
                            if ((key >= 32) && (key <= 125) && (new_branch_len < NEW_BRANCH_MAX_LEN)){
                                new_branch_name[new_branch_len] = (char)key;
                                new_branch_name[new_branch_len+1] = '\0'; 
                                new_branch_len++;
                            }
                            key = GetCharPressed(); 
                        }
                        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)){
                            new_branch_len--;
                            if (new_branch_len < 0) new_branch_len = 0;
                            new_branch_name[new_branch_len] = '\0';
                        }
                        frame_counter++;
                    }else{
                        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
                    }
                    text_layout((Clay_String){false, new_branch_len, new_branch_name}, MONO_12, 12, 1);
                    if (Clay_Hovered() && ((frame_counter/20)%2) == 0){
                        text_layout(CLAY_STRING("_"), MONO_12, 12, 1);
                    }
                }
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW()},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                    }
                }){
                    CLAY({
                        .backgroundColor = Clay_Hovered()? window_theme.hover : window_theme.accent,
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

void about_menu_layout(void)
{
    CLAY({
        .backgroundColor = {255, 255, 255, 51},
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
                .backgroundColor = window_theme.secondary,
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
                        .textColor = window_theme.text,
                        .fontId = DEFAULT, 
                        .fontSize = 12,
                        .letterSpacing = 2
                    }));
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? darken_color(window_theme.danger) : window_theme.danger,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                    },
                }){
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)}
                        },
                        .image = {&textures[SYMBOL_EXIT_16]}
                    }){
                        Clay_OnHover(HandleSceneButtonInteraction, (intptr_t) SCENE_MAIN);
                    }
                }
            }
            CLAY({
                .backgroundColor = window_theme.background,
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
                    text_layout(clay_string(version), MONO_16, 16, 0);
                }
                CLAY({
                    .layout = {
                        .sizing = {.width=CLAY_SIZING_GROW(0, 312), .height=CLAY_SIZING_FIXED(128)},
                        .padding = CLAY_PADDING_ALL(4)
                    },
                    .border = {
                        .color = window_theme.hover,
                        .width = CLAY_BORDER_OUTSIDE(2)
                    }
                }){
                    CLAY({
                        .backgroundColor = window_theme.background,
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
                                .textColor = window_theme.text,
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
            .textColor = window_theme.text,
            .fontId = DEFAULT,
            .fontSize = 16,
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
        .backgroundColor = Clay_Hovered()? window_theme.hover : window_theme.accent,
        .layout = {
            .padding = CLAY_PADDING_ALL(8),
        },
    }){
        CLAY_TEXT(string, CLAY_TEXT_CONFIG({
            .textColor = window_theme.text,
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
        .backgroundColor = Clay_Hovered()? window_theme.hover : window_theme.secondary,
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW()},
            .padding = {.left=8, .right=8, .top=4, .bottom=4},
        },
    }){
        CLAY_TEXT(string, CLAY_TEXT_CONFIG({
            .textColor = window_theme.text,
            .fontId = DEFAULT,
            .fontSize = 12,
            .letterSpacing = 2
        }));
        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t)data);
    }
}

// Layouts

Clay_RenderCommandArray test_layout(){
    Clay_BeginLayout();
    {
        CLAY({
            .backgroundColor = window_theme.background,
            .layout = {
                .sizing = { .height = CLAY_SIZING_GROW(), .width = CLAY_SIZING_GROW()},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
            }
        }){
            CLAY_TEXT(CLAY_STRING("TODO: Create GUI"), CLAY_TEXT_CONFIG({
                .textColor = window_theme.text,
                .fontId = DEFAULT,
                .fontSize = 64
            }));
            CLAY({
                .backgroundColor = Clay_Hovered()? window_theme.hover : window_theme.accent,
                .layout={.padding = CLAY_PADDING_ALL(20)},
                .border = {
                    .color = window_theme.border,
                    .width = CLAY_BORDER_OUTSIDE(4)
                },
                .cornerRadius = CLAY_CORNER_RADIUS(4)
            }){
                if (Clay_Hovered()){
                    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                }else{
                    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
                }
                CLAY_TEXT(CLAY_STRING("Click me!"), CLAY_TEXT_CONFIG({
                    .textColor = window_theme.text,
                    .fontId = DEFAULT,
                    .fontSize = 32
                }));
            }
        }
    }
    return Clay_EndLayout();
}

Clay_RenderCommandArray main_layout()
{
    Clay_BeginLayout();
    {
        CLAY({
            .backgroundColor = window_theme.background,
            .layout = {
                .sizing = {.height=CLAY_SIZING_GROW(), .width=CLAY_SIZING_GROW()},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }){
            if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            CLAY({
                .id = CLAY_ID("task_bar"),
                .backgroundColor = window_theme.secondary,
                .layout = {
                    .sizing = {.width=CLAY_SIZING_GROW()},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .padding = CLAY_PADDING_ALL(4),
                }
            }){
                CLAY({
                    .id = CLAY_ID("task_file_button"),
                    .backgroundColor = Clay_Hovered()? window_theme.hover : NO_COLOR,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                    },
                }){
                    CLAY_TEXT(CLAY_STRING("File"), CLAY_TEXT_CONFIG({
                        .textColor = window_theme.text,
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
                                .color = darken_color(window_theme.secondary),
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
                    .backgroundColor = Clay_Hovered()? window_theme.hover : NO_COLOR,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                    },
                }){
                    CLAY_TEXT(CLAY_STRING("Settings"), CLAY_TEXT_CONFIG({
                        .textColor = window_theme.text,
                        .fontId = DEFAULT,
                        .fontSize = 16
                    }));
                    Clay_OnHover(HandleSceneButtonInteraction, (intptr_t) SCENE_SETTINGS);
                    if (current_scene == SCENE_SETTINGS){
                        settings_menu_layout();
                    }
                }
                CLAY({
                    .backgroundColor = Clay_Hovered()? window_theme.hover : NO_COLOR,
                    .layout = {
                        .padding = {.left=8, .right=8, .top=4, .bottom=4},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
                    }
                }){
                    CLAY_TEXT(CLAY_STRING("About"), CLAY_TEXT_CONFIG({
                        .textColor = window_theme.text,
                        .fontId = DEFAULT,
                        .fontSize = 16
                    }));
                    Clay_OnHover(HandleSceneButtonInteraction, (intptr_t) SCENE_ABOUT);
                    if (current_scene == SCENE_ABOUT){
                        about_menu_layout();
                    }
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
                                    .textColor = window_theme.text,
                                    .fontId = DEFAULT, 
                                    .fontSize = 16
                                }));
                            }
                            CLAY({
                                .backgroundColor = Clay_Hovered()? window_theme.hover : window_theme.accent,
                                .layout = {
                                    .padding = {.left=8, .right=8, .top=4, .bottom=4},
                                    .childGap = 8
                                }
                            }){
                                if (Clay_Hovered()) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                                Clay_OnHover(HandleFuncButtonInteraction, (intptr_t)func_refresh);
                                
                                CLAY_TEXT(CLAY_STRING("Refresh"), CLAY_TEXT_CONFIG({
                                    .textColor = window_theme.text,
                                    .fontId = DEFAULT, 
                                    .fontSize = 16,
                                }));
                                CLAY({
                                    .layout = {
                                        .sizing = {CLAY_SIZING_FIXED(16), CLAY_SIZING_FIXED(16)},
                                    },
                                    .image = {&textures[SYMBOL_REFRESH_16]}
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
                                .color = window_theme.hover,
                                .width = CLAY_BORDER_OUTSIDE(2)
                            },
                            .clip = { .vertical = true, .childOffset = Clay_GetScrollOffset() },
                        }){
                            for (size_t i=0; i<branches.count; ++i){
                                Branch branch = branches.items[i];
                                CLAY({
                                    .backgroundColor = selected_branch == (int)i? window_theme.accent : Clay_Hovered()? window_theme.secondary : NO_COLOR,
                                    .layout = {
                                        .sizing = {.width=CLAY_SIZING_GROW()},
                                        .padding = CLAY_PADDING_ALL(2),
                                    }
                                }){
                                    CLAY_TEXT(branch.text, CLAY_TEXT_CONFIG({
                                        .textColor = window_theme.text,
                                        .fontId = MONO_16, 
                                        .fontSize = 16,
                                        .letterSpacing = 1
                                    }));
                                    Clay_OnHover(HandleIndexButtonInteraction, (int) i);
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
                            action_button_layout(CLAY_STRING("Make Backup"), func_backup);
                            action_button_layout(CLAY_STRING("Merge Backup"), func_merge);
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
                        branch_action_button_layout(CLAY_STRING("New"), window_theme.accent, func_branch_new);
                        branch_action_button_layout(CLAY_STRING("Remove"), window_theme.danger, func_branch_remove);
                        if (current_scene == SCENE_INPUT){
                            input_menu_layout();
                        }
                    }
                }
            }   
        }
    }
    return Clay_EndLayout();
}

int main(void) {
    if (!setup()) return 1;
    
    int value = 0;    
    cwk_path_join(program_dir, "data/backups.json", info_path, sizeof(info_path));
    
    c_info = cson_read(info_path);
    if (c_info == NULL){
        error("Could not find internal info file!\n");
        return_defer(1);
    }
    
    Cson *c_version = cson_get(c_info, key("version"));
    if (!cson_is_string(c_version)){
        eprintf("Could not find <version> field in info file!");
        return_defer(1);
    }
    version = cson_get_cstring(c_version);
    
    c_branches = cson_get(c_info, key("branches"));
    if (!cson_is_map(c_branches)){
        error("Could not find <branches> field in info file!");
        return_defer(1);
    }
    if (!set_branches()) return_defer(1);
    
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
    textures[SYMBOL_EXIT_16] = LoadTexture(font_path);
    cwk_path_join(program_dir, "resources/MaterialIcons/refresh_16.png", font_path, sizeof(font_path));
    textures[SYMBOL_REFRESH_16] = LoadTexture(font_path);
    
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
    Clay_Raylib_Close();
    for (size_t i=0; i<_FontId_Count; ++i){
        UnloadFont(fonts[i]);
    }
    for (size_t i=0; i<_TextureId_Count; ++i){
        UnloadTexture(textures[i]);
    }
    cleanup();
    return value;
}