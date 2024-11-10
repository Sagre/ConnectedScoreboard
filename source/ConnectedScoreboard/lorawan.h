


#ifndef Lorawan_H
#define Lorawan_H

#include "ESP32_Mcu.h"
#include <stdio.h>
#include "loramac/LoRaMac.h"
#include "loramac/utilities.h"
#include "ESP32_LoRaWan_102.h"
#include "HardwareSerial.h"
#include "Arduino.h"
#include "driver/board.h"
#include "driver/debug.h"

#if defined(__asr650x__)
#include "board.h"
#include "gpio.h"
#include "hw.h"
#include "low_power.h"
#include "spi-board.h"
#include "rtc-board.h"
#include "asr_timer.h"
#include "board-config.h"
#include "hw_conf.h"
#include <uart_port.h>
#endif

enum device_state_lorawan_e
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_SEND,
    DEVICE_STATE_CYCLE,
    DEVICE_STATE_SLEEP,
    DEVICE_STATE_IDLE,
    DEVICE_STATE_WAIT_FOR_ACK
};

enum device_state_lora_e
{
    LORA_INIT,
    LORA_SEND,
    LORA_RECEIVE,
    LORA_CAD,
    MCU_SLEEP,
};


extern uint8_t dev_eui[];
extern uint8_t app_eui[];
extern uint8_t app_key[];
extern uint8_t nwks_key[];
extern uint8_t apps_key[];
extern uint32_t dev_addr;
extern uint8_t app_data[LORAWAN_APP_DATA_MAX_SIZE];
extern uint8_t app_data_size;
extern uint8_t app_port;
extern uint32_t tx_duty_cycle_time;
extern bool otaa_b;
extern LoRaMacRegion_t lorawan_region;
extern bool lorawan_adr_b;
extern bool is_tx_confirmed_b;
extern uint32_t app_tx_duty_cycle;
extern DeviceClass_t lorawan_class;
extern bool passthrough_mode_b;
extern uint8_t confirmed_nb_trials;
extern bool lorawan_mode_b;
extern bool keep_net_b;
extern uint16_t users_channel_mask[6];
extern RTC_DATA_ATTR TimerEvent_t tx_next_packet_timer;

/*!
 * Defines a random delay for application data transmission duty cycle. 1s,
 * value in [ms].
 */
#define APP_TX_DUTYCYCLE_RND                        1000

class LorawanClass{
public:
  void init(DeviceClass_t lorawanClass,LoRaMacRegion_t region);
  void join();
  void send();
  void cycle(uint32_t dutyCycle);
  void sleep(DeviceClass_t classMode);
  void setDefaultDR(int8_t dataRate);
  void ifskipjoin();
  void generateDeveuiByChipID();
};

extern enum device_state_lorawan_e device_state;

extern "C" bool lorawan_send_frame( void );
extern "C" void turnOnRGB(uint32_t color,uint32_t time);
extern "C" void turnOffRGB(void);
extern "C" bool checkUserAt(char * cmd, char * content);
extern "C" void downLinkAckHandle();
extern "C" void downLinkDataHandle(McpsIndication_t *mcpsIndication);
extern "C" void lorawan_lwan_dev_params_update( void );
extern "C" void dev_time_updated( void );
extern "C" void lorawan_printf(const char *format, ...);
extern "C" void lorawan_OnTxNextPacketTimerEvent( void );
extern "C" bool lorawan_IsConnected(void);

extern LorawanClass Lorawan;
 
#endif // Lorawan_H