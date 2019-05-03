import json
import paho.mqtt.client as paho

import const

def rssi_callback(client, userdata, message):
	m = str(message.payload.decode("utf-8")).rstrip('\n').replace('\'','\"')
	data = json.loads(m)

	anchor_mac = message.topic.split('/')[2]
	device_mac, rssi = data['MAC'], data['RSSI']

	if device_mac == "4C:ED:FB:50:16:ED" or device_mac == "B8:63:4D:A2:0E:13":
		print("Data: ",anchor_mac, *data.items())
	const.data_queue.put((anchor_mac, device_mac, rssi))

def csi_callback(client, userdata, message):
	m = str(message.payload.decode("utf-8")).rstrip('\n').replace('\'','\"')
	data = json.loads(m)
	print(*data.items())

def on_connect(client, userdata, flags, rc):
	# print("Connected to broker")
	
	rssi_topic, csi_topic = "/rssi/#", "/csi/#"
	client.subscribe(rssi_topic)
	client.subscribe(csi_topic)
	# print("Subscribed to topics:[%s,%s]"%(rssi_topic,csi_topic))

	client.message_callback_add(rssi_topic, rssi_callback)
	client.message_callback_add(csi_topic, csi_callback)

def connect(broker_ip, broker_port):
	client= paho.Client("Localization listener") 
	client.on_connect = on_connect

	client.connect(broker_ip,broker_port)

	client.loop_forever()
