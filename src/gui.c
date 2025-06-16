#include <stdio.h>

#include <cebeq.h>
#include <cwalk.h>

#include <raylib.h>
#define CLAY_IMPLEMENTATION
#include <clay.h>
#include <clay_renderer_raylib.c>

#define NO_COLOR ((Clay_Color) {0})

typedef enum{
    DEFAULT,
    BODY_16,
    _FontId_Count
} FontIds;

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

const Theme charcoal_teal = {
    { 34,  43,  46,  255 },  // background
    { 217, 225, 232, 255 },  // text
    { 0,   191, 166, 255 },  // accent
    { 55,  64,  69,  255 },  // border
    { 2,   158, 136, 255 },  // hover
    { 124, 138, 148, 255 },  // secondary
    { 220,  53,  69,  255 }, // danger (muted crimson)
    { 40,  167,  69,  255 }, // success (teal green)
};

Theme window_theme = charcoal_teal;

typedef struct{
    Clay_String text;   
} Branch;

typedef struct{
    Branch *items;
    size_t size;
    size_t capacity;
} Branches;

Branch test_branches[] = {
    (Branch) {.text=CLAY_STRING("'branch1': ['d:/some/path', 'c:/path/to/somewhere/else']")},
    (Branch) {.text=CLAY_STRING("'branch2': ['d:/some/path', 'c:/path/to/somewhere/else']")},
    (Branch) {.text=CLAY_STRING("'branch3': ['d:/some/path', 'c:/path/to/somewhere/else']")},
    (Branch) {.text=CLAY_STRING("'branch4': ['d:/some/path', 'c:/path/to/somewhere/else']")},
    (Branch) {.text=CLAY_STRING("'branch5': ['d:/some/path', 'c:/path/to/somewhere/else']")},
};

//static Branches branches = {0};
static Branches branches = {
    .items = test_branches,
    .size = 5,
    .capacity = 10
};

static int selected_branch = -1;

Clay_Color backgroundColor = {43, 41, 51, 255 };
Clay_Color whiteColor = {255, 255, 255, 255};

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
    if (pointer_data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME){
        if (button_data != NULL) button_data();
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

// button functions
void func_branch_new(void)
{
    printf("Add!\n");
}

void func_branch_remove(void)
{
    printf("Remove!\n");
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
void render_branch_action_button(Clay_String string, Clay_Color color, FuncButtonData data)
{
    CLAY({
        .backgroundColor = Clay_Hovered()? darken_color(color) : color,
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW()},
            .padding = {.left=8, .right=8, .top=4, .bottom=4},
            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}
        },
    }){
        CLAY_TEXT(string, CLAY_TEXT_CONFIG({
            .textColor = window_theme.text,
            .fontId = DEFAULT,
            .fontSize = 16
        }));
        if (Clay_Hovered()){
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) data);
    }
}

void render_action_button(Clay_String string, FuncButtonData data)
{
    CLAY({
        .backgroundColor = Clay_Hovered()? darken_color(window_theme.accent) : window_theme.accent,
        .layout = {
            .padding = CLAY_PADDING_ALL(4)
        }
    }){
        CLAY_TEXT(string, CLAY_TEXT_CONFIG({
            .textColor = window_theme.text,
            .fontId = DEFAULT, 
            .fontSize = 18
        }));        
        if (Clay_Hovered()){
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        Clay_OnHover(HandleFuncButtonInteraction, (intptr_t) data);
    }
}

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
                    .sizing = {.height=CLAY_SIZING_FIT(), .width=CLAY_SIZING_GROW()},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .padding = CLAY_PADDING_ALL(4),
                }
            }){
                CLAY({
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
                CLAY_TEXT(CLAY_STRING("Select a branch:"), CLAY_TEXT_CONFIG({
                    .textColor = window_theme.text,
                    .fontId = DEFAULT, 
                    .fontSize = 16
                }));
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
                            .id = CLAY_ID("branch_selector"),
                            .layout = {
                                .sizing = {.height=CLAY_SIZING_GROW(), .width=CLAY_SIZING_GROW()},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                .padding = CLAY_PADDING_ALL(4)
                            },
                            .border = {
                                .color = window_theme.hover,
                                .width = CLAY_BORDER_OUTSIDE(2)
                            }
                        }){
                            for (size_t i=0; i<branches.size; ++i){
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
                                        .fontId = BODY_16, 
                                        .fontSize = 16
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
                            render_action_button(CLAY_STRING("Make Backup"), func_backup);
                            render_action_button(CLAY_STRING("Merge Backup"), func_merge);
                        }
                    }
                    CLAY({
                        .id = CLAY_ID("branch_action_frame"),
                        .layout = {
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
                            .childGap = 4
                        }
                    }){
                        render_branch_action_button(CLAY_STRING("New"), window_theme.accent, func_branch_new);
                        render_branch_action_button(CLAY_STRING("Remove"), window_theme.danger, func_branch_remove);
                    }
                }
            }            
        }
    }
    return Clay_EndLayout();
}

int main(void) {
    if (!setup()) return 1;
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
    cwk_path_join(program_dir, "resources/Roboto-Regular.ttf", font_path, sizeof(font_path));
    fonts[BODY_16] = LoadFontEx(font_path, 16, NULL, 400);
    SetTextureFilter(fonts[BODY_16].texture, TEXTURE_FILTER_BILINEAR);
    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);

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
    Clay_Raylib_Close();
    cleanup();
}