#ifndef __FONTLIB_H__
#define __FONTLIB_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    FontSize_12 = 1,
    FontSize_16,
} FontSize;

int32_t fontlib_get (char* str, uint8_t* rec, FontSize f_size);

#endif // #ifndef __FONTLIB_H__
