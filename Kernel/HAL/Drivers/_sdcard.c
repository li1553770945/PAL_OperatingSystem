//#include "include/printf.h"
//#include "include/types.h"
//#include "include/riscv.h"
//#include "include/gpiohs.h"
//#include "include/buf.h"
//#include "include/spinlock.h"
//
//#include "include/dmac.h"
//#include "include/spi.h"
//#include "include/sdcard.h"

#include <HAL/Drivers/_sdcard.h>

extern "C"
{
	#include <HAL/Drivers/DriverTools.h>
	#include <HAL/Drivers/_gpiohs.h>
	#include <HAL/Drivers/_spi.h>
};
	
#include <Process/Synchronize.hpp>

#include <Library/Kout.hpp>
using namespace POS;

#define NULL 0

void SD_CS_HIGH(void) {
    gpiohs_set_pin(7, GPIO_PV_HIGH);
}

void SD_CS_LOW(void) {
    gpiohs_set_pin(7, GPIO_PV_LOW);
}

void SD_HIGH_SPEED_ENABLE(void) {
    // spi_set_clk_rate(SPI_DEVICE_0, 10000000);
}

static void sd_lowlevel_init(Uint8 spi_index) {
    gpiohs_set_drive_mode(7, GPIO_DM_OUTPUT);
    // spi_set_clk_rate(SPI_DEVICE_0, 200000);     /*set clk rate*/
}

static void sd_write_data(Uint8 const *data_buff, Uint32 length) {
    spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_send_data_standard(SPI_DEVICE_0, SPI_CHIP_SELECT_3, NULL, 0, data_buff, length);
}

static void sd_read_data(Uint8 *data_buff, Uint32 length) {
    spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
    spi_receive_data_standard(SPI_DEVICE_0, SPI_CHIP_SELECT_3, NULL, 0, data_buff, length);
}

static void sd_write_data_dma(Uint8 const *data_buff, Uint32 length) {
    spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
	spi_send_data_standard_dma(DMAC_CHANNEL0, SPI_DEVICE_0, SPI_CHIP_SELECT_3, NULL, 0, data_buff, length);
}

static void sd_read_data_dma(Uint8 *data_buff, Uint32 length) {
    spi_init(SPI_DEVICE_0, SPI_WORK_MODE_0, SPI_FF_STANDARD, 8, 0);
	spi_receive_data_standard_dma((dmac_channel_number_t)-1, DMAC_CHANNEL0, SPI_DEVICE_0, SPI_CHIP_SELECT_3, NULL, 0, data_buff, length);
}

/*
 * @brief  Send 5 bytes command to the SD card.
 * @param  Cmd: The user expected command to send to SD card.
 * @param  Arg: The command argument.
 * @param  Crc: The CRC.
 * @retval None
 */
static void sd_send_cmd(Uint8 cmd, Uint32 arg, Uint8 crc) {
	Uint8 frame[6];
	frame[0] = (cmd | 0x40);
	frame[1] = (Uint8)(arg >> 24);
	frame[2] = (Uint8)(arg >> 16);
	frame[3] = (Uint8)(arg >> 8);
	frame[4] = (Uint8)(arg);
	frame[5] = (crc);
	SD_CS_LOW();
	sd_write_data(frame, 6);
}

static void sd_end_cmd(void) {
	Uint8 frame[1] = {0xFF};
	/*!< SD chip select high */
	SD_CS_HIGH();
	/*!< Send the Cmd bytes */
	sd_write_data(frame, 1);
}

/*
 * Be noticed: all commands & responses below 
 * 		are in SPI mode format. May differ from 
 * 		what they are in SD mode. 
 */

#define SD_CMD0 	0 
#define SD_CMD8 	8
#define SD_CMD58 	58 		// READ_OCR
#define SD_CMD55 	55 		// APP_CMD
#define SD_ACMD41 	41 		// SD_SEND_OP_COND
#define SD_CMD16 	16 		// SET_BLOCK_SIZE 
#define SD_CMD17 	17 		// READ_SINGLE_BLOCK
#define SD_CMD24 	24 		// WRITE_SINGLE_BLOCK 
#define SD_CMD13 	13 		// SEND_STATUS

/*
 * Read sdcard response in R1 type. 
 */
static Uint8 sd_get_response_R1(void) {
	Uint8 result;
	Uint16 timeout = 0xff;

	while (timeout--) {
		sd_read_data(&result, 1);
		if (result != 0xff)
			return result;
	}

	// timeout!
	return 0xff;
}

/* 
 * Read the rest of R3 response 
 * Be noticed: frame should be at least 4-byte long 
 */
static void sd_get_response_R3_rest(Uint8 *frame) {
	sd_read_data(frame, 4);
}

/* 
 * Read the rest of R7 response 
 * Be noticed: frame should be at least 4-byte long 
 */
static void sd_get_response_R7_rest(Uint8 *frame) {
	sd_read_data(frame, 4);
}

static int switch_to_SPI_mode(void) {
	int timeout = 0xff;

	while (--timeout) {
		sd_send_cmd(SD_CMD0, 0, 0x95);
		Uint64 result = sd_get_response_R1();
		sd_end_cmd();

		if (0x01 == result) break;
	}
	if (0 == timeout) {
//		printf("SD_CMD0 failed\n");
		kout[Error]<<"SDCard: SD_CMD0 failed!"<<endl;
		return 0xff;
	}

	return 0;
}

// verify supply voltage range 
static int verify_operation_condition(void) {
	Uint64 result;

	// Stores the response reversely. 
	// That means 
	// frame[2] - VCA 
	// frame[3] - Check Pattern 
	Uint8 frame[4];

	sd_send_cmd(SD_CMD8, 0x01aa, 0x87);
	result = sd_get_response_R1();
	sd_get_response_R7_rest(frame);
	sd_end_cmd();

	if (0x09 == result) {
//		printf("invalid CRC for CMD8\n");
		kout[Error]<<"SDCard: Invalid CRC for CMD8"<<endl;
		return 0xff;
	}
	else if (0x01 == result && 0x01 == (frame[2] & 0x0f) && 0xaa == frame[3]) {
		return 0x00;
	}

//	printf("verify_operation_condition() fail!\n");
	kout[Error]<<"SDCard: verify_operation_condition() fail!"<<endl;
	return 0xff;
}

// read OCR register to check if the voltage range is valid 
// this step is not mandotary, but I advise to use it 
static int read_OCR(void) {
	Uint64 result;
	Uint8 ocr[4];

	int timeout;

	timeout = 0xff;
	while (--timeout) {
		sd_send_cmd(SD_CMD58, 0, 0);
		result = sd_get_response_R1();
		sd_get_response_R3_rest(ocr);
		sd_end_cmd();

		if (
			0x01 == result && // R1 response in idle status 
			(ocr[1] & 0x1f) && (ocr[2] & 0x80) 	// voltage range valid 
		) {
			return 0;
		}
	}

	// timeout!
//	printf("read_OCR() timeout!\n");
//	printf("result = %d\n", result);
	kout[Error]<<"SDCard: read_OCR() timeout! result = "<<result<<endl;
	return 0xff;
}

// send ACMD41 to tell sdcard to finish initializing 
static int set_SDXC_capacity(void) {
	Uint8 result = 0xff;

	int timeout = 0xfff;
	while (--timeout) {
		sd_send_cmd(SD_CMD55, 0, 0);
		result = sd_get_response_R1();
		sd_end_cmd();
		if (0x01 != result) {
//			printf("SD_CMD55 fail! result = %d\n", result);
			kout[Error]<<"SDCard: SD_CMD55 fail! result = "<<result<<endl;
			return 0xff;
		}

		sd_send_cmd(SD_ACMD41, 0x40000000, 0);
		result = sd_get_response_R1();
		sd_end_cmd();
		if (0 == result) {
			return 0;
		}
	}

	// timeout! 
//	printf("set_SDXC_capacity() timeout!\n");
//	printf("result = %d\n", result);
	kout[Error]<<"SDCard: set_SDXC_capacity() timeout! result = "<<result<<endl;
	return 0xff;
}

// Used to differ whether sdcard is SDSC type. 
static int is_standard_sd = 0;

// check OCR register to see the type of sdcard, 
// thus determine whether block size is suitable to buffer size
static int check_block_size(void) {
	Uint8 result = 0xff;
	Uint8 ocr[4];

	int timeout = 0xff;
	while (timeout --) {
		sd_send_cmd(SD_CMD58, 0, 0);
		result = sd_get_response_R1();
		sd_get_response_R3_rest(ocr);
		sd_end_cmd();

		if (0 == result) {
			if (ocr[0] & 0x40) {
//				printf("SDHC/SDXC detected\n");
				kout[Info]<<"SDCard: SDHC/SDXC detected"<<endl;
				if (512 != sizeof(Sector)) {
//					printf("sizeof(Sector) != 512\n");
					kout[Error]<<"SDCard: sizeof(Sector) != 512"<<endl;
					return 0xff;
				}

				is_standard_sd = 0;
			}
			else {
//				printf("SDSC detected, setting block size\n");
				kout[Info]<<"SDCard: SDSC detected, setting block size"<<endl;

				// setting SD card block size to sizeof(Sector) 
				int timeout = 0xff;
				int result = 0xff;
				while (--timeout) {
					sd_send_cmd(SD_CMD16, sizeof(Sector), 0);
					result = sd_get_response_R1();
					sd_end_cmd();

					if (0 == result) break;
				}
				if (0 == timeout) {
//					printf("check_OCR(): fail to set block size");
					kout[Error]<<"SDCard: check_OCR(): fail to set block size"<<endl;
					return 0xff;
				}

				is_standard_sd = 1;
			}

			return 0;
		}
	}

	// timeout! 
//	printf("check_OCR() timeout!\n");
//	printf("result = %d\n", result);
	kout[Error]<<"SDCard: check_OCR() timeout! result = "<<result<<endl;
	return 0xff;
}

/*
 * @brief  Initializes the SD/SD communication.
 * @param  None
 * @retval The SD Response:
 *         - 0xFF: Sequence failed
 *         - 0: Sequence succeed
 */
static int sd_init(void) {
	Uint8 frame[10];
	sd_lowlevel_init(0);
	//SD_CS_HIGH();
	SD_CS_LOW();

	// send dummy bytes for 80 clock cycles 
	for (int i = 0; i < 10; i ++) 
		frame[i] = 0xff;
	sd_write_data(frame, 10);

	if (0 != switch_to_SPI_mode()) 
		return 0xff;
	if (0 != verify_operation_condition()) 
		return 0xff;
	if (0 != read_OCR()) 
		return 0xff;
	if (0 != set_SDXC_capacity()) 
		return 0xff;
	if (0 != check_block_size()) 
		return 0xff;

	return 0;
}

//static struct sleeplock sdcard_lock;
Semaphore *semSDCard=nullptr;

void sdcard_init(void) {
	int result = sd_init();
//	initsleeplock(&sdcard_lock, "sdcard");
	semSDCard=new Semaphore(1);//Used as mutex
	
	if (0 != result) {
//		panic("sdcard_init failed");
		kout[Fault]<<"SDCard: sdcard_init failed"<<endl;
	}
	else kout[Info]<<"SDCard: sdcard_init OK"<<endl;
//	#ifdef DEBUG
//	printf("sdcard_init\n");
//	#endif
}

void sdcard_read_sector(Sector *sec, int sectorno) {
	Uint8 *buf=(Uint8*)sec;
	Uint8 result;
	Uint32 address;
	Uint8 dummy_crc[2];

//	#ifdef DEBUG
//	printf("sdcard_read_sector()\n");
//	#endif
//	kout[Test]<<"SDCard read sector "<<(void*)sectorno<<" to "<<buf<<endl;

	if (is_standard_sd) {
		address = sectorno << 9;
	}
	else {
		address = sectorno;
	}

	// enter critical section!
//	acquiresleep(&sdcard_lock);
	semSDCard->Wait();
	
	sd_send_cmd(SD_CMD17, address, 0);
	result = sd_get_response_R1();

	if (0 != result) {
//		releasesleep(&sdcard_lock);
		semSDCard->Signal();
//		panic("sdcard: fail to read");
		kout[Fault]<<"SDCard: failed to read"<<endl;
	}

	int timeout = 0xffffff;
	while (--timeout) {
		sd_read_data(&result, 1);
		if (0xfe == result) break;
	}
	if (0 == timeout) {
//		panic("sdcard: timeout waiting for reading");
		kout[Fault]<<"SDCard: timeout waiting for reading"<<endl;
	}
//	sd_read_data_dma(buf, sizeof(Sector));
	sd_read_data(buf, sizeof(Sector));
	sd_read_data(dummy_crc, 2);

	sd_end_cmd();

//	releasesleep(&sdcard_lock);
	semSDCard->Signal();
	// leave critical section!
//	kout[Test]<<"SDCard read sector "<<(void*)sectorno<<" OK "<<endl;
}

void sdcard_write_sector(const Sector *sec, int sectorno) {
	const Uint8 *buf=(Uint8*)sec;
	Uint32 address;
	static Uint8 const START_BLOCK_TOKEN = 0xfe;
	Uint8 dummy_crc[2] = {0xff, 0xff};

//	#ifdef DEBUG
//	printf("sdcard_write_sector()\n");
//	#endif
//	kout[Test]<<"SDCard write sector "<<(void*)sectorno<<" from "<<buf<<endl;

	if (is_standard_sd) {
		address = sectorno << 9;
	}
	else {
		address = sectorno;
	}

	// enter critical section!
//	acquiresleep(&sdcard_lock);
	semSDCard->Wait();

	sd_send_cmd(SD_CMD24, address, 0);
	if (0 != sd_get_response_R1()) {
//		releasesleep(&sdcard_lock);
		semSDCard->Signal();
//		panic("sdcard: fail to write");
		kout[Fault]<<"SDCard: failed to write"<<endl;
	}

	// sending data to be written 
	sd_write_data(&START_BLOCK_TOKEN, 1);
//	sd_write_data_dma(buf, sizeof(Sector));
	sd_write_data(buf, sizeof(Sector));
	sd_write_data(dummy_crc, 2);

	// waiting for sdcard to finish programming 
	Uint8 result;
	int timeout = 0xfff;
	while (--timeout) {
		sd_read_data(&result, 1);
		if (0x05 == (result & 0x1f)) {
			break;
		}
	}
	if (0 == timeout) {
//		releasesleep(&sdcard_lock);
		semSDCard->Signal();
//		panic("sdcard: invalid response token");
		kout[Fault]<<"SDCard: invalid response token"<<endl;
	}
	
	timeout = 0xffffff;
	while (--timeout) {
		sd_read_data(&result, 1);
		if (0 != result) break;
	}
	if (0 == timeout) {
//		releasesleep(&sdcard_lock);
		semSDCard->Signal();
//		panic("sdcard: timeout waiting for response");
		kout[Fault]<<"SDCard: timeout waiting for response"<<endl;
	}
	sd_end_cmd();

	// send SD_CMD13 to check if writing is correctly done 
	Uint8 error_code = 0xff;
	sd_send_cmd(SD_CMD13, 0, 0);
	result = sd_get_response_R1();
	sd_read_data(&error_code, 1);
	sd_end_cmd();
	if (0 != result || 0 != error_code) {
//		releasesleep(&sdcard_lock);
		semSDCard->Signal();
//		printf("result: %x\n", result);
//		printf("error_code: %x\n", error_code);
//		panic("sdcard: an error occurs when writing");
		kout[Fault]<<"SDCard: an error occurs when writing, result = "<<(void*)result<<",error_code = "<<(void*)error_code<<endl;
	}

//	releasesleep(&sdcard_lock);
	semSDCard->Signal();
	// leave critical section!
}

// A simple test for sdcard read/write test 
void test_sdcard(void) {
	Sector sec;

	for (int t = 0; t < 5; t ++)
	{
//		for (int i = 0; i < sizeof(Sector); i ++) {
//			sec[i] = 0xaa;		// data to be written 
//		}
		MemsetT<char>((char*)&sec,0xaa,sizeof(Sector));
		sdcard_write_sector(&sec, t);

//		for (int i = 0; i < sizeof(Sector); i ++) {
//			sec[i] = 0xff;		// fill in junk
//		}
		MemsetT<char>((char*)&sec,0xff,sizeof(Sector));
		sdcard_read_sector(&sec, t);
//		for (int i = 0; i < sizeof(Sector); i ++) {
//			if (0 == i % 16) {
//				printf("\n");
//			}
//
//			printf("%x ", buf[i]);
//		}
//		printf("\n");
		kout[Test]<<"SDCard: read sector info:"<<endline<<DataWithSizeUnited(&sec,sizeof(Sector),16)<<endl;
	}

//	while (1) ;
}
