import time
import argparse
import threading

import const

from mqtt import *
from util import *

from scipy.optimize import minimize

def localize(rssi_data):
	"""
	Localizes devices based on the rssi data passed
	"""
	def loss(e, device_mac):
		return sum([ ( dist(e, const.ANCHORS[a]) - rssi_model(rssi_data[a][device_mac])**2 )**2 for a in const.ANCHORS])

	anchor_check = {}
	for a in rssi_data:
		for d in rssi_data[a]:

			if d in anchor_check:
				anchor_check[d].append(a)
			else:
				anchor_check[d] = [a]

			if len(anchor_check[d]) == len(const.ANCHORS):
				res = minimize(loss, get_position(d), args=(d))
				const.positions[d] = (res.x, res.success)

	
def data_listener():
	"""
	Listens to the queue filled up by MQTT listener
	"""
	rssi_data = {a: {} for a in const.ANCHORS}

	while True:

		base_time = time.time()

		while time.time() - base_time < 5:
			anchor_mac, device_mac, rssi = const.data_queue.get()
			
			if device_mac in rssi_data:
				rssi_data[anchor_mac].append(rssi) 
			else:
				rssi_data[anchor_mac][device_mac] = [rssi]

		for a in rssi_data:
			for d in rssi_data[a]:
				rssi_data[a][d] = (1. * sum(rssi_data[a][d])) / len(rssi_data[a][d])

		localize(rssi_data)
		rssi_data = {a: {} for a in const.ANCHORS}
		print_positions()

def print_positions():
	"""
	Prints positions of the devices encountered so far
	"""	
	for device_mac in const.positions:
		print(device_mac, *const.positions[device_mac][0], const.positions[device_mac][1])
	print()


if __name__ == '__main__':
	
	parser = argparse.ArgumentParser()
	parser.add_argument("-i","--host", type=str, help="Host IP of broker", required=True)
	parser.add_argument("-p","--port", type=int, help="Port", required=True)

	args = parser.parse_args()

	mqtt_thread = threading.Thread(target=connect, args=(args.host, args.port))
	mqtt_thread.start()

	data_listener()
