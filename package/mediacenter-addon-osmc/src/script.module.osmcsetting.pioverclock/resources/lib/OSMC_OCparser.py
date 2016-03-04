'''
DICT OF ITEMS IN CONFIG WHICH AFFECT THIS KODI SETTING

kodi_setting_id:    { 
	
	KODI_ITEM:
			{
			config_get_patterns: [	LIST OF DICTS WITH AN IDENTIFY REGEX AND AN EXTRACT REGEX

							IDENTIFY REGEX'S ARE THERE TO SPECIFICALLY FIND THE LINES
							EXTRACT REGEXES ARE THERE TO GET THE SPECIFIC SETTING VALUE
						],
			
			
			config_set : FUNCTION TO CHANGE THE SETTING IN THE CONFIG,
							# this code should cycle through the config line list, search for each config_key, then substitute that line with the new one

			config_validation: FUNCTION TO VALIDATE THE VALUE FROM THE CONFIG, this also converts the value into a form kodi settings will recognise
								i.e. it should convert binary 0:1 to 'false'|'true'

			kodi_set: additional function that can be used to do more specific conversion from the config to kodi.
				for instance, if a single setting in kodi relies upon two different settings in the config, this can combine them

			setting_stub: a stub of the string that will replace the lines in the config.txt, the value is inserted into it
			},

	# ...


NEEDS A REMOVE LIST
NEEDS A FINAL CHECK FOR HDMI_SAFE to make sure the entries related to it are removed if it is on 
		hdmi_safe can be checked in the dict immediately after the settings are extracted from kodi
		that way the other values can be set and override the ones taken from kodi

'''
import re


def config_to_kodi(MASTER_SETTINGS, config):
	''' Takes the existing config and uses the protocols in the MASTER_SETTINGS to extract the settings 
		for use in kodi.

		Returns a dictionary of kodi settings and kodi values.'''

	extracted_settings_for_kodi = {}

	for setting, protocols in MASTER_SETTINGS.iteritems():

		value = general_config_get(config, **protocols)

		extracted_settings_for_kodi[setting] = value

	return extracted_settings_for_kodi


def general_config_get(config, config_get_patterns, config_validation, kodi_set, default, **kwargs):
	''' Searches the config.txt for specific settings and returns the values when they are found.

		Uses the validation and kodi_set protocols to convert the config settings into kodi settings.

		Returns a valid kodi setting value. '''

	results = []

	# the config is reviewed in reverse order so that the final of any duplicate settings is first in the results list
	# this is consistent with how the rPi does it's config parsing
	for line in config[::-1]:

		# ignore blank lines
		if not line.strip():
			continue

		# ignore commented out lines
		if line.strip().startswith('#'):
			continue

		# strip the line of any inline comments
		if '#' in line: 
			line = line[:line.index('#')]

		for pair in config_get_patterns:

			matched = re.search(pair['identify'], line, re.IGNORECASE)

			if matched:

				raw_value = re.search(pair['extract'], line, re.IGNORECASE)

				if raw_value:

					result = config_validation(raw_value.group(1))

					if result is not None:

						results.append(result)

	kodi_setting = kodi_set(results)

	if kodi_setting is None:

		kodi_setting = retrieve_default(**default)

	return kodi_setting


def retrieve_default(function, value):

	if function is not None:

		value = function()

	return value


def kodi_to_config(MASTER_SETTINGS, config, new_settings):
	''' Takes the existing config.txt (as a list of lines) and constructs a new one using the settings
		from kodi. 

		Returns a brand new config (list of lines)'''

	for setting, new_value in new_settings.iteritems():

		setting_protocols = MASTER_SETTINGS.get(setting, None)

		if setting_protocols == None: continue

		config = general_config_set(config, new_settings, new_value, **setting_protocols)

	return config


def general_config_set(config, new_settings, new_value, config_get_patterns, config_set, already_set, setting_stub, **kwargs):
	''' Runs through the config.txt looking for a specific setting and replaces the existing 
		values when they are found. If there are duplicate entries, the last entry is kept, the others
		are commented out.

		If not found, then the setting value is added to the end.

		Returns a new list of config lines. '''

	new_config = []

	# pass the new_value through the config_set protocol to prepare it for inclusion in the config.txt
	new_value = config_set(new_value, new_settings)

	#print config
	#print config[::-1]

	# the original config is run through backwards so that the last setting of any duplicates is kept
	for line in config[::-1]:

		#print '---'
		#print 'subject line : %s ' % line.strip()

		# pass blank lines straight through
		if not line.strip():
			new_config.append(line)
			#print 'passed through blank'
			continue			

		# pass commented out lines straight through
		if line.strip().startswith('#'):
			new_config.append(line)
			#print 'passed through comment'
			continue

		# ignore inline comments on the line
		try:
			cf_line = line[:line.index('#')]
			comment = line[line.index('#'):]
		except ValueError:
			cf_line = line
			comment = ''

		line_matches = False

		for pair in config_get_patterns:

			matched = re.search(pair['identify'], cf_line, re.IGNORECASE)

			if matched:

				line_matches = True

				#print 'found'

				# if a match happens but the value is 'remove_this_line', then dont add the line to the
				# new config.txt
				if new_value == 'remove_this_line':
					#print 'setting to remove'
					continue

				# if the value has been set already, then comment out this line
				if already_set:

					# comment out any duplicated entries (this could be changed to removal if we want)
					new_config.append('#' + line.replace('\n','') + ' # DUPLICATE')
					#print 'alreadyset'
					continue

				# otherwise update the line
				new_config.append(setting_stub % new_value + '  %s' % comment)
				#print 'updated value'
				already_set = True


		# if no match actually occured then pass the line through to the new config
		if not line_matches:
			new_config.append(line)
			#print 'passed through unmatched'

	# flip the config back around the other way, so new entries are added at the end
	new_config = new_config[::-1]

	if not already_set and new_value != 'remove_this_line':
		new_config.append(setting_stub % new_value)

	#print new_config

	return new_config



'''
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@ 
										VALIDATION FUNCTIONS
                                                                                       @@@@@@@@@@@@@@@@@@@@@@@@
Settings coming FROM the config.txt pass through one of these.                         @@@@@@@@@@@@@@@@@@@@@@@@
They can be tested for accuracy, valid ranges, and (if it is a simple 1 for 1)         @@@@@@@@@@@@@@@@@@@@@@@@
converted to what is recognised by the settings in Kodi. 	            			   @@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

'''


def generic_number_validation(config_value):

	try:
		return int(config_value)
	except:
		return None


'''
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@ 
								CUSTOM DEFAULT FUNCTIONS
                                                                                       @@@@@@@@@@@@@@@@@@@@@@@@
Allows for customisable default values, i.e. default values determined by some 		   @@@@@@@@@@@@@@@@@@@@@@@@
external analysis                                                                      @@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

'''

def hdmi_boost_custom_default():
	''' Tests the users system to see which hdmi_boost figure should be used. '''
	''' Yet to be implemented '''

	return '2'


def PiVersion():
	''' Determines the version of Pi currently being used '''

	with open('/proc/cpuinfo', 'r') as f:

		cpu_count = sum([1 for x in f.readlines() if x.startswith('processor')])

	if cpu_count == 1:
		return 'PiB'
	else:
		return 'Pi2'


def arm_freq_custom_default():

	try:
		version = PiVersion()
	except IOError:
		version = "PiB"

	if version == 'PiB':
		return 850

	elif version == 'Pi2':
		return 900

	else:
		return 850


def sdram_freq_custom_default():

	try:
		version = PiVersion()
	except IOError:
		version = "PiB"

	if version == 'PiB':
		return 400

	elif version == 'Pi2':
		return 450

	else:
		return 400


def core_freq_custom_default():

	try:
		version = PiVersion()
	except IOError:
		version = "PiB"

	if version == 'PiB':
		return 375

	elif version == 'Pi2':
		return 450

	else:
		return 375


'''
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@ 
								CUSTOM CONFIG SET FUNCTIONS
                                                                                       @@@@@@@@@@@@@@@@@@@@@@@@
Converts the kodi settings into the settings values required in the config.txt  	   @@@@@@@@@@@@@@@@@@@@@@@@
							                                                           @@@@@@@@@@@@@@@@@@@@@@@@
Takes both the new value of the setting and all the other settings for reference       @@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

'''

def generic_number_config_set(kodi_setting, all_settings):

	try:
		return str(kodi_setting)
	except:
		return 'remove_this_line'


def arm_freq_config_set(kodi_setting, all_settings):
	''' Checks if the frequency setting is the same as the default Pi setting.
	If so, then remove the line from the config.txt as it is not needed. '''

	try:
		version = PiVersion()
	except IOError:
		version = "PiB"

	if version == 'PiB':

		if int(kodi_setting) == 850: return 'remove_this_line'

	elif version == 'Pi2':

		if int(kodi_setting) == 900: return 'remove_this_line'

	return kodi_setting


def sdram_freq_config_set(kodi_setting, all_settings):
	''' Checks if the frequency setting is the same as the default Pi setting.
	If so, then remove the line from the config.txt as it is not needed. '''

	try:
		version = PiVersion()
	except IOError:
		version = "PiB"

	if version == 'PiB':
		if int(kodi_setting) == 400: return 'remove_this_line' 

	elif version == 'Pi2':
		if int(kodi_setting) == 450: return 'remove_this_line'

	return kodi_setting


def core_freq_config_set(kodi_setting, all_settings):
	''' Checks if the frequency setting is the same as the default Pi setting.
	If so, then remove the line from the config.txt as it is not needed. '''

	try:
		version = PiVersion()
	except IOError:
		version = "PiB"

	if version == 'PiB':
		if int(kodi_setting) == 375: return 'remove_this_line'

	elif version == 'Pi2':
		if int(kodi_setting) == 450: return 'remove_this_line'

	return kodi_setting


def zero_value_config_set(kodi_setting, all_settings):
	''' If the value of the kodi setting is zero, then remove the 
	entry from the config.txt. This should be used where zero is
	the default (pi natural) value. '''

	try:
		if int(kodi_setting) == 0:
			return 'remove_this_line'
	except:
		pass
	
	return kodi_setting


'''
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@ 
								CUSTOM KODI SET FUNCTIONS
                                                                                       @@@@@@@@@@@@@@@@@@@@@@@@
Allows for more complex conversion of config settings (including multiple settings)	   @@@@@@@@@@@@@@@@@@@@@@@@
into specific kodi settings.                                                           @@@@@@@@@@@@@@@@@@@@@@@@                                                                                       
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

'''

def generic_passthrough_kodi_set(results):
	''' Takes a results list, and simply returns the first value. '''

	try:
		setting_value = results[0]
	except IndexError:
		setting_value = None

	return setting_value


'''
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@ 
								KODI SETTINGS DICTIONARY
                                                                                       @@@@@@@@@@@@@@@@@@@@@@@@
Houses the protocols for the settings in Kodi which come from the config.txt           @@@@@@@@@@@@@@@@@@@@@@@@
                                                                                       @@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

'arm_freq', 'sdram_freq', 'core_freq', 'initial_turbo', 'over_voltage', 'over_voltage_sdram', 'force_turbo'

'''

MASTER_SETTINGS =    {

		"arm_freq": { 
			"default"   : { 
				"function"      : arm_freq_custom_default, 
				"value"         : 850
				},
			"config_get_patterns": [
				{
				"identify"      : r"\s*arm_freq\s*=\s*",
				"extract"       : r"\s*arm_freq\s*=\s*(\d+)"
				},

				],
			"config_set"        : arm_freq_config_set,
			"config_validation" : generic_number_validation,
			"kodi_set"          : generic_passthrough_kodi_set,
			"already_set"       : False,
			"setting_stub"      : "arm_freq=%s",
		},
	
		"sdram_freq": { 
			"default"   : { 
				"function"      : sdram_freq_custom_default, 
				"value"         : 400
				},
			"config_get_patterns": [
				{
				"identify"      : r"\s*sdram_freq\s*=\s*",
				"extract"       : r"\s*sdram_freq\s*=\s*(\d+)"
				},

				],
			"config_set"        : sdram_freq_config_set,
			"config_validation" : generic_number_validation,
			"kodi_set"          : generic_passthrough_kodi_set,
			"already_set"       : False,
			"setting_stub"      : "sdram_freq=%s",
		},

		"core_freq": { 
			"default"   : { 
				"function"      : core_freq_custom_default, 
				"value"         : 375
				},
			"config_get_patterns": [
				{
				"identify"      : r"\s*core_freq\s*=\s*",
				"extract"       : r"\s*core_freq\s*=\s*(\d+)"
				},

				],
			"config_set"        : core_freq_config_set,
			"config_validation" : generic_number_validation,
			"kodi_set"          : generic_passthrough_kodi_set,
			"already_set"       : False,
			"setting_stub"      : "core_freq=%s",
		},

		"initial_turbo": { 
			"default"   : { 
				"function"      : None, 
				"value"         : 0
				},
			"config_get_patterns": [
				{
				"identify"      : r"\s*initial_turbo\s*=",
				"extract"       : r"\s*initial_turbo\s*=\s*(\d+)"
				},			
				],
			"config_set"        : zero_value_config_set,
			"config_validation" : generic_number_validation,
			"kodi_set"          : generic_passthrough_kodi_set,
			"already_set"       : False,
			"setting_stub"      : "initial_turbo=%s",
		},

		"over_voltage": { 
			"default"   : { 
				"function"      : None, 
				"value"         : 0
				},
			"config_get_patterns": [
				{
				"identify"      : r"\s*over_voltage\s*=\s*",
				"extract"       : r"\s*over_voltage\s*=\s*(\d+)"
				},

				],
			"config_set"        : zero_value_config_set,
			"config_validation" : generic_number_validation,
			"kodi_set"          : generic_passthrough_kodi_set,
			"already_set"       : False,
			"setting_stub"      : "over_voltage=%s",
		},

		"over_voltage_sdram": { 
			"default"   : { 
				"function"      : None, 
				"value"         : 0
				},
			"config_get_patterns": [
				{
				"identify"      : r"\s*over_voltage_sdram\s*=",
				"extract"       : r"\s*over_voltage_sdram\s*=\s*(\d+)"
				},	

				],
			"config_set"        : zero_value_config_set,
			"config_validation" : generic_number_validation,
			"kodi_set"          : generic_passthrough_kodi_set,
			"already_set"       : False,
			"setting_stub"      : "over_voltage_sdram=%s",
		},

		"force_turbo": { 
			"default"   : { 
				"function"      : None, 
				"value"         : 0
				},
			"config_get_patterns": [
				{
				"identify"      : r"\s*force_turbo\s*=",
				"extract"       : r"\s*force_turbo\s*=\s*(\d+)"
				},
							
				],
			"config_set"        : zero_value_config_set,
			"config_validation" : generic_number_validation,
			"kodi_set"          : generic_passthrough_kodi_set,
			"already_set"       : False,
			"setting_stub"      : "force_turbo=%s",
		},
		
	}


def read_config_file(location):

	with open(location, 'r') as f:
		config = f.readlines()

	return config


def write_config_file(location, new_config):

	new_config = [x + '\n' if not x.endswith('\n') else x for x in new_config if 'remove_this_line' not in x]

	with open(location, 'w') as f:
		f.writelines(new_config)



if __name__ == "__main__":

	'''@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

		Anything in here is only run when the script is called directly rather than imported

		I use this section for testing only.

	   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ '''

	config = read_config_file('C:\\temp\config.txt')

	extracted_settings = config_to_kodi(MASTER_SETTINGS, config)

	keys = sorted(extracted_settings.keys())




	original_values = {
			'audio' : 'true',
			'config_hdmi_boost' : '2',
			'decode_MPG2' : '0x9c7d03c1',
			'decode_WVC1' : '0x95f2b10e',
			'display_rotate' : '0',
			'gpio_in_pin' : 'off',
			'gpio_in_pull' : '18',
			'gpio_out_pin' : '17',
			'gpu_mem_1024' : '256',
			'gpu_mem_512' : '144',
			'gpu_mem_256' : '112',
			'hdmi_group' : '0',
			'hdmi_ignore_cec' : 'false',
			'hdmi_ignore_cec_init' : 'true',
			'hdmi_ignore_edid' : 'false',
			'hdmi_mode' : '0',
			'hdmi_pixel_encoding' : '0',
			'hdmi_safe' : 'false',
			'lirc-rpi-overlay' : '0',
			'max_usb_current' : 'true',
			'sdtv_aspect' : '0',
			'sdtv_mode' : '0',
			'soundcard_dac' : "0",
			'spi-bcm2835-overlay' : 'false',
			'store_hdmi_to_file' : 'false',
			'w1gpio' : '0',

	}

	test = {
		'audio' : 'true',
		'config_hdmi_boost' : '2',
		'decode_MPG2' : '0000',
		'decode_WVC1' : '0x95f2b10e',
		'display_rotate' : '0',
		'gpio_in_pin' : '18',
		'gpio_in_pull' : 'off',
		'gpio_out_pin' : '17',
		'gpu_mem_1024' : '256',
		'gpu_mem_512' : '144',
		'gpu_mem_256' : '112',
		'hdmi_group' : '0',
		'hdmi_ignore_cec' : 'false',
		'hdmi_ignore_cec_init' : 'true',
		'hdmi_ignore_edid' : 'true',
		'hdmi_mode' : '0',
		'hdmi_pixel_encoding' : '0',
		'hdmi_safe' : 'true',
		'lirc-rpi-overlay' : 'true',
		'max_usb_current' : 'true',
		'sdtv_aspect' : '0',
		'sdtv_mode' : '1',
		'soundcard_dac' : "2",
		'spi-bcm2835-overlay' : 'false',
		'store_hdmi_to_file' : 'false',
		'w1gpio' : '2',

	}

	config = read_config_file('C:\\temp\config.txt')
	
	new_settings = kodi_to_config(MASTER_SETTINGS, config, extracted_settings)

	print new_settings

	write_config_file('C:\\temp\\results.txt', new_settings)
