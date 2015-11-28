
import datetime
import random
import sys
from simple_scheduler import SimpleScheduler


'''
    settings = {

	'check_freq' 	:    1,     # 1, 2, 3
	'check_time' 	:    True, 	# True , False
	'check_weekday' :    0,  	# 0, 1, 2, 3, 4, 5, 6
	'check_day'     :    -16, 	# -16 to 16
	'check_hour' 	:    0,     # 0 to 23
	'check_minute'  :    0,     # 0 to 59

    }

'''

_freq     = [1, 2, 3]
_time     = [True, False]
_weekday  = [0, 1, 2, 3, 4, 5, 6]
_day      = list(range(-16, 16))
_hour     = list(range(23))
_minute   = list(range(59))
 

def pop_set(freq, time, weekday, day, hour, minute):

    settings = {

	'check_freq' :    freq,
	'check_time' :    time,
	'check_weekday'    :    weekday,
	'check_day'     :    day,
	'check_hour' :    hour,
	'check_minute' :    minute,
    
    }    

    return settings

 
def test(settings):
    ''' This tests whether the trigger can be set on a specific date. '''

    start_date = datetime.datetime.now()     

    for single_date in (start_date + datetime.timedelta(n) for n in range(370)):

	print single_date
	s = SimpleScheduler(settings)
	s.set_trigger(single_date)
	# print s.trigger_time



if __name__ == "__main__":

	for f in _freq:

	    print 'freq started'

	    for t in _time:

		for w in _weekday:

		    for d in _day:

			sett = pop_set(f, t, w, d, 22, 30)

			print sett

			test(sett)

		print 'freq_ended'