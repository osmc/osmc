# Standard Modules
from collections import namedtuple

# XBMC Modules
import xbmcaddon
import xbmcgui


__addon__              	= xbmcaddon.Addon('script.module.osmcsetting.pioverclock')
# __addonid__            	= __addon__.getAddonInfo('script.module.osmcsetting.piovQQQerclock').getAddonInfo('id')


def lang(id):
	san = __addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
	return san 


class overclock_gui(xbmcgui.WindowXMLDialog):

	def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):

		self.setting_values = kwargs['setting_values']
		self.model = kwargs['model']

		self.warning = False

		# setting values looks like this:
		# {'core': 500, 'arm': 800, 'sdram': 700, 'gpu': 275, 'ov': 2, 'sdrov': 6}

		self.oc_profile = namedtuple('Profile', 'arm_freq sdram_freq core_freq initial_turbo over_voltage over_voltage_sdram force_turbo')

		if self.model != 'Pi2':
			self.normal_profile = self.oc_profile(850, 400, 375, 0, 0, 0, 0)
			self.medium_profile = self.oc_profile(900, 400, 375, 0, 0, 0, 0)
			self.higher_profile = self.oc_profile(950, 450, 450, 0, 6, 0, 0)
			self.custom_profile = self.oc_profile(
				self.setting_values.get('arm_freq', 			self.normal_profile.arm_freq),
				self.setting_values.get('sdram_freq', 			self.normal_profile.sdram_freq),
				self.setting_values.get('core_freq', 			self.normal_profile.core_freq),
				self.setting_values.get('initial_turbo', 		self.normal_profile.initial_turbo),
				self.setting_values.get('over_voltage', 		self.normal_profile.over_voltage),
				self.setting_values.get('over_voltage_sdram', 	self.normal_profile.over_voltage_sdram),
				self.setting_values.get('force_turbo', 			self.normal_profile.force_turbo),
				)

			self.overclock_profiles = [self.normal_profile, self.medium_profile, self.higher_profile, self.custom_profile]

			self.descriptions = {
				101  :  lang(32088) % self.normal_profile,
				102  :  lang(32089) % self.medium_profile,
				103  :  lang(32090) % self.higher_profile,
				104  :  lang(32091) % self.custom_profile,
				402  :  lang(32092) % 700,
				502  :  lang(32093) % 400,
				602  :  lang(32094) % 250,
				702  :  lang(32095),
				802  :	lang(32096),
				902  :  lang(32097),
				1002 :  lang(32098),
				}


		else:
			# raspberry Pi2 profiles, there is not higher, only normal and turbo
			self.normal_profile = self.oc_profile(900, 450, 450, 0, 0, 0, 0)
			self.medium_profile = self.oc_profile(1000, 450, 500, 0, 2, 0, 0)
			self.custom_profile = self.oc_profile(
				self.setting_values.get('arm_freq', 			self.normal_profile.arm_freq),
				self.setting_values.get('sdram_freq', 			self.normal_profile.sdram_freq),
				self.setting_values.get('core_freq', 			self.normal_profile.core_freq),
				self.setting_values.get('initial_turbo', 		self.normal_profile.initial_turbo),
				self.setting_values.get('over_voltage', 		self.normal_profile.over_voltage),
				self.setting_values.get('over_voltage_sdram', 	self.normal_profile.over_voltage_sdram),
				self.setting_values.get('force_turbo', 			self.normal_profile.force_turbo),
				)

			self.overclock_profiles = [self.normal_profile, self.medium_profile, 'nothing', self.custom_profile]

			self.descriptions = {
				101  :  lang(32088) % self.normal_profile,
				102  :  lang(32089) % self.medium_profile,
				104  :  lang(32091) % self.custom_profile,
				402  :  lang(32092) % 900,
				502  :  lang(32093) % 450,
				602  :  lang(32094) % 250,
				702  :  lang(32095),
				802  :	lang(32106),
				902  :  lang(32097),
				1002 :  lang(32098),
				}


		self.control_matching = {
			'arm_freq'				: 402,
			'sdram_freq'			: 502,
			'core_freq'				: 602, 
			'initial_turbo'			: 702,
			'over_voltage'			: 802,
			'over_voltage_sdram'	: 902,
			'force_turbo'			: 1002,
			}


		self.oc_factor = namedtuple('oc_factor', ['min', 'range', 'step', 'custom'])

		self.metrics = {
			self.control_matching['core_freq']			: self.oc_factor(150, 500, 10, 	self.custom_profile.core_freq),
			self.control_matching['arm_freq']			: self.oc_factor(600, 600, 10, 	self.custom_profile.arm_freq),
			self.control_matching['sdram_freq']			: self.oc_factor(300, 400, 10, 	self.custom_profile.sdram_freq),
			self.control_matching['initial_turbo']		: self.oc_factor(0, 60, 5, 	self.custom_profile.initial_turbo),
			self.control_matching['over_voltage']		: self.oc_factor(0, 8, 1, 		self.custom_profile.over_voltage),
			self.control_matching['over_voltage_sdram']	: self.oc_factor(0, 8, 1, 		self.custom_profile.over_voltage_sdram),
			}

		self.variable_lists = {}
		for k, v in self.metrics.iteritems():
			self.variable_lists[k] = self.create_list(v)


	def onInit(self):

		if self.model == 'Pi2':
			# the number of cores is counted, more than 1 is Pi2
			hbutton = self.getControl(103)
			hbutton.setEnabled(False)
			hbutton.setVisible(False)

			hgroup = self.getControl(3)
			hgroup.setEnabled(False)
			hgroup.setVisible(False)

			mbutton = self.getControl(102)
			mbutton.setLabel(lang(32100))

			cbutton = self.getControl(104)

			mbutton.controlUp(self.getControl(101))
			cbutton.controlDown(self.getControl(101))

		else:
			hbutton = self.getControl(103)
			hbutton.setEnabled(True)
			hgroup = self.getControl(3)
			hgroup.setEnabled(True)


		# apply the users current settings (custom)
		self.apply_profile(104)
		self.check_for_profile(focus=True)


	def create_list(self, metric):
		''' generates a list holding the items for each variable control '''

		var_list = [metric.min + (metric.step * x) for x in xrange((metric.range / metric.step) + 1)]
		var_list.append(int(metric.custom))
		var_list = list(set(var_list))
		var_list.sort()
		return var_list


	def slider_change(self, control, up = False):
		''' Executes the change in slider value '''

		cid = control - 2 if up else control - 1
		ctl = self.getControl(cid)
		lbl = ctl.getLabel()
		lst = self.variable_lists[cid]
		idx = lst.index(int(lbl))
		if up:
			mv = 1
		else:
			mv = -1
		new = lst[(idx+mv) % len(lst)]
		ctl.setLabel(str(new))
		self.check_for_profile()


	def apply_profile(self, control):
		''' Applies specific profile to the controls '''

		idx = (control % 100) - 1 
		pfl = self.overclock_profiles[ idx ]._asdict()
		for k, v in self.control_matching.iteritems():
			ctl = self.getControl(v)
			if 'force_turbo' in k:
				if int(pfl[k]):
					ctl.setSelected(True)
				else:
					ctl.setSelected(False)
			else:
				ctl.setLabel(str(pfl[k]))

		self.check_for_profile()


	def snapshot(self):
		''' Returns the current settings in a dictionary. '''

		tmp = {}
		for k, v in self.control_matching.iteritems():
			ctl = self.getControl(v)
			if 'force' in k:
				tmp[k] = 1 if ctl.isSelected() else 0	
			else:
				tmp[k] = int(ctl.getLabel())

		return tmp


	def check_for_profile(self, focus=False):
		''' Checks the current settings to see if they match a specific profile and highlights the required button '''

		compare = self.oc_profile(**self.snapshot())

		match = False

		for i, profile in enumerate(self.overclock_profiles):
			
			if i == 3: break 	# i == 3 means the custom profile

			ctl = self.getControl(1000 + ((i + 1) * 10))

			if profile == compare:
				ctl.setVisible(True)
				match = True
				if focus:
					self.setFocusId(100 + i + 1)
					self.set_description(100 + i + 1)

			else:
				ctl.setVisible(False)

		if match:
			self.getControl(1040).setVisible(False)
		else:
			self.getControl(1040).setVisible(True)
			if focus: 
				self.setFocusId(104)
				self.set_description(104)


		# warning
		# force_turbo + over_voltage sets warranty bit, so don't allow that combination in gui (at least without a warning)
		_, _, _, _, wrn1, wrn2, wrn3 = compare
		if all([any([wrn1, wrn2]), wrn3]):
			self.warning = True
		else:
			self.warning = False



	def onClick(self, controlID):

		if controlID == 2001:
			self.close()

		elif 399 < controlID < 999 and controlID % 100 in [3,4]:
			# slider control
			if controlID % 100 == 3:
				# down
				self.slider_change(controlID, up = False)
			else:
				# up
				self.slider_change(controlID, up = True)

		elif controlID in [101, 102, 103, 104]:
			# profile change

			self.apply_profile(controlID)

		elif controlID == 1002:
			# toggle change

			self.check_for_profile()



	def onAction(self, action):

		actionID = action.getId()
		focused_control = self.getFocusId()

		if actionID in (10, 92):
			self.close()

		else:

			self.set_description(focused_control)


	def set_description(self, focused_control):

			desc = 	self.descriptions.get(focused_control, 
					self.descriptions.get(focused_control - 1, 
					self.descriptions.get(focused_control - 2)))

			if desc and self.warning:
				desc = desc + lang(32099)

			if desc:
				self.getControl(1102).setText(desc)

