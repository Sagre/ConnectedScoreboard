#include <servo_com.h>
#include <score_control.h>

uint32_t last_servo_update = 0;
#define MIN_SERVO_DELAY 1000

const int DEGREE_LUT[10] = {0, 9, 18, 27, 36, 45, 54, 63, 72, 81};

RTC_DATA_ATTR score_control_status_t score_control_status { .score_home = 0,
                                              .score_away = 0,
                                              .motor_status_home = {.angle_dec = 0,
                                                                    .angle_sing = 0,
                                                                    .channel_dec_e = CHANNEL_1,
                                                                    .channel_sing_e = CHANNEL_2},
                                              .motor_status_away = {.angle_dec = 0,
                                                                    .angle_sing = 0,
                                                                    .channel_dec_e = CHANNEL_3,
                                                                    .channel_sing_e = CHANNEL_4}
                                            };

void score_control_set_new_game_score(uint8_t score_home, uint8_t score_away)
{
  score_control_status.score_home = score_home;
  score_control_status.score_away = score_away;
  score_control_print();
}

void score_control_increment_score(score_control_team_te team_e)
{
  if (TEAM_HOME == team_e)
  {
    score_control_status.score_home++;
  }
  else if (TEAM_AWAY == team_e)
  {
    score_control_status.score_away++;
  }

  score_control_print();
  ulti_protocol_send_new_score(score_control_status.score_away, score_control_status.score_home);
}

void score_control_update_angles(void)
{
  score_control_update_angle(score_control_status.score_home, &(score_control_status.motor_status_home));
  score_control_update_angle(score_control_status.score_away, &(score_control_status.motor_status_away));
}

void score_control_update_angle(uint8_t score, struct score_control_motor_status_t *p_motor_status)
{
  p_motor_status->angle_dec = score_control_number_to_degree(score/10);
  p_motor_status->angle_sing = score_control_number_to_degree(score%10);
}

void score_control_update_motor_positions(void)
{
  if ((millis() - last_servo_update) > MIN_SERVO_DELAY)
  {
    servo_com_set_target(score_control_status.motor_status_home.channel_dec_e,
                          score_control_status.motor_status_home.angle_dec);
    servo_com_set_target(score_control_status.motor_status_home.channel_sing_e,
                          score_control_status.motor_status_home.angle_sing);
                          
    servo_com_set_target(score_control_status.motor_status_away.channel_dec_e,
                          score_control_status.motor_status_away.angle_dec);
    servo_com_set_target(score_control_status.motor_status_away.channel_sing_e,
                          score_control_status.motor_status_away.angle_sing);
    last_servo_update = millis();
  }

}

uint8_t score_control_number_to_degree(uint8_t score)
{
  if (score < 10)
  {
    return DEGREE_LUT[score];
  }
  else
  {
    return 0;
  }
  
}

void score_control_print(void)
{
  Serial.print("Home ");
  Serial.print(score_control_status.score_home);
  Serial.print(" - ");
  Serial.print(score_control_status.score_away);
  Serial.println(" Away");
}

