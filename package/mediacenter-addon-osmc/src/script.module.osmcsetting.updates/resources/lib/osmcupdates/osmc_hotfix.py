# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.updates

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import shlex
import subprocess
import sys
import traceback
from io import open

import requests

import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger

ADDON_ID = 'script.module.osmcsetting.updates'
DIALOG = xbmcgui.Dialog()
PY2 = sys.version_info.major == 2

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class HotFix(object):

    def __init__(self, addon=None):
        self._addon = addon
        self._lang = None

        self.tmp_hfo_location = '/var/tmp/uploadHotFixOutput.txt'

        hf_key = self.user_enters_key()
        hf_cypher = self.convert_key_to_cypher(hf_key)
        hf_raw_text = self.retrieve_hotfix(hf_cypher)
        hf_parsed = self.parse_hotfix(hf_raw_text)
        user_confirmation = self.confirm_instruction()

        self.display_description(hf_parsed['description'])
        self.display_instruction(hf_parsed['instruction'])

        if not user_confirmation:
            return

        results = self.apply_instruction(hf_parsed['instruction'])

        self.save_temp_hotfix_output(results)

        self.resolution_dispatcher(results, hf_parsed['resolution'])

    @property
    def addon(self):
        if not self._addon:
            self._addon = xbmcaddon.Addon(ADDON_ID)
        return self._addon

    def lang(self, value):
        if not self._lang:
            retriever = LangRetriever(self.addon)
            self._lang = retriever.lang
        return self._lang(value)

    def user_enters_key(self):
        """
            Provides a dialog through which the user enters a 5-digit key code
        """
        # DEFAULT VALUE ONLY USED FOR TESTING
        hf_key = DIALOG.input(self.lang(32115), '', type=xbmcgui.INPUT_ALPHANUM)
        log(label='User entered hotfix ID', message=hf_key)

        return hf_key

    @staticmethod
    def convert_key_to_cypher(hf_key):
        """
            Takes a 5-digit key code and converts it to the cypher used in retrieving
            the HotFix from paste.osmc.io
            *** YET TO BE IMPLEMENTED
        """
        return hf_key

    @staticmethod
    def retrieve_hotfix(hf_cypher):
        """
            Takes a five-digit hotfix key and retrieve the hotfix from paste.osmc.io.
            Returns the raw contents at that location as a string.
        """
        url = 'https://paste.osmc.tv/raw/%s' % hf_cypher
        log(label='Retrieving hotfix from', message=url)

        raw_result = requests.get(url)
        raw_text = raw_result.text

        log(label='HotFix Result', message=raw_text)
        return raw_text

    @staticmethod
    def parse_hotfix(hf_result):
        """
            Takes the raw string from the paste.osmc.io location and parses it.
            Returns a dictionary with the DESCRIPTION of the hotfix, the INSTRUCTION
            (what is run on the command line) and the RESOLUTION as a list
            (the actions to take after the INSTRUCTION is run).

            The structure of the file is as follows:

                DESCRIPTION (optional):
                    - one line string only
                    - must start with "DESCRIPTION:", otherwise treated as part of the INSTRUCTION

                INSTRUCTION:
                    - can be multiple lines
                    - multiple lines will be run consecutively only if the exit code is 0
                    - can start with "INSTRUCTION:" but not required

                RESOLUTION (optional):
                    - one line string with resolution actions
                    - can be comma, space, bar, colon, or semi-colon delimited
                    - must be the last line in the result
                    - and must start with "RESOLUTION:", otherwise it will be treated as part of
                      the INSTRUCTION SET
                    - UPLOAD: upload the results of the INSTRUCTION to paste.osmc.io and provide
                      the user with the link
                    - SAVE: to save the results to a specific file
                    - LOG: write the results to the kodi log
                    - LOG is a mandatory RESOLUTION and is done every time by default
             """

        description = 'No description available'
        instruction = []
        resolution = []

        delimiters = [' ', ',', '.', '|', ':', ':']

        result_list = hf_result.split('\n')

        for i, line in enumerate(result_list):
            if i == 0:
                if line.startswith('DESCRIPTION:'):
                    description = line.replace('DESCRIPTION:', '').strip()
                    continue

            if i == len(result_list) - 1:
                if line.startswith('RESOLUTION:'):
                    desc = line.replace('RESOLUTION:', '').strip()
                    for d in delimiters:
                        if d in desc:
                            resolution = desc.split(d)
                            break
                    continue

            instruction.append(line.replace('INSTRUCTION:', '').strip())

        log(label='Description', message=description)
        log(label='Instruction', message=instruction)
        log(label='Resolution', message=resolution)

        hf_parsed = {
            'description': description,
            'instruction': instruction,
            'resolution': resolution
        }

        return hf_parsed

    def display_description(self, description):
        """
            Displays a description of the hotfix on-screen for the user
            *** YET TO BE IMPLEMENTED
        """

    def display_instruction(self, instruction):
        """
            Displays the specific instructions on-screen for the user
            *** YET TO BE IMPLEMENTED
        """

    def confirm_instruction(self):
        """
            Asks the user to confirm that they wish to apply the instruction.
            Returns TRUE, only if user clicks Yes.
        """
        user_confirmation = DIALOG.yesno(self.lang(32116),
                                         '[CR]'.join([self.lang(32117),
                                                      self.lang(32118),
                                                      self.lang(32119)]))

        return user_confirmation

    @staticmethod
    def apply_instruction(instruction):
        """
            Applies the instruction via the command line.
            Returns the resulting output in a list of lines.
        """
        dangerous = ['rm -rf /', ]
        results = []
        for line in instruction:
            try:
                instruct = shlex.split(line)
                results.append('>>>>> INSTRUCTION >>>>> %s\n' % ' '.join(instruct))

                try:
                    results.append(subprocess.check_output(instruct))

                except subprocess.CalledProcessError as e:
                    # raise RuntimeError("command '{}' return with error (code {}): {}"
                    # .format(e.cmd, e.returncode, e.output))
                    log(label='Non-zero exit code from line', message=e.output)
                    results.append(e.output)
                    break

            except Exception as e:
                results.append('Error: %s\n%s' % (str(e), traceback.format_exc()))
                break

        return results

    def resolution_dispatcher(self, results, resolutions=None):
        """
            Applies the stated resolutions in the file.
            If LOG is not in resolutions, then add it in.
        """
        if resolutions is None:
            resolutions = []

        resolution_map = {
            'UPLOAD': self.resolution_upload,
            'SAVE': self.resolution_save,
            'LOG': self.resolution_log,
        }

        if 'LOG' not in resolutions:
            resolutions.append('LOG')

        for resolution in resolutions:
            func = resolution_map.get(resolution, self.missing_resolution)
            func(results)

    @staticmethod
    def missing_resolution(results):
        """
            Method that is run when the dispatcher receives a resolution it doesnt understand
        """
        log('Unknown request received by dispatcher.')

    def resolution_upload(self, results):
        """
            Uploads the results stored in the temporary file to paste.osmc.io and
            provide the user with the URL
        """
        with os.popen('curl -X POST -s -T "%s" https://paste.osmc.tv/documents' %
                      self.tmp_hfo_location) as open_file:
            line = open_file.readline()
            key = line.replace('{"key":"', '').replace('"}', '').replace('\n', '')
            log('pastio line: %s' % repr(line))

        if not key:
            log("OSMC HotFix upload failed.")
            save = DIALOG.yesno(self.lang(32120),
                                '[CR]'.join([self.lang(32121), self.lang(32122)]))

            if save:
                self.resolution_save(results=None)

        else:
            url = 'https://paste.osmc.tv/ %s' % key
            log(label="HotFix output uploaded to", message=url.replace(' ', ''))
            _ = DIALOG.ok(self.lang(32120), '[CR]'.join([self.lang(32123), "URL: %s" % url]))

    def save_temp_hotfix_output(self, results):
        """
            Saves the hotfix output to a temporary file
        """
        if PY2:
            results = [
                x.decode('utf-8') if isinstance(x, str) else x for x in results
            ]

        with open(self.tmp_hfo_location, 'w', encoding='utf-8') as f:
            f.writelines(results)

    def resolution_save(self, results):
        """
            Save the results to a file at the SD card
        """
        log('Copying HotFix output to /boot/')
        os.popen('sudo cp -rf %s /boot/' % self.tmp_hfo_location)

    @staticmethod
    def resolution_log(results):
        """
            Write the results to the kodi log file. Debug is not required.
        """
        for line in results:
            log(label='HotFix Output', message=line)


if __name__ == "__main__":
    hotfix = HotFix()
