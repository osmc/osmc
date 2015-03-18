''' This script is run as root by the osmc update module. '''

import apt
import socket
import sys
from datetime import datetime
import json
import os
import time
import subprocess
import traceback
from CompLogger import comprehensive_logger as clog

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


# @clog(maxlength=1500)
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

		return '%s %s failed to connect to parent - %s' % (t.now(), 'apt_cache_action.py', e)

	return 'response sent'


class Main(object):

	def __init__(self, action):

		# with apt.apt_pkg.SystemLock():
		# implements a lock on the package system, so that nothing else can alter packages

		print '==================================================================='
		print '%s %s running' % (t.now(), 'apt_cache_action.py')

		self.error_package = ''
		
		self.error_message = ''

		self.heading = 'Updater'

		self.action = action

		self.cache = apt.Cache()

		self.block_update_file = '/var/tmp/.suppress_osmc_update_checks'

		self.action_to_method = {
								'update' 		: self.update,
								'update_manual' : self.update,
								'commit' 		: self.commit,
								'fetch'  		: self.fetch,
								'action_list'	: self.action_list,
								}

		try:
			
			self.act()
		
		except Exception as e:
		
			print '%s %s exception occurred' % (t.now(), 'apt_cache_action.py')
		
			print '%s %s exception value : %s' % (t.now(), 'apt_cache_action.py', e)

			deets = 'Error Type and Args: %s : %s \n\n %s' % (type(e).__name__, e.args, traceback.format_exc())

			# send the error to the parent (parent will kill the progress bar)
			call_parent('apt_error', {'error': self.error_message, 'package': self.error_package, 'exception': deets})

		self.respond()

		print '%s %s exiting' % (t.now(), 'apt_cache_action.py')
		print '==================================================================='


	def respond(self):

		call_parent('apt_cache %s complete' % self.action)


	def act(self):

		action = self.action_to_method.get(self.action, False)

		if action:
		
			action()

		else:

			print 'Action not in action_to_method dict'


	#@clog()
	def action_list(self):

		''' This method processes a list sent in argv[2], and either installs or remove packages. 

			The list is sent as a string:

					install_packageid1|=|install_packageid2|=|removal_packageid3'''

		self.heading = 'App Store'

		action_string = sys.argv[2]

		action_dict = self.parse_argv2(action_string)

		self.update()

		self.cache.open()

		for pkg in self.cache:

			# mark packages as install or remove

			if pkg.shortname in action_dict['install']:

				pkg.mark_install()

			if pkg.shortname in action_dict['removal']:

				pkg.mark_delete(purge=True)


		# commit
		self.commit_action()

		if action_dict['removal']:
			# if there were removals then remove the packages that arent needed any more

			self.update()

			self.cache.open()

			removals = False

			for pkg in self.cache:

				if pkg.is_auto_removable:

					pkg.mark_delete(purge=True)

					removals = True

			if removals:
				
				# commit
				self.commit_action()


	# #@clog()
	def parse_argv2(self, action_string):

		install = []
		removal = []

		actions = action_string.split('|=|')

		for action in actions:

			if action.startswith('install_'):

				install.append(action[len('install_'):])

			elif action.startswith('removal_'):

				removal.append(action[len('removal_'):])

		return {'install': install, 'removal': removal}


	#@clog()
	def update(self):

		dprg = Download_Progress(partial_heading='Updating')

		self.cache.update(fetch_progress=dprg, pulse_interval=1000)

		# call the parent and kill the pDialog
		call_parent('progress_bar', {'kill': True})

		return '%s %s cache updated' % (t.now(), 'apt_cache_action.py')


	#@clog()
	def commit(self):

		# check whether any packages are broken, if they are then the install needs to take place outside of Kodi

		for pkg in self.cache:
		
			if pkg.is_inst_broken or pkg.is_now_broken:
		
				return "%s is BROKEN, cannot proceed with commit" % pkg.shortname

		print '%s %s upgrading all packages' % (t.now(), 'apt_cache_action.py')
		
		self.cache.upgrade(True)

		print '%s %s committing cache' % (t.now(), 'apt_cache_action.py')

		self.commit_action()


	#@clog()
	def commit_action(self):

		dprg = Download_Progress()
		
		iprg = Install_Progress(self)
		
		self.cache.commit(fetch_progress=dprg, install_progress=iprg)

		# call the parent and kill the pDialog
		call_parent('progress_bar', {'kill': True})

		# remove the file that blocks further update checks
		
		try:
		
			os.remove(self.block_update_file)
		
		except:
		
			return 'Failed to remove block_update_file'

		return '%s %s cache committed' % (t.now(), 'apt_cache_action.py')
		

	#@clog()
	def fetch(self):

		self.cache.upgrade(True)

		print '%s %s fetching all packages' % (t.now(), 'apt_cache_action.py')

		dprg = Download_Progress()

		self.cache.fetch_archives(progress=dprg)

		# call the parent and kill the pDialog
		call_parent('progress_bar', {'kill': True})

		return '%s %s all packages fetched' % (t.now(), 'apt_cache_action.py')


class Operation_Progress(apt.progress.base.OpProgress):

	def __init__(self):
		super(Operation_Progress, self).__init__()	

	def update(self):

		call_parent('progress_bar', {'percent': self.percent,  'heading': self.op, 'message':self.sub_op,})

	def done(self):

		call_parent('progress_bar', {'kill': True})


class Install_Progress(apt.progress.base.InstallProgress):

	def __init__(self, parent):

		self.parent = parent
		
		super(Install_Progress, self).__init__()	
		
		call_parent('progress_bar', {'percent': 0,  'heading': self.parent.heading, 'message':'Starting Installation'})

	#@clog()
	def error(self, pkg, errormsg):

		print 'ERROR!!! \n%s\n' % errormsg

		try:
			pkgname = os.path.basename(pkg).split('_')

			print 'Package affected!!! \n%s\n' % pkgname

			self.parent.error_package = pkgname[0]

			if len(pkgname) > 1:

				self.parent.error_package += ' (' + pkgname[1] + ')'

		except:

			self.parent.error_package = '(unknown package)'


		self.parent.error_message = errormsg

		''' (Abstract) Called when a error is detected during the install. '''


	# The following method should be overridden to implement progress reporting for dpkg-based runs 
	# i.e. calls to run() with a filename:

	# def processing(self, pkg, stage):
	# 	''' This method is called just before a processing stage starts. The parameter pkg is the name of the 
	# 		package and the parameter stage is one of the stages listed in the dpkg manual under the 
	# 		status-fd option, i.e. "upgrade", "install" (both sent before unpacking), "configure", "trigproc", 
	# 		"remove", "purge". '''

	# def dpkg_status_change(self, pkg, status):
	# 	''' This method is called whenever the dpkg status of the package changes. The parameter pkg is the 
	# 		name of the package and the parameter status is one of the status strings used in the status file 
	# 		(/var/lib/dpkg/status) and documented in dpkg(1). '''

	# The following methods should be overridden to implement progress reporting for run() calls 
	# with an apt_pkg.PackageManager object as their parameter:

	#@clog()
	def status_change(self, pkg, percent, status):
		''' This method implements progress reporting for package installation by APT and may be extended to 
			dpkg at a later time. This method takes two parameters: The parameter percent is a float value 
			describing the overall progress and the parameter status is a string describing the current status 
			in an human-readable manner. '''

		diff = t.now() - self.pulse_time

		if (diff.total_seconds() * 10) < 12:

			return True

		self.pulse_time = t.now()

		call_parent('progress_bar', {'percent': int(percent),  'heading': self.parent.heading, 'message': status})

	#@clog()
	def start_update(self):
		''' This method is called before the installation of any package starts. '''

		self.pulse_time = t.now()


		return 'Start !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'

	#@clog()
	def finish_update(self):
		''' This method is called when all changes have been applied. '''

		return 'Stop !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'


class Download_Progress(apt.progress.base.AcquireProgress):


	def __init__(self, partial_heading='Downloading'):
		super(Download_Progress, self).__init__()
		self.partial_heading = partial_heading
		call_parent('progress_bar', {'percent': 0,  'heading': 'Downloading Update', 'message':'Starting Download',})

	#@clog()
	def start(self):
		''' Invoked when the Acquire process starts running. '''

		self.pulse_time = t.now()


		return 'Start !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'

	#@clog()
	def stop(self):
		''' Invoked when the Acquire process stops running. '''

		return 'Stop !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'

	#@clog()
	def fetch(self, item):
		''' Invoked when an item is being fetched. '''

		dsc = item.description.split('/')

		self.fetching = self.partial_heading + ': ' + dsc[-1]

		# call_parent('progress_bar',{'message': 'Downloading: ' + dsc[-1]})

		return 'Fetch' + item.description + '++++++++++++++++++++++++++++++'

	#@clog()
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

	#@clog()
	def done(self, item):
		''' Invoked when an item has finished downloading. '''

		return 'Done ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^'



if __name__ == "__main__":

	if len(sys.argv) > 1:

		action = sys.argv[1]

		m = Main(action)

		del m
