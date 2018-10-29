#!/usr/bin/env python2

from __future__ import print_function
import xml.etree.ElementTree as ET
import os.path
import traceback
import argparse


DEFAULT_GUIFILE = '.kodi/userdata/guisettings.xml'
DEFAULT_STRINGSFILE = '/usr/share/kodi/addons/resource.language.en_gb/resources/strings.po'
DEFAULT_SETTINGSFILE = '/usr/share/kodi/system/settings/settings.xml'

# A list of tuples of the default information from guisettings to return
DEFAULT_SETTINGSLIST = [
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
    ('videoplayer', 'stretch43'),
    ('videoplayer', 'hqscalers'),
    ('videoplayer', 'useamcodec'),
    ('videoplayer', 'useamcodecmpeg2'),
    ('videoplayer', 'useamcodecmpeg4'),
    ('videoplayer', 'useamcodech264'),
]

class GuiParser(object):

    def __init__(self, 
            guifile=DEFAULT_GUIFILE, 
            stringsfile=DEFAULT_STRINGSFILE,
            settingsfile=DEFAULT_SETTINGSFILE,
            settings_list=DEFAULT_SETTINGSLIST,
            section_subset=None,
            *args, **kwargs):

        self.guifile = guifile
        self.stringsfile = stringsfile
        self.settingsfile = settingsfile

        self.settings_list = settings_list

        self.section_subset = section_subset

        self.system_settings = None
        self.system_strings = None
        self.gui_settings = None

        self.parsed_values = []

        self.version = None


    def set_version(self, *args, **kwargs):

        try:
            self.version = self.gui_settings.attrib['version']
        except:
            self.version = '1'


    def read_strings(self, *args, **kwargs):

        system_strings = {}

        try:
            with open(self.stringsfile) as f:
                for line in f:
                    if line.startswith('msgctxt'):
                        try:
                            msgctxt = line.split(' ')[1].strip().split('"')[1][1:]
                        except IndexError:
                            continue
                        for line in f:
                            if line.startswith('msgid'):
                                try:
                                    system_strings[msgctxt] = line.split(' ', 1)[1].strip().split('"')[1]
                                except IndexError:
                                    system_strings[msgctxt] = 'string failed - %s' % line
                                break
        except:
            tb = traceback.format_exc()
            system_strings['ERROR'] = tb

        self.system_strings = system_strings

        return system_strings


    def parse_settings(self, *args, **kwargs):

        system_settings = {}

        kodi_settings = ET.parse(self.settingsfile).getroot()


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

        self.system_settings = system_settings

        return None


    def parse(self, *args, **kwargs):

        if self.gui_settings is None: 
            return self.parsed_values

        for s in self.settings_list:
            if type(s) is str:
                self.parsed_values.append(s)
                continue

            sj = '.'.join(s)

            if self.version == '1':
                if s[0] == 'settings': 
                    continue
                setting = self.gui_settings.find('/'.join(s))
            else:
                setting = self.gui_settings.find('.//setting[@id="{}"]'.format(sj))

            try:
                setting_text = setting.text.strip()
            except:
                setting_text = None

            if not setting_text:
                continue

            try:
                section = self.system_settings[sj]
            except:
                try:
                    self.parsed_values.append('{}: {}'.format(sj, setting_text))
                except:
                    pass
                continue
            try:
                lb = self.system_strings[section['label']]
            except:
                lb = s
            if section.get('options', None):
                l = section.get('options', {}).get(setting.text, 'failed')
                t = self.system_strings.get(l, 'Failed to parse: %s' %  l)
            else:
                t = setting.text

            self.parsed_values.append('{}: {}'.format(lb, t))

        return self.parsed_values


    def rebuild_settings_list(self, *args, **kwargs):

        settings = []

        if self.version == '1':
            if self.section_subset == 'all':
                settings = self._parent_map(self.gui_settings)
            else:
                for sec in self.section_subset.split(','):
                    try:
                        settings = settings + self._parent_map(self.gui_settings.find(sec))
                    except:
                        settings = settings + ['=' * 40 + '\nBad section: {}\n'.format(sec) + '=' * 40]
        else:
            for s in self.gui_settings:
                try:
                    t = tuple(s.attrib['id'].split('.'))
                    if (self.section_subset != 'all' and t[0] in self.section_subset.split(',')) or self.section_subset == 'all':
                        settings.append(t)
                except:
                    pass

        self.settings_list = settings

        return None


    def _parent_map(self, gui_settings, *args, **kwargs):

        parent_map = []

        for p in gui_settings.iter():
            for c in p:
                parent_map.append((p.tag, c.tag))

        parent_map = [(p.tag, c.tag) for p in gui_settings.iter() for c in p]

        return parent_map


    def failure(self, failed_to, *args, **kwargs):

        info = (
                failed_to, 
                traceback.format_exc(), 
                str(self.__dict__).replace(',', '\n\t'))

        return ['Failed to %s:\n%s\nParser Variables:\n\t%s' % info]


    def go(self, *args, **kwargs):

        try:
            self.gui_settings = ET.parse(self.guifile).getroot()
        except:
            return self.failure(failed_to='read guisettings file')

        self.set_version()

        if self.section_subset is not None:
            self.rebuild_settings_list(self.section_subset)

        try:
            self.read_strings()
        except:
            return self.failure(failed_to='parse system strings')

        try:
            self.parse_settings()
        except:
            return self.failure(failed_to='parse system settings')

        return self.parse()


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Humanize guisettings.xml')
    parser.add_argument('--guifile', '-g',
                        default=os.path.join(os.path.expanduser('~'), DEFAULT_GUIFILE),
                        help='Name of the guisettings.xml file to parse.')
    parser.add_argument('--stringsfile', '-t',
                        default=DEFAULT_STRINGSFILE,
                        help='Strings.po file to parse')
    parser.add_argument('--settingsfile', '-s',
                        default=DEFAULT_SETTINGSFILE,
                        help='Settings.xml file to parse')
    parser.add_argument('--all', '-a', nargs='?', action='store',
                        const='all',
                        help='Show the complete guisettings.xml humanized')

    args = parser.parse_args()

    gp = GuiParser(
            guifile=args.guifile, 
            stringsfile=args.stringsfile, 
            settingsfile=args.settingsfile,
            settings_list=DEFAULT_SETTINGSLIST,
            section_subset=args.all)

    for l in gp.go():
        print(l)

