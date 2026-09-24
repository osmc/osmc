# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2026 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.updates

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.

    Kodi-free half of the HotFix mechanism: retrieval, parsing, execution and
    output handling, with no import of xbmc, xbmcgui or xbmcaddon anywhere in
    this module.

    That constraint is the whole point. osmc_hotfix.py cannot run outside Kodi
    because 'import xbmcgui' and 'DIALOG = xbmcgui.Dialog()' execute at module
    level, so there has never been a way to apply a HotFix from a terminal.
    Splitting the parts that do not need a GUI lets /usr/bin/osmc-hotfix reuse
    them rather than growing a second implementation that drifts from this one.

    Deliberately absent: any method that both fetches and runs. A front end has
    to call retrieve(), then parse(), then apply(), which means it cannot
    accidentally skip showing the user what it is about to run. The commands go
    on to execute as root, so that ordering is a safety property, not a style
    preference.
"""

import os
import re
import shlex
import subprocess
import traceback
from io import open

import requests


class HotFixCore(object):
    """
        Retrieve, parse and apply a HotFix. UI-agnostic.
    """

    MIRROR_INDEX = 'https://download.osmc.tv/hotfixes/'
    PASTE_RAW = 'https://paste.osmc.tv/raw/%s'
    PASTE_POST = 'https://paste.osmc.tv/documents'

    OUTPUT_FILE = '/var/tmp/uploadHotFixOutput.txt'

    # A HotFix that needs a restart touches REBOOT_FLAG rather than rebooting
    # the device itself, which would kill its own remaining commands and the
    # reporting that follows them. REBOOT_HANDOFF is the updater's existing
    # flag: if the user declines the restart, the request is passed there so
    # they are reminded through the normal path instead of it being lost.
    REBOOT_FLAG = '/tmp/reboot_needed_hotfix'
    REBOOT_HANDOFF = '/tmp/reboot-needed'

    SOURCE_MIRROR = 'mirror'
    SOURCE_PASTE = 'paste'

    TIMEOUT = 30

    def __init__(self, log=None):
        """
            log is any callable taking (message, label=''), which is the shape of
            osmccommon's StandardLogger.log. It is injected rather than imported
            so this module has no dependency on the add-on: /usr/bin/osmc-hotfix
            ships in base-files-osmc and passes its own.
        """
        self.log = log if log is not None else (lambda message, label='': None)

    # ------------------------------------------------------------------ fetch

    def retrieve(self, key):
        """
            Fetch a HotFix by key, mirror first.

            Returns (raw_text, source, url). source is SOURCE_MIRROR or
            SOURCE_PASTE and matters to the caller: the mirror is OSMC's own
            server, while anyone can post to paste.osmc.tv unauthenticated. A
            front end is expected to say which one it got, because the content
            is about to be run as root.

            raw_text is '' when neither source has it.
        """
        raw_text = self.retrieve_mirror(key)
        if raw_text:
            return raw_text, self.SOURCE_MIRROR, self.MIRROR_INDEX + key

        url = self.PASTE_RAW % key
        return self.retrieve_paste(key), self.SOURCE_PASTE, url

    def retrieve_mirror(self, key):
        """
            Retrieve from https://download.osmc.tv/hotfixes/.

            The key is checked against the directory listing before being
            fetched, so a key that is not published cannot be turned into an
            arbitrary URL under that path.
        """
        self.log(label='Retrieving hotfixes from', message=self.MIRROR_INDEX)

        try:
            response = requests.get(self.MIRROR_INDEX, timeout=self.TIMEOUT)
            response.raise_for_status()
            listing = response.text
        except Exception as e:
            self.log(label='Could not reach the HotFix mirror', message=str(e))
            return ''

        found = re.findall(r'<img.+?>\s*<a href.+?>(.+?)</a>', listing)
        hotfixes = [name for name in found
                    if name not in ['Name', 'Parent Directory']]

        if key not in hotfixes:
            self.log(label='Not published on the mirror', message=key)
            return ''

        try:
            response = requests.get(self.MIRROR_INDEX + key, timeout=self.TIMEOUT)
            response.raise_for_status()
            raw_text = response.text
        except Exception as e:
            self.log(label='Could not fetch from the mirror', message=str(e))
            return ''

        self.log(label='Mirror HotFix Result', message=raw_text)

        return raw_text

    def retrieve_paste(self, key):
        """
            Retrieve from paste.osmc.tv. Anyone can post there without
            authenticating, so what comes back is untrusted input that the user
            must be shown before it is run.
        """
        url = self.PASTE_RAW % key
        self.log(label='Retrieving hotfix from', message=url)

        # The status code has to be checked. paste.osmc.tv answers an unknown
        # key with HTTP 404 and a JSON body, and taking .text regardless turned
        # '{"message":"Document not found."}' into the HotFix content -- so a
        # mistyped key presented that line to the user as a command to run.
        try:
            response = requests.get(url, timeout=self.TIMEOUT)
            response.raise_for_status()
            raw_text = response.text
        except Exception as e:
            self.log(label='Could not fetch from paste.osmc.tv', message=str(e))
            return ''

        self.log(label='Pastebin HotFix Result', message=raw_text)

        return raw_text

    # ------------------------------------------------------------------ parse

    @staticmethod
    def parse(hf_result):
        """
            Parse a HotFix into its description, instructions and resolutions.

                DESCRIPTION (optional)
                    one line, must be the first line

                # comment (optional)
                    any line whose first non-space character is '#'

                INSTRUCTION
                    one command per line, run in order, stopping at the first
                    that returns non-zero. An 'INSTRUCTION:' prefix is accepted
                    and stripped. Blank lines are ignored.

                RESOLUTION (optional)
                    last line. LOG, UPLOAD, or several separated by any of
                    space, comma, full stop, bar, colon or semi-colon.
        """
        description = 'No description available'
        instruction = []
        resolution = []

        # rstrip first: a payload saved by any normal editor ends with a
        # newline, so split() produced a trailing empty element and the
        # RESOLUTION line was no longer last. It then fell through to the
        # catch-all below and was executed as a command.
        result_list = hf_result.rstrip().split('\n')

        for i, line in enumerate(result_list):
            if i == 0 and line.startswith('DESCRIPTION:'):
                description = line.replace('DESCRIPTION:', '', 1).strip()
                continue

            # tested wherever it appears rather than only on the last line, so a
            # stray blank line at the end cannot turn it into a command
            if line.startswith('RESOLUTION:'):
                # Split on any run of separators rather than on the first single
                # character present. The old loop tried each separator in turn,
                # so 'UPLOAD, LOG' split on the space into 'UPLOAD,' and 'LOG'
                # and the malformed half was discarded silently.
                resolution = [r for r in
                              re.split(r'[\s,.|:;]+',
                                       line.replace('RESOLUTION:', '', 1)) if r]
                continue

            # '#' starts a comment. Without this every line is a command, so a
            # HotFix cannot explain itself -- '# do X' would be split to
            # ['#', 'do', 'X'] and executed.
            if line.lstrip().startswith('#'):
                continue

            if line:
                instruction.append(line.replace('INSTRUCTION:', '', 1).strip())

        return {
            'description': description,
            'instruction': instruction,
            'resolution': resolution,
        }

    # ---------------------------------------------------------------- execute

    def apply(self, instruction, dry_run=False):
        """
            Run each command in turn, stopping at the first non-zero exit.

            Returns (results, failed_line). failed_line is None when every
            command returned zero, otherwise it is the command that stopped the
            run. Callers need that distinction to tell the user whether the
            HotFix worked: a successful run and one that died on its first
            command otherwise come back as the same list of output lines.

            stderr is folded into the captured output. apt writes its warnings
            and most of its errors there, so without this the saved and uploaded
            output omits exactly the part needed to diagnose a failure.

            dry_run prints what would run and executes nothing. There is no
            corresponding 'assume yes': a front end must still show the user the
            commands and get agreement, because these run as root.
        """
        results = []

        for line in instruction:
            try:
                instruct = shlex.split(line)
                results.append('>>>>> INSTRUCTION >>>>> %s\n' % ' '.join(instruct))

                if dry_run:
                    results.append('(dry run: not executed)\n')
                    continue

                try:
                    output = subprocess.check_output(instruct,
                                                     stderr=subprocess.STDOUT)
                    if isinstance(output, bytes):
                        output = output.decode('utf-8', 'replace')
                    results.append(output)

                except subprocess.CalledProcessError as e:
                    self.log(label='Non-zero exit code from line', message=line)
                    output = e.output
                    if isinstance(output, bytes):
                        output = output.decode('utf-8', 'replace')
                    results.append(output)
                    return results, line

            except Exception as e:
                results.append('Error: %s\n%s' % (str(e), traceback.format_exc()))
                return results, line

        return results, None

    # ----------------------------------------------------------------- output

    def save_output(self, results):
        """
            Write the collected output to OUTPUT_FILE. Returns the path, or None
            if it could not be written.
        """
        if isinstance(results, bytes):
            results = results.decode('utf-8', 'replace')

        try:
            with open(self.OUTPUT_FILE, 'w', encoding='utf-8') as f:
                f.writelines(results)
        except Exception as e:
            self.log(label='Could not save HotFix output', message=str(e))
            return None

        return self.OUTPUT_FILE

    def upload_output(self):
        """
            Post the saved output to paste.osmc.tv. Returns the URL, or None on
            failure.
        """
        try:
            with open(self.OUTPUT_FILE, 'r', encoding='utf-8') as f:
                body = f.read()
        except Exception as e:
            self.log(label='No HotFix output to upload', message=str(e))
            return None

        try:
            response = requests.post(self.PASTE_POST, data=body.encode('utf-8'),
                                     timeout=self.TIMEOUT)
            key = response.json().get('key')
        except Exception as e:
            self.log(label='HotFix upload failed', message=str(e))
            return None

        if not key:
            self.log('OSMC HotFix upload failed.')
            return None

        url = 'https://paste.osmc.tv/%s' % key
        self.log(label='HotFix output uploaded to', message=url)

        return url

    # ----------------------------------------------------------------- reboot

    def reboot_requested(self):
        """
            Whether the HotFix asked for a restart, by touching REBOOT_FLAG.
        """
        return os.path.isfile(self.REBOOT_FLAG)

    def clear_reboot_request(self):
        """
            Remove the flag, whether or not the user agrees to restart, so a
            later unrelated HotFix does not inherit the request.
        """
        try:
            os.remove(self.REBOOT_FLAG)
        except Exception as e:
            self.log(label='Could not remove reboot flag', message=str(e))

    def defer_reboot(self):
        """
            Hand the restart request to the updater's own flag, so declining now
            does not lose it: the user is reminded through the normal path.
        """
        try:
            with open(self.REBOOT_HANDOFF, 'a'):
                pass
        except Exception as e:
            self.log(label='Could not set %s' % self.REBOOT_HANDOFF, message=str(e))
