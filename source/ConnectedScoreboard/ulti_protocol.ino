
#define NEW_SCORE_ONFIELD_ID 0x80
#define NEW_SCORE_ONFIELD_LENGTH 4

#define NEW_ONLINE_SCORE_MSG_ID 0x20
#define NEW_ONLINE_SCORE_MSG_LENGTH 4

#define GAME_START_MSG_ID 0x40
#define GAME_START_MSG_LENGTH 2

#define GAME_END_MSG_ID 0x60
#define GAME_END_MSG_LENGTH 2

RTC_DATA_ATTR uint8_t last_msg_cnt = 0;
RTC_DATA_ATTR bool first_msg_receveived_b = false;

RTC_DATA_ATTR uint8_t tx_msg_cnt = 0;

void ulti_protocol_send_new_score(uint8_t score_away, uint8_t score_home)
{
  uint8_t msg_frame[NEW_SCORE_ONFIELD_LENGTH] = {};
  msg_frame[0] = NEW_SCORE_ONFIELD_ID;
  msg_frame[1] = score_home;
  msg_frame[2] = score_away;
  msg_frame[3] = tx_msg_cnt++;

  lora_com_send_message(msg_frame, NEW_SCORE_ONFIELD_LENGTH);
}

void ulti_protocol_print_msg(uint8_t msg_frame[], uint8_t msg_size)
{
  for (uint8_t id = 0; id < msg_size; id++)
  {
    Serial.print(msg_frame[id]);
    Serial.print(" ");
  }
  Serial.println("");
}


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
      if (ulti_protocol_validate_msg(msg_frame, msg_size, NEW_ONLINE_SCORE_MSG_ID))
      {
        Serial.println("ulti_protocol: New Score");
        score_control_set_new_game_score(msg_frame[1], msg_frame[2]);
      } else
      {
        ulti_protocol_print_msg(msg_frame, msg_size);
      }
      break;
    }
    case GAME_START_MSG_ID:
    {
      if (ulti_protocol_validate_msg(msg_frame, msg_size, GAME_START_MSG_ID))
      {
        Serial.println("ulti_protocol: Game Start");
      } else
      {
        ulti_protocol_print_msg(msg_frame, msg_size);
      }
      break;
    }
    case GAME_END_MSG_ID:
    {
      if (ulti_protocol_validate_msg(msg_frame, msg_size, GAME_END_MSG_ID))
      {
        Serial.println("ulti_protocol: Game End");
      } else
      {
        ulti_protocol_print_msg(msg_frame, msg_size);
      }
      break;
    }
    default:
    {
      Serial.print("ulti_protocol: Unknown Msg Id: ");
      ulti_protocol_print_msg(msg_frame, msg_size);
      break;
    }
  }
}

bool ulti_protocol_validate_msg(uint8_t msg_frame[], uint8_t msg_size, uint8_t msg_id)
{
  if ((msg_id == GAME_START_MSG_ID) && (msg_size != GAME_START_MSG_LENGTH))
  {
    Serial.println("ulti_protocol: Invalid msg length");
    return false;
  }
  if ((msg_id == NEW_ONLINE_SCORE_MSG_ID) && (msg_size != NEW_ONLINE_SCORE_MSG_LENGTH))
  {
    Serial.println("ulti_protocol: Invalid msg length");
    return false;
  }
  if ((msg_id == GAME_END_MSG_ID) && (msg_size != GAME_END_MSG_LENGTH))
  {
    Serial.println("ulti_protocol: Invalid msg length");
    return false;
  }
  
  
  if (first_msg_receveived_b)
  {
    if (msg_frame[msg_size-1] == last_msg_cnt)
    {
      Serial.println("ulti_protocol: Repeated message");
      return false;
    }
  }
  else
  {
    first_msg_receveived_b = true;
  }

  return true;
}