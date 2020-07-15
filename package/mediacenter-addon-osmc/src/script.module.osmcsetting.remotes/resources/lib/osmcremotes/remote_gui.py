# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.remotes

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import subprocess
import sys
import threading
from io import open

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger

ACTION_PREVIOUS_MENU = 10
ACTION_NAV_BACK = 92
SAVE = 5
HEADING = 1
ACTION_SELECT_ITEM = 7

LIRCD_PATH = '/etc/lirc/lircd.conf'
ETC_LIRC = '/etc/lirc'

if not os.path.isdir(ETC_LIRC):
    LIRCD_PATH = '/home/plaskev/temp/lirc/lircd.conf'
    ETC_LIRC = '/home/plaskev/temp/lirc'

ADDON_ID = "script.module.osmcsetting.remotes"
DIALOG = xbmcgui.Dialog()
PY2 = sys.version_info.major == 2

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


def construct_list_item(conf):
    path, filename = os.path.split(conf)

    # get conf name; check first line in file for "# name:"
    with open(conf, 'r', encoding='utf-8') as open_file:
        lines = open_file.readlines()

    first_line = lines[0]
    if first_line.startswith("# name:"):
        name = first_line[len("# name:"):]
        name2 = filename
    else:
        name = filename.replace('.conf', '')
        name2 = conf

    # check for remote image, use it if it is available
    list_item = xbmcgui.ListItem(label=name, label2=name2, offscreen=True)

    image_path = os.path.join(path, filename.replace('.conf', '.png'))
    if os.path.isfile(image_path):
        list_item.setArt({
            'icon': image_path,
            'thumb': image_path
        })

    list_item.setProperty('fullpath', conf)

    list_item.setInfo('video', {
        'title': ''.join(lines[:100])
    })

    return list_item


def test_custom(conf):
    """
        Returns a boolean indicating whether the supplied conf file is a custom conf file.
    """

    try:
        path, filename = os.path.split(conf)

        if path != ETC_LIRC:
            return True

    finally:
        return False


class RemoteGuiLauncher(object):

    def __init__(self, addon=None):
        self._addon = addon
        self._path = ''

        # flag to idicate whether the GUI should re-open upon close.
        # This is for when the remote changes do not stick.
        self.reopen = True

        # container for any confs we want to ignore
        self.excluded = ['lircd.conf']

        self.active_conf = os.path.realpath(LIRCD_PATH)

        # check if the target file actually exists, if it doesnt, then set the active
        # conf file as None, if it does, then check whether it is a custom file
        if os.path.isfile(self.active_conf):

            custom = test_custom(self.active_conf)

        else:

            custom = False
            self.active_conf = None

        # get the contents of /etc/lirc/
        local_confs_base = os.listdir(ETC_LIRC)
        local_confs_raw = [os.path.join(ETC_LIRC, conf) for conf in local_confs_base]
        local_confs_raw.sort()

        # filter list by files with size (this just removes any empty confs)
        local_confs = []
        for conf in local_confs_raw:
            if os.path.basename(conf) in self.excluded:
                continue
            if not conf.endswith('.conf'):
                continue
            try:
                if os.stat(conf).st_size == 0:
                    continue
            except:
                continue

            local_confs.append(construct_list_item(conf))

        if custom:
            # self.active_conf can only be None if custom is False, so there is no risk in this
            # reconstruction of the local_confs
            local_confs = [construct_list_item(self.active_conf)] + local_confs

        xml = "RemoteBrowser_720OSMC.xml" \
            if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' \
            else "RemoteBrowser_OSMC.xml"

        self.remote_gui = RemoteGui(xml, self.path, 'Default', local_confs=local_confs,
                                    active_conf=self.active_conf, addon=self.addon)

    @property
    def addon(self):
        if not self._addon:
            self._addon = xbmcaddon.Addon(ADDON_ID)
        return self._addon

    @property
    def path(self):
        if not self._path:
            self._path = self.addon.getAddonInfo('path')
        return self._path

    def open_gui(self):

        while self.reopen:
            self.reopen = False

            self.remote_gui.doModal()


class RemoteGui(xbmcgui.WindowXMLDialog):

    def __init__(self, strXMLname, strFallbackPath, strDefaultName,
                 local_confs, active_conf, addon=None):
        super(RemoteGui, self).__init__(xmlFilename=strXMLname,
                                        scriptPath=strFallbackPath, defaultSkin=strDefaultName)
        self._addon = addon
        self._lang = None
        self._path = None

        self.control_list = None
        self.remote_test = None

        self.local_confs = local_confs
        self.active_conf = active_conf
        self.rc6_file = '/etc/modprobe.d/blacklist-rc6.conf'
        self.rc6_file_loc = '/etc/modprobe.d'

        self.remote_selection = None

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

    @property
    def path(self):
        if not self._path:
            self._path = self.addon.getAddonInfo('path')
        return self._path

    def onInit(self):
        self.control_list = self.getControl(500)
        self.control_list.setVisible(True)

        for index, conf in enumerate(self.local_confs):
            self.control_list.addItem(conf)

        self.highlight_selected()

        try:
            self.getControl(50).setVisible(False)
        except:
            pass

        # check for RC6 file, then set the radio button appropriately
        if os.path.isfile(self.rc6_file):
            log('RC6 blacklist file located')
            self.getControl(8).setSelected(True)
        else:
            log('RC6 blacklist file not found')
            self.getControl(8).setSelected(False)

    def find_custom_item(self):
        log('Finding custom item in list')

        for idx in range(0, self.control_list.size()):
            tmp = self.control_list.getListItem(idx)
            tmp_path = tmp.getLabel2()
            if test_custom(tmp_path):
                log('Custom item found')
                return idx, tmp

        return 0, 'failed'

    def highlight_selected(self):
        log('Changing highlighting to %s' % self.active_conf)

        for i in range(0, self.control_list.size()):
            tmp = self.control_list.getListItem(i)

            tmp_path = tmp.getLabel2()

            # if self.active_conf is None (i.e. the user deleted it externally) then
            # no item will be selected
            if self.active_conf == tmp_path:
                tmp.select(True)
            else:
                tmp.select(False)

    def rc6_handling(self):
        sel = self.getControl(8).isSelected()

        if sel:
            # always overwrite the file, this will allow the contents to be updated (if ever needed)
            log('Creating RC6 blacklist file')
            blacklist_payload = 'blacklist ir_rc6_decoder\ninstall ir_rc6_decoder /bin/true'
            if PY2:
                blacklist_payload = blacklist_payload.decode('utf-8')
            with open('/var/tmp/blacklist-rc6.conf', 'w', encoding='utf-8') as f:
                f.write(blacklist_payload)

            subprocess.call(["sudo", "mv", '/var/tmp/blacklist-rc6.conf', self.rc6_file_loc])

            log('RC6 blacklist file moved')

        else:

            log('RC6 blacklist file removed')

            subprocess.call(["sudo", "rm", "-f", self.rc6_file])

    def onClick(self, controlID):
        if controlID == 500:
            # user has selected a local file from /etc/lirc

            self.remote_selection = self.getControl(500).getSelectedItem().getProperty('fullpath')
            result = self.test_selection()

            if result == 'success':

                log('User confirmed the remote changes work')

                # change the highlighted remote to the new selection
                self.active_conf = self.remote_selection

            elif result == 'service_dead':
                log('Remote service failed to restart.')

                _ = DIALOG.ok(self.lang(32006), self.lang(32013))
                self.remote_selection = None

            else:
                log('User did not confirm remote changes')
                self.remote_selection = None

            self.highlight_selected()

        elif controlID == 7:
            # user has selected Exit
            self.rc6_handling()

            self.remote_selection = None

            self.close()

        elif controlID == 62:
            # user has chosen to browse for the file
            log('User is browsing for remote conf')

            browser = xbmcgui.Dialog().browse(1, self.lang(32005), 'files', mask='.conf')
            if not browser:
                self.remote_selection = None
                return

            log('User selected remote conf: %s' % self.remote_selection)
            self.remote_selection = browser

            result = self.test_selection()
            if result == 'success':
                log('user confirmed the remote changes work')

                # change the highlighted remote to the new selection
                self.active_conf = self.remote_selection

                # see if there is a custom file in the list, delete it if there is
                idx, custom = self.find_custom_item()
                if custom:
                    self.control_list.removeItem(idx)

                # add the new custom as an item
                # self.active_conf cannot be None at this point, as the user must have selected one
                list_item = construct_list_item(self.active_conf)
                self.control_list.addItem(list_item)

                self.highlight_selected()

            elif result == 'service_dead':
                log('Remote service failed to restart.')

                _ = DIALOG.ok(self.lang(32006), self.lang(32013))
                self.remote_selection = None

            else:
                self.remote_selection = None

    def test_selection(self):
        log('Testing remote conf selection: %s' % self.remote_selection)

        if os.path.isfile(self.remote_selection):
            # read the symlink target
            original_target = os.readlink(LIRCD_PATH)

            log('Original lircd_path target: %s' % original_target)

            # symlink the master conf to the new selection
            subprocess.call(['sudo', 'ln', '-sf', self.remote_selection, LIRCD_PATH])

            # open test dialog
            xml = "OSMC_remote_testing720.xml" \
                if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' \
                else "OSMC_remote_testing.xml"

            self.remote_test = RemoteTest(xml, self.path, 'Default', self.remote_selection)
            self.remote_test.doModal()

            log('Testing complete, result: %s' % self.remote_test.test_successful)

            # if the test wasn't successful, then revert to the previous conf
            if not self.remote_test.test_successful:

                subprocess.call(['sudo', 'ln', '-sf', original_target, LIRCD_PATH])

                subprocess.call(['sudo', 'systemctl', 'restart', 'lircd_helper@*'])

                # add busy dialog, loop until service restarts

                if not self.remote_test.service_running:
                    return 'service_dead'

                return 'failed'

            return 'success'

        return 'failed'


class RemoteTest(xbmcgui.WindowXMLDialog):
    """
        control ids

        90		restarting service label

        91		service restarted label, informs the user that the service has restarted and
                to confirm using the test button

        25		test button, user clicks this to confirm that the remotes changes have
                been successful

        45		countdown label, is controlled by the timer, and counts down the seconds to revert

        55 		quick revert button
    """

    def __init__(self, strXMLname, strFallbackPath, strDefaultName, selection):
        super(RemoteTest, self).__init__(xmlFilename=strXMLname,
                                         scriptPath=strFallbackPath, defaultSkin=strDefaultName)

        self.test_successful = False
        self.service_running = True

        self.selection = selection

        self.countdown_limit = 20
        self.quick_revert = False

        self.countdown_timer = CountDownTimer(self)
        self.countdown_timer.setDaemon(True)

        self.restarting_service_label = None
        self.check_remote_label = None
        self.test_button = None
        self.progress_bar = None

        # setup the service checker straight away
        self.service_checker = ServiceChecker(self)
        self.service_checker.setDaemon(True)

    def onInit(self):

        log('Opening test dialog')

        self.restarting_service_label = self.getControl(90)
        self.check_remote_label = self.getControl(91)

        self.test_button = self.getControl(25)
        self.progress_bar = self.getControl(101)

        self.initial_state()

        # start the service_checker AFTER the class attributes have been set
        # (prevents race condition)
        self.service_checker.start()

    def initial_state(self):
        """ the dialog is telling the user that the service if restarting, and to please wait """
        log('Setting initial state of test dialog')

        # change label to say remote service restarting
        self.progress_bar.setVisible(False)
        self.restarting_service_label.setVisible(True)
        self.check_remote_label.setVisible(False)
        self.test_button.setVisible(False)

    def second_state(self):
        """
            the service has been confirmed to be running again, and now the dialog is telling the
            user to click on the Confirm button. This will confirm that they have been able to
            navigate down to the button, and click on it.
        """
        log('Setting second state of test dialog')

        # change the label to say that the remote service has restarted and does the user want
        # to keep the changes
        self.restarting_service_label.setVisible(False)
        self.check_remote_label.setVisible(True)
        self.progress_bar.setVisible(True)
        self.test_button.setVisible(True)

        # start the timer
        self.countdown_timer.start()

    def service_dead_state(self):
        """
            the service has not been detected to have started within 20 seconds.
            inform the user with OK style dialog
        """
        log('Service is dead')

        self.service_running = False
        self.close()

    def onClick(self, controlID):
        if controlID == 25:
            """ 
                user has clicked the test successful button, keep the changes,
                this is the only place that the new conf can be confirmed
            """

            log('User has confirmed that the new conf is working.')

            self.test_successful = True
            self.countdown_timer.exit = True
            try:
                self.service_checker.exit = True
            except:
                pass
            self.close()

        elif controlID == 55:
            """ 
                The user has decided to end the test, and would like to revert to the previous 
                conf. This is likely to only occur while the service is being checked. 
            """

            log('User has decided to revert to the previous conf.')
            try:
                self.service_checker.exit = True
            except:
                pass
            self.countdown_timer.exit = True
            self.close()


class ServiceChecker(threading.Thread):
    """
        Restarts the remote service, and waits for the response that it is running.
    """

    def __init__(self, parent):
        super(ServiceChecker, self).__init__(name='service_checker')
        self.parent = parent
        self.exit = False

    def run(self):
        log('Remote service checker thread active.')

        counter = 0

        # restart the service for the changes to take effect
        process = subprocess.Popen(['sudo', 'systemctl', 'restart', 'lircd_helper@*'])

        # loop until the service has restarted
        # (or too much time has elapsed, in which case fail out)
        while counter < 40 and not self.exit:

            poll_response = process.poll()

            if poll_response is None:
                counter += 1
                xbmc.sleep(250)
                continue

            elif poll_response == 0:
                break

            else:
                # if the process times out or the exit signal is recieved, then return nothing
                # on a timeout however, enter something in the log

                if counter >= 40:
                    # this is reached if the counter reaches 40, meaning the process check timed out
                    self.parent.service_dead_state()

                elif self.exit:
                    # this occurs when the user has clicked cancel or back
                    # there is no need to do anything
                    pass

                elif poll_response != 0:
                    # this occurs if there is an error code returned by the process
                    log('Error code from systemctl restart lircd-helper: %s' % poll_response)

                return

        # this point it only reached if process.poll returns 0
        self.parent.second_state()


class CountDownTimer(threading.Thread):

    def __init__(self, parent):
        super(CountDownTimer, self).__init__(name='countdown_timer')
        self.parent = parent
        self.exit = False
        self.countdown = self.parent.countdown_limit

    def run(self):
        """
            Update the label on the dialog to show how many seconds until the conf
            reverts to the previous state
        """
        log('Countdown timer thread active')

        while not self.exit and self.countdown:
            self.parent.progress_bar.setWidth(self.countdown * 60)

            xbmc.sleep(1000)

            self.countdown -= 1

        self.parent.close()
