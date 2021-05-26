# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of service.osmc.settings

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import subprocess
import traceback
from io import open
from xml.etree import ElementTree

import xbmcaddon
import xbmcgui
import xbmcvfs
from osmccommon.osmc_logging import StandardLogger

ADDON_ID = 'service.osmc.settings'


class UbiquiFonts:

    def __init__(self, addon_id=None, addon=None, window=None):
        self._addon_id = addon_id
        self._addon = addon
        self._window = window

        _logger = StandardLogger(addon_id, os.path.basename(__file__))
        self.log = _logger.log

        self.resource_folder = xbmcvfs.translatePath(
            os.path.join(addon.getAddonInfo('path'), 'resources')
        )

        self.font_folder = xbmcvfs.translatePath(
            os.path.join(self.resource_folder, 'skins', 'Default', 'fonts')
        )

        self.font_partials = os.path.join(
            self.resource_folder, 'lib', 'osmcsettings', 'fonts.txt'
        )

        self.skin_folder = xbmcvfs.translatePath('special://skin')
        self.log('Skin folder: %s' % self.skin_folder)

        self.skin_font_folder = xbmcvfs.translatePath('special://skin/fonts/')

        self._skin_xml_folder = None
        self._skin_font_xml = None

    @property
    def addon_id(self):
        if not self._addon_id:
            self._addon_id = ADDON_ID
        return self._addon_id

    @property
    def addon(self):
        if not self._addon:
            self._addon = xbmcaddon.Addon(self.addon_id)
        return self._addon

    @property
    def window(self):
        if not self._window:
            self._window = xbmcgui.Window(10000)
        return self._window

    @property
    def skin_xml_folder(self):
        if self._skin_xml_folder:
            return self._skin_xml_folder

        folder = None
        height = None
        width = None

        try:
            # first check the addon for the folder location
            tree = ElementTree.parse(os.path.join(self.skin_folder, 'addon.xml'))
            root = tree.getroot()

            for ext in root.iter('extension'):
                res = ext.find('res')
                if res is None:
                    continue

                height = res.attrib['height']
                width = res.attrib['width']
                folder = res.attrib['folder']

                self.log('Found skin\'s xml folder in addon.xml: %s' % folder)
                break

        except Exception as e:
            self.log(e.args)
            self.log(traceback.format_exc())

        # failing that, use the folder search option
        if not folder:

            xml_folder_candidates = [('1080i', 1080, 1920), ('720p', 720, 1280),
                                     ('1080p', 1080, 1920), ('16x9', None, None)]

            for candidate_folder, candidate_height, candidate_width in xml_folder_candidates:
                folder = os.path.join(self.skin_folder, candidate_folder)
                self.log('Checking if %s is skin\'s xml folder...' % folder)
                if not os.path.isdir(folder):
                    continue

                height = candidate_height
                width = candidate_width
                break

            else:
                self.log('Failed to find skin\'s xml folder.')
                return None

            self.log('Found skin\'s xml folder: %s' % folder)

        if height and width:
            self.window.setProperty('SkinHeight', str(height))
            self.window.setProperty('SkinWidth', str(width))

        self._skin_xml_folder = os.path.join(self.skin_folder, folder)
        self.log('Skin XML folder: %s' % self.skin_xml_folder)
        return self._skin_xml_folder

    @property
    def skin_font_xml(self):
        if self._skin_font_xml:
            return self._skin_font_xml

        self._skin_font_xml = os.path.join(self.skin_xml_folder, 'Font.xml')
        self.log('Skin Font.xml: %s' % self._skin_font_xml)
        return self._skin_font_xml

    def import_osmc_fonts(self):
        self.log('Importing Ubiqui fonts')

        if not self.skin_xml_folder:
            return 'failed'

        # check whether the fonts are already in the font xml, if they are then simply return.
        # the previous solution of checking for a backup fonts file is pointless as an update
        # of the skin would overwrite the Font.xml and leave the backup in place
        lines = []
        fontset_elements = 0
        osmc_font_count = 0
        with open(self.skin_font_xml, 'r', encoding='utf-8') as open_file:
            line = open_file.readline()

            while line:
                if '<fontset' in line or '</fontset' in line:
                    fontset_elements += 1
                if 'osmc_addon_XLarge' in line:
                    osmc_font_count += 1

                lines.append(line)
                line = open_file.readline()

        # fontset elements come in pairs, with a single osmc font block
        # if fontset elements divided by osmc font count is 2, each fontset has been 'ubiquited'
        try:
            completed = (fontset_elements // osmc_font_count) == 2
        except ZeroDivisionError:
            completed = False

        if completed:
            self.log('Ubiqui fonts already exist.')
            return 'ubiquited'

        # copy fonts to skins font folder
        fonts = set(os.listdir(self.skin_font_folder))
        unique_files = set(os.listdir(self.font_folder)) - fonts

        for filename in unique_files:
            subprocess.call([
                'sudo', 'cp', os.path.join(self.font_folder, filename), self.skin_font_folder
            ])

        with open(self.font_partials, 'r', encoding='utf-8') as open_file:
            font_partials_lines = open_file.readlines()

        with open(self.skin_font_xml, 'r', encoding='utf-8') as open_file:
            font_xml_lines = open_file.readlines()

        payload = []
        fonts_added = False

        for line in font_xml_lines:
            if '<fontset' in line:
                fonts_added = False

            if 'osmc_addon_XLarge' in line:
                fonts_added = True

            if '</fontset>' in line and not fonts_added:
                payload += font_partials_lines
                fonts_added = True

            payload.append(line)

        backup_file = os.path.join(self.skin_font_folder, 'backup_Font.xml')
        self.log('Creating Font.xml backup: %s' % backup_file)
        subprocess.call(['sudo', 'cp', self.skin_font_xml, backup_file])

        tmp_font_xml = '/tmp/Font.xml'
        self.log('Writing temporary Font.xml: %s' % tmp_font_xml)
        with open(tmp_font_xml, 'w', encoding='utf-8') as open_file:
            open_file.writelines(payload)

        self.log('Moving new Font.xml to skin: %s' % self.skin_font_xml)
        subprocess.call(['sudo', 'mv', tmp_font_xml, self.skin_font_xml])

        self.log('Ubiqui fonts import completed.')
        return 'reload_please'
