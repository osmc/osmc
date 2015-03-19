# Standard modules
import os
import shutil
import sys
import subprocess
import xmltodict
from xml.etree import ElementTree as ET
import traceback

# XBMC modules
import xbmc
import xbmcaddon
import xbmcgui

WINDOW = xbmcgui.Window(10000)

FOLDER = xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources'))

FONT_FOLDER = xbmc.translatePath(os.path.join(FOLDER, 'skins', 'Default', 'fonts'))

FONT_PARTIALS = os.path.join(FOLDER, 'lib', 'fonts.txt')
# FONT_PARTIALS = '/home/kubkev/.kodi/addons/service.osmc.settings/resources/lib/fonts.txt'

FONT_DICT = 	{
			'Font26': 		{
						'720': 'osmc_addon_720_Font26',
						'1080': 'osmc_addon_Font26'
							},
			'Font30': 		{
						'720': 'osmc_addon_720_Font30',
						'1080': 'osmc_addon_Font30'
							},
			'Font40': 		{
						'720': 'osmc_addon_720_Font40',
						'1080': 'osmc_addon_Font40'
							},
			'Font60': 		{
						'720': 'osmc_addon_720_Font60',
						'1080': 'osmc_addon_Font60'
							},
			'OLD_Font25': 	{
						'720': 'osmc_addon_720_OLD_Font25',
						'1080': 'osmc_addon_OLD_Font25'
							},
			'OLD_Font27': 	{
						'720': 'osmc_addon_720_OLD_Font27',
						'1080': 'osmc_addon_OLD_Font27'
							},
			'OLD_Font30': 	{
						'720': 'osmc_addon_720_OLD_Font30',
						'1080': 'osmc_addon_OLD_Font30'},
			'OLD_Font33': 	{
						'720': 'osmc_addon_720_OLD_Font33',
						'1080': 'osmc_addon_OLD_Font33'
							},
			'OLD_Font36': 	{
						'720': 'osmc_addon_720_OLD_Font36',
						'1080': 'osmc_addon_OLD_Font36'
							},
			'OLD_Font42': 	{
						'720': 'osmc_addon_720_OLD_Font42',
						'1080': 'osmc_addon_OLD_Font42'
							},
			'OLD_Font48': 	{
						'720': 'osmc_addon_720_OLD_Font48',
						'1080': 'osmc_addon_OLD_Font48'
							},
			'OLD_Font72': 	{
						'720': 'osmc_addon_720_OLD_Font72',
						'1080': 'osmc_addon_OLD_Font72'
							},
				}


def log(message):
	xbmc.log('UBIQUIFONTS ' + str(message), level=xbmc.LOGDEBUG)


def load_fonts():
	'''
		Loads the font references into Window(10000) for use in OSMC addon guis
	'''

	# get the skin height
	skin_height = WINDOW.getProperty("SkinHeight")
	
	# assign a sabot and height prefix
	if skin_height != '720':

		sabot  = 'osmc_addon_720_'
		height = '720'

	else:

		sabot  = 'osmc_addon_'
		height = '1080'

	# load the fonts into window(10000)
	for fake, fonts in FONT_DICT.iteritems():

		WINDOW.setProperty(fake, sabot + fonts[height] + fake)



def get_addon_folder(alien_skin_folder):
	folder = None
	height = None

	try:
		# first check the addon for the folder location
		tree = ET.parse(os.path.join(alien_skin_folder, 'addon.xml'))
		root = tree.getroot()

		for ext in root.iter('extension'):
			res = ext.find('res')
			if res is None: continue
			height = res.attrib['height']
			folder = res.attrib['folder']
			break

	except Exception, e:
		log(e.args)
		log(traceback.format_exc())

	# failing that, use the folder search option
	if not folder:

		possible_xml_locations = [('1080i', 1080), ('720p', 720), ('1080p', 1080), ('16x9', None)]

		for pos_loc in possible_xml_locations:

			folder = os.path.join(alien_skin_folder, pos_loc[0])

			log('POSSIBLE XML LOCATION = %s' % folder)

			try:

				test = os.listdir(folder)
				height = pos_loc[1]
				break

			except:

				pass

			log('ISNT A FOLDER')

		else:

			log('BREAKEN')

			return

		log('ACTUAL XML LOCATION = %s' % folder)

	if height:
		WINDOW.setProperty("SkinHeight", str(height))

	return folder



def import_osmc_fonts():

	alien_skin_folder = xbmc.translatePath('special://skin')

	log('alien_skin_folder: %s' % alien_skin_folder)

	alien_fonts_folder = os.path.join(alien_skin_folder, 'fonts/')
	alien_fonts = set(os.listdir(alien_fonts_folder))


	alien_xml_folder = get_addon_folder(alien_skin_folder)
	log('ACTUAL XML LOCATION = %s' % alien_xml_folder)

	if not alien_xml_folder:
		return 'failed'

	alien_font_xml = os.path.join(alien_xml_folder, 'Font.xml')
	log('alien_font_xml = %s' % alien_font_xml)


	# check whether the fonts are already in the font xml, if they are then simply return.
	# the previous solution of checking for a backup fonts file is pointless as an update of the skin
	# would overwrite the Font.xml and leave the backup in place
	with open(alien_font_xml, 'r') as f:
		lines = f.readlines()
		for line in lines:
			if 'osmc_addon_OLD_Font72' in line:
				return 'ubiquited'

	# copy fonts to skins font folder 
	unique_files = set(os.listdir(FONT_FOLDER)) - alien_fonts

	for filename in unique_files:
		subprocess.call(["sudo", "cp", os.path.join(FONT_FOLDER, filename), alien_fonts_folder])

	with open(FONT_PARTIALS, 'r') as f:
		osmc_lines = f.readlines()

	with open(alien_font_xml, 'r') as af:
		alien_lines = af.readlines()

	new_lines = []

	count = 0

	for line in alien_lines:

		if '</fontset>' in line and count == 0:

			count += 1

			for l in osmc_lines:

				new_lines.append(l)

			new_lines.append(line)

		else:

			new_lines.append(line)

	# make backup of original Font.xml
	backup_file = os.path.join(alien_fonts_folder,'backup_Font.xml')
	
	log('BACKUP FILE: %s' % backup_file)

	subprocess.call(["sudo", "cp", alien_font_xml, backup_file])
	
	with open('/tmp/Font.xml', 'w') as bf:
		bf.writelines(new_lines)

	subprocess.call(["sudo", "mv", '/tmp/Font.xml', alien_font_xml])

	return 'reload_please'

if __name__ == "__main__":

	import_osmc_fonts()