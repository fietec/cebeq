#ifndef THEME_H
#define THEME_H

#include <clay.h>

typedef struct {
    const char *name;
    Clay_Color background;
    Clay_Color text;
    Clay_Color accent;
    Clay_Color border;
    Clay_Color hover;
    Clay_Color secondary;
    Clay_Color danger;
    Clay_Color success;
    Clay_Color blur;
} Theme;

const Theme themes[] = {
    {
        "charcoal_teal",         // dark charcoal with teal accents
        { 34,  43,  46,  255 },  // background
        { 217, 225, 232, 255 },  // text
        { 0,   191, 166, 255 },  // accent
        { 55,  64,  69,  255 },  // border
        { 2,   158, 136, 255 },  // hover
        { 124, 138, 148, 255 },  // secondary
        { 220,  53,  69,  255 }, // danger 
        { 40,  167,  69,  255 }, // success
        { 255, 255, 255,  51 },  // blur (light overlay for dark bg)
    },
    {
        "light_minimal",         // bright minimal light theme
        { 250, 250, 250, 255 },  // background
        { 33,  37,  41,  255 },  // text
        { 0,  123, 255, 255 },   // accent (blue)
        { 222, 226, 230, 255 },  // border
        { 0,  105, 217, 255 },   // hover
        { 108, 117, 125, 255 },  // secondary (gray)
        { 220,  53,  69,  255 }, // danger (red)
        { 40,  167,  69,  255 }, // success (green)
        { 0,   0,   0,   51 },   // blur (dark overlay for light bg)
    },
    {
        "dark_neutral",          // soft neutral dark mode
        { 24,  26,  27,  255 },  // background
        { 236, 239, 241, 255 },  // text
        { 100, 181, 246, 255 },  // accent (light blue)
        { 38,  40,  41,  255 },  // border
        { 66,  165, 245, 255 },  // hover
        { 144, 164, 174, 255 },  // secondary (gray-blue)
        { 229,  57,  53,  255 }, // danger
        { 76,  175,  80,  255 }, // success
        { 255, 255, 255,  51 },  // blur
    },
    {
        "soft_pastel",           // subtle pastel accent theme
        { 245, 245, 250, 255 },  // background
        { 50,  50,  60,  255 },  // text
        { 244, 143, 177, 255 },  // accent (pink)
        { 220, 220, 230, 255 },  // border
        { 236,  64, 122, 255 },  // hover
        { 128, 128, 160, 255 },  // secondary
        { 239,  83,  80,  255 }, // danger
        { 102, 187, 106, 255 },  // success
        { 0,   0,   0,   51 },   // blur
    },
    {
        "midnight_blue",         // deep navy dark theme
        { 15,  23,  42,  255 },  // background
        { 226, 232, 240, 255 },  // text
        { 59,  130, 246, 255 },  // accent (blue)
        { 30,  41,  59,  255 },  // border
        { 37,  99,  235, 255 },  // hover
        { 148, 163, 184, 255 },  // secondary (cool gray)
        { 239,  68,  68,  255 }, // danger
        { 34,  197,  94,  255 }, // success
        { 255, 255, 255,  51 },  // blur
    },
    {
        "sandstone",             // warm neutral light theme
        { 250, 247, 242, 255 },  // background (sand)
        { 45,  45,  45,  255 },  // text
        { 210, 105,  30, 255 },  // accent (burnt orange)
        { 230, 225, 215, 255 },  // border
        { 200, 85,  20,  255 },  // hover
        { 125, 115, 105, 255 },  // secondary
        { 200,  50,  50, 255 },  // danger
        { 60,  150,  80, 255 },  // success
        { 0,   0,   0,   51 },   // blur
    },
    {
        "forest_dark",           // deep green forest dark
        { 20,  30,  25,  255 },  // background
        { 220, 230, 220, 255 },  // text
        { 34,  197,  94,  255 }, // accent (green)
        { 35,  55,  45,  255 },  // border
        { 22,  163,  74,  255 }, // hover
        { 134, 153, 134, 255 },  // secondary (muted gray-green)
        { 229,  57,  53,  255 }, // danger
        { 46,  204, 113, 255 },  // success
        { 255, 255, 255,  51 },  // blur
    },
    {
        "solar_breeze",          // fresh light teal theme
        { 245, 250, 250, 255 },  // background
        { 30,  50,  60,  255 },  // text
        { 0,  184, 148, 255 },   // accent (teal)
        { 220, 230, 230, 255 },  // border
        { 0,  168, 130, 255 },   // hover
        { 120, 130, 135, 255 },  // secondary
        { 231,  76,  60, 255 },  // danger
        { 46,  204, 113, 255 },  // success
        { 0,   0,   0,   51 },   // blur
    },
    {
        "plum_mist",             // soft purple pastel
        { 248, 246, 252, 255 },  // background
        { 50,  40,  70,  255 },  // text
        { 155, 89, 182, 255 },   // accent (purple)
        { 225, 220, 235, 255 },  // border
        { 142, 68, 173, 255 },   // hover
        { 120, 110, 130, 255 },  // secondary
        { 231, 76,  60,  255 },  // danger
        { 39,  174, 96,  255 },  // success
        { 0,   0,   0,   51 },   // blur
    },
    {
        "cyberpunk",             // neon cyberpunk dark
        { 10,  10,  20,  255 },  // background
        { 245, 245, 255, 255 },  // text
        { 255, 0,  204, 255 },   // accent (magenta neon)
        { 30,  30,  50,  255 },  // border
        { 0,  255,  204, 255 },  // hover (aqua neon)
        { 140, 140, 160, 255 },  // secondary
        { 255,  80,  80,  255 }, // danger
        { 0,   255,  120, 255 }, // success
        { 255, 255, 255,  51 },  // blur
    },
};

#define themes_count sizeof(themes)/sizeof(Theme)

#endif // THEME_H
