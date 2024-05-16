
#define NEW_SCORE_ONFIELD_ID 0x80
#define NEW_SCORE_ONFIELD_LENGTH 4
void ulti_protocol_send_new_score(uint8_t score_away, uint8_t score_home)
{
  uint8_t msg_frame[NEW_SCORE_ONFIELD_LENGTH] = {};
  msg_frame[0] = NEW_SCORE_ONFIELD_ID;
  msg_frame[1] = score_home;
  msg_frame[2] = score_away;
  msg_frame[3] = 0; //TODO: CRC

  lora_com_send_message(msg_frame, NEW_SCORE_ONFIELD_LENGTH);
}

#define NEW_ONLINE_SCORE_MSG_ID 0x20
#define NEW_ONLINE_SCORE_MSG_LENGTH 4

#define GAME_START_MSG_ID 0x40
#define GAME_START_MSG_LENGTH 2

#define GAME_END_MSG_ID 0x60
#define GAME_END_MSG_LENGTH 2


/**
  MSG ID: 0x20 - New Score
          0x40 - Game Start
          0x60 - Game End
**/
void ulti_protocol_decode_message(uint8_t msg_frame[], uint8_t msg_size)
{
  if (msg_size == 0)
  {
    Serial.println("ulti_protocol: Invalid msg size 0");
    return;
  }
  switch (msg_frame[0])
  {
    case NEW_ONLINE_SCORE_MSG_ID:
    {
      Serial.println("ulti_protocol: New Score");
      score_control_set_new_game_score(msg_frame[1], msg_frame[2]);
      break;
    }
    case GAME_START_MSG_ID:
    {
      Serial.println("ulti_protocol: Game Start");
      break;
    }
    case GAME_END_MSG_ID:
    {
      Serial.println("ulti_protocol: Game End");
      break;
    }
    default:
    {
      Serial.print("ulti_protocol: Unknown Msg Id: ");
      Serial.println(msg_frame[0]);
      break;
    }
  }
}

bool ulti_protocol_validate_msg(uint8_t msg_frame[], uint8_t msg_size, uint8_t msg_id)
{
  // TODO
  return true;
}