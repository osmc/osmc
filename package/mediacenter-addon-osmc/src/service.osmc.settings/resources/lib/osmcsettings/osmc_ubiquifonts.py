# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of service.osmc.settings

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import subprocess
import sys
import traceback
from io import open
from xml.etree import ElementTree

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_logging import StandardLogger

ADDON_ID = 'service.osmc.settings'
PY2 = sys.version_info.major == 2


class UbiquiFonts:

    def __init__(self, addon_id=None, addon=None, window=None):
        self._addon_id = addon_id
        self._addon = addon
        self._window = window

        _logger = StandardLogger(addon_id, os.path.basename(__file__))
        self.log = _logger.log

        self.resource_folder = xbmc.translatePath(
            os.path.join(addon.getAddonInfo('path'), 'resources')
        )

        self.font_folder = xbmc.translatePath(
            os.path.join(self.resource_folder, 'skins', 'Default', 'fonts')
        )

        self.font_partials = os.path.join(
            self.resource_folder, 'lib', 'osmcsettings', 'fonts.txt'
        )

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

    def _get_addon_folder(self, skin_folder):
        folder = None
        height = None
        width = None

        try:
            # first check the addon for the folder location
            tree = ElementTree.parse(os.path.join(skin_folder, 'addon.xml'))
            root = tree.getroot()

            for ext in root.iter('extension'):
                res = ext.find('res')
                if res is None:
                    continue
                height = res.attrib['height']
                width = res.attrib['width']
                folder = res.attrib['folder']
                break

        except Exception as e:
            self.log(e.args)
            self.log(traceback.format_exc())

        # failing that, use the folder search option
        if not folder:

            possible_xml_locations = [('1080i', 1080, 1920), ('720p', 720, 1280),
                                      ('1080p', 1080, 1920), ('16x9', None, None)]

            for location_folder, location_height, location_width in possible_xml_locations:
                folder = os.path.join(skin_folder, location_folder)
                self.log('POSSIBLE XML LOCATION = %s' % folder)
                if not os.path.isdir(folder):
                    continue

                height = location_height
                width = location_width
                break

            else:
                self.log('FAILED')
                return

            self.log('ACTUAL XML LOCATION = %s' % folder)

        if height and width:
            self.window.setProperty("SkinHeight", str(height))
            self.window.setProperty("SkinWidth", str(width))

        return os.path.join(skin_folder, folder)

    def import_osmc_fonts(self):
        skin_folder = xbmc.translatePath('special://skin')

        self.log('skin_folder: %s' % skin_folder)

        fonts_folder = os.path.join(skin_folder, 'fonts/')

        xml_folder = self._get_addon_folder(skin_folder)
        self.log('ACTUAL XML LOCATION = %s' % xml_folder)

        if not xml_folder:
            return 'failed'

        font_xml = os.path.join(xml_folder, 'Font.xml')
        self.log('font_xml = %s' % font_xml)

        # check whether the fonts are already in the font xml, if they are then simply return.
        # the previous solution of checking for a backup fonts file is pointless as an update
        # of the skin would overwrite the Font.xml and leave the backup in place
        lines = []
        with open(font_xml, 'r', encoding='utf-8') as open_file:
            line = open_file.readline()
            if 'osmc_addon_XLarge' in line:
                return 'ubiquited'
            lines.append(line)

        # copy fonts to skins font folder
        fonts = set(os.listdir(fonts_folder))
        unique_files = set(os.listdir(self.font_folder)) - fonts

        for filename in unique_files:
            subprocess.call(["sudo", "cp",
                             os.path.join(self.font_folder, filename), fonts_folder])

        with open(self.font_partials, 'r', encoding='utf-8') as open_file:
            font_partials_lines = open_file.readlines()

        with open(font_xml, 'r', encoding='utf-8') as open_file:
            font_xml_lines = open_file.readlines()

        new_lines = []
        fonts_added = False

        for line in font_xml_lines:

            if '</fontset>' in line and not fonts_added:
                fonts_added = True
                new_lines += font_partials_lines
                new_lines.append(line)

            else:
                new_lines.append(line)

        if PY2:
            new_lines = [
                x.decode('utf-8') if isinstance(x, str) else x for x in new_lines
            ]

        # make backup of original Font.xml
        backup_file = os.path.join(fonts_folder, 'backup_Font.xml')

        self.log('BACKUP FILE: %s' % backup_file)

        subprocess.call(["sudo", "cp", font_xml, backup_file])

        with open('/tmp/Font.xml', 'w', encoding='utf-8') as open_file:
            open_file.writelines(new_lines)

        subprocess.call(["sudo", "mv", '/tmp/Font.xml', font_xml])

        return 'reload_please'


if __name__ == "__main__":
    UbiquiFonts().import_osmc_fonts()
