/*==============================================================================
>File name:  	PB3P.h
>Brief:         
                
>Author:        Mikhail Berezhanov
>Date:          28.09.2018
>Version:       1.3
==============================================================================*/
#include <stdint.h>
#include <stdio.h>

#ifndef _PB3P_H_
#define _PB3P_H_

	#define PB3P_DEBUG					0
	#define PB3P_BURST_READ				0		// Read header and datablock with one iteration. Put 0 to disable this option
	#define PB3P_PROTOCOL_VER			0x02
	#define PB3P_MAX_DATA				500		// Max length in [bytes] of datablock in 1 packet
	#define PB3P_MAX_TRY				30		// Max num of tries to read bytes (data\header) 	

    #define ERR_XCH_OK                  0       // нет ошибок
    #define ERR_XCH_HEADER_TOUT         1       // таймаут ожидания заголовка 
    #define ERR_XCH_VERSION             2       // неправильная версия протокола
    #define ERR_XCH_DATABLOCK_LENGTH    3       // недопустимый размер блока данных
    #define ERR_XCH_WRONG_DIRECTION     4       // задано неверное направление
    #define ERR_XCH_DATABLOCK_TOUT      5       // таймаут ожидания блока данных от хоста
    #define ERR_XCH_DATA_FORMAT         6       // ошибка в формате данных при задании хостом команды для CL
    #define ERR_XCH_UNKNOWN_CMD         7       // неизвестная команда хоста
    #define ERR_XCH_DATABLOCK_TOUT_SAM  8       // таймаут ожидания блока данных от SAM
    #define ERR_XCH_BUFSIZE             9       // размер получаемых от SAM данных больше, чем размер буфера
    #define ERR_XCH_NOT_ACTIVE          10      // не было сброса SAM (и не было ATR)
    #define ERR_XCH_SYNC_SPI_TOUT       11      // таймаут при синхронизации с SPI-мастером
    #define ERR_XCH_CRC                 12      // ошибка при расчёте КС блока данных
    #define ERR_XCH_CRC_HEADER          13      // ошибка при расчёте КС заголовка
	#define ERR_MFRC_PSN				15		// Error reading MIFARE PSN

	#define CMD_ERR_OCCUR				0xFE	// Error while receiving message occured
	#define CMD_FIELD_ON				0x01	// Enable MFRC field
	#define CMD_FIELD_OFF				0x02	// Disable MFRC field 
	#define CMD_CRD_DETECTED			0x03	// Card detected in MFRC field
	#define CMD_GET_READER_SN 			0x10	// Fetch reader's MFRC chip serial number
	#define CMD_READ_BLOCK				0x11	// Read card block
	#define CMD_WRITE_BLOCK				0x12	// Write card block	
	#define CMD_DEC_BLOCK				0x14	// Decrement card block		
	#define CMD_DETECT_CARDS			0x16	// Detect All cards in reader's feild
	#define CMD_HALT_CARD				0x17	// Halt card
	#define CMD_SAVE_KEY				0x1C	// Save key at MFRC chip EEPROM
	#define CMD_LOCKSTATE_CH			0x20	// State of Lock changed (databytes) 
	#define CMD_READ_SKP_BLOCKS			0x21	// Read 8 SKP-01 Card blocks. Args: {Nomcard(1),nblock(1),key(1),block(1),... }
	#define CMD_WRITE_SKP_BLOCKS		0x22	// Write SKP-01 Card blocks.  Args: {Nomcard(1),nblock(1),key(1),block(1),data(16),...} 
	#define CMD_REPROG					0x23	// Load new firmware (databytes)
	#define CMD_STATUS_FETCH			0x24	// Get device's status (answer - databytes)

	#define RECV_INPROGRESS				85		// Status when receiving packet in progress (not finished yet)
	#define RECV_START					86		// Status that indicates that "ver" byte comes

	#define DIR_VCR_OCR					0x05
	#define DIR_OCR_VCR					0x04

// Neccessary functions for hardware 
typedef uint8_t (*fpSendBuf)(uint8_t *pDataBuf, uint16_t len);	// Pointer-type at HardwareSendBuf function 
typedef uint8_t (*fpRecvByte)(uint8_t *pStoreBuf);				// Pointer-type at HardwareRecieveByte function
typedef int (*fpDebugMsg)(const char* str);						// Pointer-type at DebugWriteMsg function

// States of finit-state-machine to assemble PB3P frame
typedef enum
{
	IDLE = 0,
	START,
	CHECKING,
	DATARECV,
} PB3P_STATE_enum;

// PB3P Header structure
#pragma pack(push, 1)						
typedef struct // Packed struct
{
	uint8_t   ver;                          // версия протокола
	uint16_t  datablock_len;                // длина передаваемого блока данных
	uint16_t  datablock_crc;                // кс блока данных (crc16)
	uint8_t   cmd;                          // тип команды: 0x00 - передача данных, 0xXX - сервисные команды
	uint8_t   direction;                    // направление передачи данных (0 - PIC-->host, 1 - host-->SAM, 2 - host-->CL, 3 - host-->PIC, 4 - VCR-->OCR, 5 OCR-->VCR)
	uint16_t  timeout;                      // RFU время [мс] - ожидание ответа от SAM AV2/CL663
	uint16_t  err;                          // номер ошибки
	uint8_t   RFU;                          // заполняется нулём
	uint8_t   crc8;                         // crc8 заголовка
        
} PB3P_HEADER_t;							// 13 Bytes total length

// Struct of assembling-finit-state-machine
typedef struct // Packed struct
{
	PB3P_STATE_enum CurSt;					// Current state of receiving progress
	union
	{
	PB3P_HEADER_t Header;					// Received data
	uint8_t Dump[13];						// 13 bytes dump of header
	}Recv;
} PB3P_STATE_t; 
#pragma pack(pop)

// PB3P general purpose buffer (for sending or receiving protocol data)
typedef struct
{
	uint16_t  datalen;						// data length
	uint8_t   cmd;							// command type
	uint8_t   direction;					// direction of transmission
	uint16_t  timeout;						// waiting answer time [ms]
	uint16_t  err;							// error code
	uint8_t   data[PB3P_MAX_DATA];			// data buf							

} PB3P_BUF_t;

// User interface-structure of PB3P
typedef struct
{
	PB3P_STATE_t State;						// State of PB3P_GetBuf finite - state - machine
	PB3P_BUF_t	Buf;						// Buf to send data \ with received data via PB3P
	fpSendBuf	SendBuf;
	fpRecvByte	RecvByte;
	fpDebugMsg	DbgMsg;

} PB3P_dev;								// Protocol's base structure - interface

/**
  * @brief 	Send databuf via PB3P protocol using any hardware interface 
  * @param  PB3P_dev* dev  - pointer at interface-structure of PB3P		  
  * @retval 0 - SUCCESS, 1 - FAILED
 */
uint8_t PB3P_PutBuf(PB3P_dev* dev);

/**
  * @brief 	Get PB3P protocol packet 
  * @note	This function should be used as an implementation of finite-state-machine (non-blocking)  
  * @param 	*pStoreBuf 			- pointer at structure to store data and parse further  
  * @retval  RECV_INPROGRESS 	- receiving data not finished yet
  			 ERR_XCH_CRC_HEADER	- error in CRC8 in header block
			 ERR_XCH_CRC		- error in CRC16 in data block
			 ERR_XCH_OK			- data received successfully
 */
uint8_t PB3P_GetBuf (PB3P_dev* dev);

/**
  * @brief 	Init state structure of PB3P_GetBuf finite-state-machine   
  * @param 	None			 
  * @retval None
 */
void PB3P_Init(PB3P_dev* dev);

void PB3P_CurSt_to_Start(PB3P_dev* dev);

#endif	// _PB3P_H_
