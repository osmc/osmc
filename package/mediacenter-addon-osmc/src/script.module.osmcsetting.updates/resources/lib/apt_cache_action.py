''' This script is run as root by the osmc update module. '''

import apt
import socket
import sys
from datetime import datetime
import json
import os

t = datetime

class Logger(object):
	def __init__(self, filename="Default.log"):
		self.terminal = sys.stdout
		self.log = open(filename, "a")

	def write(self, message):
		self.terminal.write(message)
		self.log.write(message)

try:
	sys.stdout = Logger("/var/tmp/OSMC_python_apt_log.txt")
except:
	pass


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


class Main(object):

	def __init__(self, action):

		print '==================================================================='
		print '%s %s running' % (t.now(), 'apt_cache_action.py')

		self.error_package = ''
		self.error_message = ''

		self.action = action

		self.cache = apt.Cache()

		self.block_update_file = '/var/tmp/.suppress_osmc_update_checks'

		self.action_to_method = {
								'update' 		: self.update,
								'commit' 		: self.commit,
								'fetch'  		: self.fetch,
								}

		try:
			self.act()
		except Exception as e:
			print '%s %s exception occurred' % (t.now(), 'apt_cache_action.py')
			print '%s %s exception value' % (t.now(), 'apt_cache_action.py', e)

			# send the error to the parent (parent will kill the progress bar)
			call_parent('apt_error', {'error': self.error_message, 'package': self.error_package, 'exception': e})

		self.respond()

		print '%s %s exiting' % (t.now(), 'apt_cache_action.py')
		print '==================================================================='


	def respond(self):

		call_parent('apt_cache %s complete' % self.action)


	def act(self):

		action = self.action_to_method.get(self.action, False)
		if action:
			action()


	def update(self):
		print '%s %s updating cache' % (t.now(), 'apt_cache_action.py')
		dprg = Download_Progress(partial_heading='Updating')
		self.cache.update(fetch_progress=dprg, pulse_interval=1000)

		# call the parent and kill the pDialog
		call_parent('progress_bar', {'kill': True})

		print '%s %s cache updated' % (t.now(), 'apt_cache_action.py')


	def commit(self):

		print '%s %s upgrading all packages' % (t.now(), 'apt_cache_action.py')
		self.cache.upgrade(True)

		print '%s %s committing cache' % (t.now(), 'apt_cache_action.py')

		dprg = Download_Progress()
		iprg = Install_Progress()
		self.cache.commit(fetch_progress=dprg, install_progress=iprg)

		# call the parent and kill the pDialog
		call_parent('progress_bar', {'kill': True})

		print '%s %s cache committed' % (t.now(), 'apt_cache_action.py')

		# remove the file that blocks further update checks
		try:
			os.remove(self.block_update_file)
		except:
			pass


	def fetch(self):

		print '%s %s upgrading all packages' % (t.now(), 'apt_cache_action.py')

		self.cache.upgrade(True)

		print '%s %s fetching all packages' % (t.now(), 'apt_cache_action.py')

		dprg = Download_Progress()

		self.cache.fetch_archives(progress=dprg)

		# call the parent and kill the pDialog
		call_parent('progress_bar', {'kill': True})

		print '%s %s all packages fetched' % (t.now(), 'apt_cache_action.py')


class Operation_Progress(apt.progress.base.OpProgress):

	def __init__(self):
		super(Operation_Progress, self).__init__()	

	def update(self):

		call_parent('progress_bar', {'percent': self.percent,  'heading': self.op, 'message':self.sub_op,})

	def done(self):

		call_parent('progress_bar', {'kill': True})


class Install_Progress(apt.progress.base.InstallProgress):

	def __init__(self):
		super(Install_Progress, self).__init__()	


	def error(self, pkg, errormsg):

		self.error_package = pkg.shortname
		self.error_message = errormsg

		''' (Abstract) Called when a error is detected during the install. '''


	# The following method should be overridden to implement progress reporting for dpkg-based runs 
	# i.e. calls to run() with a filename:

	def processing(self, pkg, stage):
		''' This method is called just before a processing stage starts. The parameter pkg is the name of the 
			package and the parameter stage is one of the stages listed in the dpkg manual under the 
			status-fd option, i.e. "upgrade", "install" (both sent before unpacking), "configure", "trigproc", 
			"remove", "purge". '''

	def dpkg_status_change(self, pkg, status):
		''' This method is called whenever the dpkg status of the package changes. The parameter pkg is the 
			name of the package and the parameter status is one of the status strings used in the status file 
			(/var/lib/dpkg/status) and documented in dpkg(1). '''

	# The following methods should be overridden to implement progress reporting for run() calls 
	# with an apt_pkg.PackageManager object as their parameter:

	def status_change(self, pkg, percent, status):
		''' This method implements progress reporting for package installation by APT and may be extended to 
			dpkg at a later time. This method takes two parameters: The parameter percent is a float value 
			describing the overall progress and the parameter status is a string describing the current status 
			in an human-readable manner. '''

		print 'status_change !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'

		diff = t.now() - self.pulse_time
		if (diff.total_seconds() * 10) < 12:
			return True

		self.pulse_time = t.now()

		call_parent('progress_bar', {'percent': int(percent),  'heading': 'Installing Update', 'message': status})


	def start_update(self):
		''' This method is called before the installation of any package starts. '''

		self.pulse_time = t.now()

		# call_parent('progress_bar', {'percent': 0,  'heading': 'Installing Update', 'message':'Starting Installation'})

		print 'Start !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'


	def finish_update(self):
		''' This method is called when all changes have been applied. '''

		print 'Stop !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'


class Download_Progress(apt.progress.base.AcquireProgress):


	def __init__(self, partial_heading='Downloading'):
		super(Download_Progress, self).__init__()
		self.partial_heading = partial_heading


	def start(self):
		''' Invoked when the Acquire process starts running. '''

		self.pulse_time = t.now()

		call_parent('progress_bar', {'percent': 0,  'heading': 'Downloading Update', 'message':'Starting Download',})

		print 'Start !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'


	def stop(self):
		''' Invoked when the Acquire process stops running. '''


		print 'Stop !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'


	def fetch(self, item):
		''' Invoked when an item is being fetched. '''

		dsc = item.description.split('/')

		self.fetching = self.partial_heading + ': ' + dsc[-1]

		# call_parent('progress_bar',{'message': 'Downloading: ' + dsc[-1]})

		print 'Fetch' + item.description + '++++++++++++++++++++++++++++++'


	def pulse(self, owner):
		''' Periodically invoked as something is being downloaded. '''

		# if the pulse is less than one second since the last one then ignore the pulse
		# this needs to be done as the parents _daemon only checks the queue once a second
		diff = t.now() - self.pulse_time
		if (diff.total_seconds() * 10) < 11:
			return True

		else:
			self.pulse_time = t.now()
			print 'Pulse ==========================================='
			print 'current_items', self.current_items
			print 'total_items', self.total_items 
			print 'total_bytes', self.total_bytes
			print 'fetched_bytes', self.fetched_bytes 
			print 'current_bytes', self.current_bytes
			print 'current_cps', self.current_cps 
			print 'Pulse ==========================================='

			pct = int(self.current_bytes / float(self.total_bytes) * 100)

			cps = self.current_cps / 1024.0

			if cps > 1024:
				cps = '{0:.2f} MBps'.format(cps / 1024)
			else:
				cps = '{0:.0f} kBps'.format(cps)

			cmb = self.current_bytes / 1048576.0

			tmb = self.total_bytes / 1048576.0

			msg = self.fetching

			hdg = '{0:d} / {1:d} items  --  {2:}  --  {3:.1f} / {4:.1f}MB'.format(self.current_items, self.total_items, cps, cmb, tmb)

			call_parent('progress_bar', {'percent': pct, 'heading': hdg, 'message': msg})

		return True


	def done(self, item):
		''' Invoked when an item has finished downloading. '''

		print 'Done ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^'



if __name__ == "__main__":

	if len(sys.argv) > 1:

		action = sys.argv[1]

		m = Main(action)

		del m