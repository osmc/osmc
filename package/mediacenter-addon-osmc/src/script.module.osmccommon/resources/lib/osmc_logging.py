import time
from functools import wraps

import xbmc

TEST_LOG_BOOL = True

def test_logger(msg):
	print 'test-' + msg


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 

class StandardLogger(object):
	''' Standard kodi logger. Used to add entries to the Kodi.log.
		Best usage:
			from osmc_logging import StandardLogger
			standard_logger = StandardLogger(__addonid__)
			log = standard_logger.log

		'''

	def __init__(self, addonid):
		self.addonid = addonid

	def log(message, label=''):

		try:
			message = str(message)
		except UnicodeEncodeError:
			message = message.encode('utf-8', 'ignore' )

		try:
			label = str(label)
		except UnicodeEncodeError:
			label = label.encode('utf-8', 'ignore' )
					
		logmsg       = '%s : %s - %s ' % (self.__addonid__ , str(label), str(message))
		xbmc.log(msg = logmsg, level=xbmc.LOGDEBUG)	




def ComprehensiveLogger(logger=None, logging=True, maxlength=250, nowait=False):
	'''
		Decorator to log the inputs and outputs of functions, as well as the time taken to run the function.

		Requires: time, functools
		logger: 	[opt] logging function, if not provided print is used
		logging: 	[opt] boolean, turn logging on and off, default is True
		maxlength:	[opt] integer, sets the maximum length an argument or returned variable cant take, default 25
		nowait:		[opt] boolean, instructs the logger not to wait for the function to finish, default is False
	'''

	def default_logger(msg):

		print msg


	if logger == None:

		logger = default_logger


	def get_args(*args, **kwargs):

		all_args = []

		for i, arg in enumerate(args):

			itm = 'pos' + str(i) + ": " + str(arg)[:maxlength]

			all_args.append(itm)

		for k, v in kwargs.iteritems():

			itm = str(k) + ": " + str(v)[:maxlength]

			all_args.append(itm)

		return all_args


	def decorater(func):

		@wraps(func)
		def wrapper(*args, **kwargs):

			if logging and logger != None:
			
				logger(func.__module__ + '.' + func.__name__ + " received: " + ", ".join(get_args(*args, **kwargs)))

			if nowait:
				
				func(*args, **kwargs)

				logger(func.__module__ + '.' + func.__name__ + " -nowait")

				return

			else:
	
				start 	= time.time()
				
				result 	= func(*args, **kwargs)

				end 	= time.time()

				if logging and logger != None:
				
					logger(func.__module__ + '.' + func.__name__ + " [" + str(end-start) + "] " + ' returns: ' + str(result)[:maxlength])

				return result

		return wrapper

	return decorater


clog = comprehensive_logger


@clog(logging=TEST_LOG_BOOL)
def arg_tester(a, b, cdef):

	print 'a: ' + str(a)
	
	print 'b: ' + str(b)
	
	print 'cdef: ' + str(cdef)


if __name__ == "__main__":
	arg_tester('han', ['chewie', 'luke'], cdef='123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890')
