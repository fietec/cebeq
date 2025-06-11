#include <stdio.h>
#include <cebeq.h>
#include <raylib.h>
#define CLAY_IMPLEMENTATION
#include <clay.h>
#include <clay_renderer_raylib.c>

typedef enum{
    BODY_16,
    _FontId_Count
} FontIds;

Clay_Color backgroundColor = {43, 41, 51, 255 };
Clay_Color whiteColor = {255, 255, 255, 255};

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

Clay_RenderCommandArray create_layout(){
    Clay_BeginLayout();
    {
        CLAY({
            .backgroundColor = backgroundColor,
            .layout = {
                .sizing = { .height = CLAY_SIZING_GROW(), .width = CLAY_SIZING_GROW()},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            }
        }){
            CLAY_TEXT(CLAY_STRING("Hello World"), CLAY_TEXT_CONFIG({
                .textColor = whiteColor,
                .fontId = BODY_16,
                .fontSize = 16
            }));
        }
    }
    return Clay_EndLayout();
}

int main(void) {
    if (!setup()) return 1;
    Clay_Raylib_Initialize(1024, 768, "Clay Testing", FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);

    uint64_t clayRequiredMemory = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(clayRequiredMemory, malloc(clayRequiredMemory));
    Clay_Initialize(clayMemory, (Clay_Dimensions) {
       .width = GetScreenWidth(),
       .height = GetScreenHeight()
    }, (Clay_ErrorHandler) { HandleClayErrors, NULL}); 
    Font fonts[_FontId_Count];
    fonts[BODY_16] = LoadFontEx("resources/Roboto-Regular.ttf", 16, NULL, 400);
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

        Clay_RenderCommandArray renderCommands = create_layout();
        
        BeginDrawing();
        Clay_Raylib_Render(renderCommands, fonts);
        EndDrawing();
    }
    Clay_Raylib_Close();
    cleanup();
}