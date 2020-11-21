# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.networking

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import re
import subprocess
from io import open

import xbmc
import xmltodict as xmltodict


class AdvancedSettingsEditor(object):

    def __init__(self, logging_function=None):
        if logging_function is None:
            self.log = self.null_log

        else:
            self.log = logging_function

    def null_log(self):
        pass

    def parse_advanced_settings(self):
        """
            Parses the advancedsettings.xml file. Returns a dict with ALL the details.
        """
        user_data = xbmc.translatePath('special://userdata')
        loc = os.path.join(user_data, 'advancedsettings.xml')

        null_doc = {
            'advancedsettings': {}
        }

        self.log('advancedsettings file exists = %s' % os.path.isfile(loc))

        if not os.path.isfile(loc):
            return null_doc

        try:
            with open(loc, 'r', encoding='utf-8') as open_file:
                lines = open_file.readlines()

            if not lines:
                self.log('advancedsettings.xml file is empty')
                raise

            with open(loc, 'r', encoding='utf-8') as open_file:
                doc = xmltodict.parse(open_file)

            # ensure empty advancedsettings nodes are ignored
            if not doc.get('advancedsettings', None):
                self.log('advancedsettings node in advancedsettings.xml file is empty')
                raise

            else:
                return doc

        except:
            self.log('error occurred reading advancedsettings.xml file')
            return null_doc

    @staticmethod
    def server_not_localhost(dictionary):
        """
            Checks the MySQL settings to ensure neither server is on the localhost
        """
        dbs = [dictionary.get('advancedsettings', {}).get('musicdatabase', {}),
               dictionary.get('advancedsettings', {}).get('videodatabase', {})]
        pattern = re.compile(r'(127.\d+.\d+.\d+|localhost|::1)')
        # local_indicators = ['127.0.0.1', '127.0.1.1','localhost', '::1']

        for db in dbs:
            host = db.get('host', None)
            if host:
                if not pattern.match(host):
                    return True

        return False

    def validate_advset_dict(self, dictionary, reject_empty=False,
                             exclude_name=False, no_pw_ok=False):
        """
            Checks whether the provided dictionary is fully populated with MySQL settings info.
            If reject_empty is False, then Blank dictionaries are rejected, but dictionaries with
            no video or music database dicts are passed.
            If reject_empty is True,  then Blank dictionaries are rejected, AND dictionaries with
            no video or music database dicts are also rejected.
            exclude_name means that the name sql item can be ignored
            (it is not strictly required, but the GUI ALWAYS adds it).
        """
        main = dictionary.get('advancedsettings', {})

        if not main:
            return False, 'empty'

        if exclude_name:
            sql_sub_items = ['host', 'port', 'user', 'pass']
        else:
            sql_sub_items = ['name', 'host', 'port', 'user', 'pass']

        if no_pw_ok:
            sql_sub_items.remove('pass')  # Don't require a password

        if 'videodatabase' in main:
            # fail if the items aren't filled in or are the default up value
            for item in sql_sub_items:
                sub_item = main.get('videodatabase', {}).get(item, False)
                if not sub_item or sub_item == '___ : ___ : ___ : ___':
                    self.log('Missing MySQL Video setting: {}'.format(item))
                    return False, 'missing mysql'

        if 'musicdatabase' in main:
            for item in sql_sub_items:
                sub_item = main.get('musicdatabase', {}).get(item, False)
                if not sub_item or sub_item == '___ : ___ : ___ : ___':
                    self.log('Missing MySQL Music setting: {}'.format(item))
                    return False, 'missing mysql'

        if reject_empty:
            if not any(['musicdatabase' in main, 'videodatabase' in main]):
                return False, 'empty db fields'

        return True, 'complete'

    def write_advancedsettings(self, loc, dictionary):
        """
            Writes the supplied dictionary back to the advancedsettings.xml file
        """
        if not dictionary.get('advancedsettings', None):
            self.log('Empty dictionary passed to advancedsettings file writer. '
                     'Preventing write, backing up and removing file.')

            subprocess.call(['sudo', 'cp', loc, loc.replace('advancedsettings.xml',
                                                            'advancedsettings_backup.xml')])

            subprocess.call(['sudo', 'rm', '-f', loc])
            return

        with open(loc, 'w', encoding='utf-8') as open_file:
            xmltodict.unparse(input_dict=dictionary, output=open_file, pretty=True)
