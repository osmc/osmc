
import datetime
import random


class SimpleScheduler(object):

	def __init__(self, setting_dict):

		self.frequency 		= setting_dict.get('check_freq', 1)				# how often the action occurs (Never, Daily, Weekly, Monthly)
		self.specific_time	= setting_dict.get('check_time', 0)				# whether the action should take place at a specific or random time (boolean)
		self.day 			= setting_dict.get('check_weekday', 0) 			# the weekday when the action should occur, Monday=0, Sunday=6
		self.daynum 		= setting_dict.get('check_day', 1)				# the days from month end that the action should occur [-16, 16]
		self.hour 			= setting_dict.get('check_hour', 3)				# the hour the action should occur (integer)
		self.minute 		= setting_dict.get('check_minute', 0)			# the minute the action should occur (integer)
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
            
			days_to_day = self.day - right_now.weekday()
			if days_to_day < 0:
				days_to_day += 7

			new_day = right_now + datetime.timedelta(days=days_to_day)
			# mon = 0, tue = 1, wed = 2, thu = 3, fri = 4, sat = 5, sun = 6

			# if the user wants a specific time, then use that, otherwise use a random time
			self.trigger_time = self.set_trigger_time( new_day )


		elif self.frequency == 3:

			# the initial trigger time will be this year and month, but the day number is the one the user has chosen, as well as the users 
			# specified hour and minute (using defaults if not provided)

			# days in the month, ditm = (right_now.replace(month = right_now.month % 12 +1, day = 1) - datetime.timedelta(days=1)).day

			if self.daynum > 0:
				trigger_day = self.daynum
			else:
				trigger_day = (right_now.replace(month = right_now.month % 12 + 1, day = 1) - datetime.timedelta(days=1)).day + self.daynum

			# if the user wants a specific time, then use that, otherwise use a random time
			self.trigger_time = self.set_trigger_time( right_now.replace(day=trigger_day) )


		# if the trigger time is before the current time, then add one month to it
		if self.trigger_time < right_now:

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

			self.trigger_time = self.trigger_time + datetime.timedelta(days=1)

		elif self.frequency == 2:

			self.trigger_time =  self.trigger_time + datetime.timedelta(days=7)

		elif self.frequency == 3:

			if self.daynum > 0:
				trigger_day = self.daynum

			else:
				trigger_day = (self.trigger_time.replace(month = self.trigger_time.month % 12 + 2, day = 1) - datetime.timedelta(days=1)).day + self.daynum

			self.trigger_time = self.trigger_time.replace(month = self.trigger_time.month + 1, day = trigger_day)


	def check_trigger(self):

		right_now = datetime.datetime.now()	

		# check if the current time is between the trigger time and the trigger time plus leeway
		if self.trigger_time < right_now < self.trigger_time + datetime.timedelta(minutes=self.leeway):

			# time is currently after the trigger time, but within the leeway
			self.step_trigger()

			return True

		else:

			return False

