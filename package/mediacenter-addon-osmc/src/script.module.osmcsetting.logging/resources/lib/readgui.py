#!/usr/bin/env python2

from __future__ import print_function
import xml.etree.ElementTree as ET
import os.path
import traceback
import argparse

# For python2/3 compatability
try:
    FileNotFoundError
except NameError:
    FileNotFoundError = IOError

DEFAULT_GUIFILE = '.kodi/userdata/guisettings.xml'
DEFAULT_STRINGSFILE = '/usr/share/kodi/addons/resource.language.en_gb/resources/strings.po' # noqa E501
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
        except Exception:
            self.version = '1'

    def read_strings(self, *args, **kwargs):

        system_strings = {}

        try:
            with open(self.stringsfile) as f:
                for line in f:
                    if line.startswith('msgctxt'):
                        try:
                            msgctxt = line.split(' ')[1].strip().split('"')[1][1:] # noqa E501
                        except IndexError:
                            continue
                        for line in f:
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
            default = setting.find('default')
            if default is not None:
                system_settings[_id]['default'] = default.text

        self.system_settings = system_settings

        return None

    def _get_resolution(self, resolution):
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
                setting = self.gui_settings.find('.//setting[@id="{}"]'.format(sj)) # noqa E501

            try:
                setting_text = setting.text.strip()
            except Exception:
                setting_text = None

            if not setting_text:
                continue

            try:
                section = self.system_settings[sj]
            except Exception:
                try:
                    self.parsed_values.append('{}: {}'.format(sj, setting_text)) # noqa E501
                except Exception:
                    pass
                continue
            try:
                lb = self.system_strings[section['label']]
            except Exception:
                lb = s
            if section.get('options', None):
                lo = section.get('options', {}).get(setting.text, 'failed')
                t = self.system_strings.get(lo, 'Failed to parse: %s' % lo)
            else:
                t = setting.text

            lb, t = self._special_cases(sj, lb, t)

            setting_formatted = "{}: {}".format(lb, t)
            default = self.system_settings.get(sj, {}).get('default', {})
            if default != setting.text:
                try:
                    setting_formatted += " ===> Default: {}".format(
                        self.system_strings[section['options'][default]])
                except Exception:
                    setting_formatted += ""  # Unknown default value
            if self.section_subset == 'all':
                setting_formatted += " ({})".format(sj)
            self.parsed_values.append(setting_formatted)

        return self.parsed_values

    def rebuild_settings_list(self, *args, **kwargs):

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
                        settings = settings + ['=' * 40 + '\nBad section: {}\n'.format(sec) + '=' * 40] # noqa E501
        else:
            for s in self.gui_settings:
                try:
                    t = tuple(s.attrib['id'].split('.'))
                    if (self.section_subset != 'all' and t[0] in self.section_subset.split(',')) or self.section_subset == 'all': # noqa E501
                        settings.append(t)
                except Exception:
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
        except FileNotFoundError:
            return ['Unable to open guisettings file: {}'.format(self.guifile)]
        except ET.ParseError as e:
            return ['There was a problem parsing {}'.format(self.guifile), e]
        except Exception:
            return self.failure(failed_to='read guisettings file')

        self.set_version()

        if self.section_subset is not None:
            self.rebuild_settings_list(self.section_subset)

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
                        default=os.path.join(os.path.expanduser('~osmc'),
                                             DEFAULT_GUIFILE),
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
