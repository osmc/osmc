''' This script is run as root by the osmc update module. '''

import apt
import socket
import sys
from datetime import datetime
import json

t = datetime

class Logger(object):
	def __init__(self, filename="Default.log"):
		self.terminal = sys.stdout
		self.log = open(filename, "a")

	def write(self, message):
		self.terminal.write(message)
		self.log.write(message)

# try:
# 	sys.stdout = Logger("/home/kubkev/test.txt")
# except:
# 	pass


def call_parent(raw_message, data={}):

	address = '/var/tmp/osmc.settings.update.sockfile'
	
	print '%s %s sending response' % (t.now(), 'apt_cache_action.py')

	message = (raw_message, data)

	message = json.dumps(message)

	try:
		sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
		sock.connect(address)
		sock.sendall(message) 
		sock.close()

	except Exception as e:
		print '%s %s failed to connect to parent - %s' % (t.now(), 'apt_cache_action.py', e)

	print '%s %s response sent' % (t.now(), 'apt_cache_action.py')


if __name__ == "__main__":

	if len(sys.argv) > 1:

		call_parent('%s' % sys.argv[1])