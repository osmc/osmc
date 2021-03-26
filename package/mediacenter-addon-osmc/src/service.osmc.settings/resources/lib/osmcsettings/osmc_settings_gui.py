# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of service.osmc.settings

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.
"""

import os
import sys
import threading
import traceback
from io import open
from itertools import cycle

import xbmc
import xbmcaddon
import xbmcgui
from osmccommon.osmc_language import LangRetriever
from osmccommon.osmc_logging import StandardLogger
from osmccommon.osmc_logging import clog

ADDON_ID = 'service.osmc.settings'

log = StandardLogger(ADDON_ID, os.path.basename(__file__)).log


class OSMCGui(xbmcgui.WindowXMLDialog):

    def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):
        super(OSMCGui, self).__init__(xmlFilename=strXMLname,
                                      scriptPath=strFallbackPath,
                                      defaultSkin=strDefaultName)
        self.order_of_fill = kwargs.get('order_of_fill', [])
        self.apply_buttons = kwargs.get('apply_buttons', [])
        self.live_modules = kwargs.get('live_modules', [])

        self._addon = kwargs.get('addon')
        self.window = kwargs.get('window', xbmcgui.Window(10000))
        self._lang = None

        log(kwargs)

        log(len(self.live_modules))

        self.module_holder = {}

        self.first_run = True

        self.visible_left_label = 4915

        self.number_of_pages = len(self.apply_buttons)
        self.active_page = 1

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

    def onInit(self):

        if self.first_run:
            self.first_run = False

            # hide the unneeded control groups
            contr = 200
            while True:
                try:
                    self.getControl(contr).setVisible(False)
                    contr += 100
                except:
                    break

            # hide the left labels
            self.getControl(4915).setLabel(self.lang(32002))
            self.getControl(4915).setVisible(True)
            self.visible_left_label = 4915
            self.getControl(4916).setVisible(False)

            # hide next and prev if they aren't needed
            if self.number_of_pages < 2:
                self.getControl(4444).setVisible(False)
                self.getControl(6666).setVisible(False)

            # place the items into the gui
            for i, module in enumerate(self.live_modules):

                try:
                    short_name = module['class_instance'].short_name
                    if isinstance(module['class_instance'].short_name_i18n, int):
                        short_name = self.lang(module['class_instance'].short_name_i18n)
                except:
                    short_name = ''

                # set the icon (texturefocus, texturenofocus)
                list_item = xbmcgui.ListItem(label=short_name, label2='', offscreen=True)
                list_item.setArt({
                    'icon': module['class_instance'].unfocused_icon,
                    'thumb': module['class_instance'].unfocused_icon
                })
                list_item.setProperty('FO_ICON', module['class_instance'].focused_icon)

                # grab the modules description for display in the textbox
                # this is a TRY just in case the module doesnt have a self.description
                try:
                    description = module['class_instance'].description
                    if isinstance(module['class_instance'].description_i18n, int):
                        description = self.lang(module['class_instance'].description_i18n)
                except:
                    description = ''

                list_item.setProperty('description', str(description))

                control_id = self.order_of_fill[i]

                self.getControl(control_id).addItem(list_item)

                self.module_holder[control_id] = module

            # set up the apply buttons
            for apply_button in self.apply_buttons:
                # set the image
                list_item = xbmcgui.ListItem(label='', label2='', offscreen=True)
                list_item.setProperty('Action', "Apply")

                self.getControl(apply_button).addItem(list_item)

            self.setFocusId(105)

            self.next_prev_direction_changer()

    def onAction(self, action):

        # log(action)

        action_id = action.getId()
        focused_control = self.getFocusId()

        if action_id in (10, 92):
            self.close()

        elif focused_control == 4444:

            # previous menu
            if self.active_page - 1 == 0:
                new_page = self.number_of_pages
            else:
                new_page = self.active_page - 1

            self.getControl(self.active_page * 100).setVisible(False)
            self.getControl(new_page * 100).setVisible(True)

            self.active_page = new_page

            self.setFocusId((self.active_page * 100) + 5)

        # self.next_prev_direction_changer()

        elif focused_control == 6666:
            # next menu
            if (self.active_page + 1) > self.number_of_pages:
                new_page = 1
            else:
                new_page = self.active_page + 1

            self.getControl(self.active_page * 100).setVisible(False)
            self.getControl(new_page * 100).setVisible(True)

            self.active_page = new_page

            self.setFocusId((self.active_page * 100) + 5)

    def onClick(self, controlID):

        if not (controlID - 5) % 100:

            self.close()

        elif controlID == 909:
            # open the advanced settings beta addon
            xbmc.executebuiltin("RunAddon(script.advancedsettingsetter)")

        else:

            module = self.module_holder.get(controlID, {})
            class_instance = module.get('class_instance', None)

            log('Checking instance: %s ' % str(class_instance))
            try:
                log(class_instance.isAlive())
            except AttributeError:
                return

            if class_instance.isAlive():
                class_instance.run()
            else:

                class_instance = module['module_instance'].OSMCSettingClass()
                class_instance.setDaemon(True)

                module['class_instance'] = class_instance
                class_instance.start()

    def left_label_toggle(self, control_id):
        # toggles the left label which displays the focused module name
        controls = cycle([4915, 4916])
        new_label = self.getControl(control_id).getListItem(0).getLabel()

        for control_id in controls:
            if control_id != self.visible_left_label:
                control = self.getControl(control_id)
                control.setLabel(new_label)
                control.setVisible(True)
                self.visible_left_label = control_id
                control = self.getControl(next(controls))
                control.setVisible(False)
                break

    def onFocus(self, controlID):

        # update the textbox 'description'
        try:
            self.getControl(2).setText(self.getControl(controlID).getSelectedItem().getProperty('description'))
        except:
            pass

        module_icons = [101, 102, 103, 104, 105, 106, 107, 108, 109,
                        201, 202, 203, 204, 205, 206, 207, 208, 209, ]

        if controlID in module_icons:
            self.left_label_toggle(controlID)

    def next_prev_direction_changer(self):
        """ Sets the direction (onLeft, onRight, etc) for the previous and next buttons in the gui """

        prev_button = self.getControl(4444)
        next_button = self.getControl(6666)

        pos5 = self.getControl((self.active_page * 100) + 5)

        try:
            pos4 = self.getControl((self.active_page * 100) + 4)
        except:
            pos4 = pos5

        try:
            pos6 = self.getControl((self.active_page * 100) + 6)
        except:
            pos6 = pos5

        prev_button.setNavigation(pos5, pos5, next_button, pos4)
        next_button.setNavigation(pos5, pos5, pos6, prev_button)


class GuiThread(threading.Thread):

    def __init__(self, **kwargs):
        super(GuiThread, self).__init__()

        self.queue = kwargs['queue']
        self._addon = kwargs.get('addon')
        self.window = kwargs.get('window', xbmcgui.Window(10000))
        self.script_path = self.addon.getAddonInfo('path')

        self._hardware_prefix = ''

        self.known_modules_order = {
            "osmcpi": 0,
            "osmcupdates": 1,
            "osmcnetworking": 2,
            "osmclogging": 3,
            "apfstore": 4,
            "osmcservices": 5,
            "osmcremotes": 6,
        }

        self.live_modules = []
        self.number_of_pages_needed = 1
        self.order_of_fill = []
        self.apply_buttons = []

        self.module_tally = 1000

        # order of addon hierarchy
        # 105 is Apply
        self.item_order = [104, 106, 102, 108, 101, 109, 103, 107]
        self.apply_button = [105]

        self.gui = None

        self.create_gui()

    @property
    def addon(self):
        if not self._addon:
            self._addon = xbmcaddon.Addon(ADDON_ID)
        return self._addon

    @clog(log)
    def create_gui(self):
        """
            known modules is a list of tuples detailing all the known and permissible modules and services
            (module name, order): the order is the hierarchy of addons (which is used to
            determine the positions of addon in the gui)
        """
        # window xml to use
        try:
            skin_height = int(xbmcgui.Window(10000).getProperty("SkinHeight"))
        except ValueError:
            skin_height = 720
        xml = "settings_gui_720.xml" if skin_height < 1080 else "settings_gui.xml"

        # check if modules and services exist, add the ones that exist to the live_modules list
        ordered_live_modules = self.retrieve_modules()
        ordered_live_modules.sort()
        self.live_modules = [x[1] for x in ordered_live_modules]

        # load the modules as widget entries
        self.load_widget_info()

        # determine which order list is used, indexed to 0
        self.number_of_pages_needed = (len(self.live_modules) // 9) + 1

        log('number_of_pages_needed')
        log(self.number_of_pages_needed)

        self.order_of_fill = [item + (100 * x) for x in range(self.number_of_pages_needed) for item in self.item_order]
        self.apply_buttons = [item + (100 * x) for x in range(self.number_of_pages_needed) for item in self.apply_button]

        # instantiate the window
        self.gui = OSMCGui(xml, self.script_path, 'Default', addon=self.addon, window=self.window,
                           order_of_fill=self.order_of_fill, apply_buttons=self.apply_buttons,
                           live_modules=self.live_modules)

    def load_widget_info(self):
        """ Takes each live_module and loads the information required for it to be included in the MyOSMC widget into the Home window.
        """

        script_location = os.path.join(self.script_path, 'resources', 'lib', 'osmcsettings', 'call_osmc_parent.py')

        self.window.setProperty('MyOSMC.Module.Script', script_location)

        for i, module in enumerate(self.live_modules):
            self.window.setProperty('MyOSMC.Module.%s.name' % i, module['class_instance'].short_name)
            self.window.setProperty('MyOSMC.Module.%s.fo_icon' % i, module['class_instance'].focused_widget)
            self.window.setProperty('MyOSMC.Module.%s.fx_icon' % i, module['class_instance'].unfocused_widget)
            self.window.setProperty('MyOSMC.Module.%s.id' % i, module['id'])

    def close(self):
        """
            Closes the gui
        """

        self.gui.close()

    def run(self):
        """
            Opens the gui window
        """

        log('Opening GUI')
        # display the window
        self.gui.doModal()

        # run the apply_settings method on all modules
        for module in self.live_modules:
            class_instance = module.get('class_instance', False)
            if class_instance:
                try:
                    class_instance.apply_settings()
                except:
                    log('apply_settings failed for %s' % class_instance.addon_id)

        # check is a reboot is required
        reboot = False
        for module in self.live_modules:
            class_instance = module.get('class_instance', False)
            try:
                if class_instance.reboot_required:
                    reboot = True
                    break
            except:
                pass

        if reboot:
            self.queue.put('reboot')

        log('Exiting GUI')

        # set the GUI back to the default first page view
        try:
            self.gui.getControl(self.gui.active_page * 100).setVisible(False)
            self.gui.getControl(100).setVisible(True)

            self.gui.active_page = 1
            self.gui.next_prev_direction_changer()
            self.gui.setFocusId(105)
        except:
            pass

    @clog(log)
    def retrieve_modules(self):
        """
            Checks to see whether the module exists and is active. If it doesnt exist (or is set to inactive)
            then return False, otherwise import the module (or the setting_module.py in the service or addons
            resources/lib/) and create then return the instance of the SettingGroup in that module.

        """

        self.module_tally = 1000

        known_modules = self.known_modules_order.keys()
        osmc_modules = [x for x in [self.inspect_module(module_name) for module_name in known_modules] if x[1]]

        return osmc_modules

    def inspect_module(self, module_name):
        """
            Checks the provided module to see if it is genuine.
            Returns a tuple.
                (preferred order of module, module name: {unfocused icon, focused icon, instance of OSMCSetting class})
        """

        # if you got this far then this is almost certainly an OSMC setting
        log('Inspecting OSMC Setting module __ %s __' % module_name)
        try:
            if module_name == 'osmcpi' and not self.hardware_prefix.startswith('rbp'):
                return -1, None

            module_instance = __import__('%s.osmc.osmc_setting' % module_name, fromlist=[''])
            class_instance = module_instance.OSMCSettingClass()
            class_instance.setDaemon(True)
        except:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            log('OSMCSetting __ %s __ failed to import' % module_name)
            log(exc_type)
            log(exc_value)
            log(traceback.format_exc())
            return -1, None

        if not (class_instance.unfocused_icon and class_instance.focused_icon and
                class_instance.unfocused_widget and class_instance.focused_widget):
            return -1, None

        log('OSMC Setting module __ %s __ found and imported' % module_name)

        # DETERMINE ORDER OF ADDONS, THIS CAN BE HARDCODED FOR SOME OR THE USER SHOULD BE ABLE TO CHOOSE THEIR OWN ORDER
        if module_name in self.known_modules_order.keys():
            order = self.known_modules_order[module_name]
        else:
            order = self.module_tally
            self.module_tally += 1

        return (order, {
            'id': module_name,
            'class_instance': class_instance,
            'module_instance': module_instance
        })

    @property
    def hardware_prefix(self):
        """ Returns the prefix for the hardware type. rbp, rbp2, etc """

        if self._hardware_prefix:
            return self._hardware_prefix

        self._hardware_prefix = ''

        with open('/proc/cmdline', 'r', encoding='utf-8') as f:
            line = f.readline()

        settings = line.split(' ')

        for setting in settings:
            if setting.startswith('osmcdev='):
                self._hardware_prefix = setting[len('osmcdev='):]
                break

        return self._hardware_prefix
