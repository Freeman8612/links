#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "Flash.h"
#include "spi.h"
#include "ffbf.h"
#include "W25Q32BV.h"

static _Bool r;

#define CR(msg) if (!r) {\
		printf((msg));\
		goto done;\
	}

#define DUMP_MEM(MEM, LEN) do {\
		printf("DUMP_MEM ================================\n");\
		for (size_t i = 0; i < (LEN); i++) {\
			printf("<%d>%d ", i, MEM[i]);\
		}\
		printf("\n");\
	} while (0)

void W25Q_ERR(char* msg)
{
	printf("[ ERROR ]: %s\n", msg);
}

void W25Q_DELAY(uint16_t delay_ms)
{
	usleep(delay_ms * 1000); // 毫秒要变成微妙
}

static void font_write (char* file_path)
{
	r = init_spi();
	CR("init spi failed!");

	W25Q_EnableWrite();
	W25Q_Erase(0, EraseChip);
	W25Q_WaitBusy();

	FFBF_Object* ffbfs = ffbf(file_path);
	if (ffbfs == NULL)
	{
		printf("ffbd == NULL!");
		return 0;
	}

	size_t start_addr = 0;
	while (true)
	{
		FFBF_Object ffbf = *(ffbfs++);
		if (ffbf.mem == NULL) break;
		printf("开始地址: %u, 数据长度: %d, 内存扩容长度: %d\n", start_addr, ffbf.len, ffbf.mem_len);

		_Bool r = Flash_Write(start_addr, ffbf.mem, ffbf.len);
		W25Q_WaitBusy();
		if (!r) 
		{
			printf("写入出错，退出!");
			break;
		}
		start_addr += ffbf.len;
	}

	done:
		printf("end\n");
		deinit_spi();
		return 0;
}

static void printf_font (uint8_t* data, size_t size)
{
	printf("\033[%d;%dH", 1, 1);
	printf("\033[J");

	for (int i = 0; i < size; i++) // y
	{
		for (int j = 0; j < size; j++) // x
		{
			int pos = (i / 8) * size + j;
			int left_move = i % 8;
			_Bool p = data[pos] & (1 << left_move);
			printf(p ? "\033[31m%s\033[0m" : "%s", p ? "x " : "o ");
		}
		printf("\n");
	}
}

static void font_read (void)
{
	r = init_spi();
	CR("init spi failed!");

	size_t len = 100;
	uint8_t buf[2000000]= { 0 };
	r = Flash_Read(531575, buf, len * 25);
	CR("flash read failed!");

	for (size_t i = 0; i < len; i++)
	{
		printf_font((buf + i * 25) + 1, 12);
		usleep(1000 * 333);
	}
	// DUMP_MEM(buf, 32);

	done:
		printf("end123\n");
		deinit_spi();
		return 0;
}

int main (int argc, char** argv)
{	
	// font_write("../font_bitmap.ffbf");
	font_read();
	return 0;	
}