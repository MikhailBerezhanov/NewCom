/*==============================================================================
>File name:  	PB3P.c

>Brief:         PB3P - Selfmaded Serial bi-directional communication protocol.
                Communication protocol realization here.
				* This API needs realization of Put_buffer, Get_buffer via 
				hardware interface that is used in system. 

>Author:        Mikhail Berezhanov
>Date:          28.09.2018
>Version:       0.3
==============================================================================*/

#include "PB3P.h"

static unsigned short crc16(unsigned char *pcBlock, unsigned short len);
static unsigned char crc8_dallas(unsigned char *block, unsigned short size);

/**
  * @brief 	Reset PB3P_GetBuf finite-state-machine   
  * @param 	PB3P_dev* dev  - pointer at interface-structure			 
  * @retval None
 */
void PB3P_Init(PB3P_dev* dev)
{
	dev->State.CurSt = IDLE;
}

/**
  * @brief 	Send databuf via PB3P protocol using any hardware interface 
  * @param  PB3P_dev* dev  - pointer at interface-structure  			 
  * @retval 0 - SUCCESS, 1 - FAILED
 */
uint8_t PB3P_PutBuf (PB3P_dev* dev)
{
//Local variables
	PB3P_HEADER_t Header;

	/* Filling PB3P header */
	Header.ver = PB3P_PROTOCOL_VER; 
	Header.datablock_len = dev->Buf.datalen;
	if ((dev->Buf.data != NULL) && dev->Buf.datalen)
		Header.datablock_crc = crc16(dev->Buf.data, dev->Buf.datalen);
	else Header.datablock_crc = 0;
	Header.cmd = dev->Buf.cmd;
	Header.direction = dev->Buf.direction;
	Header.timeout = dev->Buf.timeout;
	Header.err = dev->Buf.err;
	Header.RFU = 0x00;
	Header.crc8 = crc8_dallas((unsigned char*)&Header, (sizeof(Header)-1));
	
	if ((dev->Buf.data != NULL) && dev->Buf.datalen)
	{
	/* Send Header and Data*/
		if (dev->SendBuf((unsigned char*)&Header, sizeof(Header)) && dev->SendBuf(dev->Buf.data, dev->Buf.datalen))
		{
			return 0;
		}
	}
	/* Send Header only */
	else if (dev->SendBuf((unsigned char*)&Header, sizeof(Header))) return 0;

return 1;		
}


/**
  * @brief 	Get PB3P protocol packet 
  * @note	This function should be used as an implementation of finite-state-machine (non-blocking)  
  * @param 	PB3P_dev* dev  - pointer at interface-structure  
  * @retval  RECV_INPROGRESS 	- receiving data not finished yet
  			 ERR_XCH_DATABLOCK_LENGTH - unavalable datalength
  			 ERR_XCH_CRC_HEADER	- error in CRC8 in header block
			 ERR_XCH_CRC		- error in CRC16 in data block
			 ERR_XCH_OK			- data received successfully
			 RECV_START			- ver byte comes
 */
uint8_t PB3P_GetBuf (PB3P_dev* dev)
{
	static uint8_t cnt = 0;		// counter of received bytes

	switch (dev->State.CurSt)
	{
	case IDLE:		// Waiting for start byte "ver"

		if (dev->RecvByte(&dev->State.Recv.Header.ver) &&
			(dev->State.Recv.Header.ver == PB3P_PROTOCOL_VER))	// start of packet byte
		{
			cnt = 1; 
			dev->State.CurSt = START;
#if PB3P_DEBUG 
			dev->DbgMsg("GO TO START of PB3P packet\r\n");
#endif
			return RECV_START;
		}
	
		break;

	case START:		// Reading Header

#if PB3P_DEBUG 
		dev->DbgMsg("START of PB3P packet\r\n");
#endif
	
#if PB3P_BURST_READ	
		// burst reading of PB3P_Header
		for (uint8_t i = 1; i < sizeof(PB3P_HEADER_t); i++)
		{
			if (dev->RecvByte((uint8_t *)&dev->State.Recv.Dump[i]))
				cnt++;
		}

		if (cnt == sizeof(PB3P_HEADER_t))
		{
			dev->State.CurSt = CHECKING;
			cnt = 0; 		// now cnt will count data bytes
#if PB3P_DEBUG 
			dev->DbgMsg("GO TO CHECKING of PB3P packet\r\n");
#endif
		}
						
#else
		// one by one reading rest part (12 bytes) of PB3P_Header 
		if (dev->RecvByte((uint8_t *)&dev->State.Recv.Dump[cnt]))
		{
			if (++cnt == (sizeof(PB3P_HEADER_t)))
			{
				dev->State.CurSt = CHECKING;
				cnt = 0; 		// now cnt will count data bytes
#if PB3P_DEBUG 
				dev->DbgMsg("GO TO CHECKING of PB3P packet\r\n");
#endif
			}
		}
#endif
		break;

	case CHECKING:	// Checking CRC of Header and copying to buf	

		if (dev->State.Recv.Header.crc8 == crc8_dallas((unsigned char*)&dev->State.Recv.Header, (sizeof(PB3P_HEADER_t) - 1)))
		{
			dev->Buf.cmd = dev->State.Recv.Header.cmd;		// cmd must be copied cuz answer can contain request_cmd field

			// check if message too long
			if (dev->State.Recv.Header.datablock_len > PB3P_MAX_DATA)
			{
				dev->State.CurSt = IDLE;
				cnt = 0;
#if PB3P_DEBUG 
				dev->DbgMsg("DATA_LEN_ERROR of PB3P packet\r\n");
#endif
				return ERR_XCH_DATABLOCK_LENGTH;
			}

			// copy info to PB3P_BUF_t structure for further parsing
			dev->Buf.datalen = dev->State.Recv.Header.datablock_len;
			dev->Buf.direction = dev->State.Recv.Header.direction;
			dev->Buf.timeout = dev->State.Recv.Header.timeout;
			dev->Buf.err = dev->State.Recv.Header.err;
			if (dev->Buf.datalen)
			{
				dev->State.CurSt = DATARECV;
#if PB3P_DEBUG 
				dev->DbgMsg("GO TO DATARECV of PB3P packet\r\n");
#endif
			}
			else
			{
				dev->State.CurSt = IDLE;		// go to waiting next packet
				cnt = 0;
#if PB3P_DEBUG 
				dev->DbgMsg("PB3P SUCCESS: PACKET(without datablock) RECEIVED\r\n");
#endif
				return ERR_XCH_OK;		// packet (without datablock) recieved successfully		
			}
		}
		else
		{
			dev->State.CurSt = IDLE;
			cnt = 0;
#if PB3P_DEBUG 
			dev->DbgMsg("CRC_HEADER_ERROR of PB3P packet\r\n");
#endif
			return ERR_XCH_CRC_HEADER;
		}

		break;

	case DATARECV:	// Receiving datablock

					// burst reading of PB3P_Data
#if PB3P_BURST_READ				
		for (uint8_t i = 0; i < dev->Buf.datalen; i++)
		{
			if (dev->RecvByte(&dev->Buf.data[i]))
				cnt++;
		}

		if (cnt == dev->Buf.datalen)
		{
			// Check crc16 of recieved data
			if (crc16(dev->Buf.data, dev->Buf.datalen) == dev->State.Recv.Header.datablock_crc)
			{
				dev->State.CurSt = IDLE;		// go to waiting next packet
				cnt = 0;
#if PB3P_DEBUG 
				dev->DbgMsg("DATA RECEIVED of PB3P packet\r\n");
#endif
				return ERR_XCH_OK;		// packet recieved successfully
			}
#if PB3P_DEBUG 
			dev->DbgMsg("DATA_CRC_ERROR of PB3P packet\r\n");
#endif
			dev->State.CurSt = IDLE;		// go to waiting next packet
			cnt = 0;
			return ERR_XCH_CRC;			// else return error				
		}

		// one by one reading PB3P_Data
#else
		if (dev->RecvByte(&dev->Buf.data[cnt]))
		{
			if (++cnt == dev->Buf.datalen)
			{
				// Check crc16 of recieved data
				if (crc16(dev->Buf.data, dev->Buf.datalen) == dev->State.Recv.Header.datablock_crc)
				{
					dev->State.CurSt = IDLE;// go to waiting next packet
					cnt = 0;
#if PB3P_DEBUG 
					dev->DbgMsg("PB3P SUCCESS: full PACKET RECEIVED\r\n");
#endif
					return ERR_XCH_OK;		// packet recieved successfully
				}
#if PB3P_DEBUG 
				dev->DbgMsg("DATA_CRC_ERROR of PB3P packet\r\n");
#endif
				dev->State.CurSt = IDLE;	// go to waiting next packet
				cnt = 0;
				return ERR_XCH_CRC;			// else return error
			}
		}
#endif

		break;

	default:

		dev->State.CurSt = IDLE;
		cnt = 0;
#if PB3P_DEBUG 
		dev->DbgMsg("DEFAULT_STATE of PB3P packet\r\n");
#endif

		break;
		}

	return RECV_INPROGRESS;
}

/**
  * @brief   
			Name  : CRC-16 CCITT
		  	Poly  : 0x1021    x^16 + x^12 + x^5 + 1
			Init  : 0xFFFF
			Revert: false
			XorOut: 0x0000
			Check : 0x29B1 ("123456789")
			MaxLen: 4095 байт (32767 бит) - обнаружение
			одинарных, двойных, тройных и всех нечетных ошибок
  * @param  
  			pcBlock - pointer at block to be calculated
			len		- lengh of block to be calculated 
  * @retval CRC16 value   
 */
static unsigned short crc16(unsigned char *pcBlock, unsigned short len)
{
    unsigned short crc = 0xFFFF;
    unsigned char i;

    while (len--) 
	{
        crc ^= *pcBlock++ << 8;
        for (i = 0; i < 8; i++)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }

    return crc;
}

/**
  * @brief 	CRC8 calculation  
  * @param	
  			block - pointer at block to be calculated
			size  - lengh of block to be calculated   
  * @retval	CRC8 value  
 */
static unsigned char crc8_dallas(unsigned char *block, unsigned short size) 
{ 
    unsigned char poly = 0x8C, i, mask, byte; 
    unsigned char crc = 0x00; 

    while (size--) 
	{ 
      byte = *block++; 
        for (i = 0; i < 8; ++i) 
		{ 
          mask = (byte & 1) ^ (crc & 1); 
            byte >>= 1; 
            crc >>= 1; 
            if (mask) 
			{ 
              crc ^= poly; 
            } 
        } 
    } 
    
	return crc; 
}
