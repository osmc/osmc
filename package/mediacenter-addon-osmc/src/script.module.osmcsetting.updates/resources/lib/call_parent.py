import sys
import socket
import json

if len(sys.argv) > 1:

	msg = sys.argv[1]

	print 'OSMC settings sending response, %s' % msg

	address = '/var/tmp/osmc.settings.update.sockfile'

	message = ('settings_command', {'action': msg})

	message = json.dumps(message)

	sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
	sock.connect(address)

	sock.sendall(message) 
	sock.close()

	print 'OSMC settings sent response, %s' % msg

	