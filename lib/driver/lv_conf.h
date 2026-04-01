#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 24
#define LV_COLOR_16_SWAP 0

#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64U * 1024U)

/* Display settings */
#define LV_DPI_DEF 100

/* Feature usage */
#define LV_USE_REFR_DEBUG 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/* Compiler settings */
#define LV_BIG_ENDIAN_SYSTEM 0

/* HAL settings */
#define LV_TICK_CUSTOM 1

/* Log settings - VERBOSE for debugging */
#define LV_USE_LOG 1
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF 1
#endif

/* Layout */
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/* Widgets */
#define LV_USE_LABEL 1
#define LV_USE_BTN 1
#define LV_USE_IMG 1
#define LV_USE_LINE 1
#define LV_USE_CHECKBOX 1

/* Themes */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 0

#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1

#define LV_FONT_MONTSERRAT_12_SUBPX 0
#define LV_FONT_MONTSERRAT_14_SUBPX 0
#define LV_FONT_MONTSERRAT_16_SUBPX 0
#define LV_FONT_MONTSERRAT_18_SUBPX 0
#define LV_FONT_MONTSERRAT_20_SUBPX 0

#define LV_FONT_UNSCII_8 1
#define LV_FONT_UNSCII_16 0

#define LV_FONT_DEFAULT &lv_font_montserrat_16

/* Text settings */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

#endif 
