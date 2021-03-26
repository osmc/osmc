#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.logging

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import argparse
import os.path
import traceback
from io import open
from xml.etree import ElementTree

try:
    FileNotFoundError
except NameError:
    FileNotFoundError = IOError

GUI_FILE = '.kodi/userdata/guisettings.xml'
STRINGS_FILE = '/usr/share/kodi/addons/resource.language.en_gb/resources/strings.po'  # noqa E501
SETTINGS_FILE = '/usr/share/kodi/system/settings/settings.xml'

# A list of tuples of the default information from guisettings to return
SETTINGS_LIST = [
    ('audiooutput', 'ac3passthrough'),
    ('audiooutput', 'ac3transcode'),
    ('audiooutput', 'audiodevice'),
    ('audiooutput', 'channels'),
    ('audiooutput', 'config'),
    ('audiooutput', 'dtshdpassthrough'),
    ('audiooutput', 'dtspassthrough'),
    ('audiooutput', 'eac3passthrough'),
    ('audiooutput', 'guisoundmode'),
    ('audiooutput', 'passthrough'),
    ('audiooutput', 'truehdpassthrough'),
    ('videoplayer', 'adjustrefreshrate'),
    ('videoplayer', 'usedisplayasclock'),
    ('videoplayer', 'quitstereomodeonstop'),
    ('videoplayer', 'stereoscopicplaybackmode'),
    ('videoplayer', 'stretch43'),
    ('videoplayer', 'hqscalers'),
    ('videoplayer', 'useamcodec'),
    ('videoplayer', 'useamcodecmpeg2'),
    ('videoplayer', 'useamcodecmpeg4'),
    ('videoplayer', 'useamcodech264'),
    ('videoscreen', 'force422'),
    ('videoscreen', 'forcergb'),
    ('videoscreen', 'limitedrangeaml'),
    ('videoscreen', 'lockhpd'),
    ('videoscreen', 'screenmode'),
    ('videoscreen', 'whitelist'),
]


class GuiParser(object):

    def __init__(self, gui_file=GUI_FILE, strings_file=STRINGS_FILE, settings_file=SETTINGS_FILE,
                 settings_list=SETTINGS_LIST, section_subset=None):

        self.gui_file = gui_file
        self.strings_file = strings_file
        self.settings_file = settings_file

        self.settings_list = settings_list

        self.section_subset = section_subset

        self.system_settings = None
        self.system_strings = None
        self.gui_settings = None

        self.parsed_values = []

        self.version = None

    def set_version(self):

        try:
            self.version = self.gui_settings.attrib['version']
        except Exception:
            self.version = '1'

    def read_strings(self):

        system_strings = {}

        try:
            with open(self.strings_file, encoding='utf-8') as open_file:
                for line in open_file:
                    if line.startswith('msgctxt'):
                        try:
                            msgctxt = line.split(' ')[1].strip().split('"')[1][1:] # noqa E501
                        except IndexError:
                            continue

                        for line in open_file:
                            if line.startswith('msgid'):
                                try:
                                    system_strings[msgctxt] = line.split(' ', 1)[1].strip().split('"')[1] # noqa E501
                                except IndexError:
                                    system_strings[msgctxt] = 'string failed - %s' % line # noqa E501
                                break

        except Exception:
            tb = traceback.format_exc()
            system_strings['ERROR'] = tb

        self.system_strings = system_strings

        return system_strings

    def parse_settings(self):

        system_settings = {}

        kodi_settings = ElementTree.parse(self.settings_file).getroot()

        for setting in kodi_settings.findall('.//setting'):
            _id = setting.attrib['id']
            system_settings[_id] = {}
            if 'label' in setting.attrib:
                system_settings[_id]['label'] = setting.attrib['label']
            else:
                system_settings[_id][setting.attrib['id']] = None

            system_settings[_id]['options'] = None
            options = setting.find('constraints/options')
            if options is not None:
                system_settings[_id]['options'] = {}
                for o in options:
                    system_settings[_id]['options'][o.text] = o.attrib['label']

            default = setting.find('default')
            if default is not None:
                system_settings[_id]['default'] = default.text

        self.system_settings = system_settings

        return None

    @staticmethod
    def _get_resolution(resolution):
        try:
            return ("{}x{}".format(
                int(resolution[0:5]),
                int(resolution[5:10])),
                    float(resolution[10:18]),
                    resolution[19:20])

        except ValueError:
            return resolution, None, None

    def _special_cases(self, section, label, text):
        """ Extra processing on specified sections """
        if section == "videoscreen.screenmode":
            try:
                text = "{} @ {:0.6g}{}".format(*self._get_resolution(text))
            except ValueError:
                pass
            except TypeError:
                pass
            label = "GUI Resolution"

        if section == "videoscreen.whitelist":
            wl = {}

            for r in text.split(","):
                res, rate, inter = self._get_resolution(r)
                wl.setdefault(res, []).append((rate, inter))
            text = ""

            for r in sorted(wl, reverse=True):
                text += "\n  {:>9s}: ".format(r)
                f = ["{:0.6g}{}".format(i[0], i[1]) for i in wl[r]]
                text += ", ".join(f)

        return label, text

    def parse(self):

        if self.gui_settings is None:
            return self.parsed_values

        for _setting in self.settings_list:
            if type(_setting) is str:
                self.parsed_values.append(_setting)
                continue

            joined_setting = '.'.join(_setting)

            if self.version == '1':
                if _setting[0] == 'settings':
                    continue
                setting = self.gui_settings.find('/'.join(_setting))
            else:
                setting = self.gui_settings.find('.//setting[@id="{}"]'.format(joined_setting))  # noqa E501

            try:
                setting_text = setting.text.strip()
            except Exception:
                setting_text = None

            if not setting_text:
                continue

            try:
                section = self.system_settings[joined_setting]
            except Exception:
                try:
                    self.parsed_values.append('{}: {}'.format(joined_setting, setting_text))  # noqa E501
                except Exception:
                    pass
                continue

            try:
                label = self.system_strings[section['label']]
            except Exception:
                label = _setting

            if section.get('options', None):
                lo = section.get('options', {}).get(setting.text, 'failed')
                text = self.system_strings.get(lo, 'Failed to parse: %s' % lo)
            else:
                text = setting.text

            label, text = self._special_cases(joined_setting, label, text)

            setting_formatted = "{}: {}".format(label, text)
            default = self.system_settings.get(joined_setting, {}).get('default', {})
            if default != setting.text:
                try:
                    setting_formatted += " ===> Default: {}".format(
                        self.system_strings[section['options'][default]])
                except Exception:
                    setting_formatted += ""  # Unknown default value

            if self.section_subset == 'all':
                setting_formatted += " ({})".format(joined_setting)

            self.parsed_values.append(setting_formatted)

        return self.parsed_values

    def rebuild_settings_list(self):
        settings = []

        if self.version == '1':
            if self.section_subset == 'all':
                settings = self._parent_map(self.gui_settings)

            else:
                for sec in self.section_subset.split(','):
                    try:
                        settings = settings + self._parent_map(
                            self.gui_settings.find(sec))
                    except Exception:
                        settings = settings + \
                                   ['=' * 40 + '\nBad section: {}\n'.format(sec) + '=' * 40]  # noqa E501

        else:
            for s in self.gui_settings:
                try:
                    t = tuple(s.attrib['id'].split('.'))
                    if ((self.section_subset != 'all' and t[0] in self.section_subset.split(','))
                            or self.section_subset == 'all'):  # noqa E501
                        settings.append(t)
                except Exception:
                    pass

        self.settings_list = settings

        return None

    @staticmethod
    def _parent_map(gui_settings):
        parent_map = []

        for p in gui_settings.iter():
            for c in p:
                parent_map.append((p.tag, c.tag))

        parent_map = [(p.tag, c.tag) for p in gui_settings.iter() for c in p]

        return parent_map

    def failure(self, failed_to):
        info = (
            failed_to,
            traceback.format_exc(),
            str(self.__dict__).replace(',', '\n\t'))

        return ['Failed to %s:\n%s\nParser Variables:\n\t%s' % info]

    def go(self):
        try:
            self.gui_settings = ElementTree.parse(self.gui_file).getroot()
        except FileNotFoundError:
            return ['Unable to open guisettings file: {}'.format(self.gui_file)]
        except ElementTree.ParseError as e:
            return ['There was a problem parsing {}'.format(self.gui_file), e]
        except Exception:
            return self.failure(failed_to='read guisettings file')

        self.set_version()

        if self.section_subset is not None:
            self.rebuild_settings_list()

        try:
            self.read_strings()
        except Exception:
            return self.failure(failed_to='parse system strings')

        try:
            self.parse_settings()
        except Exception:
            return self.failure(failed_to='parse system settings')

        return self.parse()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Humanize guisettings.xml')
    parser.add_argument('--guifile', '-g',
                        default=os.path.join(os.path.expanduser('~osmc'), GUI_FILE),
                        help='Name of the guisettings.xml file to parse.')
    parser.add_argument('--stringsfile', '-t', default=STRINGS_FILE,
                        help='Strings.po file to parse')
    parser.add_argument('--settingsfile', '-s', default=SETTINGS_FILE,
                        help='Settings.xml file to parse')
    parser.add_argument('--all', '-a', nargs='?', action='store',
                        const='all',
                        help='Show the complete guisettings.xml humanized')

    parsed_args = parser.parse_args()

    gui_parser = GuiParser(gui_file=parsed_args.guifile, strings_file=parsed_args.stringsfile,
                           settings_file=parsed_args.settingsfile, settings_list=SETTINGS_LIST,
                           section_subset=parsed_args.all)

    for parser_line in gui_parser.go():
        print(parser_line)
