#ifndef SCORE_CONTROL
#define SCORE_CONTROL
struct score_control_motor_status_t {
  uint8_t angle_dec;
  uint8_t angle_sing;
  servo_com_channels_e channel_dec_e;
  servo_com_channels_e channel_sing_e;
};

struct score_control_status_t {
  uint8_t score_home;
  uint8_t score_away;
  score_control_motor_status_t motor_status_home;
  score_control_motor_status_t motor_status_away;
};

enum score_control_team_te {
  TEAM_HOME,
  TEAM_AWAY
};

void score_control_set_new_score(uint8_t score_home, uint8_t score_away);
void score_control_update_angles(uint8_t score, struct score_control_motor_status *p_motor_status);
void score_control_update_motor_positions(void);
void score_control_print(void);

uint8_t score_control_score_to_degree(uint8_t score);
#endif /*SCORE_CONTROL*/