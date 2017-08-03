/*
* The MIT License (MIT)
* 
* Copyright (c) 2015 David Ogilvy (MetalPhreak)
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef SPI_APP_H
#define SPI_APP_H

#include "spi_register.h"
// #include "ets_sys.h" // Had to comment out to use with SmingCore
#include "osapi.h"
#include "os_type.h"

//Define SPI hardware modules
#define SPI 0
#define HSPI 1
#define HSPICS 2

#define SPI_CLK_USE_DIV 0
#define SPI_CLK_80MHZ_NODIV 1

#define SPI_BYTE_ORDER_HIGH_TO_LOW 1
#define SPI_BYTE_ORDER_LOW_TO_HIGH 0

#ifndef CPU_CLK_FREQ //Should already be defined in eagle_soc.h
#define CPU_CLK_FREQ 80*1000000
#endif

//Define some default SPI clock settings
#define SPI_CLK_PREDIV 10
#define SPI_CLK_CNTDIV 2
#define SPI_CLK_FREQ CPU_CLK_FREQ/(SPI_CLK_PREDIV*SPI_CLK_CNTDIV) // 80 / 20 = 4 MHz

void spi_init();
void spi_mode(uint8 spi_cpha,uint8 spi_cpol);

void spi_setCS();
void spi_clearCS();
uint32_t spi_transfer32(const uint32_t val, const uint8_t bits);

/** @brief 	spi_transfer(uint8 *buffer, size_t numberBytes)
 * @param	buffer in/out
 * @param	numberBytes lenght of buffer
 *
 * SPI transfer is based on a simultaneous send and receive:
 * The buffered transfers does split up the conversation internaly into 64 byte blocks.
 * The received data is stored in the buffer passed by reference.
 * (the data past in is replaced with the data received).
 *
 * 		spi_transfer(buffer, size)				: memory buffer of length size
 */
void spi_transfer(uint8_t * buffer, size_t numberBytes);

//Expansion Macros
#define spi_tx8(data) (uint8_t)spi_transfer32((uint32_t)data, 8)
#define spi_tx16(data) (uint16_t)spi_transfer32((uint32_t)data, 16)
#define spi_tx32(data) (uint32_t)spi_transfer32((uint32_t)data, 32)

#endif

