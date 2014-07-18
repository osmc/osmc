import time
from struct import *
from Shairplay import *

hwaddr = pack('BBBBBB', 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB)
class SampleCallbacks(RaopCallbacks):
	def audio_init(self, bits, channels, samplerate):
		print "Initializing", bits, channels, samplerate
	def audio_process(self, session, buffer):
		print "Processing", + len(buffer), "bytes of audio"
	def audio_destroy(self, session):
		print "Destroying"
	def audio_set_volume(self, session, volume):
		print "Set volume to", volume
	def audio_set_metadata(self, session, metadata):
		print "Got", len(metadata),  "bytes of metadata"
	def audio_set_coverart(self, session, coverart):
		print "Got", len(coverart), "bytes of coverart"

shairplay = LoadShairplay(".")
callbacks = SampleCallbacks()

def log_callback(level, message):
	print "Level", level, ":", message

raop = RaopService(shairplay, 10, callbacks)
raop.set_log_level(RaopLogLevel.DEBUG)
raop.set_log_callback(log_callback)
port = raop.start(5000, hwaddr)

dnssd = DnssdService(shairplay)
dnssd.register_raop("RAOP test", port, hwaddr)

time.sleep(50)

dnssd.unregister_raop()
raop.stop()

del dnssd
del raop

