# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.updates

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import re
import shlex
import socket
import subprocess
import traceback
from io import open

import requests

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger

ADDON_ID = 'script.module.osmcsetting.updates'
DIALOG = xbmcgui.Dialog()

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class HotFix(object):

    def __init__(self, addon=None):
        self._addon = addon
        self._lang = None

        self.tmp_hfo_location = '/var/tmp/uploadHotFixOutput.txt'

        # A HotFix that needs a restart touches this rather than rebooting the
        # device itself, so the decision is the user's and the run is never cut
        # short mid-command.
        self.reboot_flag = '/tmp/reboot_needed_hotfix'

        # hf_key = 'ucawobahij'  # https://discourse.osmc.tv/t/kodi-v19-troubleshooting-faq/89968

        hf_key = self.user_enters_key()
        # hf_cypher = self.convert_key_to_cypher(hf_key)

        hf_from_mirror = True
        hf_raw_text = self.retrieve_mirror_hotfix(hf_key)
        if not hf_raw_text:
            hf_from_mirror = False
            hf_raw_text = self.retrieve_paste_hotfix(hf_key)

        hf_parsed = self.parse_hotfix(hf_raw_text)

        # show the user what this HotFix is and what it will run BEFORE asking them
        # to agree to it -- previously consent was requested first and the two display
        # methods were unimplemented stubs, so the dialog described nothing at all
        self.display_description(hf_parsed['description'])
        self.display_instruction(hf_parsed['instruction'])

        if not self.confirm_instruction(description=hf_parsed['description'],
                                        mirror=hf_from_mirror):
            return

        results, failed_line = self.apply_instruction(hf_parsed['instruction'])

        self.save_temp_hotfix_output(results)

        self.resolution_dispatcher(results, hf_parsed['resolution'])

        self.report_outcome(failed_line)

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
    def retrieve_mirror_hotfix(hf_cypher):
        """
            Takes a five-digit hotfix key and retrieve the hotfix from https://download.osmc.tv/hotfixes/.
            Returns the raw contents at that location as a string.
        """

        url = 'https://download.osmc.tv/hotfixes/'
        log(label='Retrieving hotfixes from', message=url)

        raw_result = requests.get(url)
        raw_text = raw_result.text

        result = re.findall(r'<img.+?>\s*<a href.+?>(.+?)</a>', raw_text)
        hotfixes = []
        for filename in result:
            if filename not in ['Name', 'Parent Directory']:
                hotfixes.append(filename)

        raw_text = ''
        if hf_cypher in hotfixes:
            raw_result = requests.get(url + hf_cypher)
            raw_text = raw_result.text

        log(label='Mirror HotFix Result', message=raw_text)

        return raw_text

    @staticmethod
    def retrieve_paste_hotfix(hf_cypher):
        """
            Takes a five-digit hotfix key and retrieve the hotfix from paste.osmc.io.
            Returns the raw contents at that location as a string.
        """
        url = 'https://paste.osmc.tv/raw/%s' % hf_cypher
        log(label='Retrieving hotfix from', message=url)

        raw_result = requests.get(url)
        raw_text = raw_result.text

        log(label='Pastebin HotFix Result', message=raw_text)
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

                COMMENTS (optional):
                    - any line whose first non-space character is "#" is ignored

                INSTRUCTION:
                    - can be multiple lines
                    - multiple lines will be run consecutively only if the exit code is 0
                    - can start with "INSTRUCTION:" but not required
                    - each line is one command, run directly without a shell: no
                      pipes, redirection, && or globbing

                RESOLUTION (optional):
                    - one line string with resolution actions
                    - several actions may be separated by any of space, comma,
                      full stop, bar, colon or semi-colon, in any combination
                    - must be the last line in the result
                    - and must start with "RESOLUTION:", otherwise it will be treated as part of
                      the INSTRUCTION SET
                    - UPLOAD: upload the results of the INSTRUCTION to paste.osmc.io and provide
                      the user with the link
                    - LOG: write the results to the kodi log
                    - LOG is a mandatory RESOLUTION and is done every time by default
             """

        description = 'No description available'
        instruction = []
        resolution = []


        # rstrip first: a payload saved by any normal editor ends with a newline, so
        # split() produced a trailing empty element and the RESOLUTION line was no
        # longer last. It then fell through to the catch-all below and was executed
        # as a command -- 'RESOLUTION: LOG' became argv ['RESOLUTION:', 'LOG'],
        # raising FileNotFoundError and aborting the run after the real work had
        # already been done, so a successful HotFix reported failure.
        result_list = hf_result.rstrip().split('\n')

        for i, line in enumerate(result_list):
            if i == 0:
                if line.startswith('DESCRIPTION:'):
                    description = line.replace('DESCRIPTION:', '').strip()
                    continue

            # tested wherever it appears rather than only on the last line, so a
            # stray blank line at the end of the file cannot turn it into a command
            if line.startswith('RESOLUTION:'):
                # Split on any run of separators rather than on the first single
                # character that happens to appear. The old loop tried each
                # separator in turn and split on the first one found, which broke
                # every natural way of writing more than one action:
                #   'UPLOAD, LOG' split on the space, giving 'UPLOAD,' -- silently
                #                 ignored, so the output was never uploaded
                #   'UPLOAD | LOG' split on the space, leaving a bare '|'
                #   'UPLOAD;LOG'   did not split at all: the docstring offered a
                #                  semi-colon that was never in the list
                # Only 'UPLOAD LOG' and 'UPLOAD,LOG' worked, and a HotFix author
                # had no way to tell the difference -- an ignored action is not
                # reported anywhere.
                resolution = [r for r in re.split(r'[\s,.|:;]+',
                                                  line.replace('RESOLUTION:', '', 1)) if r]
                continue

            # '#' starts a comment. Without this every line is a command, so a
            # HotFix cannot carry any explanation of itself and a template cannot
            # document its own fields -- '# do X' would be split to ['#', 'do',
            # 'X'] and executed. Nothing legitimate begins a command with '#',
            # so no existing HotFix changes behaviour.
            if line.lstrip().startswith('#'):
                continue

            if line:
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
            Displays a description of the hotfix on-screen for the user.

            Kept separate from the confirmation dialog so the description is logged
            and normalised in one place; confirm_instruction() puts it in front of
            the user as part of the question it asks.
        """
        if not description:
            description = self.lang(32195)

        log(label='Displaying description', message=description)

        return description

    def display_instruction(self, instruction):
        """
            Displays the specific instructions on-screen for the user.

            A HotFix runs arbitrary commands as root, so the user is shown exactly
            what will run before being asked to agree to it. A text viewer is used
            rather than a yes/no dialog because command lines are long and there may
            be several of them.
        """
        if not instruction:
            return

        numbered = '[CR]'.join('%d. %s' % (n, line)
                               for n, line in enumerate(instruction, start=1))

        DIALOG.textviewer(self.lang(32194), numbered)

    def confirm_instruction(self, description=None, mirror=False):
        """
            Asks the user to confirm that they wish to apply the instruction.
            Returns TRUE, only if user clicks Yes.
        """
        strings = [self.lang(32117), self.lang(32118), self.lang(32119)]
        if mirror:
            strings = [self.lang(32117), self.lang(32119)]

        # lead with what this HotFix claims to do, so the question has context
        if description:
            strings = [description, ''] + strings

        user_confirmation = DIALOG.yesno(self.lang(32116), '[CR]'.join(strings))

        return user_confirmation

    @staticmethod
    def apply_instruction(instruction):
        """
            Applies the instruction via the command line.

            Returns (results, failed_line). failed_line is None when every command
            returned zero, otherwise it is the command that stopped the run.

            The caller needs that distinction to tell the user whether the HotFix
            worked. The output alone does not say: a successful run and a run that
            died on its first command both come back as a list of lines, which is
            why nothing was reported before.

            stderr is folded into the captured output. apt writes its warnings and
            most of its errors there, so without this the saved and uploaded output
            omits exactly the part needed to diagnose a failure.
        """
        results = []
        for line in instruction:
            try:
                instruct = shlex.split(line)
                results.append('>>>>> INSTRUCTION >>>>> %s\n' % ' '.join(instruct))

                try:
                    output = subprocess.check_output(instruct, stderr=subprocess.STDOUT)
                    if isinstance(output, bytes):
                        output = output.decode('utf-8')
                    results.append(output)

                except subprocess.CalledProcessError as e:
                    log(label='Non-zero exit code from line', message=e.output)
                    output = e.output
                    if isinstance(output, bytes):
                        output = output.decode('utf-8')
                    results.append(output)
                    return results, line

            except Exception as e:
                results.append('Error: %s\n%s' % (str(e), traceback.format_exc()))
                return results, line

        return results, None

    def report_outcome(self, failed_line=None):
        """
            Tell the user whether the HotFix worked.

            Nothing was shown before: the dialog closed and the user was left to
            guess whether anything had happened, which is the most common complaint
            about the HotFix mechanism. A HotFix is applied by someone sitting in
            front of the device, so the answer belongs on screen rather than only
            in the Kodi log.
        """
        if failed_line is not None:
            log(label='HotFix stopped on line', message=failed_line)

            _ = DIALOG.ok(self.lang(32197),
                          '[CR]'.join([self.lang(32199), failed_line,
                                       self.lang(32200)]))
            return False

        log('HotFix completed: every instruction returned zero')

        _ = DIALOG.ok(self.lang(32196), self.lang(32198))

        self.offer_reboot()

        return True

    def offer_reboot(self):
        """
            A HotFix that needs a restart says so by touching self.reboot_flag,
            rather than calling reboot itself. Two reasons: a HotFix that reboots
            directly kills its own remaining commands and whatever else the device
            was doing, and deciding at run time lets a HotFix ask for a restart
            only when it actually changed something that needs one -- a fixed
            header field could not.

            The user is asked rather than counted down. A HotFix can be applied
            while media is playing, and they are at the screen anyway, having just
            confirmed the run, so a timer buys nothing and can interrupt playback
            or an apt operation that is still settling.

            Declining does not lose the request: it is handed to /tmp/reboot-needed,
            which the updater already watches, so the user is reminded through the
            normal path instead.
        """
        if not os.path.isfile(self.reboot_flag):
            return False

        try:
            os.remove(self.reboot_flag)
        except Exception as e:
            log(label='Could not remove reboot flag', message=str(e))

        reboot = DIALOG.yesno(self.lang(32201),
                              '[CR]'.join([self.lang(32202), self.lang(32080)]),
                              yeslabel=self.lang(32081), nolabel=self.lang(32082))

        if not reboot:
            try:
                with open('/tmp/reboot-needed', 'a'):
                    pass
            except Exception as e:
                log(label='Could not set /tmp/reboot-needed', message=str(e))

            return False

        # close the settings addon first, exactly as the updater does before a
        # reboot, so it is not torn down mid-write
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as open_socket:
                open_socket.connect('/var/tmp/osmc.settings.sockfile')
                open_socket.sendall(b'exit')
        except Exception as e:
            log(label='Could not signal settings addon to exit', message=str(e))

        xbmc.sleep(1000)
        xbmc.executebuiltin('Reboot')

        return True

    def resolution_dispatcher(self, results, resolutions=None):
        """
            Applies the stated resolutions in the file.
            If LOG is not in resolutions, then add it in.
        """
        if resolutions is None:
            resolutions = []

        # SAVE, which copied the output to /boot, is deliberately absent. On a
        # Raspberry Pi /boot is the small FAT partition the device boots from;
        # writing arbitrary command output there risks filling or corrupting it,
        # and the output is already kept in the Kodi log and at
        # /var/tmp/uploadHotFixOutput.txt, both of which grab-logs collects.
        resolution_map = {
            'UPLOAD': self.resolution_upload,
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

            # Previously this offered to copy the output to /boot. It no longer
            # does: an upload failure means no network, and a device with no
            # network could not have fetched the HotFix in the first place, so
            # the case is close to unreachable and not worth writing to the boot
            # partition for. Say where the output already is instead.
            _ = DIALOG.ok(self.lang(32120),
                          '[CR]'.join([self.lang(32121), self.lang(32203)]))

        else:
            url = 'https://paste.osmc.tv/ %s' % key
            log(label="HotFix output uploaded to", message=url.replace(' ', ''))
            _ = DIALOG.ok(self.lang(32120), '[CR]'.join([self.lang(32123), "URL: %s" % url]))

    def save_temp_hotfix_output(self, results):
        """
            Saves the hotfix output to a temporary file
        """
        if isinstance(results, bytes):
            results = results.decode('utf-8')

        with open(self.tmp_hfo_location, 'w', encoding='utf-8') as f:
            f.writelines(results)

    @staticmethod
    def resolution_log(results):
        """
            Write the results to the kodi log file. Debug is not required.
        """
        for line in results:
            log(label='HotFix Output', message=line)


if __name__ == "__main__":
    hotfix = HotFix()
