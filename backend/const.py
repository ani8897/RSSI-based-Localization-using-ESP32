from queue import Queue

## Shared between MQTT listener and Data collector
data_queue = Queue()

## Stores positions of devices encountered so far
positions = {}

## Coordinates of the anchor nodes
ANCHORS = {
	'3C:71:BF:99:F9:E0' : [2,3],
	'24:0A:C4:23:D9:B0' : [3,2]
}

## RSSI Model
REF_DISTANCE = 10
REF_RSSI = -28
PATH_LOSS_EXPONENT = 2