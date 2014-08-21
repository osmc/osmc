#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  example.py - demonstrating interaction with libosmc, cp libosmc.so to /usr/lib first
#  
#  Copyright 2014 Sam Nazarko <email@samnazarko.co.uk>

from ctypes import *

def main():
	# Load the library
	libosmc = cdll.LoadLibrary("libosmc.so") 
	# Function definitions
	
	# identity.h
	# ----------
	#get_hostname = libosmc.get_hostname
	#get_hostname.restype = c_char_p
	#get_hostname.argtypes = []
	#print "The hostname is " + get_hostname()
	
	#set_hostname = libosmc.set_hostname
	#set_hostname.argtypes = [c_char_p]
	#print "Setting hostname to osmc"
	#result = set_hostname("osmc")
	#if result == 0:
	#	print "Success"
	#	print "The hostname is " + get_hostname()
	#else:
	#	print "Failed -- are we root?"
	
	# services.h
	# ----------
	
	#is_service_enabled = libosmc.is_service_enabled
	#is_service_enabled.argtypes = [c_char_p]
	#result = is_service_enabled("mediacenter")
	#if result == 0:
	#	print "The service is enabled"
	#else:
	#	print "The service is not enabled or does not exist"
	
	#enable_service = libosmc.enable_service
	#enable_service.argtypes = [c_char_p]
	#disable_service = libosmc.disable_service
	#disable_service.argtypes = [c_char_p]
	
	# updating.h
	# ------------
	
	
	
	
	return 0

if __name__ == '__main__':
	main()

