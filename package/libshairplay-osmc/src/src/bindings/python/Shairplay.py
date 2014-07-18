# coding=utf-8
'''
Copyright (C) 2012  Juho Vähä-Herttua

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
'''

import os
import sys
import platform

from ctypes import *

RSA_KEY = """
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEA59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUt
wC5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDRKSKv6kDqnw4U
wPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuBOitnZ/bDzPHrTOZz0Dew0uowxf
/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJQ+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/
UAaHqn9JdsBWLUEpVviYnhimNVvYFZeCXg/IdTQ+x4IRdiXNv5hEewIDAQABAoIBAQDl8Axy9XfW
BLmkzkEiqoSwF0PsmVrPzH9KsnwLGH+QZlvjWd8SWYGN7u1507HvhF5N3drJoVU3O14nDY4TFQAa
LlJ9VM35AApXaLyY1ERrN7u9ALKd2LUwYhM7Km539O4yUFYikE2nIPscEsA5ltpxOgUGCY7b7ez5
NtD6nL1ZKauw7aNXmVAvmJTcuPxWmoktF3gDJKK2wxZuNGcJE0uFQEG4Z3BrWP7yoNuSK3dii2jm
lpPHr0O/KnPQtzI3eguhe0TwUem/eYSdyzMyVx/YpwkzwtYL3sR5k0o9rKQLtvLzfAqdBxBurciz
aaA/L0HIgAmOit1GJA2saMxTVPNhAoGBAPfgv1oeZxgxmotiCcMXFEQEWflzhWYTsXrhUIuz5jFu
a39GLS99ZEErhLdrwj8rDDViRVJ5skOp9zFvlYAHs0xh92ji1E7V/ysnKBfsMrPkk5KSKPrnjndM
oPdevWnVkgJ5jxFuNgxkOLMuG9i53B4yMvDTCRiIPMQ++N2iLDaRAoGBAO9v//mU8eVkQaoANf0Z
oMjW8CN4xwWA2cSEIHkd9AfFkftuv8oyLDCG3ZAf0vrhrrtkrfa7ef+AUb69DNggq4mHQAYBp7L+
k5DKzJrKuO0r+R0YbY9pZD1+/g9dVt91d6LQNepUE/yY2PP5CNoFmjedpLHMOPFdVgqDzDFxU8hL
AoGBANDrr7xAJbqBjHVwIzQ4To9pb4BNeqDndk5Qe7fT3+/H1njGaC0/rXE0Qb7q5ySgnsCb3DvA
cJyRM9SJ7OKlGt0FMSdJD5KG0XPIpAVNwgpXXH5MDJg09KHeh0kXo+QA6viFBi21y340NonnEfdf
54PX4ZGS/Xac1UK+pLkBB+zRAoGAf0AY3H3qKS2lMEI4bzEFoHeK3G895pDaK3TFBVmD7fV0Zhov
17fegFPMwOII8MisYm9ZfT2Z0s5Ro3s5rkt+nvLAdfC/PYPKzTLalpGSwomSNYJcB9HNMlmhkGzc
1JnLYT4iyUyx6pcZBmCd8bD0iwY/FzcgNDaUmbX9+XDvRA0CgYEAkE7pIPlE71qvfJQgoA9em0gI
LAuE4Pu13aKiJnfft7hIjbK+5kyb3TysZvoyDnb3HOKvInK7vXbKuU4ISgxB2bB3HcYzQMGsz1qJ
2gG0N5hvJpzwwhbhXqFKA4zaaSrw622wDniAK5MlIE0tIAKKP4yxNGjoD2QYjhBGuhvkWKY=
-----END RSA PRIVATE KEY-----
"""

class RaopLogLevel:
	EMERG   = 0
	ALERT   = 1
	CRIT    = 2
	ERR     = 3
	WARNING = 4
	NOTICE  = 5
	INFO    = 6
	DEBUG   = 7
	
raop_log_callback_prototype =   CFUNCTYPE(None, c_void_p, c_int, c_char_p)

audio_init_prototype =          CFUNCTYPE(py_object, c_void_p, c_int, c_int, c_int)
audio_process_prototype =       CFUNCTYPE(None, c_void_p, c_void_p, c_void_p, c_int)
audio_destroy_prototype =       CFUNCTYPE(None, c_void_p, c_void_p)

audio_flush_prototype =         CFUNCTYPE(None, c_void_p, c_void_p)
audio_set_volume_prototype =    CFUNCTYPE(None, c_void_p, c_void_p, c_float)
audio_set_metadata_prototype =  CFUNCTYPE(None, c_void_p, c_void_p, c_void_p, c_int)
audio_set_coverart_prototype =  CFUNCTYPE(None, c_void_p, c_void_p, c_void_p, c_int)

class RaopNativeCallbacks(Structure):
	_fields_ = [("cls",                 py_object),
	            ("audio_init",          audio_init_prototype),
	            ("audio_process",       audio_process_prototype),
	            ("audio_destroy",       audio_destroy_prototype),
	            ("audio_flush",         audio_flush_prototype),
	            ("audio_set_volume",    audio_set_volume_prototype),
	            ("audio_set_metadata",  audio_set_metadata_prototype),
	            ("audio_set_coverart",  audio_set_coverart_prototype)]

def InitShairplay(libshairplay):
	# Initialize dnssd related functions
	libshairplay.dnssd_init.restype = c_void_p
	libshairplay.dnssd_init.argtypes = [POINTER(c_int)]
	libshairplay.dnssd_register_raop.restype = c_int
	libshairplay.dnssd_register_raop.argtypes = [c_void_p, c_char_p, c_ushort, POINTER(c_char), c_int, c_int]
	libshairplay.dnssd_register_airplay.restype = c_int
	libshairplay.dnssd_register_airplay.argtypes = [c_void_p, c_char_p, c_ushort, POINTER(c_char), c_int]
	libshairplay.dnssd_unregister_raop.restype = None
	libshairplay.dnssd_unregister_raop.argtypes = [c_void_p]
	libshairplay.dnssd_unregister_airplay.restype = None
	libshairplay.dnssd_unregister_airplay.argtypes = [c_void_p]
	libshairplay.dnssd_destroy.restype = None
	libshairplay.dnssd_destroy.argtypes = [c_void_p]

	# Initialize raop related functions
	libshairplay.raop_init.restype = c_void_p
	libshairplay.raop_init.argtypes = [c_int, POINTER(RaopNativeCallbacks), c_char_p, POINTER(c_int)]
	libshairplay.raop_set_log_level.restype = None
	libshairplay.raop_set_log_level.argtypes = [c_void_p, c_int]
	libshairplay.raop_set_log_callback.restype = None
	libshairplay.raop_set_log_callback.argtypes = [c_void_p, raop_log_callback_prototype, c_void_p]
	libshairplay.raop_is_running.restype = c_int
	libshairplay.raop_is_running.argtypes = [c_void_p]
	libshairplay.raop_start.restype = c_int
	libshairplay.raop_start.argtypes = [c_void_p, POINTER(c_ushort), POINTER(c_char), c_int, c_char_p]
	libshairplay.raop_stop.restype = None
	libshairplay.raop_stop.argtypes = [c_void_p]
	libshairplay.raop_destroy.restype = None
	libshairplay.raop_destroy.argtypes = [c_void_p]

def LoadShairplay(path):
	if sys.maxsize < 2**32:
		libname = "shairplay32"
	else:
		libname = "shairplay64"

	if platform.system() == "Windows":
		libname = libname + ".dll"
	elif platform.system() == "Darwin":
		libname = "lib" + libname + ".dylib"
	else:
		libname = "lib" + libname + ".so"

	try:
		fullpath = os.path.join(path, libname)
		libshairplay = cdll.LoadLibrary(fullpath)
	except:
		raise RuntimeError("Couldn't load shairplay library " + libname)

	InitShairplay(libshairplay)
	return libshairplay

class RaopCallbacks:
	def audio_init(self, bits, channels, samplerate):
		raise NotImplementedError()

	def audio_process(self, session, buffer):
		raise NotImplementedError()

	def audio_destroy(self, session):
		raise NotImplementedError()

	def audio_set_volume(self, session, volume):
		pass

	def audio_flush(self, session):
		pass

	def audio_set_metadata(self, session, buffer):
		pass

	def audio_set_coverart(self, session, buffer):
		pass

class RaopService:
	def audio_init_cb(self, cls, bits, channels, samplerate):
		session = self.callbacks.audio_init(bits, channels, samplerate)
		self.sessions.append(session)
		return session

	def audio_process_cb(self, cls, sessionptr, buffer, buflen):
		session = cast(sessionptr, py_object).value
		strbuffer = string_at(buffer, buflen)
		self.callbacks.audio_process(session, strbuffer)

	def audio_destroy_cb(self, cls, sessionptr):
		session = cast(sessionptr, py_object).value
		self.callbacks.audio_destroy(session)
		if session in self.sessions:
			self.sessions.remove(session)

	def audio_flush_cb(self, cls, sessionptr):
		session = cast(sessionptr, py_object).value
		self.callbacks.audio_flush(session)

	def audio_set_volume_cb(self, cls, sessionptr, volume):
		session = cast(sessionptr, py_object).value
		self.callbacks.audio_set_volume(session, volume)

	def audio_set_metadata_cb(self, cls, sessionptr, buffer, buflen):
		session = cast(sessionptr, py_object).value
		strbuffer = string_at(buffer, buflen)
		self.callbacks.audio_set_metadata(session, strbuffer)

	def audio_set_coverart_cb(self, cls, sessionptr, buffer, buflen):
		session = cast(sessionptr, py_object).value
		strbuffer = string_at(buffer, buflen)
		self.callbacks.audio_set_coverart(session, strbuffer)


	def __init__(self, libshairplay, max_clients, callbacks):
		self.libshairplay = libshairplay
		self.callbacks = callbacks
		self.sessions = []
		self.instance = None

		# We need to hold a reference to native_callbacks
		self.native_callbacks = RaopNativeCallbacks()
		self.native_callbacks.audio_init = audio_init_prototype(self.audio_init_cb)
		self.native_callbacks.audio_process = audio_process_prototype(self.audio_process_cb)
		self.native_callbacks.audio_destroy = audio_destroy_prototype(self.audio_destroy_cb)
		self.native_callbacks.audio_flush = audio_flush_prototype(self.audio_flush_cb)
		self.native_callbacks.audio_set_volume = audio_set_volume_prototype(self.audio_set_volume_cb)
		self.native_callbacks.audio_set_metadata = audio_set_metadata_prototype(self.audio_set_metadata_cb)
		self.native_callbacks.audio_set_coverart = audio_set_coverart_prototype(self.audio_set_coverart_cb)

		# Initialize the raop instance with our callbacks
		self.instance = self.libshairplay.raop_init(max_clients, pointer(self.native_callbacks), RSA_KEY, None)
		if self.instance == None:
			raise RuntimeError("Initializing library failed")

	def __del__(self):
		if self.instance != None:
			self.libshairplay.raop_destroy(self.instance)
		self.instance = None

	def is_running(self):
		if self.libshairplay.raop_is_running(self.instance):
			return True
		else:
			return False

	def set_log_level(self, level):
		self.libshairplay.raop_set_log_level(self.instance, level)

	def set_log_callback(self, log_callback):
		# Create a new callback function for thread safety
		def log_callback_cb(cls, level, message):
			log_callback(level, message)

		# We need to hold a reference to the log callback instance
		log_callback_ptr = raop_log_callback_prototype(log_callback_cb)
		self.libshairplay.raop_set_log_callback(self.instance, log_callback_ptr, None)
		self.log_callback = log_callback_ptr

	def start(self, port, hwaddrstr, password=None):
		port = c_ushort(port)
		hwaddr = create_string_buffer(hwaddrstr, len(hwaddrstr))

		ret = self.libshairplay.raop_start(self.instance, pointer(port), hwaddr, c_int(len(hwaddr)), password)
		if ret < 0:
			raise RuntimeError("Starting RAOP instance failed")
		return port.value

	def stop(self):
		self.libshairplay.raop_stop(self.instance)

class DnssdService:
	def __init__(self, libshairplay):
		self.libshairplay = libshairplay
		self.instance = None

		error = c_int(0)

		self.instance = self.libshairplay.dnssd_init(pointer(error))
		if self.instance == None:
			raise RuntimeError("Initializing library failed: " + str(error.value))

	def __del__(self):
		if self.instance != None:
			self.libshairplay.dnssd_destroy(self.instance)
		self.instance = None

	def register_raop(self, name, port, hwaddrstr, password=False):
		hwaddr = create_string_buffer(hwaddrstr, len(hwaddrstr))
		use_pw = c_int(0)
		if password:
			use_pw = c_int(1)
		self.libshairplay.dnssd_register_raop(self.instance, name, c_ushort(port), hwaddr, len(hwaddr), use_pw)

	def unregister_raop(self):
		self.libshairplay.dnssd_unregister_raop(self.instance)

	def register_airplay(self, name, port, hwaddrstr):
		hwaddr = create_string_buffer(hwaddrstr, len(hwaddrstr))
		self.libshairplay.dnssd_register_airplay(self.instance, name, c_ushort(port), hwaddr, len(hwaddr))

	def unregister_airplay(self):
		self.libshairplay.dnssd_unregister_airplay(self.instance)

