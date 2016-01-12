# Standard modules
import os
import shutil
import subprocess
import sys
import traceback
from xml.etree import ElementTree as ET

# XBMC modules
import xbmc
import xbmcaddon
import xbmcgui

WINDOW        = xbmcgui.Window(10000)

FOLDER        = xbmc.translatePath(os.path.join(xbmcaddon.Addon().getAddonInfo('path'), 'resources'))

FONT_FOLDER   = xbmc.translatePath(os.path.join(FOLDER, 'skins', 'Default', 'fonts'))

FONT_PARTIALS = os.path.join(FOLDER, 'lib', 'fonts.txt')

# FONT_PARTIALS = '/home/kubkev/.kodi/addons/service.osmc.settings/resources/lib/fonts.txt'


def log(message):

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )

	xbmc.log('UBIQUIFONTS ' + str(message), level=xbmc.LOGDEBUG)


def get_addon_folder(alien_skin_folder):
	folder = None

	try:
		# first check the addon for the folder location
		tree = ET.parse(os.path.join(alien_skin_folder, 'addon.xml'))
		root = tree.getroot()

		for ext in root.iter('extension'):
			res = ext.find('res')
			if res is None: continue
			height = res.attrib['height']
			width  = res.attrib['width']
			folder = res.attrib['folder']
			break

	except Exception, e:
		log(e.args)
		log(traceback.format_exc())

	# failing that, use the folder search option
	if not folder:

		possible_xml_locations = [('1080i', 1080, 1920), ('720p', 720, 1280), ('1080p', 1080, 1920), ('16x9', None, None)]

		for pos_loc in possible_xml_locations:

			folder = os.path.join(alien_skin_folder, pos_loc[0])

			log('POSSIBLE XML LOCATION = %s' % folder)

			try:

				test = os.listdir(folder)
				height = pos_loc[1]
				width = pos_loc[2]
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
		WINDOW.setProperty("SkinWidth", str(width))

	return os.path.join(alien_skin_folder, folder)



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
			if 'osmc_addon_XLarge' in line:
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
