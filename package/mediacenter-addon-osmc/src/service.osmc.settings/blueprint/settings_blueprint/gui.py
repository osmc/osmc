


class gui:

	def __init__(self):

		''' Scan for all  '''
		self.module_list = self.scan_for_modules()

		# create a list of settings_units, instantiating these automatically loads the current settings
		settings_units = [self.create_module(name) for name in self.module_list]


		# run the commands that are required on start-up


	def scan_for_modules(self):

		''' 
			Scan the addon folder for all OSMC settings modules. 

			The OSMC settings modules will be named script.osmc.setting.MODULENAME .

			Returns a list of the names of the modules.

		'''

