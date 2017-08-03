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


#include "spi.h"

#define SPI_NO HSPI // Define spi mode SPI_NO: HSPI=HW SPI, SPI=SW SPI
#define SPI_GPIO_CS

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define spi_busy(SPI_NO) READ_PERI_REG(SPI_CMD(SPI_NO))&SPI_USR

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_mode
//   Description: Configures SPI mode parameters for clock edge and clock polarity.
//    Parameters: SPI_NO - SPI (0) or HSPI (1)
//				  spi_cpha - (0) Data is valid on clock leading edge
//				             (1) Data is valid on clock trailing edge
//				  spi_cpol - (0) Clock is low when inactive
//				             (1) Clock is high when inactive
//
////////////////////////////////////////////////////////////////////////////////

void ICACHE_FLASH_ATTR spi_mode(uint8_t spi_cpha,uint8_t spi_cpol){
	if(spi_cpha) {
		CLEAR_PERI_REG_MASK(SPI_USER(SPI_NO), SPI_CK_OUT_EDGE);
	} else {
		SET_PERI_REG_MASK(SPI_USER(SPI_NO), SPI_CK_OUT_EDGE);
	}

	if (spi_cpol) {
		SET_PERI_REG_MASK(SPI_PIN(SPI_NO), SPI_IDLE_EDGE);
	} else {
		CLEAR_PERI_REG_MASK(SPI_PIN(SPI_NO), SPI_IDLE_EDGE);
	}
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_init_gpio
//   Description: Initialises the GPIO pins for use as SPI pins.
//    Parameters: sysclk_as_spiclk - SPI_CLK_80MHZ_NODIV (1) if using 80MHz
//									 sysclock for SPI clock. 
//									 SPI_CLK_USE_DIV (0) if using divider to
//									 get lower SPI clock speed.
//				 
////////////////////////////////////////////////////////////////////////////////

static void ICACHE_FLASH_ATTR spi_init_gpio(uint8_t sysclk_as_spiclk){

//	if(SPI_NO > 1) return; //Not required. Valid SPI_NO is checked with if/elif below.

	uint32_t clock_div_flag = 0;
	if(sysclk_as_spiclk){
		clock_div_flag = 0x0001;	
	} 

	if(SPI_NO==SPI){
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x005|(clock_div_flag<<8)); //Set bit 8 if 80MHz sysclock required
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, 1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, 1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, 1);	
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, 1);	
	}else if(SPI_NO==HSPI){
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105|(clock_div_flag<<9)); //Set bit 9 if 80MHz sysclock required
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2); //GPIO12 is HSPI MISO pin (Master Data In)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2); //GPIO14 is HSPI CLK pin (Clock)
#ifndef SPI_GPIO_CS
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2); //GPIO15 is HSPI CS pin (Chip Select / Slave Select)
#else
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15); //GPIO15 config as GPIO for sw Chip Select
        GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, 1<<15); // GPIO15 config as output
#endif
	}
}

////////////////////////////////////////////////////////////////////////////////

void spi_setCS(){
#ifdef SPI_GPIO_CS
    GPIO_REG_WRITE((GPIO_OUT_W1TC_ADDRESS), (1<<15)); // (pin 15)
#endif
}

void spi_clearCS(){
#ifdef SPI_GPIO_CS
    GPIO_REG_WRITE(((GPIO_OUT_W1TS_ADDRESS)), (1<<15)); // (pin 15)
#endif
}

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_clock
//   Description: sets up the control registers for the SPI clock
//    Parameters: SPI_NO - SPI (0) or HSPI (1)
//				  prediv - predivider value (actual division value)
//				  cntdiv - postdivider value (actual division value)
//				  Set either divider to 0 to disable all division (80MHz sysclock)
//				 
////////////////////////////////////////////////////////////////////////////////

static void ICACHE_FLASH_ATTR spi_clock(uint16_t prediv, uint8_t cntdiv){
	
	if(SPI_NO > 1) return;

	if((prediv==0)|(cntdiv==0)){

		WRITE_PERI_REG(SPI_CLOCK(SPI_NO), SPI_CLK_EQU_SYSCLK);

	} else {
	
		WRITE_PERI_REG(SPI_CLOCK(SPI_NO), 
					(((prediv-1)&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S)|
					(((cntdiv-1)&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)|
					(((cntdiv>>1)&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)|
					((0&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S));
	}

}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_tx_byte_order
//   Description: Setup the byte order for shifting data out of buffer
//    Parameters: SPI_NO - SPI (0) or HSPI (1)
//				  byte_order - SPI_BYTE_ORDER_HIGH_TO_LOW (1) 
//							   Data is sent out starting with Bit31 and down to Bit0
//
//							   SPI_BYTE_ORDER_LOW_TO_HIGH (0)
//							   Data is sent out starting with the lowest BYTE, from 
//							   MSB to LSB, followed by the second lowest BYTE, from
//							   MSB to LSB, followed by the second highest BYTE, from
//							   MSB to LSB, followed by the highest BYTE, from MSB to LSB
//							   0xABCDEFGH would be sent as 0xGHEFCDAB
//
//				 
////////////////////////////////////////////////////////////////////////////////

static void ICACHE_FLASH_ATTR spi_tx_byte_order(uint8_t byte_order){

	if(SPI_NO > 1) return;

	if(byte_order){
		SET_PERI_REG_MASK(SPI_USER(SPI_NO), SPI_WR_BYTE_ORDER);
	} else {
		CLEAR_PERI_REG_MASK(SPI_USER(SPI_NO), SPI_WR_BYTE_ORDER);
	}
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_rx_byte_order
//   Description: Setup the byte order for shifting data into buffer
//    Parameters: SPI_NO - SPI (0) or HSPI (1)
//				  byte_order - SPI_BYTE_ORDER_HIGH_TO_LOW (1) 
//							   Data is read in starting with Bit31 and down to Bit0
//
//							   SPI_BYTE_ORDER_LOW_TO_HIGH (0)
//							   Data is read in starting with the lowest BYTE, from 
//							   MSB to LSB, followed by the second lowest BYTE, from
//							   MSB to LSB, followed by the second highest BYTE, from
//							   MSB to LSB, followed by the highest BYTE, from MSB to LSB
//							   0xABCDEFGH would be read as 0xGHEFCDAB
//
//				 
////////////////////////////////////////////////////////////////////////////////

static void ICACHE_FLASH_ATTR spi_rx_byte_order(uint8_t byte_order){

	if(SPI_NO > 1) return;

	if(byte_order){
		SET_PERI_REG_MASK(SPI_USER(SPI_NO), SPI_RD_BYTE_ORDER);
	} else {
		CLEAR_PERI_REG_MASK(SPI_USER(SPI_NO), SPI_RD_BYTE_ORDER);
	}
}
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Function Name: spi_init
//   Description: Wrapper to setup HSPI/SPI GPIO pins and default SPI clock
//    Parameters: spi_no - SPI (0) or HSPI (1)
//				 
////////////////////////////////////////////////////////////////////////////////

void ICACHE_FLASH_ATTR spi_init(){
	
	if(SPI_NO > 1) return; //Only SPI and HSPI are valid spi modules. 

	spi_init_gpio(SPI_CLK_USE_DIV);
	spi_clock(SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
	spi_tx_byte_order(SPI_BYTE_ORDER_HIGH_TO_LOW);
	spi_rx_byte_order(SPI_BYTE_ORDER_HIGH_TO_LOW); 

	SET_PERI_REG_MASK(SPI_USER(SPI_NO), SPI_CS_SETUP|SPI_CS_HOLD);
	CLEAR_PERI_REG_MASK(SPI_USER(SPI_NO), SPI_FLASH_MODE);

}

////////////////////////////////////////////////////////////////////////////////


/* @defgroup SPI hardware implementation
 * @brief spi_transfer32()
 *
 * SPI transfer is based on a simultaneous send and receive:
 * the received data is returned.
 */
uint32_t ICACHE_FLASH_ATTR spi_transfer32(uint32_t data, uint8_t bits)
{
	uint32_t regvalue = 0;

	while(READ_PERI_REG(SPI_CMD(SPI_NO))&SPI_USR);

	regvalue |=  SPI_USR_MOSI | SPI_DOUTDIN | SPI_CK_I_EDGE;
	regvalue &= ~(BIT2 | SPI_USR_ADDR | SPI_USR_DUMMY | SPI_USR_MISO | SPI_USR_COMMAND); //clear bit 2 see example IoT_Demo
	WRITE_PERI_REG(SPI_USER(SPI_NO), regvalue);


	WRITE_PERI_REG(SPI_USER1(SPI_NO),
			( (bits-1 & SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S ) |
			( (bits-1 & SPI_USR_MISO_BITLEN) << SPI_USR_MISO_BITLEN_S ) );

	// copy data to W0
	if(READ_PERI_REG(SPI_USER(SPI_NO))&SPI_WR_BYTE_ORDER) {
		WRITE_PERI_REG(SPI_W0(SPI_NO), data<<(32-bits));
	} else {
 		WRITE_PERI_REG(SPI_W0(SPI_NO), data);
	}

	SET_PERI_REG_MASK(SPI_CMD(SPI_NO), SPI_USR);   // send

	while (READ_PERI_REG(SPI_CMD(SPI_NO)) & SPI_USR);

	// wait a while before reading the register into the buffer
//	delayMicroseconds(2);

	if(READ_PERI_REG(SPI_USER(SPI_NO))&SPI_RD_BYTE_ORDER) {
		return READ_PERI_REG(SPI_W0(SPI_NO)) >> (32-bits); //Assuming data in is written to MSB. TBC
	} else {
		return READ_PERI_REG(SPI_W0(SPI_NO)); //Read in the same way as DOUT is sent. Note existing contents of SPI_W0 remain unless overwritten!
	}

}


/* @defgroup SPI hardware implementation
 * @brief spi_transfer(uint8_t *buffer, size_t numberBytes)
 *
 * SPI transfer is based on a simultaneous send and receive:
 * The buffered transfers does split up the conversation internaly into 64 byte blocks.
 * The received data is stored in the buffer passed by reference.
 * (the data past in is replaced with the data received).
 *
 * 		spi_transfer(buffer, size)				: memory buffer of length size
 */
void ICACHE_FLASH_ATTR spi_transfer(uint8_t *buffer, size_t numberBytes) {

#define BLOCKSIZE 64		// the max length of the ESP SPI_W0 registers

	uint16_t bufIndx = 0;
	uint8_t bufLenght = 0;
	uint8_t num_bits = 0;

	int blocks = ((numberBytes -1)/BLOCKSIZE)+1;
	int total = blocks;

	// loop number of blocks
	while  (blocks--) {

		// get full BLOCKSIZE or number of remaining bytes

#ifdef SPI_DEBUG
		debugf("Write/Read Block %d total %d bytes", total-blocks, bufLenght);
#endif

		// compute the number of bits to clock
		num_bits = bufLenght * 8;

		uint32_t regvalue = 0;

		while(READ_PERI_REG(SPI_CMD(SPI_NO))&SPI_USR);

		regvalue |=  SPI_USR_MOSI | SPI_DOUTDIN | SPI_CK_I_EDGE;
		regvalue &= ~(BIT2 | SPI_USR_ADDR | SPI_USR_DUMMY | SPI_USR_MISO | SPI_USR_COMMAND); //clear bit 2 see example IoT_Demo
		WRITE_PERI_REG(SPI_USER(SPI_NO), regvalue);


		// setup bit lenght
		WRITE_PERI_REG(SPI_USER1(SPI_NO),
				( (num_bits-1 & SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S ) |
				( (num_bits-1 & SPI_USR_MISO_BITLEN) << SPI_USR_MISO_BITLEN_S ) );

		// copy the registers starting from last index position
		memcpy ((void *)SPI_W0(SPI_NO), &buffer[bufIndx], bufLenght);

		// Begin SPI Transaction
		SET_PERI_REG_MASK(SPI_CMD(SPI_NO), SPI_USR);

		// wait for SPI bus to be ready
		while(READ_PERI_REG(SPI_CMD(SPI_NO))&SPI_USR);

		// copy the registers starting from last index position
		memcpy (&buffer[bufIndx], (void *)SPI_W0(SPI_NO),  bufLenght);

		// increment bufIndex
		bufIndx += bufLenght;
	}
}

