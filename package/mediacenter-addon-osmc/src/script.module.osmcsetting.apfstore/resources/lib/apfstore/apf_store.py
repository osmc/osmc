# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmcsetting.apfstore

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import datetime
import json
import os
import shutil
import subprocess
import threading
import traceback
from io import open

import requests

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger
from osmccommon.osmc_logging import clog

from .apf_class import APFListItem
from .apf_gui import APFGui

try:
    import queue as Queue
except ImportError:
    import Queue

"""
=========================
APF JSON STRUCTURE
=========================

{
   "application": [
       {
           "id": "ssh-app-osmc",
           "name": "SSH Server",
           "shortdesc": "This allows you to connect to your OSMC device via SSH",
           "longdesc": "This installs an OpenSSH server on your OSMC device allowing you to log in to your device remotely as well as transfer files via SCP.",
           "maintained-by": "OSMC",
           "version": "1.0.0",
           "lastupdated": "2015-01-23",
           "iconurl": "http://blah",
           "iconhash": 0,
       }
   ]
}
"""

ADDON_ID = "script.module.osmcsetting.apfstore"
ADDON_DATA = xbmc.translatePath('special://userdata/addon_data/%s/' % ADDON_ID)

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class APFStore(object):
    CACHE_TIMEOUT = 6
    TIME_STRING_PATTERN = '%Y/%m/%d %H:%M:%S'

    def __init__(self, addon=None):

        self._addon = addon

        # do not proceed if the version is alpha
        if self.check_for_unsupported_version() == 'alpha':
            return

        self._lang = None
        self.use_cache = False
        self.json_last_updated_record = 'never'
        self.json_cache = 'empty'
        self.package_list = ''
        self.url = ''

        self.touch_addon_data_folder()

        self.install_status_cache = {
            x.split('=')[0]: x.split('=')[1]
            for x in self.addon.getSetting('install_status_cache').split(':_:') if '=' in x
        }

        json_data = self.get_apf_data()

        if json_data == 'failed':
            log('Failed to retrieve JSON apf data')

            return

        self.apf_dict = self.generate_apf_dict(json_data)

        self.apf_gui = self.create_apf_store_gui(self.apf_dict)

        self.retrieve_install_status()

        self.retrieve_icons()

        self.apf_gui.doModal()

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

    def check_for_unsupported_version(self):
        """ Checks if this version is an Alpha, prevent updates """

        with open(os.devnull, 'w') as fnull:
            process = subprocess.call(['/usr/bin/dpkg-query', '-l', 'rbp-mediacenter-osmc'],
                                      stderr=fnull, stdout=fnull)

        if process == 0:
            _ = xbmcgui.Dialog().ok(self.lang(32017),
                                    '[CR]'.join([self.lang(32018), self.lang(32019)]))
            return 'alpha'

        return 'proceed'

    def get_apf_data(self):
        self.use_cache = self.check_last_updated()

        if self.use_cache is True:

            cache = self.read_json_cache()

            if cache == 'failed':
                return self.get_remote_json()

            return cache

        return self.get_remote_json()

    def get_remote_json(self):

        json_req = self.get_list_from_sam()

        if json_req == 'failed':
            log('Failed to retrieve osmcdev= from /proc/cmdline')
            return 'failed'

        elif not json_req:
            log('Failed to retrieve data from %s' % self.url)
            return 'failed'

        return json_req

    def read_json_cache(self):

        self.json_cache = self.addon.getSetting('json_cache')

        if self.json_cache == 'empty':
            return 'failed'

        try:
            return json.loads(self.json_cache)
        except:
            return 'failed'

    def check_last_updated(self):
        current_time = self.get_current_time()

        if current_time == 'failed':
            return False

        self.json_last_updated_record = self.addon.getSetting('json_lastupdated')

        if self.json_last_updated_record == 'never':
            return False

        try:
            date_object = datetime.strptime(self.json_last_updated_record, self.TIME_STRING_PATTERN)
        except:
            return False

        trigger = date_object + datetime.timedelta(hours=self.CACHE_TIMEOUT)

        if trigger > current_time:
            log('JSON Cache is fresh')
            return True

        return False

    @clog(logger=log, maxlength=10000)
    def generate_apf_dict(self, json_req):
        apf_list = json_req.get('application', [])
        for item in apf_list:
            item.update({
                'addon': self.addon
            })
        obj_list = [APFListItem() for x in apf_list if x['id']]

        return {x['id']: obj_list[i - 1].populate(x) for i, x in enumerate(apf_list) if x['id']}

    @staticmethod
    def get_current_time():
        # this method is necessary as datetime.now() has issues with the GIL
        # and throws an error at random
        for x in range(50):

            try:
                return datetime.datetime.now()
            except:
                pass

        else:
            log('retrieving current time failed')
            return 'failed'

    @clog(logger=log)
    def get_list_from_sam(self):

        try:

            # generate the URL
            with open('/proc/cmdline', 'r', encoding='utf-8') as f:
                lines = f.readlines()

            for line in lines:
                settings = line.split(' ')

                for setting in settings:

                    if setting.startswith('osmcdev='):
                        self.url = 'http://download.osmc.tv/apps/%s' % setting[len('osmcdev='):]
                        break

                else:
                    # this is for testing only
                    self.url = 'http://download.osmc.tv/apps/rbp2'

        except:
            self.url = 'http://download.osmc.tv/apps/rbp2'

        log('APF data URL: %s' % self.url)

        try:
            response = requests.get(self.url.replace('\n', '').replace('\t', '').replace('\n', ''))
        except:
            log('Connection to %s failed' % self.url)

            return 'failed'

        try:
            json_payload = response.json()
            self.addon.setSetting('json_cache', json.dumps(json_payload))

            current_time = self.get_current_time()

            if current_time != 'failed':
                self.addon.setSetting('json_lastupdated',
                                      current_time.strftime(self.TIME_STRING_PATTERN))

            return json_payload

        except:
            log('JSON couldn\'t be read: %s' % response.text)
            return 'failed'

    @clog(logger=log)
    def retrieve_icons(self):

        thread_queue = Queue.Queue()

        for ident, apf in self.apf_dict.items():

            if apf.retrieve_icon:
                thread_queue.put(apf)

        t = threading.Thread(target=self.grab_icon_from_sam, args=(thread_queue,))
        t.daemon = True
        t.start()

    @clog(logger=log)
    def grab_icon_from_sam(self, thread_queue):

        while True:

            try:
                # grabs the item from the queue
                # the get BLOCKS and waits 1 second before throwing a Queue Empty error
                q_item = thread_queue.get(True, 1)

                thread_queue.task_done()

                # download the icon and save it in ADDON_DATA
                response = requests.get(q_item.iconurl, stream=True)

                icon_name = q_item.iconurl.split('/')[-1]

                with open(os.path.join(ADDON_DATA, icon_name), 'wb') as out_file:
                    shutil.copyfileobj(response.raw, out_file)

                del response

                q_item.refresh_icon()

            except Queue.Empty:
                log('Queue.Empty error')
                break

    @clog(logger=log)
    def retrieve_install_status(self):

        with os.popen('dpkg -l') as f:
            self.package_list = ''.join(f.readlines())

        thread_queue = Queue.Queue()

        for ident, apf in self.apf_dict.items():
            thread_queue.put(apf)

        t = threading.Thread(target=self.grab_install_status, args=(thread_queue,))
        t.daemon = True

        t.start()

    @clog(logger=log)
    def grab_install_status(self, thread_queue):

        while True:

            try:
                # grabs the item from the queue
                # the get BLOCKS and waits 1 second before throwing a Queue Empty error
                apf = thread_queue.get(True, 1)

                install_query = ['dpkg-query', '-W', '-f="${Status}"', apf.id]

                with open(os.devnull, 'w') as fnull:
                    try:
                        output = subprocess.check_output(install_query, stderr=fnull)
                    except subprocess.CalledProcessError as e:
                        output = e.output

                if isinstance(output, bytes):
                    output = output.decode('utf-8')

                if "ok installed" in output:
                    log('%s IS Installed' % apf.name)
                    apf.set_installed(True)

                else:
                    log('%s is NOT Installed' % apf.name)

                thread_queue.task_done()

            except Queue.Empty:

                log('Queue.Empty error')
                break

            except Exception:
                log(traceback.format_exc())
                break

    @clog(logger=log)
    def touch_addon_data_folder(self):
        if not os.path.isdir(ADDON_DATA):
            os.makedirs(ADDON_DATA)

        return ADDON_DATA

    @clog(logger=log)
    def create_apf_store_gui(self, apf_dict):
        xml = "APFBrowser_720OSMC.xml" \
            if xbmcgui.Window(10000).getProperty("SkinHeight") == '720' \
            else "APFBrowser_OSMC.xml"

        return APFGui(xml, self.addon.getAddonInfo('path'), 'Default',
                      apf_dict=apf_dict, addon=self.addon)
