#!/usr/bin/env python2

import curses
import fcntl
import json
import os
import requests
import sys    
import termios
import traceback

settings = {
	'ip'		: '127.0.0.1',
	'port'		: '80',
	'user'		: '',
	'pssw'		: '',

}

keymap = {
	
	'i' : 'ip',
	'p' : 'port',
	'u' : 'user',
	'w' : 'pssw',
}

try:
	with open('/home/osmc/cli_remote.conf' , 'r') as f:
		lines 		= f.readlines()
		single 		= ''.join(lines)
		raw_sets 	= json.loads(single)
		settings.update(raw_sets)
except:

	print 'USAGE     : cli-remote i=Your_ip_address p=your_port u=your_username w=your_password'
	print 'All the settings are optional. The default will be used in their place if you dont specifiy them.'
	print 'Defaults:'
	print '		ip   : 127.0.0.1'
	print '		port : 80'       
	print '		user : ""'
	print '		pass : ""'
	print ''
	print 'If you are using this script on the device (via ssh or something) then you dont need to put in the IP address.'
	print 'The default of 127.0.0.1 already points to the local host.'
	print ''
	print 'Alternatively, you can save a file called /home/osmc/cli_remote.conf with this:'
	print '{"ip": "your_ip", "port": "your_port", "user" : "your_user", "pssw": "your_pass"}'
	print 'Or just {"port": "your_port"} if that is all you would like to change.'
	print ''

	for arg in sys.argv[1:]:
		try:
			k, v = arg.split('=')
			key = keymap.get(k, None)
			if key is not None:
				settings[key] = v

		except:
			continue


def call(settings, action, params=None):

	url = 'http://%s:%s/jsonrpc' % (settings['ip'], settings['port'])
	headers = {'Content-Type': 'application/json'}

	command = {"jsonrpc":"2.0","method":"%s" % action, "id": 1}
	if params is not None:
		command['params'] = params

	data=json.dumps(command)

	data = data.replace('"true"', 'true').replace('"false"', 'false')

	r = requests.post( url, data=data, headers=headers, auth=(settings['user'], settings['pssw']))


def call_keyboard(settings, text, params=None):

	url = 'http://%s:%s/jsonrpc' % (settings['ip'], settings['port'])
	headers = {'Content-Type': 'application/json'}
	command = {"jsonrpc":"2.0","method":"Input.SendText", "params": {"text": text}, "id": 1}
	data=json.dumps(command)
	r = requests.post( url, data=data, headers=headers, auth=(settings['user'], settings['pssw']))


def test(settings):

	url = 'http://%s:%s/jsonrpc' % (settings['ip'], settings['port'])
	headers = {'Content-Type': 'application/json'}
	data=json.dumps({"jsonrpc":"2.0","method":"JSONRPC.Ping", "id": 1})
	r = requests.post( url, data=data, headers=headers, auth=(settings['user'], settings['pssw']))
	data=json.dumps({"jsonrpc":"2.0", "method":"GUI.ShowNotification", "params":{"title":"Kodi CLI Remote", "message":"Connected!"}, "id":1})
	r = requests.post( url, data=data, headers=headers, auth=(settings['user'], settings['pssw']))

def redraw(stdscr):
	stdscr.erase()
	stdscr.refresh()
	stdscr.addstr(2,0,">>> 'Arrow Keys' to navigate")
	stdscr.addstr(3,0,">>> 'Enter' to select")
	stdscr.addstr(4,0,">>> 'Backspace' or 'Esc' to navigate back")
	stdscr.addstr(5,0,">>> 'c' for the context menu")
	stdscr.addstr(6,0,">>> 'i' for info")
	stdscr.addstr(7,0,">>> 'o' to toggle the OSD")
	stdscr.addstr(8,0,">>> 's' to show codec info")
	stdscr.addstr(9,0,">>> '[' and ']' volume up and down")
	stdscr.addstr(10,0,">>> 'm' to toggle mute")
	stdscr.addstr(11,0,">>> 'k' to enter keyboard mode (send text to Kodi's keyboard)")
	stdscr.addstr(12,0,">>> 'd' debugger on, 'f' debugger off")
	stdscr.addstr(13,0,">>> 'q' to quit")
	stdscr.refresh()

key_map = {
	
	curses.KEY_UP			: {'name' : 'Up', 			'action' : 'Input.Up'},
	curses.KEY_DOWN			: {'name' : 'Down', 		'action' : 'Input.Down'},
	curses.KEY_LEFT			: {'name' : 'Left', 		'action' : 'Input.Left'},
	curses.KEY_RIGHT		: {'name' : 'Right', 		'action' : 'Input.Right'},
	curses.KEY_BACKSPACE	: {'name' : 'Back', 		'action' : 'Input.Back'},
	27						: {'name' : 'Back', 		'action' : 'Input.Back'},	# ESC
	99						: {'name' : 'ContextMenu', 	'action' : 'Input.ContextMenu'},	# c
	13						: {'name' : 'Select', 		'action' : 'Input.Select'},	# ENTER
	105						: {'name' : 'Info', 		'action' : 'Input.Info'},	# i
	104						: {'name' : 'Home', 		'action' : 'Input.Home'},	# h
	111						: {'name' : 'ShowOSD', 		'action' : 'Input.ShowOSD'},	# o
	115						: {'name' : 'ShowCodec', 	'action' : 'Input.ShowCodec'},	#s
	91						: {'name' : 'VolDown', 		'action' : 'Application.SetVolume',	# [		
								"params": { "volume": "decrement" }},	

	93						: {'name' : 'VolUp', 		'action' : 'Application.SetVolume',		# ]
								"params": { "volume": "increment" }},

	100						: {'name' : 'Debugger On', 	'action'	: 'Settings.SetSettingValue', 	 # d
								"params": {"setting":"debug.showloginfo", "value":"true"}},

	102						: {'name' : 'Debugger Off', 'action'	: 'Settings.SetSettingValue', 	# f
								"params": {"setting":"debug.showloginfo", "value":"false"}},

	109						: {'name' : 'Toggle Mute', 	'action'	: 'Application.SetMute', 	 # m
								"params": {"mute":"toggle"}},
}



try:
	test(settings)
except requests.ConnectionError:
	print 'Failed to connect.'
	print 'Ensure that Kodi is able to be controlled via HTTP'
	print 'Open the Kodi settings, Service, Web Server, and Enable HTTP remote.'
	sys.exit()


stdscr = curses.initscr()
curses.cbreak()
curses.nonl()
stdscr.keypad(1)

redraw(stdscr)
curses.noecho()
key = ''
name = ''

while key != ord('q'):

	redraw(stdscr)

	if name:
		stdscr.addstr(0,0, name)
	
	key = stdscr.getch()
	stdscr.refresh()

	action = key_map.get(key, {}).get('action', None)
	params = key_map.get(key, {}).get('params', None)
	name   = key_map.get(key, {}).get('name'  , None)

	if action is not None:
		curses.setsyx(0, 0)
		call(settings, action, params)
		continue

	if key == ord('k'):

		curses.echo()
		redraw(stdscr)
		stdscr.addstr(0,0,"<<< KEYBOARD MODE >>>")
		text = stdscr.getstr(0,23)
		call_keyboard(settings, text)
		curses.noecho()
		redraw(stdscr)

curses.endwin()
