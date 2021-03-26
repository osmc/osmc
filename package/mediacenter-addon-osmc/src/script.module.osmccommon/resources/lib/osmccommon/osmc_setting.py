# -*- coding: utf-8 -*-
"""
    Copyright (C) 2014-2020 OSMC (KodeKarnage)

    This file is part of script.module.osmccommon

    SPDX-License-Identifier: GPL-2.0-or-later
    See LICENSES/GPL-2.0-or-later for more information.

    The settings for OSMC are handled by the OSMC Settings Addon (OSA).

    In order to more easily accommodate future changes and enhancements, each OSMC settings bundle (module) is a separate addon.
    The module can take the form of an xbmc service, an xbmc script, or an xbmc module, but it must be installed into the users'
    /usr/share/kodi/addons folder.

    The OSA collects the modules it can find, loads their icons, and launches them individually when the user clicks on an icon.

    The modules can either have their own GUI, or they can leverage the settings interface provided by XBMC. If the OSG uses the XBMC
    settings interface, then all of their settings must be stored in the addons settings.xml. This is true even if the source of record
    is a separate config file.

    An example of this type is the Pi settings module; the actual settings are read from the config.txt, then written to the
    settings.xml for display in kodi, then finally all changes are written back to the config.txt. The Pi module detects user
    changes to the settings by identifying the differences between a newly read settings.xml and the values from a previously
    read settings.xml.

    The values of the settings displayed by this module are only ever populated by the items in the settings.xml. [Note: meaning that
    if the settings data is retrieved from a different source, it will need to be populated in the module before it is displayed
    to the user.]

    Each module must have in its folder, a sub-folder called 'resources/osmc'. Within that folder must reside this script (OSMCSetting.py),
    and the icons to be used in the OSG to represent the module (FX_Icon.png and FO_Icon.png for unfocused and focused images
    respectively).

    When the OSA creates the OSMC Settings GUI (OSG), these modules are identified and the OSMCSetting.py script in each of them
    is imported. This script provides the mechanism for the OSG to apply the changes required from a change in a setting.

    The OSMCSetting.py file must have a class called OSMCSettingClass.

    The key variables in this class are:

        addon_id						: The id for the addon. This must be the id declared in the addons addon.xml.

        short_name                      : The name for the module, shown in the OSA

        [optional] short_name_i18n      : The string id in service.osmc.settings of the name for the module

        description 					: The description for the module, shown in the OSA

        [optional] description_i18n     : The string id in service.osmc.settings of the description for the module

        path                            : The path for the OSMCSettings modules

        reboot_required					: A boolean to declare if the OS needs to be rebooted. If a change in a specific setting
                                          requires an OS reboot to take affect, this is flag that will let the OSG know.

        setting_data_method 			: This dictionary contains:
                                                - the name of all settings in the module
                                                - the current value of those settings
                                                - [optional] apply - a method to call for each setting when the value changes
                                                - [optional] translate - a method to call to translate the data before adding it to the
                                                  setting_data_method dict. The translate method must have a 'reverse' argument which
                                                  when set to True, reverses the transformation.



    The key methods of this class are:

        run                 			: This is called by the OSG when the icon is clicked. This will open the settings window.
                                          Usually this would be addon.OpenSettings(), but it could be any other script.
                                          This allows the creation of action buttons in the GUI, as well as allowing developers
                                          to script and skin their own user interfaces.

        [optional] first_method			: called before any individual settings changes are applied.

        [optional] final_method			: called after all the individual settings changes are done.

        [optional] boot_method			: called when the OSA is first started.

        apply_settings					: This is called by the OSG to apply the changes to any settings that have changed.
                                          It calls the first setting method, if it exists.
                                          Then it calls the method listed in setting_data_method for each setting. Then it
                                          calls the final method, again, if it exists.

        populate_setting_data_method	: This method is used to populate the setting_data_method with the current settings data.
                                          Usually this will be from the addons setting data stored in settings.xml and retrieved
                                          using the settings_retriever_xml method.

                                          Sometimes the user is able to edit external setting files (such as the Pi's config.txt).
                                          If the developer wants to use this source in place of the data stored in the
                                          settings.xml, then they should edit this method to include a mechanism to retrieve and
                                          parse that external data. As the window shown in the OSG populates only with data from
                                          the settings.xml, the developer should ensure that the external data is loaded into that
                                          xml before the settings window is opened.

        settings_retriever_xml			: This method is used to retrieve all the data for the settings listed in the
                                          setting_data_method from the addons settings.xml.

    The developer is free to create any methods they see fit, but the ones listed above are specifically used by the OSA.
    Specifically, the apply_settings method is called when the OSA closes.

    Settings changes are applied when the OSG is called to close. But this behaviour can be changed to occur when the addon
    settings window closes by editing the open_settings_window. The method apply_settings will still be called by OSA, so
    keep that in mind.

"""

import os
import sys
import threading

import xbmcaddon

from .osmc_language import LangRetriever


class OSMCSettingClass(threading.Thread):
    """
        A OSMCSettingClass is way to substantiate the settings of an OSMC settings module,
        and make them available to the OSMC Settings Addon (OSA).
    """

    def __init__(self):
        super(OSMCSettingClass, self).__init__()

        self.addon_id = None
        self._me = None

        self._path = ''

        # this is what is displayed in the main settings gui
        self.short_name = ''
        self.short_name_i18n = None

        self.description = ''
        self.description_i18n = None

        self.setting_data_method = {}

        # a flag to determine whether a setting change requires a reboot to take effect
        self.reboot_required = False

        self._unfocused_icon = ''
        self._focused_icon = ''
        self._unfocused_widget = ''
        self._focused_widget = ''

        self._lang = None

    @property
    def path(self):
        if not self._path:
            self._path = \
                os.path.dirname(os.path.abspath(sys.modules[self.__class__.__module__].__file__))
        return self._path

    @property
    def me(self):
        if not self._me:
            self._me = xbmcaddon.Addon(self.addon_id)
        return self._me

    def lang(self, string_id):
        if not self._lang:
            retriever = LangRetriever(self.me)
            self._lang = retriever.lang
        return self._lang(string_id)

    def run(self):
        """
            The method determines what happens when the item is clicked in the settings GUI.
            Usually this would be addon.openSettings(), but it could be any other script.
            This allows the creation of action buttons in the GUI, as well as allowing developers
            to script and skin their own user interfaces.
        """
        self.me.openSettings()

    def first_method(self):
        """ 
            The method to call before all the other setting methods are called.

            For example, this could be a call to stop a service. The final method could then
            restart the service again.
            This can be used to apply the setting changes.

        """

    def final_method(self):
        """ 
            The method to call after all the other setting methods have been called.

            For example, in the case of the Raspberry Pi's settings module, the final writing
            to the config.txt can be delayed until all the settings have been updated in
            the pi_settings_dict.
        """

    def apply_settings(self):
        """
            This method will apply all of the settings. It calls the first_method, if it exists.
            Then it calls the method listed in pi_settings_dict for each setting. Then it calls the
            final_method, again, if it exists.
        """

    def settings_retriever_xml(self):
        """
            Reads the stored settings (in settings.xml) and returns a dictionary with the
            setting_name: setting_value. This method cannot be overwritten.
        """

    def populate_setting_data_method(self):
        """
            Populates the setting_value in the setting_data_method.
        """

    def _get_full_path(self, filename):
        """
            Check if filename exists in the instances path and return a string
            containing the full path and filename, or an empty string if the
            file doesn't exist.
        """
        filename = os.path.join(self.path, filename)
        if not os.path.isfile(filename):
            filename = ''
        return filename

    @property
    def unfocused_icon(self):
        """
            path to unfocused icon image, lazy load
        """
        if not self._unfocused_icon:
            self._unfocused_icon = self._get_full_path("FX_Icon.png")
        return self._unfocused_icon

    @property
    def focused_icon(self):
        """
            path to focused icon image, lazy load
        """
        if not self._focused_icon:
            self._focused_icon = self._get_full_path("FO_Icon.png")
        return self._focused_icon

    @property
    def unfocused_widget(self):
        """
            path to unfocused widget image, lazy load
        """
        if not self._unfocused_widget:
            self._unfocused_widget = self._get_full_path("FX_Icon_Widget.png")
        return self._unfocused_widget

    @property
    def focused_widget(self):
        """
            path to focused widget image, lazy load
        """
        if not self._focused_widget:
            self._focused_widget = self._get_full_path("FO_Icon_Widget.png")
        return self._focused_widget
