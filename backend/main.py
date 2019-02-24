import json
import argparse
import paho.mqtt.client as paho


def rssi_callback(client, userdata, message):
	m = str(message.payload.decode("utf-8")).rstrip('\n').replace('\'','\"')
	data = json.loads(m)
	print(*data.items())

def csi_callback(client, userdata, message):
	m = str(message.payload.decode("utf-8")).rstrip('\n').replace('\'','\"')
	data = json.loads(m)
	print(*data.items())

def on_connect(client, userdata, flags, rc):
	print("Connected to broker")
	
	rssi_topic, csi_topic = "/rssi/#", "/csi/#"
	client.subscribe(rssi_topic)
	client.subscribe(csi_topic)
	print("Subscribed to topics:[%s,%s]"%(rssi_topic,csi_topic))

	client.message_callback_add(rssi_topic, rssi_callback)
	client.message_callback_add(csi_topic, csi_callback)

def connect(broker_ip, broker_port):
	client= paho.Client("Localization listener") 
	client.on_connect = on_connect

	client.connect(broker_ip,broker_port)

	client.loop_forever()

if __name__ == '__main__':
	
	parser = argparse.ArgumentParser()
	parser.add_argument("-i","--host", type=str, help="Host IP of broker", required=True)
	parser.add_argument("-p","--port", type=int, help="Port", required=True )

	args = parser.parse_args()

	connect(args.host, args.port)
