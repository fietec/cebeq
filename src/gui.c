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
} Theme;

const Theme charcoal_teal = {
    { 34,  43,  46,  255 },  // background
    { 217, 225, 232, 255 },  // text
    { 0,   191, 166, 255 },  // accent
    { 55,  64,  69,  255 },  // border
    { 2,   158, 136, 255 },  // hover
    { 124, 138, 148, 255 },  // secondary
};

Theme window_theme = charcoal_teal;

Clay_Color backgroundColor = {43, 41, 51, 255 };
Clay_Color whiteColor = {255, 255, 255, 255};

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
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
                    .id = CLAY_ID("branch_selector"),
                    .layout = {
                        .sizing = {.height=CLAY_SIZING_FIXED(412)},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .padding = CLAY_PADDING_ALL(4)
                    },
                    .border = {
                        .color = window_theme.hover,
                        .width = CLAY_BORDER_OUTSIDE(2)
                    }
                }){
                    for (size_t i=0; i<5; ++i){
                        CLAY({
                            .backgroundColor = Clay_Hovered()? window_theme.hover : NO_COLOR,
                            .layout = {
                                .sizing = {.width=CLAY_SIZING_FIT(732)},
                                .padding = CLAY_PADDING_ALL(2),
                            }
                        }){
                            CLAY_TEXT(CLAY_STRING("'content': ['c:/path/to/somewhere', 'd:/wow/somewhere/else']"), CLAY_TEXT_CONFIG({
                                .textColor = window_theme.text,
                                .fontId = BODY_16, 
                                .fontSize = 16
                            }));
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
    Clay_Raylib_Initialize(1024, 768, "Cebeq Gui", FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);

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