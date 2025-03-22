#ifndef __FFBF_H__
#define __FFBF_H__

#include <stdlib.h>
#include <stdint.h>

typedef struct {
    size_t len;
    size_t mem_len;
    uint8_t* mem;
} FFBF_Object;

FFBF_Object* ffbf (char* file_path);

void ffbf_release_objects (FFBF_Object* objects);


#endif // #define __FFBF_H__