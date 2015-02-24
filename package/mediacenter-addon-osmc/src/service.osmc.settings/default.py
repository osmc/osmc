import socket
import xbmc
import xbmcgui
import xbmcaddon

__addon__        = xbmcaddon.Addon()
__setting__      = __addon__.getSetting
DIALOG           = xbmcgui.Dialog()

def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 

def log(message):
	xbmc.log(str(message), level=xbmc.LOGDEBUG)


log('default started')


try:

	address = '/var/tmp/osmc.settings.sockfile'
	sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
	sock.connect(address)
	sock.sendall('open')
	sock.close()

except:

	log('default failed to open')

	ok = DIALOG.ok(lang(32007), lang(32005), lang(32006))

log('default closing')


