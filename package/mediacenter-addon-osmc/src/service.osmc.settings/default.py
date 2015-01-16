import socket
import xbmc

def log(message):
	xbmc.log(str(message))

log('default started')

address = '/var/run/osmc.settings.sockfile'
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect(address)
sock.sendall('open')
# sock.send('new_device:farts')
sock.close()

log('default closing')


