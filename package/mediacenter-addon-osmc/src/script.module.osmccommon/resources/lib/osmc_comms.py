# Standard modules
import os
import socket
import subprocess
import threading

# XBMC modules
import xbmc
import xbmcaddon
import xbmcgui


class OSMC_Communicator(threading.Thread):
	''' Class to setup and manage the socket to allow communications between OSMC settings modules and external scripts.
		For example, this communicator is set up by the Main OSMC settings service, and is used by the default.py of that service to request
		the opening of the MyOSMC user interface.

		Class requires:
				- a queue object to allow message to be communicated back to the parent
				- a string describing the location of a unique socket file that other scripts can contact.
				- a logging function
		'''

	def __init__(self, parent_queue, socket_file, logger):

		# queue back to parent
		self.parent_queue = parent_queue

		# logging function
		self.log = logger

		# not sure I need this, but oh well
		#self.wait_evt = threading.Event()

		threading.Thread.__init__(self)

		self.daemon = True

		# create the listening socket, it creates new connections when connected to
		self.address = socket_file

		if os.path.exists(self.address):
			subprocess.call(['sudo', 'rm', self.address])
			try:
				# I need this for testing on my laptop
				os.remove(self.address)
			except:
				self.log('Connection failed to delete socket file.')
				pass

		self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

		# allows the address to be reused (helpful with testing)
		self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self.timeout = 3
		self.sock.settimeout(self.timeout)
		self.sock.bind(self.address)
		self.sock.listen(1)
		
		self.stopped = False


	def stop(self):
		''' Orderly shutdown of the socket, sends message to run loop
			to exit. '''

		self.log('Connection stop called')		

		try:

			self.log('Connection stopping.')
			self.stopped = True
			sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
			sock.connect(self.address)
			sock.send('exit')
			sock.close()
			self.sock.close()

			self.log('Exit message sent to socket.')
				
		except Exception, e:

			self.log('Comms error trying to stop: {}'.format(e))



	def run(self):

		self.log('Comms started')

		while not xbmc.abortRequested and not self.stopped:

			try:
				# wait here for a connection
				conn, addr = self.sock.accept()
			except socket.timeout:
				continue
			except:
				self.log('An error occured while waiting for a connection.')
				break

			self.log('Connection active.')

			# turn off blocking for this temporary connection
			# this will allow the loop to collect all parts of the message
			conn.setblocking(0)

			passed = False
			total_wait = 0
			wait = 5

			while not passed and total_wait < 100:
				try:
					data = conn.recv(81920)
					passed = True
					self.log('data = %s' % data)
				except:
					total_wait += wait
					xbmc.sleep(wait)

			if not passed:
				self.log('Connection failed to collect data.')
				self.stopped = True
				conn.close()
				break

			self.log('data = %s' % data)

			# if the message is to stop, then kill the loop
			if data == 'exit':
				self.log('Connection called to "exit"')
				self.stopped = True
				conn.close()
				break

			# send the data to Main for it to process
			self.parent_queue.put(data)

			conn.close()

		try:
			os.remove(self.address)

		except Exception, e:
			self.log('Comms error trying to delete socket: {}'.format(e))	

		self.log('Comms Ended')
