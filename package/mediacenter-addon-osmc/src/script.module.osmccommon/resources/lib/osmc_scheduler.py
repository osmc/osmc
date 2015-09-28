
import datetime
import random


class SimpleScheduler(object):
	''' A simple scheduler class.

		Takes a dictionary with the following settings:
			check_freq: 	(default = 1)			# how often the action occurs (Never, Daily, Weekly, Monthly)
			check_time: 	(default = 0)			# whether the action should take place at a specific or random time (boolean)
			check_weekday: 	(default = 0) 			# the weekday when the action should occur, Monday=0, Sunday=6
			check_day: 		(default = 1)			# the days from month end that the action should occur [-16, 16]
			check_hour: 	(default = 3)			# the hour the action should occur (integer)
			check_minute: 	(default = 0)			# the minute the action should occur (integer) 
		'''

	def __init__(self, setting_dict):

		self.frequency 		= setting_dict.get('check_freq', 1)				
		self.specific_time	= setting_dict.get('check_time', 0)				
		self.day 			= setting_dict.get('check_weekday', 0) 			
		self.daynum 		= setting_dict.get('check_day', 1)				
		self.hour 			= setting_dict.get('check_hour', 3)				
		self.minute 		= setting_dict.get('check_minute', 0)			
		self.trigger_time 	= datetime.datetime.now().replace(year=2222)	# the time of the next update check
		self.leeway			= 15											# the number of minutes past the action time that the action can still take place

		self.set_trigger()


	def set_trigger(self):

		# use right_nows year and month

		right_now = datetime.datetime.now()

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
			trigger_day = right_now.replace(month = right_now.month % 12 + 1, day = 1) + datetime.timedelta(days=self.daynum-1)
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

		right_now = datetime.datetime.now()

		if self.frequency == 1:

			# jump one say ahead from the current trigger date
			self.trigger_time = self.trigger_time + datetime.timedelta(days=1)

		elif self.frequency == 2:

			# jump 7 days ahead from teh current trigger date
			self.trigger_time =  self.trigger_time + datetime.timedelta(days=7)

		elif self.frequency == 3:

			if self.daynum > 0:
				# if the daynum is 1 to 16 then just add one month to the existing month and set the day to be the users chosen date
				self.trigger_time = self.trigger_time.replace(month = (self.trigger_time.month % 12) + 1, day = self.daynum)

			else:
				# if the daynum is negative, that is, the user wants the update to run a certain number of days BEFORE month-end,
				# then jump to the first day of the month two months ahead of the current one, and then move back one day to get
				# next months month-end date, then subtract the number of days the user has chosen
				new_month = (((self.trigger_time.month % 12) + 1 ) % 12 ) + 1

				self.trigger_time = self.trigger_time.replace(month = new_month, day = 1) + datetime.timedelta(days=self.daynum-1)


	def check_trigger(self):

		right_now = datetime.datetime.now()	

		# check if the current time is between the trigger time and the trigger time plus leeway
		if self.trigger_time < right_now < self.trigger_time + datetime.timedelta(minutes=self.leeway):

			# time is currently after the trigger time, but within the leeway
			self.step_trigger()

			return True

		else:

			return False

