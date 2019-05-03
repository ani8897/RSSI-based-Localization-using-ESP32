from queue import Queue
from collections import OrderedDict

## Shared between MQTT listener and Data collector
data_queue = Queue()

## Stores positions of devices encountered so far
positions = OrderedDict()

## Coordinates of the anchor nodes
ANCHORS = {
	'3C:71:BF:99:F9:E0' : [0,0],
	'24:0A:C4:23:D9:B0' : [520,580],
	'24:0A:C4:23:C1:EC'	: [980,0]
}

## RSSI Model
REF_DISTANCE = 20
REF_RSSI = -70
PATH_LOSS_EXPONENT = 2.7

TIME_WINDOW = 15