
import datetime
import random


class SimpleScheduler(object):

	def __init__(self, setting_dict, right_now=None):

		self.frequency 		= setting_dict.get('check_freq', 1)				# how often the action occurs (Never, Daily, Weekly, Monthly)
		self.specific_time	= setting_dict.get('check_time', 0)				# whether the action should take place at a specific or random time (boolean)
		self.day 			= setting_dict.get('check_weekday', 0) 			# the weekday when the action should occur, Monday=0, Sunday=6
		self.daynum 		= setting_dict.get('check_day', 1)				# the days from month end that the action should occur [-16, 16]
		self.hour 			= setting_dict.get('check_hour', 3)				# the hour the action should occur (integer)
		self.minute 		= setting_dict.get('check_minute', 0)			# the minute the action should occur (integer)
		self.trigger_time 	= datetime.datetime.now().replace(year=2222)	# the time of the next update check
		self.leeway			= 15											# the number of minutes past the action time that the action can still take place

		if right_now is None:
			right_now = datetime.datetime.now()

		self.set_trigger(right_now)


	def set_trigger(self, right_now):

		# use right_nows year and month

		if self.frequency == 1:

			# the initial trigger time will be this day, and the users specified hour and minute (using defaults if not provided)

			# if the user wants a specific time, then use that, otherwise use a random time
			self.trigger_time = self.set_trigger_time(right_now)
			

		elif self.frequency == 2:

			# the initial trigger time will be this year and month, but the day is the one the user has chosen, as well as the users 
			# specified hour and minute (using defaults if not provided)

			right_now_weekday = right_now.weekday()

			delta_days = self.day - right_now_weekday

			# mon = 0, tue = 1, wed = 2, thu = 3, fri = 4, sat = 5, sun = 6

			# if the user wants a specific time, then use that, otherwise use a random time
			self.trigger_time = self.set_trigger_time( right_now + datetime.timedelta(days=delta_days) )


		elif self.frequency == 3:

			# the initial trigger time will be this year and month, but the day number is the one the user has chosen, as well as the users 
			# specified hour and minute (using defaults if not provided)

			# End of this current month plus or minus the number of days the user has chosen in settings

			month = max([1, (right_now.month + 1) % 13])
			year  = right_now.year if month != 1 else right_now.year + 1

			trigger_day = right_now.replace(year=year, month=month, day = 1) + datetime.timedelta(days=self.daynum-1)
			#             today, with day replaced by 1 and adding 1 to the month  	plus the number of days the user has chosen
			#																		minus an extra day to rebase it to month-end

			# if the user wants a specific time, then use that, otherwise use a random time
			self.trigger_time = self.set_trigger_time(trigger_day)


		# if the trigger time is before the current time, then step it to the next period
		while self.trigger_time < right_now:

			self.step_trigger()

		


	def set_trigger_time(self, trigger_time):
		''' Applies either the users desired time, or a random one, to the trigger '''

		if self.specific_time:

			new_trigger = trigger_time.replace(hour=self.hour, minute=self.minute)

		else:
			new_trigger = trigger_time.replace(hour=random.randint(0,23), minute=random.randint(0,59))

		return new_trigger


	def step_trigger(self):
		''' Progress the trigger time from its current position to its next position '''

		if self.frequency == 1:

			# jump one say ahead from the current trigger date
			self.trigger_time = self.trigger_time + datetime.timedelta(days=1)

		elif self.frequency == 2:

			# jump 7 days ahead from teh current trigger date
			self.trigger_time =  self.trigger_time + datetime.timedelta(days=7)

		elif self.frequency == 3:

			if self.daynum > 0:
				# if the daynum is 1 to 16 then just add one month to the existing month and set the day to be the users chosen date
				month = (self.trigger_time.month % 12) + 1
				year  = self.trigger_time.year + 1 if month == 1 else self.trigger_time.year

				self.trigger_time = self.trigger_time.replace(year=year, month = month, day = self.daynum)

			else:
				# if the daynum is negative, that is, the user wants the update to run a certain number of days BEFORE month-end,
				# then jump to the first day of the month two months ahead of the current one, and then move back one day to get
				# next months month-end date, then subtract the number of days the user has chosen
				month = (((self.trigger_time.month % 12) + 1) % 12) + 1
				if month < 3:
					year  = self.trigger_time.year + 1
				else:
					year = self.trigger_time.year

				self.trigger_time = self.trigger_time.replace(year=year, month = month, day = 1) + datetime.timedelta(days=self.daynum-1)


	def check_trigger(self):

		right_now = datetime.datetime.now()	

		# check if the current time is between the trigger time and the trigger time plus leeway
		if self.trigger_time < right_now < self.trigger_time + datetime.timedelta(minutes=self.leeway):

			# time is currently after the trigger time, but within the leeway
			self.step_trigger()

			return True

		else:

			return False

def test(settings=None):

	if settings is not None:
		x = settings
	else:
		x = {'check_freq':3, 'check_time':0, 'check_weekday':1, 'check_day': 5, 'check_hour':22, 'check_minute':00}

	right_now = datetime.datetime.now()

	for z in range(3700):
		right_now += datetime.timedelta(days=1)

		print z
		print right_now
		s = SimpleScheduler(x, right_now)
		print  '%s\n' % s.trigger_time

