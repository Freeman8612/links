#include "W25Q32BV.h"
#include <stdio.h>
#include <string.h>
#include "Flash.h"

_Bool r = false;

#define DUMMY 0x00

#define CR(msg, return_v) if (!r) {\
        W25Q_ERR((msg));\
        return (return_v);\
    }

#define CR_G(msg) if (!r) {\
        W25Q_ERR((msg));\
        goto done;\
    }

#define DUMP_MEM(mem, len, title) {\
        printf("W25Q DUMP_MEM ================================= %s\n", (title));\
        for (size_t i = 0; i < (len); i++) {\
            printf("[%d]0x%02x ", i, *((uint8_t*)((mem) + i)));\
        }\
        printf("\n");\
    }

#define CMD_ManufacturerDeviceID 0x90
#define CMD_ReadUniqueID         0x48 
#define CMD_WriteEnable          0x06
#define CMD_WriteDisable         0x04
#define CMD_PageProgram          0x02
#define CMD_SectorErase          0x20 // 扇区清除,4Kbit
#define CMD_BlockErase           0xD8 // (64kb) A23–A16 A15–A8 A7–A0
#define CMD_ChipErase            0xC7 // C7h/60h都可以
#define CMD_Read                 0x03 // A23-A16 A15-A8 A7-A0 (D7-D0)
#define CMD_FastRead             0x0B // A23-A16 A15-A8 A7-A0 dummy (D7-D0)
#define CMD_ReadStatusRegister_1 0x05 // (S7–S0))
#define CMD_ReadStatusRegister_2 0x35 // (S15-S8)

/**
 * @brief 读取唯一ID
 * @param uid 
 * @param bool
 * @ref 7.2.4 Instruction Set Table 3 (ID, Security Instructions)
 */
_Bool W25Q_ReadUniqueID (uint8_t uid[8])
{
    uint8_t buf[13] = { CMD_ReadUniqueID, };
    uint8_t rec[13] = { 0 };
    r = W25Q_SPI_TRANSMIT(buf, rec, 13);
    CR("[W25q_ReadUniqueID] spi!", false);
    memcpy(uid, (uint8_t*)(rec + 5), 8);
    return true;
}

/**
 * @brief  获取制造商代码和ID
 * @param  MF_ID 前1byte是制造商，后一字节是ID
 * @return bool
 * @ref    7.2.1 Manufacturer and Device Identification
 */
_Bool W25Q_ManufacturerAndID (uint8_t MF_ID[2])
{
    uint8_t buf[6] = { CMD_ManufacturerDeviceID, };
    uint8_t rec[6] = { 0 };
    r = W25Q_SPI_TRANSMIT(buf, rec, 6);
    CR("[W25Q_ManufacturerAndID] spi!", false);
    memcpy(MF_ID, (uint8_t*)(rec + 4), 2);
    return true;
}

/**
 * @brief 写使能
 */
_Bool W25Q_EnableWrite (void)
{
    uint8_t buf[1] = { CMD_WriteEnable };
    r = W25Q_SPI_TRANSMIT(buf, NULL, 1);
    CR("[W25Q_EnableWrite] spi!", false);
    return true;
}

/**
 * @brief 写失能
 */
_Bool W25Q_DisableWrite (void)
{
    uint8_t buf[1] = { CMD_WriteDisable };
    r = W25Q_SPI_TRANSMIT(buf, NULL, 1);
    CR("[W25Q_DisableWrite] spi!", false);
    return true;
}


_Bool W25Q_ReadStatusRegisters (uint8_t res[2])
{
    uint8_t rec_buf[2] = { 0x00 };
    r = W25Q_SPI_TRANSMIT((uint8_t[2]){ CMD_ReadStatusRegister_1 }, rec_buf, 2);
    CR("[W25Q_ReadStatusRegisters] spi1!", false);
    res[0] = rec_buf[1];
    r = W25Q_SPI_TRANSMIT((uint8_t[2]){ CMD_ReadStatusRegister_2 }, rec_buf, 2);
    CR("[W25Q_ReadStatusRegisters] spi2!", false);
    res[1] = rec_buf[1];
    return true;
}

/**
 * NOTE 全片擦除实测要好几秒到十秒
 */
_Bool W25Q_Erase (__attribute__((unused)) uint32_t addr, EraseAction action)
{
    uint8_t buf[4];
    size_t buf_len = 0;

    switch (action)
    {
        case EraseSelector:
            memcpy(buf, (uint8_t[]){ CMD_SectorErase, addr >> 16, addr >> 8, addr & 0xff }, 4);
            buf_len = 4;
        break;

        case EraseBlock:
            memcpy(buf, (uint8_t[]){ CMD_BlockErase, addr >> 16, addr >> 8, addr & 0xff }, 4);
            buf_len = 4;
        break;

        case EraseChip:
            buf[0] = CMD_ChipErase;
            buf_len = 1;
        break;
    }
    r = W25Q_SPI_TRANSMIT(buf, NULL, buf_len);
    CR("[W25Q_EraseSeletor] spi!", false);
    return true;
}

_Bool W25Q_PageProgram (uint32_t addr, uint8_t* data, size_t data_byte_len)
{
    if (data_byte_len > 256)
    {
        W25Q_ERR("[W25Q_PageProgram] param data_byte_len overflow range!");
        return false;
    }
    uint8_t tx_buf[256 + 4] = { 0 };
    tx_buf[0] = CMD_PageProgram;
    tx_buf[1] = addr >> 16;
    tx_buf[2] = addr >> 8;
    tx_buf[3] = addr & 0xff;
    memcpy((uint8_t*)(tx_buf + 4), data, data_byte_len);
    r = W25Q_SPI_TRANSMIT(tx_buf, NULL, data_byte_len + 4);
    CR("[W25Q_PageProgram] spi!", false);
    return true;
}

_Bool W25Q_Read (uint32_t addr, uint8_t* data, size_t read_byte_len)
{
    if (read_byte_len > 4096 - 4)
    {
        W25Q_ERR("[W25Q_ReadData] read_byte_len超过单个transfer大小！");
        goto done;
    }
    _Bool s = false;
    uint8_t* rec_buf = (uint8_t*)malloc(sizeof(uint8_t) * read_byte_len + 4);
    r = W25Q_SPI_TRANSMIT((uint8_t[]){ CMD_Read, addr >> 16, addr >> 8, addr & 0xff }, rec_buf, read_byte_len + 4);
    CR_G("[W25Q_ReadData] spi!");
    memcpy(data, (uint8_t*)(rec_buf + 4), read_byte_len);
    s = true;
    done:
        free(rec_buf);
        return s;
}

_Bool W25Q_FastRead (uint32_t addr, uint8_t* pdata, size_t read_byte_len)
{
    // _Bool s = false;
    // uint8_t* buf = (uint8_t*)malloc(sizeof(uint8_t) * read_byte_len + 5);
    // r = W25Q_SPI_TRANSMIT((uint8_t[]){ CMD_FastRead, addr >> 16, addr >> 8, addr & 0xff, DUMMY }, buf, read_byte_len + 5);
    // CR_G("[W25Q_FastRead] spi!");
    // memcpy(pdata, (uint8_t*)(buf + 5), read_byte_len);
    // s = true;
    // done:
    //     free(buf);
    //     return s;

    // 实测linux上单个transfer最大为4096，所以分块处理

    _Bool s = false;

    const size_t block_size = 4090; // 顶着上限干
    const size_t block_num = read_byte_len / block_size + 1;
    uint8_t buf[block_size];
    size_t i = 0;

    for (; i < block_num; i++)
    {
        size_t copy_len;
        if ((read_byte_len - i * block_size) >= block_size) copy_len = block_size;
        else copy_len = read_byte_len - i * block_size;

        // printf(">>> addr: %d, copy_len: %d\n", addr + i * block_size, copy_len);

        size_t addr_ = addr + i * block_size;
        r = W25Q_SPI_TRANSMIT((uint8_t[]){ CMD_FastRead, addr_ >> 16, addr_ >> 8, addr_ & 0xff, DUMMY }, buf, 5 + copy_len);
        CR_G("[W25Q_FastRead] spi!");

        memcpy(pdata + i * block_size, buf + 5, copy_len);
    }

    s = true;
    done:
        return s;
}

void W25Q_WaitBusy (void)
{
    size_t i = 0;
    uint8_t res[2];
    while (true)
    {
        i++;
        W25Q_ReadStatusRegisters(res);
        if (res[0] & 0x01) continue;
        break;
    }
    // printf("W25Q_WaitBusy count: %d\n", i);
}

void W25Q_Test (void) 
{
    printf("hello!\n");
    uint8_t rec[1024 * 8] = { 0 };
    memset(rec, 2, 1024 * 8);

    // W25Q_EnableWrite();
    // W25Q_Erase(0, EraseChip);

    Flash_Write(0x05, rec, 1024 * 8);

    W25Q_FastRead(0, rec, 5);
    DUMP_MEM(rec, 5, "W25Q_ReadData 0");

    W25Q_FastRead(1 * 256, rec, 8);
    DUMP_MEM(rec, 8, "W25Q_ReadData 1");

    W25Q_FastRead(2 * 256, rec, 8);
    DUMP_MEM(rec, 8, "W25Q_ReadData 2");

    W25Q_FastRead(3 * 256, rec, 8);
    DUMP_MEM(rec, 8, "W25Q_ReadData 3");

    W25Q_FastRead(4 * 256, rec, 8);
    DUMP_MEM(rec, 8, "W25Q_ReadData 4");

    W25Q_FastRead(32 * 256, rec, 16);
    DUMP_MEM(rec, 16, "W25Q_ReadData 32 * 256");
}
