#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

_Bool Flash_Write (size_t addr, uint8_t* pdata, size_t data_byte_len);

_Bool Flash_Read (size_t addr, uint8_t* pdata, size_t read_byte_len);

#endif // #ifndef __FLASH_H__