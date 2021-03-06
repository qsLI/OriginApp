/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/**
 * @file   tools.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 * @brief  配置文件读写
 *
 *
 */
#include "stdafx.h"

#include "tools.h"

#include <stdio.h>
#include <stdlib.h>
//szj #include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

#include "ui.h"
#include "MainWindow.h"
#include "InputWindow.h"
#include "PYFA.h"
#include "py.h"
#include "sp.h"
#include "ime.h"
#include "KeyList.h"

extern Display *dpy;
extern int      iScreen;
extern int      MAINWND_WIDTH;
extern int      iMainWindowX;
extern int      iMainWindowY;
extern int      iInputWindowX;
extern int      iInputWindowY;
extern uint      iInputWindowWidth;
extern uint      iInputWindowHeight;

extern int      iMaxCandWord;
extern Bool     _3DEffectMainWindow;
extern _3D_EFFECT _3DEffectInputWindow;
extern WINDOW_COLOR inputWindowColor;
extern WINDOW_COLOR mainWindowColor;
extern MESSAGE_COLOR IMNameColor[];
extern MESSAGE_COLOR messageColor[];
extern MESSAGE_COLOR inputWindowLineColor;
extern MESSAGE_COLOR mainWindowLineColor;
extern MESSAGE_COLOR cursorColor;
extern WINDOW_COLOR VKWindowColor;
extern MESSAGE_COLOR VKWindowFontColor;
extern MESSAGE_COLOR VKWindowAlphaColor;
extern ENTER_TO_DO enterToDo;

extern HOTKEYS  hkTrigger[];
extern HOTKEYS  hkGBK[];
extern HOTKEYS  hkCorner[];
extern HOTKEYS  hkPunc[];
extern HOTKEYS  hkPrevPage[];
extern HOTKEYS  hkNextPage[];
extern HOTKEYS  hkWBAddPhrase[];
extern HOTKEYS  hkWBDelPhrase[];
extern HOTKEYS  hkWBAdjustOrder[];
extern HOTKEYS  hkPYAddFreq[];
extern HOTKEYS  hkPYDelFreq[];
extern HOTKEYS  hkPYDelUserPhr[];
extern HOTKEYS  hkLegend[];
extern HOTKEYS  hkTrack[];
extern HOTKEYS  hkGetPY[];
extern HOTKEYS  hkGBT[];
extern HOTKEYS    hkHideMainWindow[];
extern HOTKEYS    hkSaveAll[];
extern HOTKEYS    hkVK[];

extern KEY_CODE switchKey;
extern XIMTriggerKey *Trigger_Keys;
extern INT8     iTriggerKeyCount;

extern Bool     bUseGBK;
extern Bool     bEngPuncAfterNumber;

//extern Bool     bAutoHideInputWindow;
extern XColor   colorArrow;
extern Bool     bTrackCursor;
extern Bool     bCenterInputWindow;
extern HIDE_MAINWINDOW hideMainWindow;
extern Bool     bCompactMainWindow;
extern HIDE_MAINWINDOW hideMainWindow;
extern int      iFontSize;
extern int      iMainWindowFontSize;

extern Bool     bUseGBKT;

extern Bool     bChnPunc;
extern Bool     bCorner;
extern Bool     bUseLegend;

extern Bool     bPYCreateAuto;
extern Bool     bPYSaveAutoAsPhrase;
extern Bool     bPhraseTips;
extern SEMICOLON_TO_DO semicolonToDo;
extern Bool     bEngAfterCap;

//显示打字速度
extern Bool     bShowUserSpeed;
extern Bool     bShowVersion;
extern Bool     bShowVK;

extern char     strNameOfPinyin[];
extern char     strNameOfShuangpin[];;
extern char     strNameOfQuwei[];

extern Bool     bFullPY;
extern Bool     bDisablePagingInLegend;

extern int      i2ndSelectKey;
extern int      i3rdSelectKey;

extern char     strFontName[];
extern char     strFontEnName[];

extern ADJUSTORDER baseOrder;
extern ADJUSTORDER phraseOrder;
extern ADJUSTORDER freqOrder;

extern INT8     iIMIndex;
extern Bool     bLocked;

extern MHPY     MHPY_C[];
extern MHPY     MHPY_S[];

extern Bool     bUsePinyin;
extern Bool     bUseSP;
extern Bool     bUseQW;
extern Bool     bUseTable;

extern char     strDefaultSP[];
extern SP_FROM  iSPFrom;

extern char     cPYYCDZ[];
extern char    strExternIM[];

extern Bool     bDoubleSwitchKey;
extern Bool     bPointAfterNumber;
extern Bool     bConvertPunc;
extern unsigned int iTimeInterval;
extern uint     iFixedInputWindowWidth;
extern Bool     bShowInputWindowTriggering;

#ifdef _USE_XFT
extern Bool     bUseAA;
#endif
extern char     strUserLocale[];

extern Bool     bUseBold;

extern int      iOffsetX;
extern int      iOffsetY;

#ifdef _ENABLE_TRAY
extern Bool    bUseTrayIcon;
#endif

extern char  respath[];
#define inline

Bool MyStrcmp (const char *str1, const char *str2)
{
    return !strncmp (str1, str2, strlen (str2));
}

/* 其他函数需要知道传递给 LoadConfig 的参数 */
Bool    bIsReloadConfig = True;
/* 在载入 profile 文件过程中传递状态信息 */
Bool    bIsNeedSaveConfig = True;

/*
 * 配置项值的类型：
 *
 * 整数(integer)、字符串(string)、颜色(color) 都可以用通用读写函数来读写。
 * 但是其他(other)类型，则需要提供专门的读写函数。
 */

#define CONFIG_INTEGER  1
#define CONFIG_STRING   2
#define CONFIG_COLOR    3
#define CONFIG_SWITCHKEY    4
#define CONFIG_HOTKEY   5
#define CONFIG_OTHER    6

/*
 * int(*configure_readwrite)(Configure *c, void *str_file, int isread)
 *
 * 用来读取或者写入对应的配置项
 *
 * c        -   读取/写入的配置项
 * str_file - 如果是读取，则为 char *；如果是写入，则为 FILE *
 * isread   - 如果是读取，则为 True，否则为 False
 *
 * configure_readwrite 返回零表示成功，其他值为失败。
 */

typedef struct Configure Configure;
typedef int(*config_readwrite)(Configure *, void *, int);

struct Configure {
    const char *name;         /* configure name */
    const char *comment;      /* configure comment */
    config_readwrite config_rw; /* read/write configure */
    int value_type;     /* type of this configure's value */
    union {
        struct {
            char *string;
            int string_length;
        } str_value;
        int *integer;
        XColor *color;
        HOTKEYS *hotkey;
    } value;
};

typedef struct Configure_group {
    const char *name;     /* configure group's name */
    const char *comment;  /* configure group's comment */
    struct Configure *configure;    /* configures belong to this group */
} Configure_group;

static int generic_config_integer(Configure *c, void *a, int isread)
 {
    if(isread)
        *(c->value.integer) = atoi((const char *)a);
    else
        fprintf((FILE *)a, "%s=%d\n", c->name, *(c->value.integer));

    return 0;
}

static int generic_config_string(Configure *c, void *a, int isread)
{
    if(isread){
        strncpy(c->value.str_value.string, (char *)a, c->value.str_value.string_length);
        c->value.str_value.string[c->value.str_value.string_length - 1] = '\0';
    } else
        fprintf((FILE *)a, "%s=%s\n", c->name, c->value.str_value.string);

    return 0;
}

static int generic_config_color(Configure *c, void *a, int isread)
{
    int r, g, b;

    if(isread){
        if(sscanf((char *)a, "%d %d %d", &r, &g, &b) != 3){
            fprintf(stderr, "error: configure file: color\n");
            exit(1);
        }
        c->value.color->red   = r << 8;
        c->value.color->green = g << 8;
        c->value.color->blue  = b << 8;
    }else
        fprintf((FILE *)a, "%s=%d %d %d\n", c->name,
                c->value.color->red   >> 8,
                c->value.color->green >> 8,
                c->value.color->blue  >> 8);

    return 0;
}

/* FIXME: 实现通用读写设置 switch key 的配置 */
#if 0
static int generic_config_switchkey(Configure *c, void *a, int isread)
{
    return -1;
}
#endif

/* FIXME: 实现通用读写设置 hot key 的配置 */
#if 0
static int generic_config_hotkey(Configure *c, void *a, int isread)
{
    return -1;
}
#endif

/** 将 configures 中的配置信息写入 fp */
#if 0
static int write_configures(FILE *fp, Configure *configures)
{
    Configure *tc;

    for(tc = configures; tc->name; tc++){
        if(tc->comment)
            fprintf(fp, "# %s\n", tc->comment);
        if(tc->config_rw)
            tc->config_rw(tc, fp, 0);
        else{
            switch(tc->value_type){
                case CONFIG_INTEGER:
                    generic_config_integer(tc, fp, 0);
                    break;
                case CONFIG_STRING:
                    generic_config_string(tc, fp, 0);
                    break;
                case CONFIG_COLOR:
                    generic_config_color(tc, fp, 0);
                    break;
                default:
                    fprintf(stderr, "error: shouldn't be here\n");
                    exit(1);
            }
        }
     }
    return 0;
}
#endif
/* 从 str 读取配置信息 */
static int read_configure(Configure *config, char *str)
{
    if(config->config_rw)
        config->config_rw(config, str, 1);
    else{
        switch(config->value_type){
            case CONFIG_INTEGER:
                generic_config_integer(config, str, 1);
                break;
            case CONFIG_STRING:
                generic_config_string(config, str, 1);
                break;
            case CONFIG_COLOR:
                generic_config_color(config, str, 1);
                break;
            default:
                fprintf(stderr, "error: shouldn't be here\n");
                exit(1);
        }
    }
    return 0;
}

/* 主窗口输入法名称色 */
inline static int main_window_input_method_name_color(Configure *c, void *a, int isread)
{
    int r[3], b[3], g[3], i;
    FILE *fp;

    if(isread){
        if(sscanf((char *)a, "%d %d %d %d %d %d %d %d %d",
                    &r[0], &g[0], &b[0], &r[1], &g[1], &b[1], &r[2], &g[2], &b[2]) != 9)
        {
            fprintf(stderr, "error: invalid configure format\n");
            exit(1);
        }

        for(i = 0; i < 3; i++){
            IMNameColor[i].color.red   = r[i] << 8;
            IMNameColor[i].color.green = g[i] << 8;
            IMNameColor[i].color.blue  = b[i] << 8;
        }
    }else{
        fp = (FILE *)a;
        fprintf(fp, "%s=", c->name);
        for(i = 0; i < 3; i++)
            fprintf(fp, "%d %d %d ",
                    IMNameColor[i].color.red   >> 8,
                    IMNameColor[i].color.green >> 8,
                    IMNameColor[i].color.blue  >> 8);
        fprintf(fp, "\n");
    }

    return 0;
}

/* 打开/关闭输入法 */
inline static int trigger_input_method(Configure *c, void *a, int isread)
{
    if(isread){
        if(bIsReloadConfig){
            SetTriggerKeys((char *)a);
            SetHotKey((char *)a, hkTrigger);
        }
    }else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_SPACE");

    return 0;
}

/* 中英文快速切换键 */
inline static int fast_chinese_english_switch(Configure *c, void *a, int isread)
{
    if(isread)
        SetSwitchKey((char *)a);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "L_SHIFT");

    return 0;
}

/* 光标跟随 */
inline static int cursor_follow(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkTrack);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_K");
    return 0;
}

/* 隐藏主窗口 */
inline static int hide_main_window(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkHideMainWindow);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_ALT_H");
    return 0;
}

/* 切换虚拟键盘 */
inline static int switch_vk(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkVK);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_ALT_K");

    return 0;
}

/* GBK支持 */
inline static int gbk_support(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkGBK);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_M");

    return 0;
}

/* GBK繁体切换键 */
inline static int gbk_traditional_simplified_switch(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkGBT);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_ALT_F");

    return 0;
}

/* 联想 */
inline static int association(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkLegend);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_L");

    return 0;
}

/* 反查拼音 */
inline static int lookup_pinyin(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkGetPY);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_ALT_E");

    return 0;
}

/*
 * DBC case = Double Byte Character case
 * SBC case = Single Byte Character case
 */

/* 全半角 */
inline static int sbc_dbc_switch(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkCorner);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "SHIFT_SPACE");

    return 0;
}

/* 中文标点 */
inline static int chinese_punctuation(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkPunc);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "ALT_SPACE");

    return 0;
}

/* 上一页 */
inline static int prev_page(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkPrevPage);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "-");

    return 0;
}

/* 下一页 */
inline static int next_page(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkNextPage);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "=");

    return 0;
}

/* 第二三候选词选择键 */
inline static int second_third_candidate_word(Configure *c, void *a, int isread)
{
//    char *pstr = (char *)a;
#if 0 //szj
    if(isread){
        if (!strcasecmp (pstr, "SHIFT")) {
            i2ndSelectKey = 50;        //左SHIFT的扫描码
            i3rdSelectKey = 62;        //右SHIFT的扫描码
        }
        else if (!strcasecmp (pstr, "CTRL")) {
            i2ndSelectKey = 37;        //左CTRL的扫描码
            i3rdSelectKey = 109;       //右CTRL的扫描码
        }
        else {
            i2ndSelectKey = pstr[0] ^ 0xFF;
            i3rdSelectKey = pstr[1] ^ 0xFF;
        }
    }else
        fprintf((FILE *)a, "%s=%s\n", c->name, "0");
#endif
    return 0;
}

/* 保存词库 */
inline static int save_all(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkSaveAll);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_ALT_S");

    return 0;
}

/* 默认双拼方案 */
inline static int default_shuangpin_scheme(Configure *c, void *a, int isread)
{
    if(isread){
        strncpy(strDefaultSP, (char *)a, 100);  /* FIXME: 不应在此硬编码字符串长度，下同 */
        iSPFrom = SP_FROM_SYSTEM_CONFIG;
    }
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, strDefaultSP);

    return 0;
}

/* 增加拼音常用字 */
inline static int add_pinyin_frequently_used_word(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkPYAddFreq);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_8");

    return 0;
}

/* 删除拼音用户词组 */
inline static int delete_pinyin_user_create_phrase(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkPYDelUserPhr);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_DELETE");

    return 0;
}

/* 删除拼音常用字 */
inline static int delete_pinyin_frequently_used_word(Configure *c, void *a, int isread)
{
    if(isread)
        SetHotKey((char *)a, hkPYDelFreq);
    else
        fprintf((FILE *)a, "%s=%s\n", c->name, "CTRL_7");

    return 0;
}

/* 拼音以词定字键 */
inline static int pinyin_get_word_from_phrase(Configure *c, void *a, int isread)
{
    char *pstr = (char *)a;
    if(isread){
        cPYYCDZ[0] = pstr[0];
        cPYYCDZ[1] = pstr[1];
    }else
        fprintf((FILE *)a, "%s=%c%c\n", c->name, cPYYCDZ[0], cPYYCDZ[1]);

    return 0;
}

/* 模糊an和ang */
inline static int blur_an_ang(Configure *c, void *a, int isread)
{
    if(isread){
        MHPY_C[0].bMode = MHPY_S[5].bMode = atoi((const char *)a);
    }else
        fprintf((FILE *)a, "%s=%d\n", c->name, MHPY_C[0].bMode);

    return 0;
}
#if 1//szj

Configure program_config[100];
Configure output_config[100];

Configure interface_config[100];
Configure hotkey_config[100];
Configure input_method_config[100] ;
Configure pinyin_config[100];

Configure profiles[100];

int  initConfig()
{
    program_config[0].name            = "显示字体(中)";
    program_config[0].value_type    = CONFIG_STRING;
    program_config[0].value.str_value.string            = strFontName;
    program_config[0].value.str_value.string_length    = 100;

    program_config[1].name            = "显示字体(英)";
    program_config[1].value_type    = CONFIG_STRING;
    program_config[1].value.str_value.string            = strFontName;
    program_config[1].value.str_value.string_length    = 100;

    program_config[2].name            = "显示字体大小";
    program_config[2].value_type    = CONFIG_STRING;
    program_config[2].value.integer            = &iFontSize;


    program_config[3].name            = "主窗口字体大小";
    program_config[3].value_type    = CONFIG_STRING;
    program_config[3].value.integer            = &iMainWindowFontSize;

    program_config[4].name            = "字体区域";
    program_config[4].value_type    = CONFIG_STRING;
    program_config[4].value.str_value.string            = strUserLocale;
    program_config[4].value.str_value.string_length    = 50;

//#ifdef _USE_XFT
//    program_config[5].name            = "使用AA字体";
//    program_config[5].value_type    = CONFIG_INTEGER;
//    program_config[5].value.integer            =  &bUseAA;

    program_config[5].name            = "使用粗体";
    program_config[5].value_type    = CONFIG_INTEGER;
    program_config[5].value.integer            =  &bUseBold;

    program_config[6].name = NULL;

// #ifdef _ENABLE_TRAY

//    program_config[7].name            = "使用托盘图标";
//    program_config[7].value_type    = CONFIG_INTEGER;
//    program_config[7].value.integer            =  &bUseTrayIcon;


    output_config[0].name = "数字后跟半角符号";
    output_config[0].value_type = CONFIG_INTEGER;
    output_config[0].value.integer = &bEngPuncAfterNumber;

    output_config[1].name = "Enter键行为";
    output_config[1].value_type = CONFIG_INTEGER;
    output_config[1].value.integer = (int*)&enterToDo; /* FIXME: 这种转换方式也许并不是个好主意，下同 */

    output_config[2].name = "分号键行为";
    output_config[2].value_type = CONFIG_INTEGER;
    output_config[2].value.integer = (int*)&semicolonToDo;

    output_config[3].name = "大写字母输入英文";
    output_config[3].value_type = CONFIG_INTEGER;
    output_config[3].value.integer = (int*)&bEngAfterCap;

    output_config[4].name = "转换英文中的标点";
    output_config[4].value_type = CONFIG_INTEGER;
    output_config[4].value.integer = (int*)&bConvertPunc;

    output_config[5].name = "联想方式禁止翻页";
    output_config[5].value_type = CONFIG_INTEGER;
    output_config[5].value.integer =(int*)&bDisablePagingInLegend;
    output_config[6].name = NULL;



    interface_config[0].name = "候选词个数";
    interface_config[0].value_type = CONFIG_INTEGER;
    interface_config[0].value.integer = &iMaxCandWord;

    interface_config[1].name = "主窗口使用3D界面";
    interface_config[1].value_type = CONFIG_INTEGER;
    interface_config[1].value.integer = &_3DEffectMainWindow;

    interface_config[2].name = "输入条使用3D界面";
    interface_config[2].value_type = CONFIG_INTEGER;
    interface_config[2].value.integer = (int *)&_3DEffectInputWindow;

    interface_config[3].name = "主窗口隐藏模式";
    interface_config[3].value_type = CONFIG_INTEGER;
    interface_config[3].value.integer = (int *)&hideMainWindow;

    interface_config[4].name = "显示虚拟键盘";
    interface_config[4].value_type = CONFIG_INTEGER;
    interface_config[4].value.integer = &bShowVK;

    interface_config[5].name = "输入条居中";
    interface_config[5].value_type = CONFIG_INTEGER;
    interface_config[5].value.integer = &bCenterInputWindow;

    interface_config[6].name = "首次显示输入条";
    interface_config[6].value_type = CONFIG_INTEGER;
    interface_config[6].value.integer = &bShowInputWindowTriggering;

    interface_config[7].name = "输入条固定宽度";
    interface_config[7].comment = "输入条固定宽度(仅适用于码表输入法)，0表示不固定宽度";
    interface_config[7].value_type = CONFIG_INTEGER;
    interface_config[7].value.integer = (int *)&iFixedInputWindowWidth;

    interface_config[8].name = "输入条偏移量x";
    interface_config[8].value_type = CONFIG_INTEGER;
    interface_config[8].value.integer = &iOffsetX;

    interface_config[9].name = "输入条偏移量y";
    interface_config[9].value_type = CONFIG_INTEGER;
    interface_config[9].value.integer = &iOffsetY;

    interface_config[10].name = "序号后加点";
    interface_config[10].value_type = CONFIG_INTEGER;
    interface_config[10].value.integer = &bPointAfterNumber;

    interface_config[11].name = "显示打字速度";
    interface_config[11].value_type = CONFIG_INTEGER;
    interface_config[11].value.integer = &bShowUserSpeed;

    interface_config[12].name = "显示版本";
    interface_config[12].value_type = CONFIG_INTEGER;
    interface_config[12].value.integer = &bShowVersion;

    interface_config[13].name = "光标色";
    interface_config[13].value_type = CONFIG_COLOR;
    interface_config[13].value.color = &(cursorColor.color);

    interface_config[14].name = "主窗口背景色";
    interface_config[14].value_type = CONFIG_COLOR;
    interface_config[14].value.color = &(mainWindowColor.backColor);

    interface_config[15].name = "主窗口线条色";
    interface_config[15].value_type = CONFIG_COLOR;
    interface_config[15].value.color = &(mainWindowLineColor.color);

    interface_config[16].name = "主窗口输入法名称色";
    interface_config[16].value_type = CONFIG_OTHER;
    interface_config[16].config_rw = main_window_input_method_name_color;

    interface_config[17].name = "输入窗背景色";
    interface_config[17].value_type = CONFIG_COLOR;
    interface_config[17].value.color = &(inputWindowColor.backColor);

    interface_config[18].name = "输入窗提示色";
    interface_config[18].value_type = CONFIG_COLOR;
    interface_config[18].value.color = &(messageColor[0].color);

    interface_config[19].name = "输入窗用户输入色";
    interface_config[19].value_type = CONFIG_COLOR;
    interface_config[19].value.color = &(messageColor[1].color);

    interface_config[20].name = "输入窗序号色";
    interface_config[20].value_type = CONFIG_COLOR;
    interface_config[20].value.color = &(messageColor[2].color);

    interface_config[21].name = "输入窗第一个候选字色";
    interface_config[21].value_type = CONFIG_COLOR;
    interface_config[21].value.color = &(messageColor[3].color);

    interface_config[22].name = "输入窗用户词组色";
    interface_config[22].comment = "该颜色值只用于拼音中的用户自造词";
    interface_config[22].value_type = CONFIG_COLOR;
    interface_config[22].value.color = &(messageColor[4].color);

    interface_config[23].name = "输入窗提示编码色";
    interface_config[23].value_type = CONFIG_COLOR;
    interface_config[23].value.color = &(messageColor[5].color);

    interface_config[24].name = "输入窗其它文本色";
    interface_config[24].comment = "五笔、拼音的单字/系统词组均使用该颜色";
    interface_config[24].value_type = CONFIG_COLOR;
    interface_config[24].value.color = &(messageColor[6].color);

    interface_config[25].name = "输入窗线条色";
    interface_config[25].value_type = CONFIG_COLOR;
    interface_config[25].value.color = &(inputWindowLineColor.color);

    interface_config[26].name = "输入窗箭头色";
    interface_config[26].value_type = CONFIG_COLOR;
    interface_config[26].value.color = &(colorArrow);

    interface_config[27].name = "虚拟键盘窗背景色";
    interface_config[27].value_type = CONFIG_COLOR;
    interface_config[27].value.color = &(VKWindowColor.backColor);

    interface_config[28].name = "虚拟键盘窗字母色";
    interface_config[28].value_type = CONFIG_COLOR;
    interface_config[28].value.color = &(VKWindowAlphaColor.color);

    interface_config[29].name = "虚拟键盘窗符号色";
    interface_config[29].value_type = CONFIG_COLOR;
    interface_config[29].value.color = &(VKWindowFontColor.color);

    interface_config[30].name = NULL;

    hotkey_config[0].name = "打开/关闭输入法";
    hotkey_config[0].value_type = CONFIG_OTHER;
    hotkey_config[0].config_rw = trigger_input_method;

    hotkey_config[1].name = "中英文快速切换键";
    hotkey_config[1].comment ="中英文快速切换键 可以设置为L_CTRL R_CTRL L_SHIFT R_SHIFT L_SUPER R_SUPER";
    hotkey_config[1].value_type = CONFIG_OTHER; /* FIXME: 应该为 CONFIG_SWITCHKEY */
    hotkey_config[1].config_rw = fast_chinese_english_switch;

    hotkey_config[2].name = "双击中英文切换";
    hotkey_config[2].value_type = CONFIG_INTEGER;
    hotkey_config[2].value.integer = &bDoubleSwitchKey;

    hotkey_config[3].name = "击键时间间隔";
    hotkey_config[3].value_type = CONFIG_INTEGER;
    hotkey_config[3].value.integer = (int *)&iTimeInterval;

    hotkey_config[4].name = "光标跟随";
    hotkey_config[4].value_type = CONFIG_HOTKEY;
    hotkey_config[4].config_rw = cursor_follow;

    hotkey_config[5].name = "隐藏主窗口";
    hotkey_config[5].value_type = CONFIG_HOTKEY;
    hotkey_config[5].config_rw = hide_main_window;

    hotkey_config[6].name = "切换虚拟键盘";
    hotkey_config[6].value_type = CONFIG_HOTKEY;
    hotkey_config[6].config_rw = switch_vk;

    hotkey_config[7].name = "GBK支持";
    hotkey_config[7].value_type = CONFIG_HOTKEY;
    hotkey_config[7].config_rw = gbk_support;

    hotkey_config[8].name = "GBK繁体切换键";
    hotkey_config[8].value_type = CONFIG_HOTKEY;
    hotkey_config[8].config_rw = gbk_traditional_simplified_switch;

    hotkey_config[9].name = "联想";
    hotkey_config[9].value_type = CONFIG_HOTKEY;
    hotkey_config[9].config_rw = association;

    hotkey_config[10].name = "反查拼音";
    hotkey_config[10].value_type = CONFIG_HOTKEY;
    hotkey_config[10].config_rw = lookup_pinyin;

    hotkey_config[11].name = "全半角";
    hotkey_config[11].value_type = CONFIG_HOTKEY;
    hotkey_config[11].config_rw = sbc_dbc_switch;

    hotkey_config[12].name = "中文标点";
    hotkey_config[12].value_type = CONFIG_HOTKEY;
    hotkey_config[12].config_rw = chinese_punctuation;

    hotkey_config[13].name = "上一页";
    hotkey_config[13].value_type = CONFIG_HOTKEY;
    hotkey_config[13].config_rw = prev_page;

    hotkey_config[14].name = "下一页";
    hotkey_config[14].value_type = CONFIG_HOTKEY;
    hotkey_config[14].config_rw = next_page;

    hotkey_config[15].name = "第二三候选词选择键";
    hotkey_config[15].value_type = CONFIG_HOTKEY;
    hotkey_config[15].config_rw = second_third_candidate_word;


    hotkey_config[16].name = NULL;

    hotkey_config[17].name = "保存词库";
    hotkey_config[17].value_type = CONFIG_HOTKEY;
    hotkey_config[17].config_rw = save_all;

    hotkey_config[18].name = NULL;

    input_method_config[0].name = "使用拼音";
    input_method_config[0].value_type = CONFIG_INTEGER;
    input_method_config[0].value.integer = &bUsePinyin;

    input_method_config[1].name = "拼音名称";
    input_method_config[1].value_type = CONFIG_STRING;
    input_method_config[1].value.str_value.string = strNameOfPinyin,
    input_method_config[1].value.str_value.string_length = 41;   /* FIXME: 不应在此硬编码字符串长度，下同 */

    input_method_config[2].name = "使用双拼";
    input_method_config[2].value_type = CONFIG_INTEGER;
    input_method_config[2].value.integer = &bUseSP;

    input_method_config[3].name = "双拼名称";
    input_method_config[3].value_type = CONFIG_STRING;
    input_method_config[3].value.str_value.string = strNameOfShuangpin;
    input_method_config[3].value.str_value.string_length = 41;

    input_method_config[4].name = "默认双拼方案";
    input_method_config[4].value_type = CONFIG_OTHER;
    input_method_config[4].config_rw = &default_shuangpin_scheme;

    input_method_config[5].name = "使用区位";
    input_method_config[5].value_type = CONFIG_INTEGER;
    input_method_config[5].value.integer = &bUseQW;

    input_method_config[6].name = "区位名称";
    input_method_config[6].value_type = CONFIG_STRING;
    input_method_config[6].value.str_value.string = strNameOfQuwei;
    input_method_config[6].value.str_value.string_length = 41;

    input_method_config[7].name = "使用码表";
    input_method_config[7].value_type = CONFIG_INTEGER;
    input_method_config[7].value.integer = &bUseTable;

    input_method_config[8].name = "提示词库中的词组";
    input_method_config[8].value_type = CONFIG_INTEGER;
    input_method_config[8].value.integer = &bPhraseTips;

    input_method_config[9].name = "其他输入法";
    input_method_config[9].value_type = CONFIG_STRING;
    input_method_config[9].value.str_value.string = strExternIM;
    input_method_config[9].value.str_value.string_length = PY_PATH_MAX;

    input_method_config[10].name = NULL;


    pinyin_config[0].name = "使用全拼";
    pinyin_config[0].value_type = CONFIG_INTEGER;
    pinyin_config[0].value.integer = &bFullPY;

    pinyin_config[1].name = "拼音自动组词";
    pinyin_config[1].value_type = CONFIG_INTEGER;
    pinyin_config[1].value.integer = &bPYCreateAuto;

    pinyin_config[2].name = "保存自动组词";
    pinyin_config[2].value_type = CONFIG_INTEGER;
    pinyin_config[2].value.integer = &bPYSaveAutoAsPhrase;

    pinyin_config[3].name = "增加拼音常用字";
    pinyin_config[3].value_type = CONFIG_HOTKEY;
    pinyin_config[3].config_rw = &add_pinyin_frequently_used_word;

    pinyin_config[4].name = "删除拼音常用字";
    pinyin_config[4].value_type = CONFIG_HOTKEY;
    pinyin_config[4].config_rw = &delete_pinyin_frequently_used_word;

    pinyin_config[5].name = "删除拼音用户词组";
    pinyin_config[5].value_type = CONFIG_HOTKEY;
    pinyin_config[5].config_rw = &delete_pinyin_user_create_phrase;

    pinyin_config[6].name = "拼音以词定字键";
    pinyin_config[6].comment = "拼音以词定字键，等号后面紧接键，不要有空格";
    pinyin_config[6].value_type = CONFIG_OTHER;
    pinyin_config[6].config_rw = &pinyin_get_word_from_phrase;

    pinyin_config[7].name = "拼音单字重码调整方式";
    pinyin_config[7].comment = "重码调整方式说明：0-->不调整  1-->快速调整  2-->按频率调整";
    pinyin_config[7].value_type = CONFIG_INTEGER;
    pinyin_config[7].value.integer = (int *)&baseOrder;

    pinyin_config[8].name = "拼音词组重码调整方式";
    pinyin_config[8].value_type = CONFIG_INTEGER;
    pinyin_config[8].value.integer = (int *)&phraseOrder;

    pinyin_config[9].name = "拼音常用词重码调整方式";
    pinyin_config[9].value_type = CONFIG_INTEGER;
    pinyin_config[9].value.integer = (int *)&freqOrder;

    pinyin_config[10].name = "模糊an和ang";
    pinyin_config[10].value_type = CONFIG_OTHER;
    pinyin_config[10].config_rw = blur_an_ang;

    pinyin_config[11].name = "模糊en和eng";
    pinyin_config[11].value_type = CONFIG_INTEGER;
    pinyin_config[11].value.integer = &(MHPY_C[1].bMode);

    pinyin_config[12].name = "模糊ian和iang";
    pinyin_config[12].value_type = CONFIG_INTEGER;
    pinyin_config[12].value.integer = &(MHPY_C[2].bMode);

    pinyin_config[13].name = "模糊in和ing";
    pinyin_config[13].value_type = CONFIG_INTEGER;
    pinyin_config[13].value.integer = &(MHPY_C[3].bMode);

    pinyin_config[14].name = "模糊ou和u";
    pinyin_config[14].value_type = CONFIG_INTEGER;
    pinyin_config[14].value.integer = &(MHPY_C[4].bMode);

    pinyin_config[15].name = "模糊uan和uang";
    pinyin_config[15].value_type = CONFIG_INTEGER;
    pinyin_config[15].value.integer = &(MHPY_C[5].bMode);

    pinyin_config[16].name = "模糊c和ch";
    pinyin_config[16].value_type = CONFIG_INTEGER;
    pinyin_config[16].value.integer = &(MHPY_S[0].bMode);

    pinyin_config[17].name = "模糊f和h";
    pinyin_config[17].value_type = CONFIG_INTEGER;
    pinyin_config[17].value.integer = &(MHPY_S[1].bMode);

    pinyin_config[18].name = "模糊l和n";
    pinyin_config[18].value_type = CONFIG_INTEGER;
    pinyin_config[18].value.integer = &(MHPY_S[2].bMode);

    pinyin_config[19].name = "模糊s和sh";
    pinyin_config[19].value_type = CONFIG_INTEGER;
    pinyin_config[19].value.integer = &(MHPY_S[3].bMode);

    pinyin_config[20].name = "模糊z和zh";
    pinyin_config[20].value_type = CONFIG_INTEGER;
    pinyin_config[20].value.integer = &(MHPY_S[4].bMode);

    pinyin_config[21].name = NULL;




    return 0;

}


Configure_group configure_groups[] = {
    {
        "程序",
        NULL,
        program_config,
    },
    {
        "输出",
        NULL,
        output_config,
    },
    {
        "界面",
        NULL,
        interface_config,
    },
    {
        "热键",
        "除了“中英文快速切换键”外，其它的热键均可设置为两个，中间用空格分隔",
        hotkey_config,
    },
    {
        "输入法",
         NULL,
         input_method_config,
    },
    {
        "拼音",
        NULL,
        pinyin_config,
    },
    {
        NULL,
    },
};


#else
Configure program_config[] = {
    {
        .name = "显示字体(中)",
        .value_type = CONFIG_STRING,
        .value.str_value.string = strFontName,
        .value.str_value.string_length = 100,   /* FIXME: 不应在此硬编码字符串的长度，下同 */
    },
    {
        .name = "显示字体(英)",
        .value_type = CONFIG_STRING,
        .value.str_value.string = strFontEnName,
        .value.str_value.string_length = 100,
    },
    {
        .name = "显示字体大小",
        .value_type = CONFIG_INTEGER,
        .value.integer = &iFontSize,
    },
    {
        .name = "主窗口字体大小",
        .value_type = CONFIG_INTEGER,
        .value.integer = &iMainWindowFontSize,
    },
    {
        .name = "字体区域",
        .value_type = CONFIG_STRING,
        .value.str_value.string = strUserLocale,
        .value.str_value.string_length = 50,
    },
 #ifdef _USE_XFT
    {
        .name = "使用AA字体",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bUseAA,
    },
 #endif
    {
        .name = "使用粗体",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bUseBold,
    },
 #ifdef _ENABLE_TRAY
    {
        .name = "使用托盘图标",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bUseTrayIcon,
    },
 #endif
    {
        .name = NULL,
    },

};

/* piaoairy: gcc 默认enum 类型使用int */
Configure output_config[] = {
    {
        .name = "数字后跟半角符号",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bEngPuncAfterNumber,
    },
    {
        .name = "Enter键行为",
        .value_type = CONFIG_INTEGER,
        .value.integer = (int *)&enterToDo, /* FIXME: 这种转换方式也许并不是个好主意，下同 */
    },
    {
        .name = "分号键行为",
        .value_type = CONFIG_INTEGER,
        .value.integer = (int *)&semicolonToDo,
    },
    {
        .name = "大写字母输入英文",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bEngAfterCap,
    },
    {
        .name = "转换英文中的标点",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bConvertPunc,
    },
    {
        .name = "联想方式禁止翻页",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bDisablePagingInLegend,
    },
    {
        .name = NULL,
    },
};

Configure interface_config[] = {
    {
        .name = "候选词个数",
        .value_type = CONFIG_INTEGER,
        .value.integer = &iMaxCandWord,
    },
    {
        .name = "主窗口使用3D界面",
        .value_type = CONFIG_INTEGER,
        .value.integer = &_3DEffectMainWindow,
    },
    {
        .name = "输入条使用3D界面",
        .value_type = CONFIG_INTEGER,
        .value.integer = (int *)&_3DEffectInputWindow,
    },
    {
        .name = "主窗口隐藏模式",
        .value_type = CONFIG_INTEGER,
        .value.integer = (int *)&hideMainWindow,
    },
    {
        .name = "显示虚拟键盘",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bShowVK,
    },
    {
        .name = "输入条居中",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bCenterInputWindow,
    },
    {
        .name = "首次显示输入条",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bShowInputWindowTriggering,
    },
    {
        .name = "输入条固定宽度",
        .comment = "输入条固定宽度(仅适用于码表输入法)，0表示不固定宽度",
        .value_type = CONFIG_INTEGER,
        .value.integer = &iFixedInputWindowWidth,
    },
    {
        .name = "输入条偏移量X",
        .value_type = CONFIG_INTEGER,
        .value.integer = &iOffsetX,
    },
    {
        .name = "输入条偏移量Y",
        .value_type = CONFIG_INTEGER,
        .value.integer = &iOffsetY,
    },
    {
        .name = "序号后加点",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bPointAfterNumber,
    },
    {
        .name = "显示打字速度",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bShowUserSpeed,
    },
    {
        .name = "显示版本",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bShowVersion,
    },
    {
        .name = "光标色",
        .value_type = CONFIG_COLOR,
        .value.color = &(cursorColor.color),
    },
    {
        .name = "主窗口背景色",
        .value_type = CONFIG_COLOR,
        .value.color = &(mainWindowColor.backColor),
    },
    {
        .name = "主窗口线条色",
        .value_type = CONFIG_COLOR,
        .value.color = &(mainWindowLineColor.color),
    },
    {
        .name = "主窗口输入法名称色",
        .value_type = CONFIG_OTHER,
        .config_rw = main_window_input_method_name_color,
    },
    {
        .name = "输入窗背景色",
        .value_type = CONFIG_COLOR,
        .value.color = &(inputWindowColor.backColor),
    },
    {
        .name = "输入窗提示色",
        .value_type = CONFIG_COLOR,
        .value.color = &(messageColor[0].color),
    },
    {
        .name = "输入窗用户输入色",
        .value_type = CONFIG_COLOR,
        .value.color = &(messageColor[1].color),
    },
    {
        .name = "输入窗序号色",
        .value_type = CONFIG_COLOR,
        .value.color = &(messageColor[2].color),
    },
    {
        .name = "输入窗第一个候选字色",
        .value_type = CONFIG_COLOR,
        .value.color = &(messageColor[3].color),
    },
    {
        .name = "输入窗用户词组色",
        .comment = "该颜色值只用于拼音中的用户自造词",
        .value_type = CONFIG_COLOR,
        .value.color = &(messageColor[4].color),
    },
    {
        .name = "输入窗提示编码色",
        .value_type = CONFIG_COLOR,
        .value.color = &(messageColor[5].color),
    },
    {
        .name = "输入窗其它文本色",
        .comment = "五笔、拼音的单字/系统词组均使用该颜色",
        .value_type = CONFIG_COLOR,
        .value.color = &(messageColor[6].color),
    },
    {
        .name = "输入窗线条色",
        .value_type = CONFIG_COLOR,
        .value.color = &(inputWindowLineColor.color),
    },
    {
        .name = "输入窗箭头色",
        .value_type = CONFIG_COLOR,
        .value.color = &(colorArrow),
    },
    {
        .name = "虚拟键盘窗背景色",
        .value_type = CONFIG_COLOR,
        .value.color = &(VKWindowColor.backColor),
    },
    {
        .name = "虚拟键盘窗字母色",
        .value_type = CONFIG_COLOR,
        .value.color = &(VKWindowAlphaColor.color),
    },
    {
        .name = "虚拟键盘窗符号色",
        .value_type = CONFIG_COLOR,
        .value.color = &(VKWindowFontColor.color),
    },
    {
        .name = NULL,
    },
};

Configure hotkey_config[] = {
    {
        .name = "打开/关闭输入法",
        .value_type = CONFIG_OTHER,
        .config_rw = trigger_input_method,
    },
    {
        .name = "中英文快速切换键",
        .comment = "中英文快速切换键 可以设置为L_CTRL R_CTRL L_SHIFT R_SHIFT L_SUPER R_SUPER",
        .value_type = CONFIG_OTHER, /* FIXME: 应该为 CONFIG_SWITCHKEY */
        .config_rw = fast_chinese_english_switch,
    },
    {
        .name = "双击中英文切换",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bDoubleSwitchKey,
    },
    {
        .name = "击键时间间隔",
        .value_type = CONFIG_INTEGER,
        .value.integer = (int *)&iTimeInterval,
    },
    {
        .name = "光标跟随",
        .value_type = CONFIG_HOTKEY,
        .config_rw = cursor_follow,
    },
    {
        .name = "隐藏主窗口",
        .value_type = CONFIG_HOTKEY,
        .config_rw = hide_main_window,
    },
    {
        .name = "切换虚拟键盘",
        .value_type = CONFIG_HOTKEY,
        .config_rw = switch_vk,
    },
    {
        .name = "GBK支持",
        .value_type = CONFIG_HOTKEY,
        .config_rw = gbk_support,
    },
    {
        .name = "GBK繁体切换键",
        .value_type = CONFIG_HOTKEY,
        .config_rw = gbk_traditional_simplified_switch,
    },
    {
        .name = "联想",
        .value_type = CONFIG_HOTKEY,
        .config_rw = association,
    },
    {
        .name = "反查拼音",
        .value_type = CONFIG_HOTKEY,
        .config_rw = lookup_pinyin,
    },
    {
        .name = "全半角",
        .value_type = CONFIG_HOTKEY,
        .config_rw = sbc_dbc_switch,
    },
    {
        .name = "中文标点",
        .value_type = CONFIG_HOTKEY,
        .config_rw = chinese_punctuation,
    },
    {
        .name = "上一页",
        .value_type = CONFIG_HOTKEY,
        .config_rw = prev_page,
    },
    {
        .name = "下一页",
        .value_type = CONFIG_HOTKEY,
        .config_rw = next_page,
    },
    {
        .name = "第二三候选词选择键",
        .value_type = CONFIG_HOTKEY,
        .config_rw = second_third_candidate_word,
    },
    {
        .name = NULL,
    },
    {
        .name = "保存词库",
        .value_type = CONFIG_HOTKEY,
        .config_rw = save_all,
    },
    {
        .name = NULL,
    },
};

Configure input_method_config[] = {
    {
        .name = "使用拼音",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bUsePinyin,
    },
    {
        .name = "拼音名称",
        .value_type = CONFIG_STRING,
        .value.str_value.string = strNameOfPinyin,
        .value.str_value.string_length = 41,    /* FIXME: 不应在此硬编码字符串长度，下同 */
    },
    {
        .name = "使用双拼",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bUseSP,
    },
    {
        .name = "双拼名称",
        .value_type = CONFIG_STRING,
        .value.str_value.string = strNameOfShuangpin,
        .value.str_value.string_length = 41,
    },
    {
        .name = "默认双拼方案",
        .value_type = CONFIG_OTHER,
        .config_rw = default_shuangpin_scheme,
    },
    {
        .name = "使用区位",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bUseQW,
    },
    {
        .name = "区位名称",
        .value_type = CONFIG_STRING,
        .value.str_value.string = strNameOfQuwei,
        .value.str_value.string_length = 41,
    },
    {
        .name = "使用码表",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bUseTable,
    },
    {
        .name = "提示词库中的词组",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bPhraseTips,
    },
    {
        .name = "其他输入法",
        .value_type = CONFIG_STRING,
        .value.str_value.string = strExternIM,
        .value.str_value.string_length = PY_PATH_MAX,
    },
    {
        .name = NULL,
    },
};

Configure pinyin_config[] = {
    {
        .name = "使用全拼",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bFullPY,
    },
    {
        .name = "拼音自动组词",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bPYCreateAuto,
    },
    {
        .name = "保存自动组词",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bPYSaveAutoAsPhrase,
    },
    {
       .name = "增加拼音常用字",
        .value_type = CONFIG_HOTKEY,
        .config_rw = add_pinyin_frequently_used_word,
    },
    {
        .name = "删除拼音常用字",
        .value_type = CONFIG_HOTKEY,
        .config_rw = delete_pinyin_frequently_used_word,
    },
    {
        .name = "删除拼音用户词组",
        .value_type = CONFIG_HOTKEY,
        .config_rw = delete_pinyin_user_create_phrase,
    },
    {
        .name = "拼音以词定字键",
        .comment = "拼音以词定字键，等号后面紧接键，不要有空格",
        .value_type = CONFIG_OTHER,
        .config_rw = pinyin_get_word_from_phrase,
    },
    {
        .name = "拼音单字重码调整方式",
        .comment = "重码调整方式说明：0-->不调整  1-->快速调整  2-->按频率调整",
        .value_type = CONFIG_INTEGER,
        .value.integer = (int *)&baseOrder,
    },
    {
        .name = "拼音词组重码调整方式",
        .value_type = CONFIG_INTEGER,
        .value.integer = (int *)&phraseOrder,
    },
    {
        .name = "拼音常用词重码调整方式",
        .value_type = CONFIG_INTEGER,
        .value.integer = (int *)&freqOrder,
    },
    {
        .name = "模糊an和ang",
        .value_type = CONFIG_OTHER,
        .config_rw = blur_an_ang,
    },
    {
        .name = "模糊en和eng",
        .value_type = CONFIG_INTEGER,
        .value.integer = &(MHPY_C[1].bMode),
    },
    {
        .name = "模糊ian和iang",
        .value_type = CONFIG_INTEGER,
        .value.integer = &(MHPY_C[2].bMode),
    },
    {
        .name = "模糊in和ing",
        .value_type = CONFIG_INTEGER,
        .value.integer = &(MHPY_C[3].bMode),
    },
    {
        .name = "模糊ou和u",
        .value_type = CONFIG_INTEGER,
        .value.integer = &(MHPY_C[4].bMode),
    },
    {
        .name = "模糊uan和uang",
        .value_type = CONFIG_INTEGER,
        .value.integer = &(MHPY_C[5].bMode),
    },
    {
        .name = "模糊c和ch",
        .value_type = CONFIG_INTEGER,
        .value.integer = &(MHPY_S[0].bMode),
    },
    {
        .name = "模糊f和h",
        .value_type = CONFIG_INTEGER,
        .value.integer = &(MHPY_S[1].bMode),
    },
    {
        .name = "模糊l和n",
        .value_type = CONFIG_INTEGER,
        .value.integer = &(MHPY_S[2].bMode),
    },
    {
        .name = "模糊s和sh",
        .value_type = CONFIG_INTEGER,
        .value.integer = &(MHPY_S[3].bMode),
    },
    {
        .name = "模糊z和zh",
        .value_type = CONFIG_INTEGER,
        .value.integer = &(MHPY_S[4].bMode),
    },
    {
        .name = NULL,
    },
};

Configure_group configure_groups[] = {
    {
        .name = "程序",
        .configure = program_config,
    },
    {
        .name = "输出",
        .configure = output_config,
    },
    {
        .name = "界面",
        .configure = interface_config,
    },
    {
        .name = "热键",
        .comment = "除了“中英文快速切换键”外，其它的热键均可设置为两个，中间用空格分隔",
        .configure = hotkey_config,
    },
    {
        .name = "输入法",
        .configure = input_method_config,
    },
    {
        .name = "拼音",
        .configure = pinyin_config,
    },
    {
        .name = NULL,
    },
};

#endif

/**
 * @brief 读取用户的配置文件
 * @param bMode 标识配置文件是用户家目录下的，还是从安装目录下拷贝过来的
 * @return void
 */
void LoadConfig (Bool bMode)
{
    FILE    *fp;
    char    buf[PY_PATH_MAX], *pbuf, *pbuf1;
    Bool    bFromUser = True;
    //用于标示group的index，在配置文件里面配置是分组的，类似与ini文件的分组
    int     group_idx;
    int        i;
    Configure   *tmpconfig;

    //用以标识配置文件是用户家目录下的，还是从安装目录下拷贝过来的
    bIsReloadConfig = bMode;    // 全局变量，定义于“src/tool.c[193]"

//szj    pbuf = getenv("HOME");        // 从环境变量中获取当前用户家目录的绝对路径
//    if(!pbuf){
//        fprintf(stderr, "error: get environment variable HOME\n");
//        exit(1);    // 此时可以不退出，但直接退出处理起来明了简单
//    }
    //获取配置文件的绝对路径
//szj    snprintf(buf, PY_PATH_MAX, "%s/.fcitx/config", pbuf);
//    sprintf(buf, "%s//config", "f://data");

    strcpy (buf, respath);
    strcat (buf, "config");

    fp = fopen(buf, "r");
    if(!fp && errno == ENOENT){ /* $HOME/.fcitx/config does not exist */
//szj        snprintf(buf, PY_PATH_MAX, PKGDATADIR "/data/config");
        bFromUser = False;
        fp = fopen(buf, "r");
        if(!fp){
            perror("fopen");
            exit(1);    // 如果安装目录里面也没有配置文件，那就只好告诉用户，无法运行了
        }
    }

    if(!bFromUser) /* create default configure file */
        SaveConfig();

    group_idx = -1;

    /* FIXME: 也许应该用另外更恰当的缓冲区长度 */
    while(fgets(buf, PY_PATH_MAX, fp)){        //每次最多读入PY_PATH_MAX大小的数据
        i = strlen(buf);

        /*fcitx的配置文件每行最多是PY_PATH_MAX个字符，因此有上面的FIXME*/
        if(buf[i-1] != '\n'){
            fprintf(stderr, "error: configure file: line length\n");
            exit(1);
        }else
            buf[i-1] = '\0';

        pbuf = buf;
        while(*pbuf && isspace(*pbuf))    //将pbuf指向第一个非空字符
            pbuf++;
        if(!*pbuf || *pbuf == '#')        //如果改行是空数据或者是注释(以#开头为注释)
            continue;

        if(*pbuf == '['){ /* get a group name(组名的格式为"[组名]")*/
            pbuf++;
            pbuf1 = strchr(pbuf, ']');
            if(!pbuf1){
                fprintf(stderr, "error: configure file: configure group name\n");
                exit(1);
            }

            //根据group的名字找到其在全局变量configure_groups中的index
            group_idx = -1;
            for(i = 0; configure_groups[i].name; i++)
                if(strncmp(configure_groups[i].name, pbuf, pbuf1-pbuf) == 0){
                    group_idx = i;
                    break;
                }
            if(group_idx < 0){
                fprintf(stderr, "error: invalid configure group name\n");
                exit(1); /* 我认为这儿没有必要退出。此处完全可以忽略这个错误，
                          * 并且在后面也忽略这个组的配置即可。
                          * 因为这儿退出只会带来一个坏处，那就是扩展性。
                          * 以后再添加新的组的时候，老版本的程序就无法使用
                          * 新版本的配置文件了。或者，添加了一个可选扩展，
                          * 该扩展新添加一个组等等。所以，此处应该给一个警告，
                          * 而不是退出。*/
            }
            continue;
        }

        //pbuf1指向第一个非空字符与=之间的字符
        pbuf1 = strchr(pbuf, '=');
        if(!pbuf1){
            fprintf(stderr, "error: configure file: configure entry name\n");
            exit(1);    // 和前面一样，这儿也应该是一个警告而不应该是提示出错并退出。
        }

        /*
         * 这儿避免的是那样一种情况，即从文件头到第一个配置项(即类似与“配置名=配置值”
         * 的一行字符串)并没有任何分组。也就是防止出现下面的“配置1”和“配置2”
         * #文件头
         * 配置1=123 配置2=123
         * [组名]
         * ...
         * #文件尾
         */


        if(group_idx < 0){
            fprintf(stderr, "error: configure file: no group name at beginning\n");
            exit(1);
        }
        //找到该组中的配置项，并将其保存到对应的全局变量里面去
        for(tmpconfig = configure_groups[group_idx].configure;
                tmpconfig->name; tmpconfig++)
        {

            if(strncmp(tmpconfig->name, pbuf, pbuf1-pbuf) == 0)
                read_configure(tmpconfig, ++pbuf1);
        }
    }

    fclose(fp);

    /* 如果配置文件中没有设置打开/关闭输入法的热键，那么设置CTRL-SPACE为默认热键 */
    if (!Trigger_Keys) {
    iTriggerKeyCount = 0;
    Trigger_Keys = (XIMTriggerKey *) malloc (sizeof (XIMTriggerKey) * (iTriggerKeyCount + 2));
    Trigger_Keys[0].keysym = XK_space;
    Trigger_Keys[0].modifier = ControlMask;
    Trigger_Keys[0].modifier_mask = ControlMask;
    Trigger_Keys[1].keysym = 0;
    Trigger_Keys[1].modifier = 0;
    Trigger_Keys[1].modifier_mask = 0;
    }
}

/**
 * 保存配置信息
 */
void SaveConfig (void)
{
#if 0 //szj
    FILE    *fp;
    char    buf[PY_PATH_MAX], *pbuf;
    Configure_group *tmpgroup;

    pbuf = getenv("HOME");
    if(!pbuf){
        fprintf(stderr, "error: get environment variable HOME\n");
        exit(1);
    }

    snprintf(buf, PY_PATH_MAX, "%s/.fcitx", pbuf);
    if(mkdir(buf, S_IRWXU) < 0 && errno != EEXIST){
        perror("mkdir");
        exit(1);
    }

//    snprintf(buf, PY_PATH_MAX, "%s/.fcitx/config", pbuf);
    strcpy (buf, respath);
    strcat (buf, "config");

    fp = fopen (buf, "w");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    /* 实际上，写配置文件很简单，就是从全局数组configure_groups里面分别把每个组的配置
     * 写入到文件里面去*/
    for(tmpgroup = configure_groups; tmpgroup->name; tmpgroup++){
        if(tmpgroup->comment)
            fprintf(fp, "# %s\n", tmpgroup->comment);    // 如果存在注释，先写入
        fprintf(fp, "[%s]\n", tmpgroup->name);        // 接下来写入组的名字
        write_configures(fp, tmpgroup->configure);    // 最后将该组的每个配置项写入到文件中
        fprintf(fp, "\n");        // 为增加可读性插入一个空行
    }
    fclose(fp);
#endif
}

/** 版本 */
inline static int get_version(Configure *c, void *a, int isread)
{
#if 0//szj
    if(isread){
        if(!strcasecmp(FCITX_VERSION, (char *)a))
            bIsNeedSaveConfig = False;
    }else
        fprintf((FILE *)a, "%s=%s\n", c->name, FCITX_VERSION);
#endif
    return 0;
}

/** 主窗口位置X */
inline static int get_main_window_offset_x(Configure *c, void *a, int isread)
{
#if 0

    if(isread){
        iMainWindowX = atoi((const char*)a);
        if(iMainWindowX < 0)
            iMainWindowX = 0;
        else if((iMainWindowX + MAINWND_WIDTH) > DisplayWidth(dpy, iScreen))
            iMainWindowX = DisplayWidth(dpy, iScreen) - MAINWND_WIDTH;
    }else
        fprintf((FILE *)a, "%s=%d\n", c->name, iMainWindowX);
#endif
    return 0;
}

/** 主窗口位置Y */
inline static int get_main_window_offset_y(Configure *c, void *a, int isread)
{
#if 0
    if(isread){
        iMainWindowY = atoi((const char*)a);
        if(iMainWindowY < 0)
            iMainWindowY = 0;
        else if((iMainWindowY + MAINWND_HEIGHT) > DisplayHeight(dpy, iScreen))
            iMainWindowY = DisplayHeight(dpy, iScreen) - MAINWND_HEIGHT;
    }else
        fprintf((FILE *)a, "%s=%d\n", c->name, iMainWindowY);
#endif
    return 0;
}

/** 输入窗口位置X */
inline static int get_input_window_offset_x(Configure *c, void *a, int isread)
{
#if 0
    if(isread){
        iInputWindowX = atoi((const char*)a);
        if(iInputWindowX < 0)
            iInputWindowX = 0;
        else if((iInputWindowX + iInputWindowWidth) > DisplayWidth(dpy, iScreen))
            iInputWindowX = DisplayWidth(dpy, iScreen) - iInputWindowWidth - 3;
    }else
        fprintf((FILE *)a, "%s=%d\n", c->name, iInputWindowX);
#endif
    return 0;
}

/** 输入窗口位置Y */
inline static int get_input_window_offset_y(Configure *c, void *a, int isread)
{
#if 0
    if(isread){
        iInputWindowY = atoi((const char*)a);
        if(iInputWindowY < 0)
            iInputWindowY = 0;
        else if((iInputWindowY + iInputWindowHeight) > DisplayHeight(dpy, iScreen))
            iInputWindowY = DisplayHeight(dpy, iScreen) - iInputWindowHeight - 3;
    }else
        fprintf((FILE *)a, "%s=%d\n", c->name, iInputWindowY);
#endif
    return 0;
}

static int iIMIndex_tmp = 0;        /* Issue 11: piaoairy add 20080518 */

#if 1   //szj
int  initConfig2()
{
    profiles[0].name = "版本";
    profiles[0].value_type = CONFIG_OTHER;
    profiles[0].config_rw = get_version;

    profiles[1].name = "主窗口位置X";
    profiles[1].value_type = CONFIG_OTHER;
    profiles[1].config_rw = get_main_window_offset_x;

    profiles[2].name = "主窗口位置Y";
    profiles[2].value_type = CONFIG_OTHER;
    profiles[2].config_rw = get_main_window_offset_y;

    profiles[3].name = "输入窗口位置X";
    profiles[3].value_type = CONFIG_OTHER;
    profiles[3].config_rw = get_input_window_offset_x;


    profiles[4].name = "输入窗口位置Y";
    profiles[4].value_type = CONFIG_OTHER;
    profiles[4].config_rw = get_input_window_offset_y;

    profiles[5].name = "全角";
    profiles[5].value_type = CONFIG_INTEGER;
    profiles[5].value.integer = &bCorner;

    profiles[6].name = "中文标点";
    profiles[6].value_type = CONFIG_INTEGER;
    profiles[6].value.integer = &bChnPunc;

    profiles[7].name = "GBK";
    profiles[7].value_type = CONFIG_INTEGER;
    profiles[7].value.integer = &bUseGBK;

    profiles[8].name = "光标跟随";
    profiles[8].value_type = CONFIG_INTEGER;
    profiles[8].value.integer = &bTrackCursor;

    profiles[9].name = "联想";
    profiles[9].value_type = CONFIG_INTEGER;
    profiles[9].value.integer = &bUseLegend;

    profiles[10].name = "当前输入法";//  Issue 11: piaoairy: 本来打算将iIMIndex 改为int类型,
    profiles[10].value_type = CONFIG_INTEGER;// 无奈使用的地方太多,
    profiles[10].value.integer = &iIMIndex_tmp;// 只好重新定义个iIMIndex_tmp搭桥

    profiles[11].name = "禁止键盘切换";
    profiles[11].value_type = CONFIG_INTEGER;
    profiles[11].value.integer = &bLocked;

    profiles[12].name = "简洁模式";
    profiles[12].value_type = CONFIG_INTEGER;
    profiles[12].value.integer = &bCompactMainWindow;

    profiles[13].name = "GBK繁体";
    profiles[13].value_type = CONFIG_INTEGER;
    profiles[13].value.integer = &bUseGBKT;

    profiles[14].name = NULL;

    return 0;
}
#else

Configure profiles[] = {
    {
        .name = "版本",
        .value_type = CONFIG_OTHER,
        .config_rw = get_version,
    },
    {
        .name = "主窗口位置X",
        .value_type = CONFIG_OTHER,
        .config_rw = get_main_window_offset_x,
    },
    {
        .name = "主窗口位置Y",
        .value_type = CONFIG_OTHER,
        .config_rw = get_main_window_offset_y,
    },
    {
        .name = "输入窗口位置X",
        .value_type = CONFIG_OTHER,
        .config_rw = get_input_window_offset_x,
    },
    {
        .name = "输入窗口位置Y",
        .value_type = CONFIG_OTHER,
        .config_rw = get_input_window_offset_y,
    },
    {
        .name = "全角",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bCorner,
    },
    {
        .name = "中文标点",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bChnPunc,
    },
    {
        .name = "GBK",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bUseGBK,
    },
    {
        .name = "光标跟随",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bTrackCursor,
    },
    {
        .name = "联想",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bUseLegend,
    },
    {
        .name = "当前输入法",    //  Issue 11: piaoairy: 本来打算将iIMIndex 改为int类型,
        .value_type = CONFIG_INTEGER,    // 无奈使用的地方太多,
        .value.integer = &iIMIndex_tmp,    // 只好重新定义个iIMIndex_tmp搭桥.
    },
    {
        .name = "禁止键盘切换",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bLocked,
    },
    {
        .name = "简洁模式",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bCompactMainWindow,
    },
    {
        .name = "GBK繁体",
        .value_type = CONFIG_INTEGER,
        .value.integer = &bUseGBKT,
    },
    {
        .name = NULL,
    },
};
#endif

/**
 * @brief 加载配置文件
 * @param void
 * @return void
 */
void LoadProfile (void)
{
    FILE           *fp;
    char            buf[PY_PATH_MAX], *pbuf, *pbuf1;
    int             i;
    Configure       *tmpconfig;


    /* 前将窗口的位置设定为最原始的默认值，接下来如果配置文件有，
     * 会从配置文件中读取，如果没有就使用这个了*/
    iMainWindowX = MAINWND_STARTX;        //主窗口位置X
    iMainWindowY = MAINWND_STARTY;        //主窗口位置Y
    iInputWindowX = INPUTWND_STARTX;    //输入窗口位置X
    iInputWindowY = INPUTWND_STARTY;    //输入窗口位置Y

    return ;  //szj
//szj    pbuf = getenv("HOME");
//    if(!pbuf){
//        fprintf(stderr, "error: get environment variable HOME\n");
//        exit(1);
//    }
//szj    snprintf(buf, PY_PATH_MAX, "%s/.fcitx/profile", pbuf);
    sprintf(buf, "%s//profile", "f://data");

    fp = fopen(buf, "r");
    if(!fp){
        if(errno == ENOENT)
            SaveProfile();
        return;
    }

    /* FIXME: 也许应该用另外更恰当的缓冲区长度 */
    while(fgets(buf, PY_PATH_MAX, fp)){
        i = strlen(buf);
        if(buf[i-1] != '\n'){
            fprintf(stderr, "error: profile file: line length\n");
            exit(1);
        }else
            buf[i-1] = '\0';

        pbuf = buf;
        while(*pbuf && isspace(*pbuf))
            pbuf++;
        if(!*pbuf || *pbuf == '#')
            continue;

        pbuf1 = strchr(pbuf, '=');
        if(!pbuf1){
            fprintf(stderr, "error: profile file: configure entry name\n");
            exit(1);
        }

        for(tmpconfig = profiles; tmpconfig->name; tmpconfig++)
            if(strncmp(tmpconfig->name, pbuf, pbuf1-pbuf) == 0)
                read_configure(tmpconfig, ++pbuf1);
    }

    fclose(fp);

    iIMIndex = iIMIndex_tmp;        /* piaoairy add 20080518 */

    if(bIsNeedSaveConfig){
        SaveConfig();
        SaveProfile();
     }
}

void SaveProfile (void)
{
    return;
#if 0
    FILE           *fp;
    char            buf[PY_PATH_MAX], *pbuf;

//    pbuf = getenv("HOME");
//    if(!pbuf){
//        fprintf(stderr, "error: get environment variable HOME\n");
//        exit(1);
//    }

//    snprintf(buf, PY_PATH_MAX, "%s/.fcitx", pbuf);
//    if(mkdir(buf, S_IRWXU) < 0 && errno != EEXIST){
//        perror("mkdir");
//        exit(1);
//    }

//    snprintf(buf, PY_PATH_MAX, "%s/.fcitx/profile", pbuf);

    strcpy (buf, respath);
    strcat (buf, "profile");

    fp = fopen (buf, "w");

    if (!fp) {
    perror("fopen");
        exit(1);
    }

    iIMIndex_tmp = iIMIndex;        /* piaoairy add 20080518 */
    write_configures(fp, profiles);
    fclose(fp);
#endif
}

void SetHotKey (char *strKeys, HOTKEYS * hotkey)
{
    char           *p;
    char            strKey[30];
    int             i, j;

    p = strKeys;

    while (*p == ' ')
    p++;
    i = 0;
    while (p[i] != ' ' && p[i] != '\0')
    i++;
    strncpy (strKey, p, i);
    strKey[i] = '\0';
    p += i + 1;
    j = ParseKey (strKey);
    if (j != -1)
    hotkey[0] = j;
    if (j == -1)
    j = 0;
    else
    j = 1;

    i = 0;
    while (p[i] != ' ' && p[i] != '\0')
    i++;
    if (p[0]) {
    strncpy (strKey, p, i);
    strKey[i] = '\0';

    i = ParseKey (strKey);
    if (i == -1)
        i = 0;
    }
    else
    i = 0;

    hotkey[j] = i;
}

/*
 * 计算文件中有多少行
 * 注意:文件中的空行也做为一行处理
 */
int CalculateRecordNumber (FILE * fpDict)
{
    char            strText[101];
    int             nNumber = 0;

    for (;;) {
    if (!fgets (strText, 100, fpDict))
        break;

    nNumber++;
    }
    rewind (fpDict);

    return nNumber;
}

void SetSwitchKey (char *str)
{
#if 0  //szj
    if (!strcasecmp (str, "R_CTRL"))
    switchKey = XKeysymToKeycode (dpy, XK_Control_R);
    else if (!strcasecmp (str, "R_SHIFT"))
    switchKey = XKeysymToKeycode (dpy, XK_Shift_R);
    else if (!strcasecmp (str, "L_SHIFT"))
    switchKey = XKeysymToKeycode (dpy, XK_Shift_L);
    else if (!strcasecmp (str, "R_SUPER"))
    switchKey = XKeysymToKeycode (dpy, XK_Super_R);
    else if (!strcasecmp (str, "L_SUPER"))
    switchKey = XKeysymToKeycode (dpy, XK_Super_L);
    else
    switchKey = XKeysymToKeycode (dpy, XK_Control_L);
#endif
}

void SetTriggerKeys (char *str)
{
    int             i;
    char            strKey[2][30];
    char           *p;

    //首先来判断用户设置了几个热键，最多为2
    p = str;

    i = 0;
    iTriggerKeyCount = 0;
    while (*p) {
    if (*p == ' ') {
        iTriggerKeyCount++;
        while (*p == ' ')
        p++;
        strcpy (strKey[1], p);
        break;
    }
    else
        strKey[0][i++] = *p++;
    }
    strKey[0][i] = '\0';

    Trigger_Keys = (XIMTriggerKey *) malloc (sizeof (XIMTriggerKey) * (iTriggerKeyCount + 2));
    for (i = 0; i <= (iTriggerKeyCount + 1); i++) {
    Trigger_Keys[i].keysym = 0;
    Trigger_Keys[i].modifier = 0;
    Trigger_Keys[i].modifier_mask = 0;
    }

    for (i = 0; i <= iTriggerKeyCount; i++) {
    if (MyStrcmp (strKey[i], "CTRL_")) {
        Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | ControlMask;
        Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | ControlMask;
    }
    else if (MyStrcmp (strKey[i], "SHIFT_")) {
        Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | ShiftMask;
        Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | ShiftMask;
    }
    else if (MyStrcmp (strKey[i], "ALT_")) {
        Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | Mod1Mask;
        Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | Mod1Mask;
    }
    else if (MyStrcmp (strKey[i], "SUPER_")) {
        Trigger_Keys[i].modifier = Trigger_Keys[i].modifier | Mod4Mask;
        Trigger_Keys[i].modifier_mask = Trigger_Keys[i].modifier_mask | Mod4Mask;
    }

    if (Trigger_Keys[i].modifier == 0) {
        Trigger_Keys[i].modifier = ControlMask;
        Trigger_Keys[i].modifier_mask = ControlMask;
    }

    p = strKey[i] + strlen (strKey[i]) - 1;
    while (*p != '_') {
        p--;
        if (p == strKey[i] || (p == strKey[i] + strlen (strKey[i]) - 1)) {
        Trigger_Keys = (XIMTriggerKey *) malloc (sizeof (XIMTriggerKey) * (iTriggerKeyCount + 2));
        Trigger_Keys[i].keysym = XK_space;
        return;
        }
    }
    p++;

#if 0 //szj

    if (strlen (p) == 1)
        Trigger_Keys[i].keysym = tolower (*p);
    else if (!strcasecmp (p, "LCTRL"))
        Trigger_Keys[i].keysym = XK_Control_L;
    else if (!strcasecmp (p, "RCTRL"))
        Trigger_Keys[i].keysym = XK_Control_R;
    else if (!strcasecmp (p, "LSHIFT"))
        Trigger_Keys[i].keysym = XK_Shift_L;
    else if (!strcasecmp (p, "RSHIFT"))
        Trigger_Keys[i].keysym = XK_Shift_R;
    else if (!strcasecmp (p, "LALT"))
        Trigger_Keys[i].keysym = XK_Alt_L;
    else if (!strcasecmp (p, "RALT"))
        Trigger_Keys[i].keysym = XK_Alt_R;
    else if (!strcasecmp (p, "LSUPER"))
        Trigger_Keys[i].keysym = XK_Super_L;
    else if (!strcasecmp (p, "RSUPER"))
        Trigger_Keys[i].keysym = XK_Super_R;
    else
        Trigger_Keys[i].keysym = XK_space;
#endif
    }

}

Bool CheckHZCharset (char *strHZ)
{
    return True;
    if (!bUseGBK) {
    //GB2312的汉字编码规则为：第一个字节的值在0xA1到0xFE之间(实际为0xF7)，第二个字节的值在0xA1到0xFE之间
    //由于查到的资料说法不一，懒得核实，就这样吧
    int             i;

    for (i = 0; i < (int)strlen (strHZ); i++) {
        if ((unsigned char) strHZ[i] < (unsigned char) 0xA1 || (unsigned char) strHZ[i] > (unsigned char) 0xF7 || (unsigned char) strHZ[i + 1] < (unsigned char) 0xA1 || (unsigned char) strHZ[i + 1] > (unsigned char) 0xFE)
        return False;
        i++;
    }
    }

    return True;
}

static char    *gGBKS2TTable = NULL;
static int      gGBKS2TTableSize = -1;

/**
 * 该函数装载data/gbks2t.tab的简体转繁体的码表，
 * 然后按码表将GBK字符转换成GBK繁体字符。
 *
 * WARNING： 该函数返回新分配内存字符串，请调用者
 * 注意释放。
 */
char           *ConvertGBKSimple2Tradition (char *strHZ)
{
    FILE           *fp;
    char           *ret;
    char            strPath[PY_PATH_MAX];
    int             i, len;
    int                idx;

    if (strHZ == NULL)
    return NULL;

    if (!gGBKS2TTable) {
    len = 0;

//    strcpy (strPath, PKGDATADIR "/data/");
//    strcat (strPath, TABLE_GBKS2T);

    strcpy (strPath, respath);
    strcat (strPath, TABLE_GBKS2T);


    fp = fopen (strPath, "rb");
    if (!fp) {
        ret = (char *) malloc (sizeof (char) * (strlen (strHZ) + 1));
        strcpy (ret, strHZ);
        return ret;
    }

    fseek (fp, 0, SEEK_END);
    fgetpos (fp, (fpos_t *) & len);
    if (len > 0) {
        gGBKS2TTable = (char *) malloc (sizeof (char) * len);
        gGBKS2TTableSize = len;
        fseek (fp, 0, SEEK_SET);
        fread (gGBKS2TTable, sizeof (char), len, fp);
    }
    fclose (fp);
    }

    i = 0;
    len = strlen (strHZ);
    ret = (char *) malloc (sizeof (char) * (len + 1));
    for (; i < len; ++i) {
    if (i < (len - 1))
        if ((unsigned char) strHZ[i] >= (unsigned char) 0x81
        && (unsigned char) strHZ[i] <= (unsigned char) 0xfe &&
        (((unsigned char) strHZ[i + 1] >= (unsigned char) 0x40 && (unsigned char) strHZ[i + 1] <= (unsigned char) 0x7e) || ((unsigned char) strHZ[i + 1] > (unsigned char) 0x7f && (unsigned char) strHZ[i + 1] <= (unsigned char) 0xfe))) {
        idx = (((unsigned char) strHZ[i] - (unsigned char) 0x81)
               * (unsigned char) 0xbe + ((unsigned char) strHZ[i + 1] - (unsigned char) 0x40)
               - ((unsigned char) strHZ[i + 1] / (unsigned char) 0x80)) * 2;
        if (idx >= 0 && idx < gGBKS2TTableSize - 1) {
            //if ((unsigned char)gGBKS2TTable[idx] != (unsigned char)0xa1 && (unsigned char) gGBKS2TTable[idx + 1] != (unsigned char) 0x7f) {
            if ((unsigned char) gGBKS2TTable[idx + 1] != (unsigned char) 0x7f) {
            ret[i] = gGBKS2TTable[idx];
            ret[i + 1] = gGBKS2TTable[idx + 1];
            i += 1;
            continue;
            }
        }
        }
    ret[i] = strHZ[i];
    }
    ret[len] = '\0';

    return ret;
}

int CalHZIndex (char *strHZ)
{
    return (strHZ[0] + 127) * 255 + strHZ[1] + 128;
}
