#ifndef __W25QXXBV_H__
#define __W25QXXBV_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// ==============  需要外部提供的函数, 供W25Q库通过SPI传输数据 ==============================>>
extern _Bool W25Q_SPI_TRANSMIT (const uint8_t* tx, uint8_t* rx, size_t transmit_byte_len);
extern void W25Q_ERR(char*);
extern void W25Q_DELAY(uint16_t delay_ms);
// ==============  需要外部提供的函数, 供W25Q库通过SPI传输数据 ==============================<<

typedef enum {
    EraseSelector = 1,
    EraseBlock,
    EraseChip,
} EraseAction;

void W25Q_Test (void);

void W25Q_WaitBusy (void);

_Bool W25Q_ReadUniqueID (uint8_t uid[8]);

_Bool W25Q_ManufacturerAndID (uint8_t MF_ID[2]);

_Bool W25Q_EnableWrite (void);

_Bool W25Q_DisableWrite (void);

_Bool W25Q_ReadStatusRegisters (uint8_t res[2]);

_Bool W25Q_Erase (__attribute__((unused)) uint32_t addr, EraseAction action);

_Bool W25Q_PageProgram (uint32_t addr, uint8_t* data, size_t data_byte_len);

_Bool W25Q_Read (uint32_t addr, uint8_t* data, size_t read_byte_len);

_Bool W25Q_FastRead (uint32_t addr, uint8_t* pdata, size_t read_byte_len);

#endif // #ifndef __W25QXXBV_H__
