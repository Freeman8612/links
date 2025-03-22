/**
 * @file Flash.c
 * @brief 对W32QXXBV的读写底层封装，只管读写即可，不用管底层的分页，擦除等等问题
 * @author Freeman.Li
 * @date 2025/3/17
 */

#include "Flash.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "W25Q32BV.h"

static _Bool r;

#define CR(msg, return_v) if (!r) {\
        W25Q_ERR((msg));\
        return (return_v);\
    }

_Bool Flash_Write (size_t addr, uint8_t* pdata, size_t data_byte_len)
{
    if ((addr + data_byte_len) > (256 * 16 * 16 * 64))
    {
        W25Q_ERR("addr + data_byte_len overflow range of the chip!");
        return false;
    }

    #undef WRITE_SELECTOR
    #define WRITE_SELECTOR(ADDR, DATA) do {\
            W25Q_EnableWrite();\
            W25Q_Erase((ADDR), EraseSelector);\
            W25Q_WaitBusy();\
            /* NOTE 一个扇区是4K，原来整成了1k，导致BUG */\
            for (size_t i = 0; i < 16; i++) {\  
                W25Q_EnableWrite();\
                W25Q_PageProgram(ADDR + (256 * i), (uint8_t*)((DATA) + (256 * i)), 256);\
                W25Q_WaitBusy();\
            }\
        } while (0)
    
    #undef SELECTOR_LEN
    #define SELECTOR_LEN 4096

    size_t copyed_len = 0;
    size_t selector = addr / SELECTOR_LEN;
    size_t begin_pos = addr - SELECTOR_LEN * selector;
    uint8_t selector_data[SELECTOR_LEN] = { 0 };
    while (true)
    {
        if (copyed_len >= data_byte_len) break;

        size_t copy_len = SELECTOR_LEN - begin_pos;
        if (data_byte_len - copyed_len <= copy_len) 
        {
            copy_len = data_byte_len - copyed_len;
        }
        printf("copyed_len: %d, copy_len: %d\n", copyed_len, copy_len);
        printf("selector: %d\n", selector);
        r = W25Q_FastRead(selector * SELECTOR_LEN, selector_data, SELECTOR_LEN);
        if (!r) return false;
        memcpy((uint8_t*)(selector_data + begin_pos), (uint8_t*)(pdata + copyed_len), copy_len);

        WRITE_SELECTOR(selector * SELECTOR_LEN, selector_data);

        begin_pos = 0;
        copyed_len += copy_len;
        selector++;
    }

    return true;

    #undef WRITE_SELECTOR
    #undef SELECTOR_LEN
}   

_Bool Flash_Read (size_t addr, uint8_t* pdata, size_t read_byte_len)
{
    return W25Q_FastRead(addr, pdata, read_byte_len);
}