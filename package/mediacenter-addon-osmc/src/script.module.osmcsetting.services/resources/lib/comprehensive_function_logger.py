import time
from functools import wraps

TEST_LOG_BOOL = True

def test_logger(msg):
	print msg

def comprehensive_logger(logger, logging=False, max_length=25):
	'''
		Decorator to log the inputs and outputs of functions, as well as the time taken
		to run the function.
	'''

	def get_args(*args, **kwargs):

		all_args = []

		for i, arg in enumerate(args):
			itm = 'pos' + str(i) + ": " + str(arg)[:max_length]
			all_args.append(itm)

		for k, v in kwargs.iteritems():
			itm = str(k) + ": " + str(v)[:max_length]
			all_args.append(itm)

		return all_args

	def decorater(func):

		@wraps(func)
		def wrapper(*args, **kwargs):

			if logging and logger != None:
				logger(func.__module__ + '.' + func.__name__ + " received: " + ", ".join(get_args(*args, **kwargs)))

			start 	= time.time()

			# @@@@@@@@@@@@ FUNCTION @@@@@@@@@@@@
			result 	= func(*args, **kwargs)
			# @@@@@@@@@@@@ FUNCTION @@@@@@@@@@@@

			end 	= time.time()

			if logging and logger != None:
				logger(func.__module__ + '.' + func.__name__ + " [" + str(end-start) + "] " + ' returns: ' + str(result)[:max_length])

			return result

		return wrapper

	return decorater

clog = comprehensive_logger

@clog(test_logger, TEST_LOG_BOOL)
def arg_tester(a, b, cdef):
	print 'a: ' + str(a)
	print 'b: ' + str(b)
	print 'cdef: ' + str(cdef)


arg_tester('han', ['chewie', 'luke'], cdef='123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890')