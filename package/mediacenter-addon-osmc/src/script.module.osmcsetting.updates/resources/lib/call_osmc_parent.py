import sys
import socket
import json

if len(sys.argv) > 1:

	msg = sys.argv[1]

	print 'OSMC settings sending response, %s' % msg

	address = '/var/tmp/osmc.settings.sockfile'

	sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
	sock.connect(address)

	sock.sendall(msg) 
	sock.close()

	print 'OSMC settings sent response, %s' % msg

	