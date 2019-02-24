import json
import argparse
import paho.mqtt.client as paho

def on_connect(client, userdata, flags, rc):
	topic = "/loc/#"
	print("Connected to broker")
	client.subscribe(topic)
	print("Subscribed to topic: %s"%topic)

def on_message(client, userdata, message):
	m = str(message.payload.decode("utf-8")).rstrip('\n').replace('\'','\"')
	data = json.loads(m) 
	print(data['MAC'], data['RSSI'], data['Channel'])

def connect(broker_ip, broker_port):
	client= paho.Client("Localization listener") 
	client.on_connect = on_connect
	client.on_message = on_message

	client.connect(broker_ip,broker_port)

	client.loop_forever()

if __name__ == '__main__':
	
	parser = argparse.ArgumentParser()
	parser.add_argument("-i","--host", type=str, help="Host IP of broker", required=True)
	parser.add_argument("-p","--port", type=int, help="Port", required=True )

	args = parser.parse_args()

	connect(args.host, args.port)
