#!/usr/bin/env python
from ScoreboardConnection import ScoreboardConnection 
import time

def test_cb(score_home, score_away):
	print(str(score_home) + ": " + str(score_away))

if __name__ == '__main__':
	scoreboard_connection = ScoreboardConnection("cb2736dd-9b37-471c-ae3e-7836a482520c", "fae04c61cb694c9e", test_cb)
	scoreboard_connection.init()
	cnt = 0
	while(1):
		time.sleep(1)
		cnt = cnt +1
		if cnt == 10:
			scoreboard_connection.publish_new_score(2, 2)
