/**
 * @file   fontlib.c
 * @brief  字库操作封装
 *         绝对参数需要参考字库生成工具 @ref https://github.com/code4freeman/links/tree/master/font_bitmap
 *         以及生成时的输出日志和FFBF文件
 * @author Freeman.Li
 * @date   2025/3/25
 */

#include "fontlib.h"
#include <string.h>
#include <stdio.h>
#include "W25Q32BV.h"

#define LOG_TAG "FONTLIB"

#define DUMP_MEM(mem, len) do {\
        printf("DUMP_MEM ===============\n");\
        for (size_t i = 0; i < (len); i++) {\
            printf("[%d]%lu ", i, (uint32_t)*(mem+i));\
        }\
        printf("\n");\
    } while (0)

#define ESP_LOGE(tag, msg) printf("%s: %s\n", (tag), (msg));

/**
 * @brief utf8编码字节序列转unicode码点序列
 * @param str 字符串
 * @param rec_points unicode码点接收指针
 * @return 返回unicode码点的数量
 */
static size_t utf8_to_points (const char* str, uint32_t* rec_points)
{
    uint32_t* const start_rec = rec_points;
    uint8_t b;
    uint32_t temp = 0;

    #define RB() do {\
            b = *(str++);\
            if (b == '\0') {\
                ESP_LOGE(LOG_TAG, "utf8 字节不够!\n");\
                goto done;\
            }\
            if ( (b >> 6) != 0b10) {\
                ESP_LOGE(LOG_TAG, "utf8编码不合法，遇到剩余字节不复合UTF8编码!\n");\
                goto done;\
            }\
        } while(0)

    while (true)
    {
        b = *(str++);
        if (b == '\0') break;
        
        if (b >> 7 == 0) // 1byte
        {
            *(rec_points++) = b;
        }
        else if ( (b >> 5) == 0b110 ) // 2byte
        {
            temp = b & 0x0b00011111;
            RB();
            *(rec_points++) = (temp << 6) | (b & 0x3f);
        }
        else if ( (b >> 4) == 0b1110 ) // 3byte
        {
            temp = (b & 0x0f) << 12;
            RB();
            temp |= ((b & 0x3f) << 6);
            RB();
            temp |= b & 0x3f;
            *(rec_points++) = temp;
        }
        else if ( (b >> 3) == 0b11110 ) // 4byte
        {
            temp = (b & 0x07) << 18;
            RB();
            temp |= (b & 0x3f) << 12;
            RB();
            temp |= (b & 0x3f) << 6;
            RB();
            temp |= b & 0x3f;
            *(rec_points++) = temp;
        }
        else 
        {
           ESP_LOGE(LOG_TAG, "utf8编码可能超过了4byted的表示范围！\n");
        }
    }

    done:
        return ((size_t)rec_points - (size_t)start_rec) / sizeof(uint32_t);
}

static int32_t get_real_addr (uint32_t unicode, FontSize f_size)
{
    int32_t v = -1;
    if (unicode <= 126 && unicode >= 32) // ascii
    {
        switch (f_size)
        {
            case FontSize_12: v = (unicode - 32) * 25;
            break;
            case FontSize_16: v = 538375 + ((unicode - 32) * 33);
            break;
        }
    }
    else if (unicode >= 0x4e00 && unicode <= 0x9fff) // CJK表意文字
    {
        switch (f_size)
        {
            case FontSize_12: v = 2375 + ((unicode - 0x4e00) * 25);
            break;
            case FontSize_16: v = 541510 + ((unicode - 0x4e00) * 33);
            break;
        }
    }
    else if (unicode >= 0x3000 && unicode <= 0x303f) // CJK 符号与标点（CJK Symbols and Punctuation）字符
    {
        switch (f_size)
        {
            case FontSize_12: v = 527175 + ((unicode - 0x3000) * 25);
            break;
            case FontSize_16: v = 1234246 + ((unicode - 0x3000) * 33);
            break;
        }
    }
    else if (unicode >= 0x2000 && unicode <= 0x206f) // General Punctuation（通用标点符号）字符
    {
        switch (f_size)
        {
            case FontSize_12: v = 528775 + ((unicode - 0x2000) * 25);
            break;
            case FontSize_16: v = 1236358 + ((unicode - 0x2000) * 33);
            break;
        }
    }
    else if (unicode >= 0xfe30 && unicode <= 0xfe4f) // CJK Compatibility Forms（CJK 兼容形态）字符
    {
        switch (f_size)
        {
            case FontSize_12: v = 531575 + ((unicode - 0xfe30) * 25);
            break;
            case FontSize_16: v = 1240054 + ((unicode - 0xfe30) * 33);
            break;
        }
    }
    else if (unicode >= 0xff00 && unicode <= 0xffef) // Halfwidth and Fullwidth Forms（半角与全角字符）字符
    {
        switch (f_size)
        {
            case FontSize_12: v = 532375 + ((unicode - 0xff00) * 25);
            break;
            case FontSize_16: v = 1241110 + ((unicode - 0xff00) * 33);
            break;
        }
    }
    else 
    {
        ESP_LOGE(LOG_TAG, "[read_bitmap] 码点溢出,超出字库容纳范围！");
        goto done;
    }

    done:
        return v;
} 

/**
 * @brief 根据utf8字符串获取字库码点
 * @param str utf8编码的字符串
 * @param rec 接受码点的指针（外部根据获取的字库大小是提前知道接收所需内存的大小的）,否则段错误/越界
 * @param f_size 字模大小
 * @param bool
 */
int32_t fontlib_get (char* str, uint8_t* rec, FontSize f_size)
{
    int32_t c = -1;

    uint32_t points[strlen(str)]; // 按最坏的可能（每个字符都是ascii）来准备码点接收内存
    size_t count = utf8_to_points(str, points);
    // DUMP_MEM(points, count);
    printf("count: %d\n", count);

    uint8_t step_size;
    switch (f_size)
    {
        case FontSize_12: step_size = 25; // 1byte width description + 24 byte data
        break;
        case FontSize_16: step_size = 33; // 1byte width description + 32 byte data
        break;
        default:
            ESP_LOGE(LOG_TAG, "[fontlib_get] f_size 参数不正确！");
            goto done;
    }

    _Bool r;
    for (size_t i = 0; i < count; i++)
    {
        int32_t real_addr = get_real_addr(points[i], f_size);
        if (real_addr < 0) 
        { 
            goto done;
        }
        r = W25Q_FastRead(real_addr, (uint8_t*)(rec + i * step_size), step_size);
        if (!r)
        {
            ESP_LOGE(LOG_TAG, "[fontlib_get] falsh read failed!");
            goto done;
        }
    }

    c = count;

    done:
        return c;
}
