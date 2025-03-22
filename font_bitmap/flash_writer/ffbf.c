#include "ffbf.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define BLOCK_LEN 1024
#define OBJECT_LEN 16

int read_to_symbol (FILE* fd, uint8_t* bytes, char cha)
{	
	int len = 0;
	while (true)
	{
		uint32_t c = fgetc(fd);
		if (len > 50) 
		{
			len = -1;
			printf("太长了！");
			break;
		}
		if (feof(fd) != 0) break;
		if (c == cha) break;
		if (bytes != NULL) *(bytes + len) = c;
		len++;
	}
	return len;
}

static _Bool is_number (char c)
{
	for (int i = 0; i < 10; i++) 
		if (c == (48 + i)) return true;
	return false;
}

static void handle_data (FILE* fd, FFBF_Object* ffbf)
{	
	int c;
	uint8_t i = 0;
	uint8_t s[8];
	while (true)
	{
		c = fgetc(fd);
		if (c == ' ') continue;
		else if ((c == 13 || c == ',' || c == EOF) && i > 0)
		{
			if (c == 13)
			{
				c = fgetc(fd);
				if (feof(fd) != 0) break;
				if (c != 10) break;
			}

			uint64_t v; // NOTE NOTE  血泪！ 这个类型一定要给大，时间给到最大，不然sscanf捕获不到数据然后还会出发一些很奇怪的BUG
			sscanf(s, "%lld", &v);

            if (ffbf->len >= ffbf->mem_len) 
            { 
                ffbf->mem = realloc(ffbf->mem, ffbf->mem_len + BLOCK_LEN);
                if (ffbf->mem != NULL) ffbf->mem_len += BLOCK_LEN;
                else {
                    printf("内存扩容失败！\n");
                    break;
                }
            }
            
            ffbf->mem[ffbf->len++] = v & 0xff;
			printf("%lld ", v);

			i = 0;
			memset(s, 0, 8);
			
			if (c == EOF) break;
		}
		else if (is_number(c))
		{
			s[i] = c;
			i++;
		}
		else 
		{
			// printf("Oh shit! Don't come in here!\n");
			fseek(fd, -1, SEEK_CUR);
			break;
		}
	}
}

void ffbf_release_objects (FFBF_Object* objects)
{
    for (size_t i = 0; i < OBJECT_LEN; i++)
    {
        free(objects[i].mem);
    }
    free(objects);
}

FFBF_Object* ffbf (char* file_path)
{
    if (file_path == NULL) file_path = (char*)"../12x0ascii.ffbf";
    FILE* fd = fopen(file_path, "r");
	if (fd == NULL)
	{
		printf("open file failed: %s\n", file_path);
		return NULL;
	}

    FFBF_Object* ffbf_objects = (FFBF_Object*)malloc(sizeof(FFBF_Object) * OBJECT_LEN); // TODO 最大16，应该不会超过，需要注意
    memset(ffbf_objects, 0, sizeof(FFBF_Object) * OBJECT_LEN);
    int ffbf_objects_len = 0;

	int c;
	uint8_t s[51];
	int l;
	#define C() memset(s, 0, 51);

	while (true)
	{	
		c = fgetc(fd);
		if (c == EOF) break;
		
		if (c == '#')
		{	
			C();
			l = read_to_symbol(fd, s, 13);
			if (l > 0) printf("comment: %s\n", s);
		}
		else if (c == 'W')
		{
			C();
			fseek(fd, -1, SEEK_CUR);
			l = read_to_symbol(fd, s, 13);
			if (l > 0) printf("%s\n", s);
            ffbf_objects[ffbf_objects_len].mem = malloc(BLOCK_LEN * sizeof(uint8_t));
            ffbf_objects_len++;
		}
		else if (is_number(c))
		{
			fseek(fd, -1, SEEK_CUR);
			handle_data(fd, ffbf_objects + (ffbf_objects_len - 1));
		}
		else if (c == 13 || c == 10 || c == ' ')
		{
			continue;
		}
		else {
			printf("非法符号,进程退出！\n");
            ffbf_release_objects(ffbf_objects);
            ffbf_objects = NULL;
			break;
		}
	}
	fclose(fd);

    if (ffbf_objects != NULL)
    {
        for (size_t i = 0; i < OBJECT_LEN; i++)
        {
            if (ffbf_objects[i].mem == NULL) break;
            printf("\n i:%d - len:%d, mem_len:%d\n", i, ffbf_objects[i].len, ffbf_objects[i].mem_len);
        }
    }
    
    return ffbf_objects;
}