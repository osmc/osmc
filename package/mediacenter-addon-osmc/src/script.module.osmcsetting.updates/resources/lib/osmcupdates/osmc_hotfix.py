# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2026 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.updates

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.

    Kodi front end for the HotFix mechanism. Everything that does not need a GUI
    lives in osmc_hotfix_core, which /usr/bin/osmc-hotfix shares, so the two
    front ends cannot drift in what they fetch, parse or run.
"""

import os
import socket

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger

try:
    from .osmc_hotfix_core import HotFixCore
except ImportError:  # run directly by RunScript, so not inside the package
    from osmc_hotfix_core import HotFixCore

ADDON_ID = 'script.module.osmcsetting.updates'
DIALOG = xbmcgui.Dialog()

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class HotFix(object):

    def __init__(self, addon=None):
        self._addon = addon
        self._lang = None

        self.core = HotFixCore(log=log)

        hf_key = self.user_enters_key()

        hf_raw_text, source, url = self.core.retrieve(hf_key)

        hf_parsed = self.core.parse(hf_raw_text)

        # show the user what this HotFix is and what it will run BEFORE asking
        # them to agree to it -- previously consent was requested first and the
        # two display methods were unimplemented stubs, so the dialog described
        # nothing at all
        self.display_description(hf_parsed['description'])
        self.display_instruction(hf_parsed['instruction'])

        if not self.confirm_instruction(description=hf_parsed['description'],
                                        mirror=source == HotFixCore.SOURCE_MIRROR):
            return

        results, failed_line = self.core.apply(hf_parsed['instruction'])

        self.core.save_output(results)

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
            Provides a dialog through which the user enters a key code
        """
        hf_key = DIALOG.input(self.lang(32115), '', type=xbmcgui.INPUT_ALPHANUM)
        log(label='User entered hotfix ID', message=hf_key)

        return hf_key

    def display_description(self, description):
        """
            Displays a description of the hotfix on-screen for the user.

            Kept separate from the confirmation dialog so the description is
            logged and normalised in one place; confirm_instruction() puts it in
            front of the user as part of the question it asks.
        """
        if not description:
            description = self.lang(32195)

        log(label='Displaying description', message=description)

        return description

    def display_instruction(self, instruction):
        """
            Displays the specific instructions on-screen for the user.

            A HotFix runs arbitrary commands as root, so the user is shown
            exactly what will run before being asked to agree to it. A text
            viewer is used rather than a yes/no dialog because command lines are
            long and there may be several of them.
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

            A HotFix that did not come from OSMC's own mirror gets an extra
            warning: anyone can post to paste.osmc.tv without authenticating, so
            the provenance is worth saying out loud.
        """
        strings = [self.lang(32117), self.lang(32118), self.lang(32119)]
        if mirror:
            strings = [self.lang(32117), self.lang(32119)]

        # lead with what this HotFix claims to do, so the question has context
        if description:
            strings = [description, ''] + strings

        user_confirmation = DIALOG.yesno(self.lang(32116), '[CR]'.join(strings))

        return user_confirmation

    def report_outcome(self, failed_line=None):
        """
            Tell the user whether the HotFix worked.

            Nothing was shown before: the dialog closed and the user was left to
            guess whether anything had happened, which is the most common
            complaint about the HotFix mechanism. A HotFix is applied by someone
            sitting in front of the device, so the answer belongs on screen
            rather than only in the Kodi log.
        """
        if failed_line is not None:
            log(label='HotFix stopped on line', message=failed_line)

            _ = DIALOG.ok(self.lang(32197),
                          '[CR]'.join([self.lang(32199), failed_line,
                                       self.lang(32200), '', self.lang(32204)]))
            return False

        log('HotFix completed: every instruction returned zero')

        _ = DIALOG.ok(self.lang(32196), self.lang(32198))

        self.offer_reboot()

        return True

    def offer_reboot(self):
        """
            A HotFix that needs a restart says so by touching the core's reboot
            flag, rather than calling reboot itself. Two reasons: a HotFix that
            reboots directly kills its own remaining commands and the reporting
            that follows them, and deciding at run time lets a HotFix ask for a
            restart only when it actually changed something that needs one -- a
            fixed header field could not.

            The user is asked rather than counted down. These devices run
            recordings and are used by people other than whoever applied the
            HotFix, so a timer that fires into an empty room is worse than a
            prompt that waits.
        """
        if not self.core.reboot_requested():
            return False

        self.core.clear_reboot_request()

        reboot = DIALOG.yesno(self.lang(32201),
                              '[CR]'.join([self.lang(32202), self.lang(32080)]),
                              yeslabel=self.lang(32081), nolabel=self.lang(32082))

        if not reboot:
            self.core.defer_reboot()
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
        # writing arbitrary command output there risks filling or corrupting it.
        # Nothing is lost by not writing there: LOG is always applied, and it
        # puts the output in the Kodi log, which grab-logs does collect. The
        # core's own output file is not collected by grab-logs -- it is there so
        # the user still has the output locally, including after a restart the
        # HotFix itself asked for.
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
            Method that is run when the dispatcher receives a resolution it
            doesnt understand
        """
        log('Unknown request received by dispatcher.')

    def resolution_upload(self, results):
        """
            Uploads the collected output to paste.osmc.tv and gives the user the
            URL, which is what they should quote on the forum.
        """
        url = self.core.upload_output()

        if not url:
            # Previously this offered to copy the output to /boot. It no longer
            # does: an upload failure means no network, and a device with no
            # network could not have fetched the HotFix in the first place, so
            # the case is close to unreachable and not worth writing to the boot
            # partition for. Say where the output already is instead.
            _ = DIALOG.ok(self.lang(32120),
                          '[CR]'.join([self.lang(32121), self.lang(32203)]))
            return

        _ = DIALOG.ok(self.lang(32120),
                      '[CR]'.join([self.lang(32123), "URL: %s" % url]))

    @staticmethod
    def resolution_log(results):
        """
            Write the results to the kodi log file. Debug is not required.
        """
        for line in results:
            log(label='HotFix Output', message=line)


if __name__ == "__main__":
    hotfix = HotFix()
