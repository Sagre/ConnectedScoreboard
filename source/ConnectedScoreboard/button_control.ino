#include "lorawan.h"
#include "driver/rtc_io.h"

#define INCREASE_AWAY GPIO_NUM_7
#define DECREASE_AWAY GPIO_NUM_6
#define INCREASE_HOME GPIO_NUM_5
#define DECREASE_HOME GPIO_NUM_4

#define GPIO_BITMASK (1 << INCREASE_AWAY | 1 << DECREASE_AWAY | 1 << INCREASE_HOME | 1 << DECREASE_HOME)

#define COMMAND_INCREASE_AWAY 1
#define DEBOUNCE_MS 400

RTC_DATA_ATTR uint8_t hello = 0;

void button_control_init()
{
  pinMode(INCREASE_AWAY, INPUT_PULLUP);
  rtc_gpio_pullup_en(INCREASE_AWAY);
  rtc_gpio_pulldown_dis(INCREASE_AWAY);
  rtc_gpio_hold_en(INCREASE_AWAY);

  pinMode(DECREASE_AWAY, INPUT_PULLUP);
  rtc_gpio_pullup_en(DECREASE_AWAY);
  rtc_gpio_pulldown_dis(DECREASE_AWAY);
  rtc_gpio_hold_en(DECREASE_AWAY);

  pinMode(INCREASE_HOME, INPUT_PULLUP);
  rtc_gpio_pullup_en(INCREASE_HOME);
  rtc_gpio_pulldown_dis(INCREASE_HOME);
  rtc_gpio_hold_en(INCREASE_HOME);

  pinMode(DECREASE_HOME, INPUT_PULLUP);
  rtc_gpio_pullup_en(DECREASE_HOME);
  rtc_gpio_pulldown_dis(DECREASE_HOME);
  rtc_gpio_hold_en(DECREASE_HOME);

  delay(1000);
  esp_sleep_enable_gpio_switch(false);
  esp_sleep_enable_ext1_wakeup(GPIO_BITMASK, ESP_EXT1_WAKEUP_ANY_LOW);
}


#define DETECTION_TIME_MS 50

void button_control_determine_button_command()
{
  static RTC_DATA_ATTR bool detection_running_b = false;
  static RTC_DATA_ATTR uint32_t detection_start_ms = 0;
  static RTC_DATA_ATTR bool reacted_b = false;

  uint8_t button_command = ((~(digitalRead(INCREASE_AWAY) | digitalRead(DECREASE_AWAY) << 1 | digitalRead(INCREASE_HOME) << 2 | digitalRead(DECREASE_HOME) << 3)) & 0b1111);

  if (0 != button_command)
  {
    // Button pressed
    if (!detection_running_b)
    {
      detection_running_b = true;
      detection_start_ms = millis();
    }
    else
    {
      // Detection already running, check timeout
      if ((millis() - detection_start_ms > DETECTION_TIME_MS) && !reacted_b)
      {
        reacted_b = true;
        switch (button_command)
        {
          case COMMAND_INCREASE_AWAY:
          {
            score_control_increment_score(TEAM_AWAY);
            
            break;
          }
          default:
          {

          }
        }
      }
    }
  }
  else
  {
    reacted_b = false;
    detection_running_b = false;
  }

}
