import paho.mqtt.client as mqtt
import time
from google.protobuf.json_format import Parse
import codecs
from chirpstack_api import integration
import base64
import json

client_lut = {}

def on_connect(client, userdata, flags, reason_code, properties):
	print(f"Connected with result code {reason_code}")
	if userdata[1] in client_lut:
		topic = "application/" + str(userdata[0]) +"/device/" + str(userdata[1]) + "/#"
		client.subscribe(topic)

def on_message(client, userdata, msg):
	if userdata[1] in client_lut:
		client_lut[userdata[1]].on_message(userdata, msg)
	else:
		print("Message received for unknown device: " + str(msg.topic))

class ScoreboardConnection:
	app_id = 0
	dev_id = 0
	last_recv_msg_id = -1
	tx_msg_cnt = 0
	mqttc = None
	new_score_callback = None

	def __init__(self, app_id, dev_id, new_score_callback):
		global client_lut
		self.app_id = app_id
		self.dev_id = dev_id
		client_lut[dev_id] = self
		self.new_score_callback = new_score_callback
				
	def log(self, log_msg):
		print(self.dev_id + ": " + str(log_msg))

	def init(self):
		self.log("Init")

		self.mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
		self.mqttc.on_connect = on_connect
		self.mqttc.on_message = on_message
		self.mqttc.user_data_set([self.app_id, self.dev_id])
		self.mqttc.connect("127.0.0.1", 1883, 60)
		self.mqttc.loop_start()

	def handle_event(self, payload):
		try:
			up = Parse(payload, integration.UplinkEvent())
			hex_data = bytearray.fromhex(up.data.hex())
		except:
			self.log("Failed to parse msg: " + str(payload))
			return

		self.log(hex_data)
		
		if len(hex_data) == 0:
			self.log("Received empty data, ignoring...")
			return

		match hex_data[0]:
			case 128:
				if len(hex_data) != 4:
					self.log("Invalid length of New Score msg. Ignoring...")
					return
				if self.last_recv_msg_id == hex_data[3]:
					self.log("Repeated msg. Ignoring...")
					return
				
				self.last_recv_msg_id = hex_data[3]
				self.new_score_callback(int(hex_data[1]), int(hex_data[2]))
			case _:
				self.log("Unhandled payload message" + str(hex_data[0]))


	def on_message(self, userdata, msg):	
		msg_type = msg.topic.split("/")[4]

		match msg_type:
			case "event":
				self.log("Event message")
				self.handle_event(msg.payload)
			case _:
				self.log("Unhandled message type: " + str(msg_type))

	def publish_new_score(self, score_home, score_away):
		self.publish(32, [score_home, score_away])

	def publish(self, msg_id, data):
		data_to_send = [msg_id]
		data_to_send.extend(data)
		data_to_send.append(self.tx_msg_cnt)

		self.tx_msg_cnt = self.tx_msg_cnt+1
		
		data_to_send_b64 = base64.b64encode(bytes(data_to_send))
		
		down_packet = {"devEui": str(self.dev_id), "fPort": 2, "data": str(data_to_send_b64)}

		json_packet_to_send = json.dumps(down_packet)
		self.log(json_packet_to_send)
		topic = "application/" + self.app_id + "/device/" + self.dev_id + "/command/down"
		self.log(topic)
		self.mqttc.publish(topic, json_packet_to_send, 0, False)
