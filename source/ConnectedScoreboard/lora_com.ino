#include "lorawan.h"
#include "Arduino.h"

/* OTAA para*/
uint8_t dev_eui[] = { 0xfa, 0xe0, 0x4c, 0x61, 0xcb, 0x69, 0x4c, 0x9e };
uint8_t app_eui[] = { 0x75, 0x30, 0xad, 0x3e, 0x18, 0x67, 0xb4, 0x8e };
uint8_t app_key[] = { 0x7d, 0x49, 0x51, 0xc6, 0xcb, 0x68, 0x26, 0x5f, 0xcd, 0xc5, 0x32, 0xc7, 0x7e, 0xd5, 0xfb, 0xb5 };

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t users_channel_mask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t lorawan_region = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  lorawan_class = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t app_tx_duty_cycle = 5000;

/*ADR enable*/
bool lorawan_adr_b = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool is_tx_confirmed_b = false;

/* Application port */
uint8_t app_port = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmed_nb_trials = 4;

bool new_msg_to_be_send_b = false;
uint32_t last_sent_try_ms = 0;
uint8_t retry_nb = 0;
bool ack_received_b = false;

#define RETRY_TIMEOUT_MS 10000
#define MAX_RETRIES 3

/* Prepares the payload of the frame */
static void lora_com_send_message( uint8_t msg_frame[], uint8_t msg_size )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */
  app_data_size = msg_size;
  
  for (uint8_t idx = 0; idx < msg_size; idx++)
  {
    app_data[idx] = msg_frame[idx];
  }

  new_msg_to_be_send_b = true;
  retry_nb = 0;
  ack_received_b = false;

  lorawan_OnTxNextPacketTimerEvent();
}

void downLinkAckHandle(void)
{
  //Serial.println("lora_com: Ack received");
  ack_received_b = true;
}

static enum device_state_lorawan_e old_State = DEVICE_STATE_INIT;

static void lora_com_print_state_change(void)
{
  if (device_state != old_State)
  {
    switch (device_state)
    {
      case DEVICE_STATE_INIT:
      {
        Serial.println("lora_com: State: INIT");
        break;
      }
      case DEVICE_STATE_JOIN:
      {
        Serial.println("lora_com: State: JOIN");
        break;
      }
      case DEVICE_STATE_CYCLE:
      {
        Serial.println("lora_com: State: CYCLE");
        break;
      }
      case DEVICE_STATE_SLEEP:
      {
        Serial.println("lora_com: State: SLEEP");
        break;
      }
      case DEVICE_STATE_SEND:
      {
        Serial.println("lora_com: State: SEND");
        break;
      }
      case DEVICE_STATE_IDLE:
      {
        Serial.println("lora_com: State: IDLE");
        break;
      }
      case DEVICE_STATE_WAIT_FOR_ACK:
      {
        Serial.println("lora_com: State: DEVICE_STATE_WAIT_FOR_ACK");
        break;
      }
    }
    old_State = device_state;
  }

}

static void lora_com_task(void)
{
  //lora_com_print_state_change();

  switch( device_state )
  {
    case DEVICE_STATE_INIT:
    {
      Lorawan.init(lorawan_class,lorawan_region);
      //both set join DR and DR when ADR off 
      Lorawan.setDefaultDR(3);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      Lorawan.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      Lorawan.send();
      new_msg_to_be_send_b = false;
      last_sent_try_ms = millis();
      device_state = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_WAIT_FOR_ACK:
    {
      if ((millis() - last_sent_try_ms > RETRY_TIMEOUT_MS) && (retry_nb < MAX_RETRIES))
      {
        Serial.println("lora_com: Retry");
        retry_nb++;
        device_state = DEVICE_STATE_SEND;
      }
      else if (ack_received_b)
      {
        Serial.println("lora_com: ACK received");
        device_state = DEVICE_STATE_IDLE;
      }
      else if (retry_nb >= MAX_RETRIES)
      {
        Serial.println("lora_com: Transmission failed");
        device_state = DEVICE_STATE_IDLE;
      }
      break;
    }
    case DEVICE_STATE_IDLE:
    {
      if (new_msg_to_be_send_b)
      {
        device_state = DEVICE_STATE_SEND;
      }

      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      tx_duty_cycle_time = app_tx_duty_cycle;
      Lorawan.cycle(tx_duty_cycle_time);
      device_state = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      Lorawan.sleep(lorawan_class);
      break;
    }
    default:
    {
      device_state = DEVICE_STATE_INIT;
      break;
    }
  }
}

void lora_com_init(void)
{
  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
}

void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
  Serial.printf("lora_com: +REV DATA:%s,RXSIZE %d,PORT %d\r\n",mcpsIndication->RxSlot?"RXWIN2":"RXWIN1",mcpsIndication->BufferSize,mcpsIndication->Port);
  Serial.print("lora_com: +REV DATA:");
  for(uint8_t i=0;i<mcpsIndication->BufferSize;i++)
  {
    Serial.printf("%02X",mcpsIndication->Buffer[i]);
  }
  Serial.println();

  ulti_protocol_decode_message(mcpsIndication->Buffer, mcpsIndication->BufferSize);
}
