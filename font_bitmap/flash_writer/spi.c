#include "spi.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include <linux/spi/spidev.h>

static int r = -1;

static int spi_fd = -1;
static char* dev = "/dev/spidev0.0";
static uint32_t spi_hz = 10000 * 10;
static uint8_t spi_mode = SPI_MODE_0;
static uint8_t word_width = 8;

#define CR(msg) if (r < 0) {\
		printf((msg));\
		return false;\
	}

#define DUMP_MEM(MEM, LEN) do {\
		printf("DUMP_MEM ================================\n");\
		for (size_t i = 0; i < (LEN); i++) {\
			printf("[%d]0x%02x ", i, MEM[i]);\
		}\
		printf("\n");\
	} while (0)

void deinit_spi (void)
{
    close(spi_fd);
}

_Bool init_spi (void) 
{	
	spi_fd = open(dev, O_RDWR);
	r = spi_fd;
	CR("spi open failed!\n");

	r = ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode);
	CR("set spi mode failed!\n");

	// r = ioctl(spi_fd, SPI_IOC_RD_MODE, &spi_mode);
	// CR("set spi mode failed!\n");

	r = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &word_width);
	CR("set word width failed!\n");

	// r = ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &word_width);
	// CR("set word width failed!\n");

	r = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_hz);
	CR("set spi hz failed!");

	// r = ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_hz);
	// CR("set spi hz failed!");

	return true;
}

static _Bool spi_transmit (uint8_t* tx, uint8_t* rx, size_t byte_len)
{
	struct spi_ioc_transfer transfer = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = byte_len,
		.speed_hz = spi_hz,
		.bits_per_word = word_width,
		.delay_usecs = 0,
		.cs_change = 0,
	};
	r = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &transfer);
	CR("spi_transmit failed!");
	return true;
}

_Bool W25Q_SPI_TRANSMIT (const uint8_t* tx, uint8_t* rx, size_t transmit_byte_len)
{
    return spi_transmit(tx, rx, transmit_byte_len);
}