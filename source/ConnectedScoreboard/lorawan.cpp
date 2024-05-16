#include <lorawan.h>
#include <Arduino.h>
#include "loramac/region/RegionEU868.h"

/*loraWan default Dr when adr disabled*/
RTC_DATA_ATTR int8_t default_dr_for_no_adr = 5;

RTC_DATA_ATTR uint8_t debug_level = LoRaWAN_DEBUG_LEVEL;

/*AT mode, auto into low power mode*/
RTC_DATA_ATTR bool auto_lpwm_b = true;

/*loraWan current Dr when adr disabled*/
RTC_DATA_ATTR int8_t current_dr_nor_no_adr;

/*!
 * User application data size
 */
RTC_DATA_ATTR uint8_t app_data_size = 0;

/*!
 * User application data
 */
RTC_DATA_ATTR uint8_t app_data[LORAWAN_APP_DATA_MAX_SIZE];


/*!
 * Defines the application data transmission duty cycle
 */
RTC_DATA_ATTR uint32_t tx_duty_cycle_time ;

/*!
 * Timer to handle the application data transmission duty cycle
 */
RTC_DATA_ATTR TimerEvent_t tx_next_packet_timer;

/*!
 * PassthroughMode mode enable/disable. don't modify it here. 
 * when use PassthroughMode, set it true in app.ino , Reference the example PassthroughMode.ino 
 */
bool passthrough_mode_b = false;

/*!
 * when use PassthroughMode, Mode_LoraWan to set use lora or lorawan mode . don't modify it here. 
 * it is used to set mode lora/lorawan in PassthroughMode.
 */
bool mode_lorawan_b = true;

/*!
 * Indicates if a new packet can be sent
 */
RTC_DATA_ATTR static bool next_tx_b = true;


RTC_DATA_ATTR enum device_state_lorawan_e device_state = DEVICE_STATE_INIT;


/*!
 * \brief   Prepares the payload of the frame
 *
 * \retval  [0: frame could be send, 1: error]
 */
bool lorawan_send_frame( void )
{
	lorawan_lwan_dev_params_update();
	
	McpsReq_t mcpsReq;
	LoRaMacTxInfo_t txInfo;
	LORAWANLOG;
	if( LoRaMacQueryTxPossible( app_data_size, &txInfo ) != LORAMAC_STATUS_OK )
	{
		// Send empty frame in order to flush MAC commands
		printf("lorawan: payload length error ...\r\n");
		mcpsReq.Type = MCPS_UNCONFIRMED;
		mcpsReq.Req.Unconfirmed.fBuffer = NULL;
		mcpsReq.Req.Unconfirmed.fBufferSize = 0;
		mcpsReq.Req.Unconfirmed.Datarate = current_dr_nor_no_adr;
		//return false;
	}
	else
	{
		if( is_tx_confirmed_b == false )
		{
			printf("lorawan: unconfirmed uplink sending ...\r\n");
			mcpsReq.Type = MCPS_UNCONFIRMED;
			mcpsReq.Req.Unconfirmed.fPort = app_port;
			mcpsReq.Req.Unconfirmed.fBuffer = app_data;
			mcpsReq.Req.Unconfirmed.fBufferSize = app_data_size;
			mcpsReq.Req.Unconfirmed.Datarate = current_dr_nor_no_adr;
		}
		else
		{
			printf("lorawan: confirmed uplink sending ...\r\n");
			mcpsReq.Type = MCPS_CONFIRMED;
			mcpsReq.Req.Confirmed.fPort = app_port;
			mcpsReq.Req.Confirmed.fBuffer = app_data;
			mcpsReq.Req.Confirmed.fBufferSize = app_data_size;
			mcpsReq.Req.Confirmed.NbTrials = confirmed_nb_trials;
			mcpsReq.Req.Confirmed.Datarate = current_dr_nor_no_adr;
		}
	}

	if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK )
	{
		return false;
	}
	return true;
}

/*!
 * \brief Function executed on TxNextPacket Timeout event
 */
void lorawan_OnTxNextPacketTimerEvent( void )
{
  MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;

	TimerStop( &tx_next_packet_timer );

	mibReq.Type = MIB_NETWORK_JOINED;
	status = LoRaMacMibGetRequestConfirm( &mibReq );

	if( status == LORAMAC_STATUS_OK )
	{
		if( mibReq.Param.IsNetworkJoined == true )
		{
			device_state = DEVICE_STATE_SEND;
			next_tx_b = true;
		}
		else
		{
			// Network not joined yet. Try to join again
			MlmeReq_t mlmeReq;
			mlmeReq.Type = MLME_JOIN;
			mlmeReq.Req.Join.DevEui = dev_eui;
			mlmeReq.Req.Join.AppEui = app_eui;
			mlmeReq.Req.Join.AppKey = app_key;
			mlmeReq.Req.Join.NbTrials = 1;

			if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
			{
				device_state = DEVICE_STATE_SLEEP;
			}
			else
			{
				device_state = DEVICE_STATE_CYCLE;
			}
		}
	}
}

/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] mcpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void McpsConfirm( McpsConfirm_t *mcpsConfirm )
{
	if( mcpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
	{
		switch( mcpsConfirm->McpsRequest )
		{
			case MCPS_UNCONFIRMED:
			{
				// Check Datarate
				// Check TxPower
				break;
			}
			case MCPS_CONFIRMED:
			{
				// Check Datarate
				// Check TxPower
				// Check AckReceived
				// Check NbTrials
				break;
			}
			case MCPS_PROPRIETARY:
			{
				break;
			}
			default:
				break;
		}
	}
	next_tx_b = true;
}

/*!
 * \brief   MCPS-Indication event function
 *
 * \param   [IN] mcpsIndication - Pointer to the indication structure,
 *               containing indication attributes.
 */
RTC_DATA_ATTR int revr_ssi,rev_snr;
static void McpsIndication( McpsIndication_t *mcpsIndication )
{
	if( mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK )
	{
		return;
	}
	revr_ssi=mcpsIndication->Rssi;
	rev_snr=mcpsIndication->Snr;



	LORAWANLOG;
	printf( "lorawan: received ");
	switch( mcpsIndication->McpsIndication )
	{
		case MCPS_UNCONFIRMED:
		{
			printf( "unconfirmed ");
			break;
		}
		case MCPS_CONFIRMED:
		{
			printf( "confirmed ");
			lorawan_OnTxNextPacketTimerEvent( );
			break;
		}
		case MCPS_PROPRIETARY:
		{
			printf( "proprietary ");
			break;
		}
		case MCPS_MULTICAST:
		{
			printf( "multicast ");
			break;
		}
		default:
			break;
	}
	printf( "downlink: rssi = %d, snr = %d, datarate = %d\r\n", mcpsIndication->Rssi, (int)mcpsIndication->Snr,(int)mcpsIndication->RxDoneDatarate);

	if(mcpsIndication->AckReceived)
	{
		downLinkAckHandle();
	}

	if( mcpsIndication->RxData == true )
	{
		downLinkDataHandle(mcpsIndication);
	}

	// Check Multicast
	// Check Port
	// Check Datarate
	// Check FramePending
	if( mcpsIndication->FramePending == true )
	{
		// The server signals that it has pending data to be sent.
		// We schedule an uplink as soon as possible to flush the server.
		lorawan_OnTxNextPacketTimerEvent( );
	}
	// Check Buffer
	// Check BufferSize
	// Check Rssi
	// Check Snr
	// Check RxSlot

	delay(10);
}


void __attribute__((weak)) dev_time_updated()
{
	printf("device time updated\r\n");
}

/*!
 * \brief   MLME-Confirm event function
 *
 * \param   [IN] mlmeConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void MlmeConfirm( MlmeConfirm_t *mlmeConfirm )
{
	switch( mlmeConfirm->MlmeRequest )
	{
		case MLME_JOIN:
		{
			if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
			{
  			LORAWANLOG;
				printf("joined\r\n");
				
				//in PassthroughMode,do nothing while joined
				if(passthrough_mode_b == false)
				{
					// Status is OK, node has joined the network
					device_state = DEVICE_STATE_SEND;
				}
			}
			else
			{
				uint32_t rejoin_delay = 30000;
				printf("join failed, join again at 30s later\r\n");
				delay(5);
				TimerSetValue( &tx_next_packet_timer, rejoin_delay );
				TimerStart( &tx_next_packet_timer );
			}
			break;
		}
		case MLME_LINK_CHECK:
		{
			if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
			{
				// Check DemodMargin
				// Check NbGateways
			}
			break;
		}
		case MLME_DEVICE_TIME:
		{
			if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
			{
				dev_time_updated();
			}
			break;
		}
		default:
			break;
	}
	next_tx_b = true;
}

/*!
 * \brief   MLME-Indication event function
 *
 * \param   [IN] mlmeIndication - Pointer to the indication structure.
 */
static void MlmeIndication( MlmeIndication_t *mlmeIndication )
{
	switch( mlmeIndication->MlmeIndication )
	{
		case MLME_SCHEDULE_UPLINK:
		{// The MAC signals that we shall provide an uplink as soon as possible
			lorawan_OnTxNextPacketTimerEvent( );
			break;
		}
		default:
			break;
	}
}


void lorawan_lwan_dev_params_update( void )
{
	LoRaMacChannelAdd( 3, ( ChannelParams_t )EU868_LC4 );
	LoRaMacChannelAdd( 4, ( ChannelParams_t )EU868_LC5 );
	LoRaMacChannelAdd( 5, ( ChannelParams_t )EU868_LC6 );
	LoRaMacChannelAdd( 6, ( ChannelParams_t )EU868_LC7 );
	LoRaMacChannelAdd( 7, ( ChannelParams_t )EU868_LC8 );


	MibRequestConfirm_t mibReq;

	mibReq.Type = MIB_CHANNELS_DEFAULT_MASK;
	mibReq.Param.ChannelsMask = users_channel_mask;
	LoRaMacMibSetRequestConfirm(&mibReq);

	mibReq.Type = MIB_CHANNELS_MASK;
	mibReq.Param.ChannelsMask = users_channel_mask;
	LoRaMacMibSetRequestConfirm(&mibReq);
}

void lorawan_print_hex(uint8_t *para,uint8_t size)
{
	for(int i=0;i<size;i++)
	{
		printf("%02X",*para++);
	}
}

void lorawan_print_dev_param(void)
{
		printf("+Class=%X\r\n",lorawan_class+10);
		printf("+ADR=%d\r\n",lorawan_adr_b);
		printf("+IsTxConfirmed=%d\r\n",is_tx_confirmed_b);
		printf("+AppPort=%d\r\n",app_port);
		printf("+DutyCycle=%u\r\n",app_tx_duty_cycle);
		printf("+ConfirmedNbTrials=%u\r\n",confirmed_nb_trials);
		printf("+ChMask=%04X%04X%04X%04X%04X%04X\r\n",users_channel_mask[5],users_channel_mask[4],users_channel_mask[3],users_channel_mask[2],users_channel_mask[1],users_channel_mask[0]);
		printf("+DevEui=");lorawan_print_hex(dev_eui,8);	printf("(For OTAA Mode)\r\n");
		printf("+AppEui=");lorawan_print_hex(app_eui,8);	printf("(For OTAA Mode)\r\n");
		printf("+AppKey=");lorawan_print_hex(app_key,16);	printf("(For OTAA Mode)\r\n");
}


RTC_DATA_ATTR LoRaMacPrimitives_t loramac_primitive;
RTC_DATA_ATTR LoRaMacCallback_t loramac_callback;

void LorawanClass::generateDeveuiByChipID()
{
	uint32_t uniqueId[2];
#if defined(ESP_PLATFORM)
	uint64_t id = getID();
	uniqueId[0]=(uint32_t)(id>>32);
	uniqueId[1]=(uint32_t)id;
#endif
	for(int i=0;i<8;i++)
	{
		if(i<4)
			dev_eui[i] = (uniqueId[1]>>(8*(3-i)))&0xFF;
		else
			dev_eui[i] = (uniqueId[0]>>(8*(7-i)))&0xFF;
	}
}


void LorawanClass::init(DeviceClass_t LorawanClass,LoRaMacRegion_t region)
{
	MibRequestConfirm_t mibReq;

	loramac_primitive.MacMcpsConfirm = McpsConfirm;
	loramac_primitive.MacMcpsIndication = McpsIndication;
	loramac_primitive.MacMlmeConfirm = MlmeConfirm;
	loramac_primitive.MacMlmeIndication = MlmeIndication;
	loramac_callback.GetBatteryLevel = BoardGetBatteryLevel;
	loramac_callback.GetTemperatureLevel = NULL;
	LoRaMacInitialization( &loramac_primitive, &loramac_callback,region);
	TimerStop( &tx_next_packet_timer );
	TimerInit( &tx_next_packet_timer, lorawan_OnTxNextPacketTimerEvent );

	Serial.println();
	
	LORAWANLOG;
	Serial.print("lorawan: ");
	switch(region)
	{
		case LORAMAC_REGION_AS923_AS1:
			Serial.print("AS923(AS1:922.0-923.4MHz)");
			break;
		case LORAMAC_REGION_AS923_AS2:
			Serial.print("AS923(AS2:923.2-924.6MHz)");
			break;
		case LORAMAC_REGION_AU915:
			Serial.print("AU915");
			break;
		case LORAMAC_REGION_CN470:
			Serial.print("CN470");
			break;
		case LORAMAC_REGION_CN779:
			Serial.print("CN779");
			break;
		case LORAMAC_REGION_EU433:
			Serial.print("EU433");
			break;
		case LORAMAC_REGION_EU868:
			Serial.print("EU868");
			break;
		case LORAMAC_REGION_KR920:
			Serial.print("KR920");
			break;
		case LORAMAC_REGION_IN865:
			Serial.print("IN865");
			break;
		case LORAMAC_REGION_US915:
			Serial.print("US915");
			break;
		case LORAMAC_REGION_US915_HYBRID:
			Serial.print("US915_HYBRID ");
			break;
		default:
			break;
	}
	Serial.printf(" Class %X start!\r\n\r\n",lorawan_class+10);
	lorawan_print_dev_param();
	Serial.println();

  mibReq.Type = MIB_ADR;
  mibReq.Param.AdrEnable = lorawan_adr_b;
  LoRaMacMibSetRequestConfirm( &mibReq );

  mibReq.Type = MIB_PUBLIC_NETWORK;
  mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
  LoRaMacMibSetRequestConfirm( &mibReq );

  lorawan_lwan_dev_params_update();

  mibReq.Type = MIB_DEVICE_CLASS;
  LoRaMacMibGetRequestConfirm( &mibReq );

  if(lorawan_class != mibReq.Param.Class)
  {
    mibReq.Param.Class = lorawan_class;
    LoRaMacMibSetRequestConfirm( &mibReq );
  }

  device_state = DEVICE_STATE_JOIN;

}


void LorawanClass::join()
{
		LORAWANLOG;
		Serial.println("lorawan: joining...");
		MlmeReq_t mlmeReq;
		
		mlmeReq.Type = MLME_JOIN;

		mlmeReq.Req.Join.DevEui = dev_eui;
		mlmeReq.Req.Join.AppEui = app_eui;
		mlmeReq.Req.Join.AppKey = app_key;
		mlmeReq.Req.Join.NbTrials = 1;

		if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
		{
			device_state = DEVICE_STATE_SLEEP;
		}
		else
		{
			device_state = DEVICE_STATE_CYCLE;
		}
}

void LorawanClass::send()
{
	if( next_tx_b == true )
	{
		MibRequestConfirm_t mibReq;
		mibReq.Type = MIB_DEVICE_CLASS;
		LoRaMacMibGetRequestConfirm( &mibReq );

		if(lorawan_class != mibReq.Param.Class)
		{
			mibReq.Param.Class = lorawan_class;
			LoRaMacMibSetRequestConfirm( &mibReq );
		}
		next_tx_b = lorawan_send_frame( );
	}
}

void LorawanClass::cycle(uint32_t dutyCycle)
{
	TimerSetValue( &tx_next_packet_timer, dutyCycle );
	TimerStart( &tx_next_packet_timer );
}

void LorawanClass::sleep(DeviceClass_t classMode)
{
	Mcu.timerhandler();
	Radio.IrqProcess();
	Mcu.sleep(classMode,debug_level,HELTEC_BOARD,SLOW_CLK_TPYE);
}
void LorawanClass::setDefaultDR(int8_t dataRate)
{
	default_dr_for_no_adr = dataRate;
}


void lorawan_printf(const char *format, ...)
{
    char loc_buf[64];
    char * temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    size_t len = vsnprintf(NULL, 0, format, arg);
    va_end(copy);
    if(len >= sizeof(loc_buf)){
        temp = new char[len+1];
        if(temp == NULL) {
            return ;
        }
    }
    len = vsnprintf(temp, len+1, format, arg);
    Serial.write((uint8_t*)temp, len);
    va_end(arg);
    if(len > 64){
        delete[] temp;
    }
}
extern uint32_t storedlicense[4];

void lorawan_check_input_license()
{
	int i;
	i=0;
	char rxData[50];
	uint8_t inputlicense[32];
	uint32_t timestart;
	timestart = millis();
	Serial.flush();
	  while((millis()-timestart)<1000)
	  {
		if(Serial.available()>0)
		{
		  rxData[i++]=(uint8_t)Serial.read();
		}
		if(i==41)
		{
		  break;
		}
	  }
	  rxData[i]=0;
	  if(i!=41||rxData[8]!='=')
	  {
		return;
	  }
	  for(int j=0;j<32;j++)
	  {
		inputlicense[j]=rxData[j+9];
		if(inputlicense[j]>='a')
		{
		  inputlicense[j]-=32;
		}
		if(inputlicense[j]<='9')
		{
		  inputlicense[j]-='0';
		}
		else
		{
		  inputlicense[j]-=('A'-10);
		}
	  }
	
	  for(int j=0;j<4;j++)
	  {
		storedlicense[j]=0;
		for(int k=0;k<8;k++)
		{
		 storedlicense[j]|=(uint32_t)inputlicense[8*j+k]<<(28-k*4);
		}
	  }

}

LorawanClass Lorawan;
