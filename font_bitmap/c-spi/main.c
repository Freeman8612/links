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
#include "fontlib.h"

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

static void font_write (size_t addr, char* file_path)
{
	FFBF_Object* ffbfs = ffbf(file_path);
	if (ffbfs == NULL)
	{
		printf("ffbd == NULL!");
		return;
	}
	
	r = init_spi();
	CR("init spi failed!");

	W25Q_EnableWrite();
	W25Q_Erase(0, EraseChip);
	W25Q_WaitBusy();

	size_t start_addr = addr;
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
		return;
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

static void font_read (uint64_t start_addr, uint64_t read_count, uint8_t size)
{
	int char_step = 0;

	if (size == 12) char_step = 25;
	else if (size == 16) char_step = 33;
	else {
		printf("size 只能是12或者16!\n");
		return;
	}

	uint8_t buf[2000000]= { 0 };

	r = Flash_Read(start_addr, buf, read_count * char_step);
	CR("flash read failed!");

	for (size_t i = 0; i < read_count; i++)
	{
		printf_font((buf + i * char_step) + 1, size);
		usleep(1000 * 333);
	}
	// DUMP_MEM(buf, 32);

	done:
		printf("end123\n");
		deinit_spi();
		return;
}

void printf_help (char* msg)
{
	printf("%s\n", msg);
	printf("========= command help ====================================\n");
	printf("	read  <start_addr> <read_count> <size>  # size只能为12或者16\n");
	printf("	write <write_addr> <file_path>\n");
	printf("	show  <strings>    <size>               # size只能位12或16\n");
}

int main (int argc, char** argv)
{	
	int8_t r = init_spi();
	if (r < 0)
	{
		printf("SPI init failed!\n");
	}
	

	if (argc < 1 + 2) 
	{
		printf_help("paramer error!\n");
		return 0;
	}

	#define C() if (*endptr != '\0') {\
			printf_help("read 参数不正确!\n");\
			return 0;\
		}

	if (strcmp(*(argv + 1), "read") == 0)
	{
		if (argc < 5) 
		{
			printf_help("read 参数数量不足！\n");
			return 0;
		}

		char* endptr = NULL;
		uint64_t start_addr = strtoull(*(argv + 2), &endptr, 10);
		C();
		uint64_t read_count = strtoull(*(argv + 3), &endptr, 10);
		C()
		uint64_t size       = strtoull(*(argv + 4), &endptr, 10);
		C();

		printf("%llu, %llu, %llu\n", start_addr, read_count, size);
		font_read(start_addr, read_count, size);
	}
	else if (strcmp(*(argv + 1), "write") == 0)
	{
		char* endptr = NULL;
		uint64_t start_addr = strtoull(*(argv + 2), &endptr, 10);
		C();
		font_write(start_addr, *(argv + 3));
	}
	else if (strcmp(*(argv + 1), "show") == 0) // 展示输入的中文
	{
		if (argc != 4)
		{
			printf_help("show 参数数量不足\n！");
			return 0;
		}

		uint8_t rec[1024 * 1] = { 0x00 };
		uint8_t font_size = strtoul(*(argv + 3), NULL, 10);
		uint8_t char_step = 0;
		if (font_size == 12)
		{
			char_step = 24 + 1;
		}
		else if (font_size == 16)
		{
			char_step = 32 + 1;
		}
		else 
		{
			printf_help("show size参数不正确!\n");
			return 0;
		}
		int32_t count = fontlib_get(*(argv + 2), rec, font_size == 12 ? FontSize_12 : FontSize_16);
		for (size_t i = 0; i < count; i++)
		{
			printf_font(rec + i * char_step + 1, font_size);
			usleep(1000 * 333);
		}
	}
	else 
	{
		printf_help("command invalidate!\n");
	}

	// font_write("../font_bitmap.ffbf");
	return 0;	
}