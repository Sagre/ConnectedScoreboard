#ifndef SERVO_COM
#define SERVO_COM
#include "Arduino.h"

enum servo_com_channels_e {CHANNEL_1, CHANNEL_2, CHANNEL_3, CHANNEL_4, CHANNEL_NUM};
void servo_com_init(void);
void servo_com_set_target(servo_com_channels_e channel_e, uint16_t deg);
#endif /*SERVO_COM*/