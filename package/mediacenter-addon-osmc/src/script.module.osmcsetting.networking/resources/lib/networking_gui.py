# Standard Modules
from collections import namedtuple, OrderedDict
import os
import socket
import subprocess
import sys
import threading
import time
import traceback
import xmltodict

# XBMC Modules
import xbmcaddon
import xbmcgui
import xbmc

__addon__ = xbmcaddon.Addon('script.module.osmcsetting.networking')
DIALOG    = xbmcgui.Dialog()

# Custom modules
sys.path.append(xbmc.translatePath(os.path.join(__addon__.getAddonInfo('path'), 'resources', 'lib')))

import osmc_bluetooth
import osmc_network
from osmc_advset_editor import AdvancedSettingsEditor

WIFI_THREAD_NAME      = 'wifi_population_thread'
BLUETOOTH_THREAD_NAME = 'bluetooth_population_thread'
WIFI_SCAN_THREAD_NAME = 'wifi_scan_thread'

def log(message):

    try:
        message = str(message)
    except UnicodeEncodeError:
        message = message.encode('utf-8', 'ignore' )
        
    xbmc.log(str(message), level=xbmc.LOGDEBUG)


def lang(id):
    san = __addon__.getLocalizedString(id).encode('utf-8', 'ignore')
    return san


gui_ids = { 
         
            1000:   'Header Group list',
            101:    'Wired Network',
            102:    'Wireless Network',
            103:    'Bluetooth',
            10111:  'Wired - Manual/ DHCP toggle',
            10112:  'Wired - IP Address',
            910112: 'Wired - IP Address VALUE',
            10113:  'Wired - Subnet Mask',
            910113: 'Wired - Subnet Mask VALUE',
            10114:  'Wired - Default Gateway',
            910114: 'Wired - Default Gateway VALUE',
            10115:  'Wired - Primary DNS',
            910115: 'Wired - Primary DNS VALUE',
            10116:  'Wired - Secondary DNS',
            910116: 'Wired - Secondary DNS VALUE',
            10118:  'Wired - Apply',
            10119:  'Wired - Reset',
            10120:  'Wired - Enable Adapter',
            10121:  'Wired - Toggle wait for network service',
            10211:  'Wireless - Automatically configure the network toggle',
            10212:  'Wireless - IP Address',
            910212: 'Wireless - IP Address VALUE',
            10213:  'Wireless - Subnet Mask',
            910213: 'Wireless - Subnet Mask VALUE',
            10214:  'Wireless - Default Gateway',
            910214: 'Wireless - Default Gateway VALUE',
            10215:  'Wireless - Primary DNS',
            910215: 'Wireless - Primary DNS VALUE',
            10216:  'Wireless - Secondary DNS',
            910216: 'Wireless - Secondary DNS VALUE',
            10217:  'Wireless - Enable Adapter',
            10218:  'Wireless - Apply',
            10219:  'Wireless - Reset',
            10221:  'Wireless - Toggle wait for network service',
            10301:  'Bluetooth - Toggle Bluetooth Adapter',
            10303:  'Bluetooth - Toggle Discovery',
            10401:  'Tethering (wifi) - Hotspot SSID label',
            910401: 'Tethering (Wifi) - Hotspot SSID VALUE',
            10402:  'Tethering (Wifi) - Hotspot passphrase label',
            910402: 'Tethering (Wifi) - Hotspot passphrase VALUE',
            10403:  'Tethering (Wifi) - Enable Button',
            10404:  'Tethering (Wifi) - Disable Button',
            10405:  'Tethering (Ethernet) - Enable Button',
            10406:  'Tethering (Ethernet) - Disable Button',
            5000:   'WiFi panel',
            6000:   'Bluetooth paired devices panel',
            7000:   'Bluetooth discoverd devices panel',
            105:    'MySQL',
            1050:   'MySQL Panel',
            10510:  'MySQL Video Database toggle',
            10511:  'MySQL Video Database Name',
            910511: 'MySQL Video Database Name Value',
            10512:  'MySQL Video Database host',
            910512: 'MySQL Video Database host Value',
            10513:  'MySQL Video Database port',
            910513: 'MySQL Video Database port Value',
            10514:  'MySQL Video Database user',
            910514: 'MySQL Video Database user Value',
            10515:  'MySQL Video Database pass',
            910515: 'MySQL Video Database pass Value',
            10580:  'MySQL Video Database Import Watched Status',
            10590:  'MySQL Video Database Import Resum Point',            
            10520:  'MySQL Music Database toggle',
            10521:  'MySQL Music Database Name',
            910521: 'MySQL Music Database Name Value',         
            10522:  'MySQL Music Database host',
            910522: 'MySQL Music Database host Value',
            10523:  'MySQL Music Database port',
            910523: 'MySQL Music Database port Value',
            10524:  'MySQL Music Database user',
            910524: 'MySQL Music Database user Value',
            10525:  'MySQL Music Database pass',  
            910525: 'MySQL Music Database pass Value',

        }

ip_controls                     = [ 10112, 10113, 10114, 10115, 10116, 910112, 910113, 910114, 910115, 910116, 10212, 10213, 10214, 10215,
                                    10216, 910212, 910213, 910214, 910215, 910216, 10512, 910512, 10522, 910522, ]

SELECTOR_WIRED_NETWORK          = 101
SELECTOR_WIRELESS_NETWORK       = 102
SELECTOR_BLUETOOTH              = 103
SELECTOR_TETHERING              = 104
SELECTOR_MYSQL                  = 105

MAIN_MENU                       = [SELECTOR_WIRED_NETWORK, SELECTOR_WIRELESS_NETWORK, SELECTOR_BLUETOOTH, SELECTOR_TETHERING, SELECTOR_MYSQL]

BLUETOOTH_CONTROLS              = [10303, 6000, 7000]

BLUETOOTH_DISCOVERY             = 10303
BLUETOOTH_ENABLE_TOGGLE         = 10301

ALL_WIRED_CONTROLS              = [ 10111, 10112, 10113, 10114, 10115, 10116, 10118, 10119, 910112, 910113, 910114,
                                    910115, 910116]
WIRED_STATUS_LABEL              = 81000
WIRED_IP_VALUES                 = [910112, 910113, 910114, 910115, 910116]
WIRED_IP_LABELS                 = [10112, 10113, 10114, 10115, 10116]
WIRED_APPLY_BUTTON              = 10118
WIRED_RESET_BUTTON              = 10119
WIRED_DHCP_MANUAL_BUTTON        = 10111
WIRED_ADAPTER_TOGGLE            = 10120
WIRED_WAIT_FOR_NETWORK          = 10121

ALL_WIRELESS_CONTROLS           = [ 5000, 910212, 910213, 910214, 910215, 910216, 10211, 10212, 10213, 10214, 10215, 10216,
                                    10218, 10219]

WIRELESS_STATUS_LABEL           = 82000
WIRELESS_IP_VALUES              = [910212, 910213, 910214, 910215, 910216]
WIRELESS_IP_LABELS              = [10212, 10213, 10214, 10215, 10216]
WIRELESS_ADAPTER_TOGGLE         = 10217
WIRELESS_APPLY_BUTTON           = 10218
WIRELESS_RESET_BUTTON           = 10219
WIRELESS_DHCP_MANUAL_BUTTON     = 10211
WIRELESS_NETWORKS               = 5000
WIRELESS_WAIT_FOR_NETWORK       = 10221

ALL_TETHERING_CONTROLS          = [10401, 910401, 10402, 910402, 10403, 10404, 10405, 10406, 10407]
TETHERING_WIFI_SSID_LABEL       = 10401
TETHERING_WIFI_SSID_VALUE       = 910401
TETHERING_WIFI_PASSPHRASE_LABEL = 10402
TETHERING_WIFI_PASSPHRASE_VALUE = 910402
TETHERING_WIFI_RADIOBUTTON      = 10403
TETHERING_ETHERNET_RADIOBUTTON  = 10404
TETHERING_ENABLE                = 10405
TETHERING_DISABLE               = 10406
TETHERING_WARNING               = 10407

ALL_MYSQL_CONTROLS              = [ 10510, 10512, 910512, 10513, 910513, 10514, 910514, 10515, 910515, 10520, 
                                    10522, 910522, 10523, 910523, 10524, 910524, 10525, 910525,
                                    10511, 10521, 910511, 910521, 10580, 10590]
MYSQL_VIDEO_VALUES              = [910511, 910512, 910513, 910514, 910515, 810515, 10580, 10590]
MYSQL_MUSIC_VALUES              = [910521, 910522, 910523, 910524, 910525, 810525]
MYSQL_TOGGLES                   = [10510, 10520]
MYSQL_VIDEO_TOGGLE              = 10510
MYSQL_MUSIC_TOGGLE              = 10520
MYSQL_PANEL                     = 1050
MYSQL_USER                      = [10514, 10524]
MYSQL_PASS                      = [10515, 10525, 910515, 910525]
MYSQL_PORT                      = [10523, 10513]
MYSQL_NAME                      = [10511, 10521]
MYSQL_IMPORT                    = [10580, 10590]
MYSQL_IMPORT_WATCHED            = 10580
MYSQL_IMPORT_RESUME             = 10590
EXIT_CONTROL                    = 666


class networking_gui(xbmcgui.WindowXMLDialog):

    current_network_config = {}  # holds the current network config
    reboot_required_file   = '/tmp/.reboot-needed'
    use_preseed            = False
    internet_protocol      = 'IPV4'


    def setUsePreseed(self, value):
        self.use_preseed = value


    def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):
        self.setting_values = kwargs.get('setting_values', {})

        # this stores the wifi password for sending to connman (or equivalent)
        self.wireless_password = None

        # current panel we are on
        self.current_panel = -1

        # Wired Network List (of one a way of showing connected icon )
        self.WDP = None

        # wifi panel (WFP)
        self.WFP = None

        # bluetooth paired device panel (BTP)
        self.BTP = None

        # bluetooth discovered device panel (BTD)
        self.BTD = None
        
        # list containing list items of all wifi networks
        self.wifis  = []
        
        # connected SSID, the ssid we are currently connected to
        self.conn_ssid  = None
        
        # list containing list items of all paired bluetooth devices
        self.paired_bluetooths = []
        
        # list containing list items of all discovered bluetooth devices
        self.discovered_bluetooths = []

        # flag to identify when a MySQL setting has been changed
        self.mysql_changed = False

        # dictionary to hold advanced settings from file
        self.advs_dict = {}
        
        # Bluetooth GUI update Thread
        self.bluetooth_population_thread = None
        self.preseed_data                = None
        self.wired_status_label          = None
        self.wireless_status_label       = None
        self.hotspot_ssid                = None
        self.hotspot_passphrase          = None


    def onInit(self):
        
        # Wired Network Label
        self.wired_status_label = self.getControl(WIRED_STATUS_LABEL)

        # wifi panel (WFP)
        self.WFP = self.getControl(5000)

        # Wireless Network Label
        self.wireless_status_label = self.getControl(WIRELESS_STATUS_LABEL)

        # bluetooth paired device panel (BTP)
        self.BTP = self.getControl(6000)

        # bluetooth discovered device panel (BTD)
        self.BTD = self.getControl(7000)

        # Hide panel selectors if devices are not present
        if not osmc_network.is_wifi_available():

            self.toggle_controls(False, [SELECTOR_WIRELESS_NETWORK])
            self.toggle_controls(False, [SELECTOR_TETHERING])

        if not osmc_bluetooth.is_bluetooth_available():
            self.toggle_controls(False, [SELECTOR_BLUETOOTH])

        panel_to_show = SELECTOR_WIRED_NETWORK

        if self.use_preseed and not osmc_network.get_nfs_ip_cmdline_value():

            self.preseed_data = osmc_network.parse_preseed()

            if self.preseed_data:
                if self.preseed_data['Interface'].startswith('wlan') and osmc_network.is_wifi_available():
                    panel_to_show = SELECTOR_WIRELESS_NETWORK

                else:
                    panel_to_show = SELECTOR_WIRED_NETWORK

        # create the advanced settings reader
        self.ASE = AdvancedSettingsEditor(log)

        # read advancedsettings.xml file and parse the details, then load those into the fields in the MySQL panel
        self.advs_dict = self.ASE.parse_advanced_settings()
        self.populate_mysql(self.advs_dict)

        # set all the panels to invisible except the first one
        for ctl in MAIN_MENU:
            self.getControl(ctl * 10).setVisible(True if ctl == panel_to_show else False)

        if panel_to_show == SELECTOR_WIRED_NETWORK:
            self.populate_wired_panel()
            
        if panel_to_show == SELECTOR_WIRELESS_NETWORK:
            self.populate_wifi_panel(False)

        if self.use_preseed and not osmc_network.get_nfs_ip_cmdline_value():
            self.setup_networking_from_preseed()

        self.getControl(MYSQL_PANEL).setVisible(False)


    def setup_networking_from_preseed(self):

        wired     = False
        connected = False

        if self.preseed_data['Interface'].startswith('wlan') and osmc_network.is_wifi_available():
            if not osmc_network.is_wifi_enabled():
                self.toggle_wifi()

            ssid             = self.preseed_data['SSID']
            encrypted        = False
            preseed_password = None

            if 'Password' in self.preseed_data:
                encrypted = True
                preseed_password = self.preseed_data['Password']

            connected = self.connect_to_wifi(ssid, encrypted, preseed_password, True)

        if self.preseed_data is None or self.preseed_data['Interface'].startswith('eth') or not connected:

            wired = True
            if not osmc_network.is_ethernet_enabled():
                self.toggle_ethernet()

            self.current_network_config = self.get_wired_config()
            connected = True

        if connected:

            if self.preseed_data is None or 'dhcp' in self.preseed_data['Method']:
                self.current_network_config['IPV4']['Method'] = 'dhcp'

            else:

                self.current_network_config['IPV4']['Method']  = 'manual'
                self.current_network_config['IPV4']['Address'] = self.preseed_data['Address']
                self.current_network_config['IPV4']['Netmask'] = self.preseed_data['Netmask']

                if self.preseed_data['Gateway']:
                    self.current_network_config['IPV4']['Gateway'] = self.preseed_data['Gateway']

                if self.preseed_data['DNS_1']:
                    self.current_network_config['IPV4']['DNS_1']   = self.preseed_data['DNS_1']

                if self.preseed_data['DNS_2']:
                    self.current_network_config['IPV4']['DNS_2']   = self.preseed_data['DNS_2']

            osmc_network.apply_network_changes(self.current_network_config, 'IPV4')

        if wired:
            self.populate_wired_panel()

        else:
            self.populate_wifi_panel(False)


    def onClick(self, controlID):

        if controlID in ip_controls:
            self.edit_ip_address(controlID)

        elif controlID in BLUETOOTH_CONTROLS + [BLUETOOTH_ENABLE_TOGGLE]:
            self.handle_bluetooth_selection(controlID)
            self.populate_bluetooth_panel()

        elif controlID in ALL_WIRED_CONTROLS + [WIRED_ADAPTER_TOGGLE]:
            self.handle_wired_selection(controlID)

        elif controlID in ALL_WIRELESS_CONTROLS + [WIRELESS_ADAPTER_TOGGLE]:
            self.handle_wireless_selection(controlID)

        elif controlID == WIRED_WAIT_FOR_NETWORK:

            osmc_network.toggle_wait_for_network(not osmc_network.is_connman_wait_for_network_enabled())

            waitForNetworkRadioButton = self.getControl(WIRED_WAIT_FOR_NETWORK)
            waitForNetworkRadioButton.setSelected(osmc_network.is_connman_wait_for_network_enabled())

        elif controlID == WIRELESS_WAIT_FOR_NETWORK:

            osmc_network.toggle_wait_for_network(not osmc_network.is_connman_wait_for_network_enabled())

            waitForNetworkRadioButton = self.getControl(WIRELESS_WAIT_FOR_NETWORK)
            waitForNetworkRadioButton.setSelected(osmc_network.is_connman_wait_for_network_enabled())

        elif controlID in ALL_TETHERING_CONTROLS:
            self.handle_tethering_selection(controlID)

        elif controlID in ALL_MYSQL_CONTROLS:
            self.user_entry_mysql(controlID)

        elif controlID == EXIT_CONTROL:
            self.shutdown_process()


    def shutdown_process(self):
        ''' Actions that are done when the user chooses to Go Back, Escape, or clicks Exit '''

        self.stop_wifi_population_thread()

        self.stop_bluetooth_population_thread()

        self.read_mysql_settings(self.advs_dict)
        write = self.write_advancedsettings(self.advs_dict)

        # Enable WaitForNetwork if either server is not local and the settings include valid music or video db info
        if self.ASE.server_not_localhost(self.advs_dict):
            if self.ASE.validate_advset_dict(self.advs_dict, reject_empty=True):
                osmc_network.toggle_wait_for_network(True)

        wifi_thread = self.is_thread_running(WIFI_THREAD_NAME)
        wifi_scan_thread = self.is_thread_running(WIFI_SCAN_THREAD_NAME)
        bluetooth_thread = self.is_thread_running(BLUETOOTH_THREAD_NAME)

        while wifi_thread or wifi_scan_thread or bluetooth_thread:
            log("Wifi Thread " + str(wifi_thread))
            log("Wifi Scan  Thread " + str(wifi_scan_thread))
            log("BT Scan Thread " + str(bluetooth_thread))
            xbmc.sleep(10)
            wifi_thread = self.is_thread_running(WIFI_THREAD_NAME)
            wifi_scan_thread = self.is_thread_running(WIFI_SCAN_THREAD_NAME)
            bluetooth_thread = self.is_thread_running(BLUETOOTH_THREAD_NAME)

        log("All Threads dead proceeding to exit")

        if write is None:
            self.close()
        

    def onAction(self, action):

        actionID        = action.getId()
        focused_control = self.getFocusId()

        log('actionID = ' + str(actionID))
        log('focused_control = %s,   %s' % (type(focused_control), focused_control))

        if actionID in (10, 92):

            self.shutdown_process()

        if focused_control in MAIN_MENU:

            if focused_control != self.current_panel:

                self.current_panel = focused_control
                # stop any running threads on change of panel
                self.stop_wifi_population_thread()
                self.stop_bluetooth_population_thread()

                if   focused_control == SELECTOR_WIRED_NETWORK:     self.populate_wired_panel()
                elif focused_control == SELECTOR_WIRELESS_NETWORK:  self.populate_wifi_panel()
                elif focused_control == SELECTOR_BLUETOOTH:         self.populate_bluetooth_panel()
                elif focused_control == SELECTOR_TETHERING:         self.populate_tethering_panel()

            # change to the required settings panel
            for ctl in MAIN_MENU:

                self.getControl(ctl * 10).setVisible(True if ctl == focused_control else False)

        if focused_control in WIRED_IP_LABELS:

            self.update_current_ip_settings(WIRED_IP_VALUES)
            self.update_apply_reset_button('WIRED')

        if focused_control in WIRELESS_IP_LABELS:

            self.update_current_ip_settings(WIRELESS_IP_VALUES)
            self.update_apply_reset_button('WIRELESS')


    def edit_ip_address(self, controlID):

        relevant_label_control  = self.getControl(900000 + controlID)
        current_label           = relevant_label_control.getLabel().replace('___ : ___ : ___ : ___', '')

        user_input = DIALOG.input(lang(32004), current_label, type=xbmcgui.INPUT_IPADDRESS)

        if not user_input or user_input == '0.0.0.0':

            relevant_label_control.setLabel(current_label)

        else:

            # validate ip_address format
            try:
                socket.inet_aton(user_input)

            except:
                'The address provided is not a valid IP address.', 'OSMC Network Setup'
                ok = DIALOG.ok(lang(32004), lang(32005))

                self.edit_ip_address(controlID)

                return

            # ip_string = ' : '.join(str(user_input).split('.'))
            relevant_label_control.setLabel(user_input)

            return


    def stop_wifi_population_thread(self):

        # call the wifi checking bot to exit, if it exists
        try:
            self.wifi_populate_bot.stop_thread()
        except:
            pass


    def stop_bluetooth_population_thread(self):

        try:
            self.bluetooth_population_thread.stop_thread()
        except:
            pass


    def show_busy_dialogue(self):

        xbmc.executebuiltin("ActivateWindow(busydialog)")


    def clear_busy_dialogue(self):

        xbmc.executebuiltin("Dialog.Close(busydialog)")


    def toggle_controls(self, enabled, control_ids):

        for control_id in control_ids:
            control = self.getControl(control_id)
            control.setEnabled(enabled)
            control.setVisible(True)


    def hide_controls(self, control_ids):

        map(lambda x:  self.getControl(x).setVisible(False), control_ids)


    def get_wired_config(self):

        return osmc_network.get_ethernet_settings()


    def populate_wired_panel(self):

        if os.path.isfile(self.reboot_required_file):
            # 'NFS Network Settings'
            # 'The displayed network configuration may be out dated - A reboot is recommended before proceeding'
            DIALOG.ok(lang(32036), lang(32038))

        # Clear wired network Panel
        self.hide_controls(ALL_WIRED_CONTROLS)
        if osmc_network.is_ethernet_enabled():
            self.current_network_config = self.get_wired_config()
            if self.current_network_config:
                interface = self.current_network_config['Interface']

                if self.current_network_config['State'] == 'configuration':
                    status = lang(32044) + ': ' + interface + ' ('+ lang(32076) + ')'

                else:
                    if osmc_network.has_network_connection(True):
                        # 'Status'                               'Connected'
                        status = lang(32044) + ': ' + interface + ' (' + lang(32046) + ')'
                    else:
                        # 'Status'                               'No internet'
                        status = lang(32044) + ': ' + interface + ' (' + lang(32047) + ')'

                self.wired_status_label.setLabel(status)

                if self.current_network_config['State'] == 'configuration':
                    self.current_network_config[self.internet_protocol]['Method'] = 'manual'

                if self.current_network_config['State'] in ['online', 'ready', 'configuration']:
                    self.toggle_controls(True, ALL_WIRED_CONTROLS)
                    self.update_manual_DHCP_button( WIRED_DHCP_MANUAL_BUTTON, 
                                                    WIRED_IP_VALUES, 
                                                    WIRED_IP_LABELS
                                                    )
                    self.populate_ip_controls(self.current_network_config, WIRED_IP_VALUES)

                    # enable reset and apply button
                    self.update_apply_reset_button('WIRED')

            else:  # no wired connection
                self.hide_controls(WIRED_IP_VALUES + WIRELESS_IP_VALUES)

                # 'Status'     'no wired connection'
                status = lang(32044) + ': ' + lang(32049)

                self.wired_status_label.setLabel(status)
                self.clear_ip_controls(WIRED_IP_VALUES)

        else:  # Disabled
            self.clear_ip_controls(WIRED_IP_VALUES)
            self.hide_controls(ALL_WIRED_CONTROLS)

            # 'Status'     'disabled'
            status = lang(32044) + ': ' + lang(32048)

            self.wired_status_label.setLabel(status)
            self.update_apply_reset_button('WIRED')

        adapterRadioButton = self.getControl(WIRED_ADAPTER_TOGGLE)
        adapterRadioButton.setSelected(osmc_network.is_ethernet_enabled())
        adapterRadioButton.setEnabled(True)

        waitForNetworkRadioButton = self.getControl(WIRED_WAIT_FOR_NETWORK)
        waitForNetworkRadioButton.setSelected(osmc_network.is_connman_wait_for_network_enabled())
        waitForNetworkRadioButton.setEnabled(True)


    def update_manual_DHCP_button(self, button_id, ip_values, ip_labels):

        manualDHCPButton = self.getControl(button_id)

        if 'dhcp' in self.current_network_config[self.internet_protocol]['Method']:
            # 'Configure Network Manually'
            manualDHCPButton.setLabel(lang(32006))
            # if configuration is by DHCP disable controls
            self.toggle_controls(False, ip_values)
            self.toggle_controls(False, ip_labels)

        else:
            # 'Configure Network Using DHCP'
            manualDHCPButton.setLabel(lang(32033))
            self.toggle_controls(True, ip_values)
            self.toggle_controls(True, ip_labels)


    def populate_mysql(self, dictionary):
        ''' Reads the MySQL information from the CAS and loads it into the local addon '''

        main = dictionary.get('advancedsettings', {})

        if not main:
            video = False
            music = False
            
        else:
            video = main.get('videodatabase', {})
            vidlb = main.get('videolibrary',  {})
            music = main.get('musicdatabase', {})

        sql_subitems = ['name','host', 'port', 'user', 'pass']

        if video:

            self.getControl(MYSQL_VIDEO_TOGGLE).setSelected(True)

            name, host, port, user, pswd, hpwd, impw, impr = (self.getControl(x) for x in MYSQL_VIDEO_VALUES)

            name.setLabel(video.get('name', 'MyVideos'))
            host.setLabel(video.get('host', '___ : ___ : ___ : ___'))
            port.setLabel(video.get('port', ''))
            user.setLabel(video.get('user', 'kodi'))
            try:
                pswd.setLabel('*' * len(video.get('pass', 'kodi')))
                hpwd.setLabel(video.get('pass', 'kodi'))
            except:
                pswd.setLabel('kodi')
                hpwd.setLabel('kodi')
            impw.setSelected(vidlb.get('importwatchedstate', 'true') == 'true')
            impr.setSelected(vidlb.get('importresumepoint', 'true') == 'true')

        else:
            self.getControl(MYSQL_VIDEO_TOGGLE).setSelected(False)

        if music:

            self.getControl(MYSQL_MUSIC_TOGGLE).setSelected(True)

            name, host, port, user, pswd, hpwd = (self.getControl(x) for x in MYSQL_MUSIC_VALUES)

            name.setLabel(music.get('name', 'MyMusic'))
            host.setLabel(music.get('host', '___ : ___ : ___ : ___'))
            port.setLabel(music.get('port', ''))
            user.setLabel(music.get('user', 'kodi'))
            try:
                pswd.setLabel('*' * len(music.get('pass', 'kodi')))
                hpwd.setLabel(music.get('pass', 'kodi'))
            except:
                pswd.setLabel('kodi')
                hpwd.setLabel('kodi')

        else:
            self.getControl(MYSQL_MUSIC_TOGGLE).setSelected(False)

        return


    def read_mysql_settings(self, dictionary):
        ''' Reads the MySQL settings from the gui, and writes them directly into the ADVS. '''
        
        sub_dict = dictionary.get('advancedsettings',{})

        sql_subitems = ['name', 'host', 'port', 'user', 'pass']

        video = {'type': 'mysql'}
        music = {'type': 'mysql'}

        if self.getControl(MYSQL_VIDEO_TOGGLE).isSelected():

            for sql_item, ctl in zip(sql_subitems, MYSQL_VIDEO_VALUES):
                if ctl in MYSQL_PASS:
                    ctl -= 100000
                video[sql_item] = self.getControl(ctl).getLabel()

                log('ctl %s : sql item %s : %s' %(ctl, sql_item, self.getControl(ctl).getLabel()))
    
            sub_dict['videodatabase'] = video

            if 'videolibrary' not in sub_dict:
                sub_dict['videolibrary'] = {}

            if self.getControl(MYSQL_IMPORT_WATCHED).isSelected():
                sub_dict['videolibrary']['importwatchedstate'] = 'true'

            if self.getControl(MYSQL_IMPORT_RESUME).isSelected():
                sub_dict['videolibrary']['importresumepoint'] = 'true'

        else:
            try:
                del sub_dict['videodatabase']
            except:
                pass
            try:
                del sub_dict['videolibrary']['importwatchedstate']
            except:
                pass
            try:
                del sub_dict['videolibrary']['importresumepoint']
            except:
                pass

        if self.getControl(MYSQL_MUSIC_TOGGLE).isSelected():

            for sql_item, ctl in zip(sql_subitems, MYSQL_MUSIC_VALUES):
                if ctl in MYSQL_PASS:
                    ctl = ctl - 100000
                music[sql_item] = self.getControl(ctl).getLabel()

            sub_dict['musicdatabase'] = music

        else:
            try:
                del sub_dict['musicdatabase']
            except:
                pass        

        return {'advancedsettings': sub_dict}


    def write_advancedsettings(self, dictionary):
        ''' Takes a dictionary and writes it to the advancedsettings.xml file '''

        # check the dictionary to see if it is valid
        dictionary_valid, invalidity_type = self.ASE.validate_advset_dict(dictionary)

        if not dictionary_valid:
            if invalidity_type == 'missing mysql':

                discard = DIALOG.yesno(   'OSMC',
                            'MySQL settings are incomplete.', 'Discard MySQL settings changes?')

                if discard:

                    return

                else:

                    return 'stay'

            elif invalidity_type == 'empty':

                # if the dictionary is empty, dont write anything

                return

        user_data = xbmc.translatePath( "special://userdata")
        loc       = os.path.join(user_data,'advancedsettings.xml')

        try:
            self.ASE.write_advancedsettings(loc, dictionary)

        except IOError:

            log('IOError trying to write to advancedsettings.xml')

            resp = DIALOG.yesno(    'OSMC', 
                                    'Writing to advancedsettings.xml failed: Permission restrictions',
                                    'Will you allow OSMC to change those restrictions?')

            if resp:

                try:
                    res = subprocess.check_call(['sudo', 'chmod', '777', loc])

                    log('response from subprocess chmod: %s, %s' % (res, type(res)))

                except subprocess.CalledProcessError:
                    log('Attempt to change permission on advancedsettings.xml file has failed.')
                    ok = DIALOG.ok('OSMC', 'Failed to change permission restrictions.', 'More details may be found in the log.')
                    res = 1

                if res == 0:
                    log('Second attempt at writing MySQL changes to advancedsettings.xml file.')

                    try:
                        self.ASE.write_advancedsettings(loc, dictionary)

                    except:
                        ok = DIALOG.ok('OSMC', 'Failed to write to advancedsettings.xml again.', 'More details may be found in the log.')
                        log('Failed to write to advancedsettings.xml again.\n\n%s' % traceback.format_exc())
                else:
                    ok = DIALOG.ok('OSMC', 'Failed to change file permissions.', 'More details may be found in the log.')
                    log('subprocess chmod returned non-zero response')
        except:
            log('Failed to write to advancedsettings.xml.\n\n%s' % traceback.format_exc())


    def user_entry_mysql(self, controlID):
        ''' Handles the user input for MySQL panel. '''

        # as soon as a control is touched in the MySQL panel, the trigger for rewriting the advancedsettings.xml file is flipped, and cannot be reversed
        if not self.mysql_changed:
            self.mysql_changed = True

        new_val = None

        try:
            current = self.getControl(controlID + 900000 - (100000 if controlID in MYSQL_PASS else 0)).getLabel()
        except:
            log('control non-existent or has no label: %s' % controlID)
            return

        log('current label = %s' % current)

        # this section handles changes to the text or IP entry fields
        if controlID in MYSQL_USER:

            new_val = DIALOG.input( "Please Enter MySQL Username",
                                    current,
                                    type=xbmcgui.INPUT_ALPHANUM,
                                    )

        elif controlID in MYSQL_PASS:

            new_val = DIALOG.input( "Please Enter MySQL Password",
                                    current, 
                                    type=xbmcgui.INPUT_ALPHANUM,
                                    option=xbmcgui.ALPHANUM_HIDE_INPUT
                                    )            

        elif controlID in MYSQL_PORT:

            new_val = DIALOG.input( "Please Enter MySQL Port Number", 
                                    current,
                                    type=xbmcgui.INPUT_NUMERIC,
                                    )

        elif controlID in MYSQL_NAME:

            new_val = DIALOG.input( "Please Enter New Name",
                                    current,
                                    type=xbmcgui.INPUT_ALPHANUM,
                                    )           

        elif controlID in MYSQL_TOGGLES:

            if any([self.getControl(ctl).isSelected() for ctl in MYSQL_TOGGLES]):

                # if not osmc_network.is_connman_wait_for_network_enabled():
                # osmc_network.toggle_wait_for_network(True) -- we have moved this code to the shutdown process
                self.getControl(WIRED_WAIT_FOR_NETWORK).setSelected(True)
                self.getControl(WIRELESS_WAIT_FOR_NETWORK).setSelected(True)


        if new_val and new_val != -1:

            if not self.mysql_changed and new_val != current: 

                self.mysql_changed = True

            if controlID in MYSQL_PASS:

                self.getControl(800000 + controlID).setLabel(new_val)
                log('new val: %s ' % new_val)

                new_val = '*' * len(new_val)

            log('new val: %s ' % new_val)

            self.getControl(900000 + controlID).setLabel(new_val)


    def populate_ip_controls(self, settings_dict, controls):

        ip_address, subnet, defaultGateway, primaryDNS, secondaryDNS = (self.getControl(x) for x in controls)

        i_set = settings_dict[self.internet_protocol]

        ip_address.setLabel(i_set['Address'])
        
        subnet.setLabel(i_set['Netmask'])
        
        defaultGateway.setLabel(i_set.get('Gateway', ''))

        if 'Gateway' not in i_set: defaultGateway.setEnabled(False)
               
        primaryDNS.setLabel(settings_dict['Nameservers'].get('DNS_1', ''))
        
        secondaryDNS.setLabel(settings_dict['Nameservers'].get('DNS_2', ''))


    def clear_ip_controls(self, controls):

        for control in controls:
            self.getControl(control).setLabel('')
      
        self.toggle_controls(False, controls)


    def update_current_ip_settings(self, controls):

        ip_address, subnet, defaultGateway, primaryDNS, secondaryDNS = (self.getControl(x).getLabel() for x in controls)

        self.current_network_config[self.internet_protocol]['Address'] = ip_address
        self.current_network_config[self.internet_protocol]['Netmask'] = subnet
        self.current_network_config[self.internet_protocol]['Gateway'] = defaultGateway
        
        self.current_network_config['Nameservers']['DNS_1'] = primaryDNS
        
        if secondaryDNS: 
            self.current_network_config['Nameservers']['DNS_2'] = secondaryDNS


    def handle_wired_selection(self, control_id):

        if control_id == WIRED_DHCP_MANUAL_BUTTON:

            if self.current_network_config[self.internet_protocol]['Method'] == 'dhcp':
                self.current_network_config[self.internet_protocol]['Method'] = 'manual'

            elif self.current_network_config[self.internet_protocol]['Method'] == 'nfs_dhcp':
                self.current_network_config[self.internet_protocol]['Method'] = 'nfs_manual'

            elif self.current_network_config[self.internet_protocol]['Method'] == 'manual':
                self.current_network_config[self.internet_protocol]['Method'] = 'dhcp'

            elif self.current_network_config[self.internet_protocol]['Method'] == 'nfs_manual':
                self.current_network_config[self.internet_protocol]['Method'] = 'nfs_dhcp'

            self.update_manual_DHCP_button( WIRED_DHCP_MANUAL_BUTTON, 
                                            WIRED_IP_VALUES, 
                                            WIRED_IP_LABELS
                                            )

            if 'dhcp' in self.current_network_config[self.internet_protocol]['Method']:
                self.hide_controls(WIRED_IP_VALUES)

        if control_id == WIRED_RESET_BUTTON:
            self.current_network_config = self.get_wired_config()
            self.populate_ip_controls(self.current_network_config, WIRED_IP_VALUES)

            self.update_manual_DHCP_button( WIRED_DHCP_MANUAL_BUTTON, 
                                            WIRED_IP_VALUES, 
                                            WIRED_IP_LABELS
                                            )
            self.setFocusId(WIRED_DHCP_MANUAL_BUTTON)

        if control_id == WIRED_APPLY_BUTTON:
            if self.current_network_config:
                osmc_network.apply_network_changes(self.current_network_config, self.internet_protocol)
                if self.current_network_config[self.internet_protocol]['Method'] in ['nfs_dhcp', 'nfs_manual']:
                    with open(self.reboot_required_file, 'w') as f:
                        f.write('d')
                    # 'NFS Network Settings'
                    # 'Your Settings will not take effect until you reboot. Reboot Now?''
                    if DIALOG.yesno(lang(32036), lang(32037)):
                        xbmc.executebuiltin('Reboot')
                else:
                    self.populate_wired_panel()
                self.setFocusId(WIRED_DHCP_MANUAL_BUTTON)

        if control_id == WIRED_ADAPTER_TOGGLE:
            self.toggle_ethernet()
            self.populate_wired_panel()

        self.update_apply_reset_button('WIRED')


    def toggle_ethernet(self):

        self.show_busy_dialogue()
        self.hide_controls(ALL_WIRED_CONTROLS)

        # 'Status'              'Configuring...'
        self.wired_status_label.setLabel(lang(32044) + ' : ' + lang(32016))

        ethernet_state = not osmc_network.is_ethernet_enabled();
        osmc_network.toggle_ethernet_state(ethernet_state)
        
        # 5 second wait to allow connman to make the changes before refreshing
        changed_state = 1
        while changed_state != 0 and changed_state < 5:
            if  osmc_network.is_ethernet_enabled() == ethernet_state:
                 changed_state = 0;
            else:
                changed_state += 1
            xbmc.sleep(1000)
        if osmc_network.is_ethernet_enabled() != ethernet_state:
            log("Failed to change ethernet state to " + str(ethernet_state))
        # now wait for the ip address to be assigned to the adapter
        if ethernet_state:
            changed_state = 1
            while changed_state != 0 and changed_state < 20:
                settings = osmc_network.get_ethernet_settings()
                log(settings)
                if settings and 'State' in settings.keys() and settings['State'] in ('ready', 'online'):
                     changed_state = 0;
                else:
                    changed_state += 1
                xbmc.sleep(500)
        self.clear_busy_dialogue()


    def update_apply_reset_button(self, net_type):

        if net_type == 'WIRED':
            if cmp(self.get_wired_config(), self.current_network_config) == 0 or not self.get_wired_config():
                self.toggle_controls(False, [WIRED_RESET_BUTTON, WIRED_APPLY_BUTTON])

            else:
                self.toggle_controls(True, [WIRED_RESET_BUTTON, WIRED_APPLY_BUTTON])

        if net_type == 'WIRELESS':
            wireless_config = self.get_wireless_config(self.conn_ssid)
            if cmp(wireless_config, self.current_network_config) == 0:
                self.toggle_controls(False, [WIRELESS_RESET_BUTTON, WIRELESS_APPLY_BUTTON])

            else:
                self.toggle_controls(True, [WIRELESS_RESET_BUTTON, WIRELESS_APPLY_BUTTON])


    def get_wireless_config(self, ssid):

        if ssid is not None:

            wifi = None
            for adapterAddress, wifis in osmc_network.get_wifi_networks().iteritems():
                wifi = wifis.get(ssid, None)

            if wifi:
                config = wifi
                if self.wireless_password:
                    config['Password'] = self.wireless_password

                return config

        return {}

    def is_thread_running(self, thread_name):
        for t in threading.enumerate():
            if t.getName() == thread_name:
                return True
        return False

    def populate_wifi_panel(self, scan=False):

        if osmc_network.is_wifi_available():
            if osmc_network.is_wifi_enabled():

                # Start the wifi population thread

                if not self.is_thread_running(WIFI_THREAD_NAME):

                    self.wifi_populate_bot = wifi_populate_bot(scan, self.getControl(5000), self.conn_ssid)

                    self.wifi_populate_bot.setDaemon(True)
                    self.wifi_populate_bot.start()

                self.current_network_config = osmc_network.get_connected_wifi()
                self.conn_ssid              = None

                if 'SSID' in self.current_network_config:
                    self.conn_ssid = self.current_network_config['SSID']

                if self.conn_ssid:

                    if 'Address' in self.current_network_config[self.internet_protocol]:

                        self.update_manual_DHCP_button( WIRELESS_DHCP_MANUAL_BUTTON, 
                                                        WIRELESS_IP_VALUES, 
                                                        WIRELESS_IP_LABELS
                                                        )
                        self.populate_ip_controls(self.current_network_config, WIRELESS_IP_VALUES)

                    self.toggle_controls(True, [    WIRELESS_ADAPTER_TOGGLE,  
                                                    WIRELESS_NETWORKS,
                                                    WIRELESS_DHCP_MANUAL_BUTTON
                                                    ])

                    if osmc_network.has_network_connection(True):
                        # 'Status'            'Connected'
                        status = lang(32044) + ': ' + lang(32046)

                    else:
                        # 'Status'            'No internet'
                        status = lang(32044) + ':  ' + lang(32047)

                    self.wireless_status_label.setLabel(status)

                else:  # not connected to a network
                    self.hide_controls(WIRELESS_IP_VALUES + WIRELESS_IP_LABELS + [WIRELESS_DHCP_MANUAL_BUTTON,
                                                                                  WIRELESS_APPLY_BUTTON,
                                                                                  WIRELESS_RESET_BUTTON
                                                                                  ])

                    self.toggle_controls(True, [WIRELESS_ADAPTER_TOGGLE, WIRELESS_NETWORKS])

                    self.clear_ip_controls(WIRELESS_IP_VALUES)

                    # 'Status'           'No wireless Connection'
                    status = lang(32044) + ': ' + lang(32050)

                    self.wireless_status_label.setLabel(status)

            else:  # Wifi disabled
                self.hide_controls(ALL_WIRELESS_CONTROLS)

                # 'Status'            'disabled'
                status = lang(32044) + ': ' + lang(32048)
                self.wireless_status_label.setLabel(status)


            adapterRadioButton = self.getControl(WIRELESS_ADAPTER_TOGGLE)

            adapterRadioButton.setSelected(osmc_network.is_wifi_enabled())
            adapterRadioButton.setEnabled(True)

        else:  # Wifi not available
            self.toggle_controls(False, ALL_WIRELESS_CONTROLS)

        waitForNetworkRadioButton = self.getControl(WIRELESS_WAIT_FOR_NETWORK)

        waitForNetworkRadioButton.setSelected(osmc_network.is_connman_wait_for_network_enabled())
        waitForNetworkRadioButton.setEnabled(True)


    def handle_wireless_selection(self, control_id):

        if control_id == 5000:  # wireless network
            self.handle_selected_wireless_network()

        elif control_id == WIRELESS_DHCP_MANUAL_BUTTON:

            if self.current_network_config[self.internet_protocol]['Method'] == 'dhcp':
                
                self.current_network_config[self.internet_protocol]['Method'] = 'manual'

            elif self.current_network_config[self.internet_protocol]['Method'] == 'manual':

                self.current_network_config[self.internet_protocol]['Method'] = 'dhcp'
                self.hide_controls(WIRELESS_IP_VALUES)

            self.update_manual_DHCP_button( WIRELESS_DHCP_MANUAL_BUTTON, 
                                            WIRELESS_IP_VALUES, 
                                            WIRELESS_IP_LABELS)

            if 'dhcp' in self.current_network_config[self.internet_protocol]['Method']:
                self.hide_controls(WIRELESS_IP_VALUES)

        elif control_id == WIRELESS_RESET_BUTTON:

            self.current_network_config = self.get_wireless_config(self.conn_ssid)

            self.update_manual_DHCP_button( WIRELESS_DHCP_MANUAL_BUTTON, 
                                            WIRELESS_IP_VALUES, 
                                            WIRELESS_IP_LABELS
                                            )
            self.setFocusId(WIRELESS_DHCP_MANUAL_BUTTON)

        elif control_id == WIRELESS_APPLY_BUTTON:
            if self.current_network_config:

                osmc_network.apply_network_changes(self.current_network_config, self.internet_protocol)
                self.populate_wifi_panel()
                self.setFocusId(WIRELESS_DHCP_MANUAL_BUTTON)

        elif control_id == WIRELESS_ADAPTER_TOGGLE:
            self.toggle_wifi()
            self.populate_wifi_panel()

        self.update_apply_reset_button('WIRELESS')


    def toggle_wifi(self):

        self.show_busy_dialogue()
        self.WFP.reset()
        self.hide_controls(ALL_WIRELESS_CONTROLS)
        self.toggle_controls(True, [WIRELESS_NETWORKS])
        wifi_state = not osmc_network.is_wifi_enabled()
        osmc_network.toggle_wifi_state(wifi_state)
        changed_state  = 1
        # 5 second wait to allow connman to make the changes before refreshing
        while changed_state != 0 and changed_state < 5:
            if  osmc_network.is_wifi_enabled() == wifi_state:
                 changed_state = 0;
            else:
                changed_state += 1
            xbmc.sleep(1000)
        if osmc_network.is_wifi_enabled() != wifi_state:
            log("Failed to change wifi state to " + str(wifi_state))
        if not wifi_state:
            self.stop_wifi_population_thread()
            self.current_network_config = {}

        self.clear_busy_dialogue()


    def handle_selected_wireless_network(self):

        item        = self.WFP.getSelectedItem()
        path        = item.getProperty('Path')
        encrypted   = item.getProperty('Encrypted') == 'True'
        connected   = item.getProperty('Connected')
        ssid        = item.getProperty('SSID')

        if connected == 'False':
            if not self.conn_ssid:  # if we are not connected to a network connect
                if self.connect_to_wifi(ssid, encrypted):
                    if '_hidden_' not in path:

                        strength = self.current_network_config['Strength']
                        icon     = wifi_populate_bot.get_wifi_icon(encrypted, strength, True)

                        item.setIconImage(icon)
                        item.setProperty('Connected', 'True')

            else:  # Display a toast asking the user to disconnect first
                # 'Please disconnect from the current network before connecting'
                message = lang(32053)
                # 'Wireless'
                xbmc.executebuiltin("XBMC.Notification(%s,%s,%s)" % (lang(32041), message, "2500"))

        else:

            if ssid == self.conn_ssid:
                #                             'Disconnect from'
                selection = DIALOG.select(lang(32042) + ' ' + self.conn_ssid + '?',
                                          [lang(32051),lang(32058), lang(32059)])

                if selection == -1 or selection == 0:
                    return

                self.show_busy_dialogue()

                self.conn_ssid                  = None
                self.wireless_password          = None
                self.current_network_config     = {}

                self.hide_controls(WIRELESS_IP_VALUES + WIRELESS_IP_LABELS)
                osmc_network.wifi_disconnect(path)

                if selection == 2: # we also want to remove/forget this network
                    osmc_network.wifi_remove(path)

                self.WFP.removeItem(self.WFP.getSelectedPosition())
                self.toggle_controls(False, [SELECTOR_TETHERING])

                self.populate_wifi_panel()

                self.clear_busy_dialogue()


    def connect_to_wifi(self, ssid, encrypted, password=None, scan=False):

        if scan:
            self.show_busy_dialogue()
            osmc_network.scan_wifi()
            self.clear_busy_dialogue()

        wifi = None
        for adapterAddress, wifis in osmc_network.get_wifi_networks().iteritems():

            wifi = wifis.get(ssid, None)

        if wifi:
            # 'Configuring'
            self.wireless_status_label.setLabel(lang(32016))

            path                = wifi['path']
            connection_status   = False
            hiddenssid          = None
            connect             = False

            if '_hidden_' in path :
                # 'Enter SSID to connect to this hidden network'
                hiddenssid = DIALOG.input(lang(32073))

            else:
                # 'Wireless'   'Connect to'
                if DIALOG.yesno(lang(32041), lang(32052) + ' ' + ssid + '?'):
                    connect = True

            if connect or hiddenssid:

                self.show_busy_dialogue()

                # try without a password see if connman has the password
                script_base_path  = os.path.join(__addon__.getAddonInfo('path'), 'resources', 'lib') + '/'
                connection_status = osmc_network.wifi_connect(path, None, hiddenssid, script_base_path)

                self.clear_busy_dialogue()

                if not connection_status and encrypted:
                    if password is None:
                        password = DIALOG.input(lang(32013), 
                                                type=xbmcgui.INPUT_ALPHANUM,
                                                option=xbmcgui.ALPHANUM_HIDE_INPUT
                                                )

                    if password:
                        self.wireless_password = password
                        self.show_busy_dialogue()

                        connection_status = osmc_network.wifi_connect(path, password, hiddenssid, script_base_path)
                        self.clear_busy_dialogue()

                if not connection_status:

                    # 'Connection to '                  'failed'
                    message = lang(32043) + ' ' + ssid + ' ' + lang(32025)

                    #                                                   'Wireless'
                    xbmc.executebuiltin("XBMC.Notification(%s,%s,%s)" % (lang(32041), message, "2500"))

                    self.current_network_config = {}

                    self.clear_ip_controls(WIRELESS_IP_VALUES)

                    self.toggle_controls(False, [WIRELESS_DHCP_MANUAL_BUTTON])

                    #         'Status'           'Not connected'
                    status = lang(32044) + ': ' + lang(32050)

                    self.wireless_status_label.setLabel(status)
                    self.clear_busy_dialogue()

                    return False

                else:
                    self.show_busy_dialogue()

                    self.current_network_config = osmc_network.get_connected_wifi()

                    self.update_manual_DHCP_button( WIRELESS_DHCP_MANUAL_BUTTON, 
                                                    WIRELESS_IP_VALUES, 
                                                    WIRELESS_IP_LABELS
                                                    )

                    self.populate_ip_controls(self.current_network_config, WIRELESS_IP_VALUES)

                    self.toggle_controls(True, [WIRELESS_ADAPTER_TOGGLE, 
                                                WIRELESS_NETWORKS,
                                                WIRELESS_DHCP_MANUAL_BUTTON
                                                ])

                    self.conn_ssid  = self.current_network_config['SSID']
                    interface       = self.current_network_config['Interface']

                    if osmc_network.has_network_connection(True):
                        # 'Status'                               'Connected'
                        status = lang(32044) + ': ' + interface + ' (' + lang(32046) + ')'
                    
                    else:
                        # 'Status'                               'No internet'
                        status = lang(32044) + ': ' + interface + ' (' + lang(32047) + ')'
                    
                    self.wireless_status_label.setLabel(status)
                    self.toggle_controls(True, [SELECTOR_TETHERING])

                    self.clear_busy_dialogue()
                    
                    return True
        
        else:
        
            return False


    def sort_strength(self, itm):

        try:
            metric = int(itm.getProperty('strength'))
            if itm.getProperty('Connected') == 'True':
                # make sure the connected network is always at the top
                metric += 100
        except:
            metric = 0

        return metric


    def populate_bluetooth_panel(self):

        bluetoothRadioButton    = self.getControl(BLUETOOTH_ENABLE_TOGGLE)
        bluetooth_enabled       = osmc_bluetooth.is_bluetooth_enabled()

        bluetoothRadioButton.setSelected(bluetooth_enabled)

        # disable all if bluetooth is not enabled
        if not bluetooth_enabled:
            # disable all controls
            self.toggle_controls(False, BLUETOOTH_CONTROLS)
            return

        self.toggle_controls(True, BLUETOOTH_CONTROLS)

        discoveryRadioButton = self.getControl(BLUETOOTH_DISCOVERY)
        discoveryRadioButton.setSelected(osmc_bluetooth.is_discovering())

        # Start Bluetooth Population Thread
        if not self.is_thread_running(BLUETOOTH_THREAD_NAME):

            self.bluetooth_population_thread = bluetooth_population_thread(self.BTD, self.BTP)

            self.bluetooth_population_thread.setDaemon(True)
            self.bluetooth_population_thread.start()


    def handle_bluetooth_selection(self, control_id):

        if control_id == BLUETOOTH_ENABLE_TOGGLE:  # Enable Bluetooth

            self.show_busy_dialogue()

            if osmc_bluetooth.is_bluetooth_enabled():

                osmc_bluetooth.toggle_bluetooth_state(False)
                self.stop_bluetooth_population_thread()

            else:

                osmc_bluetooth.toggle_bluetooth_state(True)

            self.clear_busy_dialogue()

        elif control_id == BLUETOOTH_DISCOVERY:  # Discovery

            self.show_busy_dialogue()

            target_state = not osmc_bluetooth.is_discovering()
            
            if target_state: 

                osmc_bluetooth.start_discovery()

            else:

                osmc_bluetooth.stop_discovery()

            changed_state  = 1

            # up to 10 second wait to allow discovery to change state

            while changed_state != 0 and changed_state < 10:

                if osmc_bluetooth.is_discovering() == target_state:

                    changed_state = 0;

                else:

                    changed_state += 1

                xbmc.sleep(1000)

                if osmc_bluetooth.is_discovering() != target_state:
                    log("Failed to change discovery state to " + str(wifi_state))
                    
                self.clear_busy_dialogue()

        elif control_id == 6000:  # paired devices

            item = self.BTP.getSelectedItem()

            if item:
                address     = item.getProperty('address')
                alias       = item.getProperty('alias')
                connected   = item.getProperty('connected') == 'True'

                #                                'Cancel'     'Re-connect'  'Remove Device'
                selection = DIALOG.select(alias, [lang(32051), lang(32075), lang(32021)])

                if selection == -1 or selection == 0:
                    return

                elif selection == 1: # Reconnect

                    if not connected:
                        self.show_busy_dialogue()

                        if self.connect_bluetooth(address, alias):
                            self.bluetooth_population_thread.update_bluetooth_lists()

                        self.clear_busy_dialogue()

                elif selection == 2:

                    osmc_bluetooth.remove_device(address)
                    self.setFocusId(BLUETOOTH_ENABLE_TOGGLE)
                    self.bluetooth_population_thread.update_bluetooth_lists()

        elif control_id == 7000:  # Discovered devices

            item = self.BTD.getSelectedItem()
        
            if item:
        
                address = item.getProperty('address')
                alias   = item.getProperty('alias')

                #              'Connect With Device'                        'No'        'Pair and Connect' 'pair'
                selection = DIALOG.select(lang(32022) + ' ' + alias + '?', [lang(32055),lang(32056), lang(32057)])
                
                if selection == -1 or selection == 0:
                    return
                
                self.show_busy_dialogue()
                self.setFocusId(BLUETOOTH_DISCOVERY)
                
                if selection == 1:
                
                    script_base_path = os.path.join(__addon__.getAddonInfo('path'), 'resources', 'lib') + '/'
                    result           = osmc_bluetooth.pair_device(address, script_base_path)

                    if not result:
                        #         'Connection to '                       'failed'
                        message = lang(32024) + ' ' + alias + ' ' + lang(32025)

                        #                                                     'Bluetooth'
                        xbmc.executebuiltin("XBMC.Notification(%s,%s,%s)" % (lang(32020), message, "2500"))

                        self.clear_busy_dialogue()
                        return

                if self.connect_bluetooth(address, alias):

                    osmc_bluetooth.set_device_trusted(address, True)
                    self.bluetooth_population_thread.update_bluetooth_lists()

                self.clear_busy_dialogue()


    def connect_bluetooth(self, address, alias):

        connected = osmc_bluetooth.connect_device(address)

        if not connected:
             #         'Connection to'                       'failed'
            message = lang(32024) + ' ' + alias + ' ' + lang(32025)
            #                                                     'Bluetooth'
            xbmc.executebuiltin("XBMC.Notification(%s,%s,%s)" % (lang(32020), message, "2500"))

        return connected


    def populate_tethering_panel(self):

        wifi_tethering      = osmc_network.is_tethering_wifi()
        ethernet_tethering  = osmc_network.is_tethering_ethernet()

        self.toggle_controls(True, ALL_TETHERING_CONTROLS)

        if wifi_tethering:
            self.handle_tethering_selection(TETHERING_WIFI_RADIOBUTTON)

        if ethernet_tethering:
            self.handle_tethering_selection(TETHERING_ETHERNET_RADIOBUTTON)

        if not wifi_tethering and not ethernet_tethering:
            self.handle_tethering_selection(TETHERING_WIFI_RADIOBUTTON)

        # disable controls if tethering is active
        if wifi_tethering or ethernet_tethering:
            self.toggle_controls(False, [
                                        TETHERING_WIFI_SSID_LABEL,
                                        TETHERING_WIFI_SSID_VALUE, 
                                        TETHERING_WIFI_PASSPHRASE_LABEL,
                                        TETHERING_WIFI_PASSPHRASE_VALUE, 
                                        TETHERING_ENABLE, 
                                        TETHERING_WIFI_RADIOBUTTON,
                                        TETHERING_ETHERNET_RADIOBUTTON
                                        ])
        else:
            self.toggle_controls(False, [TETHERING_DISABLE])

        wifiSSIDLabel      = self.getControl(TETHERING_WIFI_SSID_VALUE)
        self.hotspot_ssid  = osmc_network.get_tethering_SSID()

        if not self.hotspot_ssid:
            self.hotspot_ssid = 'osmc_wifi'

        wifiSSIDLabel.setLabel(self.hotspot_ssid)
        self.hotspot_passphrase = osmc_network.get_tethering_passphrase()

        if not self.hotspot_passphrase:
            self.hotspot_passphrase = 'h0tsp0t0smc'

        control_label = ''.join([x if i % 33 else ' '+x for i, x in enumerate(self.hotspot_passphrase)])

        wifiPassphaseLabel = self.getControl(TETHERING_WIFI_PASSPHRASE_VALUE)
        wifiPassphaseLabel.setLabel(control_label)


    def handle_tethering_selection(self, control_id):

        if control_id in [TETHERING_WIFI_RADIOBUTTON, TETHERING_ETHERNET_RADIOBUTTON]:

            wifi_radiobutton     = self.getControl(TETHERING_WIFI_RADIOBUTTON)
            ethernet_radiobutton = self.getControl(TETHERING_ETHERNET_RADIOBUTTON)

            wifi_radiobutton.setSelected(False)
            ethernet_radiobutton.setSelected(False)
            
            if control_id == TETHERING_WIFI_RADIOBUTTON:
                self.hide_controls([TETHERING_WARNING])
                wifi_radiobutton.setSelected(True)
                ethernet_radiobutton.setSelected(False)
                self.toggle_controls(True, [
                                            TETHERING_WIFI_SSID_LABEL,
                                            TETHERING_WIFI_SSID_VALUE,
                                            TETHERING_WIFI_PASSPHRASE_LABEL, 
                                            TETHERING_WIFI_PASSPHRASE_VALUE
                                            ])
            else:
                self.toggle_controls(True, [TETHERING_WARNING])
                wifi_radiobutton.setSelected(False)
                ethernet_radiobutton.setSelected(True)
                self.toggle_controls(False, [
                                            TETHERING_WIFI_SSID_LABEL,
                                            TETHERING_WIFI_SSID_VALUE,
                                            TETHERING_WIFI_PASSPHRASE_LABEL, 
                                            TETHERING_WIFI_PASSPHRASE_VALUE
                                            ])

        if control_id == TETHERING_WIFI_SSID_LABEL:

            wifiSSIDLabel = self.getControl(TETHERING_WIFI_SSID_VALUE)
            enteredSSID   = DIALOG.input(lang(32068), self.hotspot_ssid)

            if enteredSSID:
                self.hotspot_ssid = enteredSSID
                wifiSSIDLabel.setLabel(enteredSSID)

        if control_id == TETHERING_WIFI_PASSPHRASE_LABEL:

            wifiPassphraseLabel = self.getControl(TETHERING_WIFI_PASSPHRASE_VALUE)
            currentPassphrase   = self.hotspot_passphrase
            enteredPassphrase   = DIALOG.input(lang(32069), currentPassphrase)
            
            if enteredPassphrase:
                self.hotspot_passphrase = enteredPassphrase             
                control_label           = ''.join([x if i % 33 else ' '+x for i, x in enumerate(enteredPassphrase)])

                wifiPassphraseLabel.setLabel(control_label)

        if control_id == TETHERING_ENABLE:

            wifi_radiobutton = self.getControl(TETHERING_WIFI_RADIOBUTTON)

            technology       = None
            ssid             = None
            passphrase       = None

            if wifi_radiobutton.isSelected():

                technology = 'wifi'
                ssid = self.hotspot_ssid

                if len(ssid) == 0:
                    # 'Portable Hotspot'
                    # 'SSID must be set to enable WiFi Tethering'
                    DIALOG.ok(lang(32063), lang(32070))
                    return

                passphrase = self.hotspot_passphrase
                if len(passphrase) > 63:
                    # 'Portable Hotspot'
                    # 'Passphrase must be 63 characters or less"'
                    DIALOG.ok(lang(32063), lang(32074))
                    return

                if len(passphrase) < 8:
                    # 'Portable Hotspot'
                    # 'Passphrase must at least 8 characters long'
                    DIALOG.ok(lang(32063), lang(32071))
                    return
            else:

                technology = 'ethernet'

            log('Enabling '+ technology +' Hotspot')

            if technology is 'wifi':
                log('Hotspot ssid = ' + ssid)

            if osmc_network.tethering_enable(technology, ssid, passphrase):

                self.setFocusId(TETHERING_DISABLE)

                self.toggle_controls(False, [
                                            TETHERING_WIFI_SSID_LABEL,
                                            TETHERING_WIFI_SSID_VALUE, 
                                            TETHERING_WIFI_PASSPHRASE_LABEL,
                                            TETHERING_WIFI_PASSPHRASE_VALUE, 
                                            TETHERING_ENABLE, 
                                            TETHERING_WIFI_RADIOBUTTON,
                                            TETHERING_ETHERNET_RADIOBUTTON, 
                                            TETHERING_ENABLE
                                            ])

                self.toggle_controls(True, [TETHERING_DISABLE])

            else:
                # 'Portable Hotspot'
                # 'Error enabling hotspot - see log for details'
                DIALOG.ok(lang(32063), lang(32072))

        if control_id == TETHERING_DISABLE:

            log('Disabling Hotspot')
            
            osmc_network.tethering_disable()

            self.setFocusId(SELECTOR_TETHERING)

            self.toggle_controls(True, [
                                        TETHERING_WIFI_SSID_LABEL,
                                        TETHERING_WIFI_SSID_VALUE, 
                                        TETHERING_WIFI_PASSPHRASE_LABEL,
                                        TETHERING_WIFI_PASSPHRASE_VALUE, 
                                        TETHERING_ENABLE, 
                                        TETHERING_WIFI_RADIOBUTTON,
                                        TETHERING_ETHERNET_RADIOBUTTON, 
                                        TETHERING_ENABLE
                                        ])

            self.toggle_controls(False, [TETHERING_DISABLE])


class bluetooth_population_thread(threading.Thread):


    def __init__(self, discovered_list_control, trusted_list_control):

        super(bluetooth_population_thread, self).__init__(name=BLUETOOTH_THREAD_NAME)

        self.exit = False

        self.discovered_list_control = discovered_list_control
        self.trusted_list_control    = trusted_list_control

        self.trusted_dict    = {}
        self.discovered_dict = {}


    def run(self):

        runs = 0
        while not self.exit:

            # update gui every 2 seconds
            if runs % 200 == 0 and not self.exit:
                self.update_bluetooth_lists()

            # every 4 seconds output debug info
            if runs % 400 == 0 and not self.exit:

                log('-- DISCOVERED ---')
                log(self.discovered_dict)
                log('-- TRUSTED --')
                log(self.trusted_dict)

            xbmc.sleep(10)
            runs += 1


    def update_bluetooth_lists(self):

        self.trusted_dict    = self.populate_bluetooth_dict(True)
        self.discovered_dict = self.populate_bluetooth_dict(False)

        self.update_list_control(dict(self.discovered_dict), self.discovered_list_control)
        self.update_list_control(dict(self.trusted_dict), self.trusted_list_control)


    def stop_thread(self):
        self.exit = True


    def update_list_control(self, devices_dict, list_control):

        items_to_be_removed = []

        for itemIndex in range(0, list_control.size()):

            listItem = list_control.getListItem(itemIndex)
            address  = listItem.getProperty('address')

            if address in devices_dict.keys():

                connected = listItem.getProperty('connected') == 'True'

                # connected status differs
                if not connected == devices_dict[address]['connected']:

                    log('Connected status differs')
                    items_to_be_removed.append(itemIndex)

                else:
                    devices_dict.pop(address)

            else:
                items_to_be_removed.append(itemIndex)

        for itemIndex in items_to_be_removed:
            try:
                list_control.removeItem(itemIndex)
            except:
                pass

        map(lambda (address, info): list_control.addItem(self.create_bluetooth_item(address, info)), devices_dict.iteritems())


    def create_bluetooth_item(self, address, info):
        label       = ""
        try:
            label       = info['alias'].decode('utf-8')
        except:
            log('Failed to decode BT Device Label.\n\n%s' % traceback.format_exc())
            label = address
        icon_image =  'bluetooth_nc.png'
        if info['connected']:
            if info['paired']:
                icon_image  = 'bluetooth_cap.png'
            else:
                icon_image = 'bluetooth_cnp.png'

        item        = xbmcgui.ListItem(label)

        item.setIconImage(icon_image)

        item.setProperty('address'  , address)
        item.setProperty('alias'    , info['alias'])
        item.setProperty('connected', str(info['connected']))

        return item


    def populate_bluetooth_dict(self, paired):

        devices         = {}
        bluetooth_dict  = {}

        devices         = osmc_bluetooth.list_trusted_devices() if paired else osmc_bluetooth.list_discovered_devices()
            
        for address in devices.keys():

            bluetooth_dict[address] = {
                                        'alias'     : osmc_bluetooth.get_device_property(address, 'Alias'), 
                                        'paired'    :     osmc_bluetooth.get_device_property(address, 'Paired'),
                                        'connected' :     osmc_bluetooth.get_device_property(address, 'Connected'),
                                        'trusted'   :     osmc_bluetooth.get_device_property(address, 'Trusted'),
                                       }
        return bluetooth_dict


    def sort_alias(self, itm):

        try:
            metric = int(itm.getProperty('alias'))
        except:
            metric = 0

        return metric


class wifi_scanner_bot(threading.Thread):

    def __init__(self):

        super(wifi_scanner_bot, self).__init__(name=WIFI_SCAN_THREAD_NAME)

    def run(self):
        
        osmc_network.scan_wifi()



class wifi_populate_bot(threading.Thread):

    def __init__(self, scan, wifi_list_control,conn_ssid):

        super(wifi_populate_bot, self).__init__(name=WIFI_THREAD_NAME)
        
        self.scan      = scan
        self.WFP       = wifi_list_control
        self.conn_ssid = conn_ssid 
        self.exit      = False
        self.wifis     = []
 
        self.current_network_config = None

        if self.scan:
            self.wifi_scanner_bot = wifi_scanner_bot()
            self.wifi_scanner_bot.setDaemon(True)
            self.wifi_scanner_bot.start()


    def run(self):

        running_dict = {}
        runs         = 0

        while not self.exit:
            # only run the network check every 2 seconds, but allow the exit command to be checked every 10ms
            if runs % 200 == 0 and not self.exit:

                log('Updating Wifi networks')
                wifis = osmc_network.get_wifi_networks()

                running_dict.update(wifis)

                self.update_list_control(running_dict, len(wifis.keys()) > 1)


             # every minute re-scan wifi unless the thread has been asked to exit
            if not self.exit and runs % 600 == 0:

                self.wifi_scanner_bot = wifi_scanner_bot()
                self.wifi_scanner_bot.setDaemon(True)
                self.wifi_scanner_bot.start()

            xbmc.sleep(10)
            runs += 1


    def stop_thread(self):
        
        self.exit = True


    def update_list_control(self, running_dict, multiAdpter):

        items_to_be_removed = []

        for itemIndex in range(0, self.WFP.size()):

            listItem               = self.WFP.getListItem(itemIndex)
            listItemAdapterAddress = listItem.getProperty('AdapterAddress')
            listItemSSID           = listItem.getProperty('SSID')
            listItemConnected      = listItem.getProperty('Connected') == 'True'
            
            if listItemAdapterAddress in running_dict.keys():
                    if listItemSSID in running_dict[listItemAdapterAddress].keys():

                        wifi             = running_dict[listItemAdapterAddress][listItemSSID]
                        networkConnected = wifi['State'] in ('ready', 'online')
                        expectedLabel    = self.getListItemLabel(wifi, multiAdpter)

                        if listItem.getLabel() == expectedLabel and listItemConnected == networkConnected:

                            running_dict[listItemAdapterAddress].pop(listItemSSID)

                        else:
                            items_to_be_removed.append(itemIndex)
                    else:
                        items_to_be_removed.append(itemIndex)
            else:
                items_to_be_removed.append(itemIndex)

        for itemIndex in reversed(items_to_be_removed):
            self.WFP.removeItem(itemIndex)

        if len(running_dict.keys()) > 0:
            for adapterAddress, wifis in running_dict.iteritems():

                for ssid, info in wifis.iteritems():
                    self.WFP.addItem(self.convert_wifi_to_listitem(info, multiAdpter))
                    connected = True if info['State'] in ('ready', 'online') else False

                    if connected and ssid == self.conn_ssid:
                        self.WFP.selectItem(self.WFP.size() - 1)
                        

    def getListItemLabel(self, wifi, multiAdapter):

        ssid      = wifi['SSID']
        interface = wifi['Interface']

        if multiAdapter:
            return ssid + ' (' + interface + ')'
        else:
            return ssid


    def convert_wifi_to_listitem(self, wifi, multiAdapter):

        # {'path': str(path),
        # 'SSID': str(dbus_properties['Name']),
        # 'Strength': int(dbus_properties['Strength']),
        # 'State' : str(dbus_properties['State'])}

        ssid      = wifi['SSID']
        strength  = wifi['Strength']
        state     = wifi['State']
        path      = wifi['path']
        encrypted = wifi['Security'] != 'none'
        connected = wifi['State'] in ('ready', 'online')
        address   = wifi['AdapterAddress']

        # icon_tuple = (connected, encrypted, strength)
        icon_image = self.get_wifi_icon(encrypted, strength, connected)

        item = xbmcgui.ListItem(self.getListItemLabel(wifi, multiAdapter))
        
        item.setIconImage(icon_image)

        item.setProperty('Strength', str(strength))
        item.setProperty('Encrypted', str(encrypted))
        item.setProperty('Path', path)
        item.setProperty('Connected', str(connected))
        item.setProperty('SSID', ssid)
        item.setProperty('AdapterAddress', address)

        if connected:
            self.conn_ssid = ssid

        return item
        

    @staticmethod
    def get_wifi_icon(encrypted, strength, connected):

        filename = ''

        filename += 'c' if connected else 'nc'

        filename += 'e' if encrypted else 'ne'

        if   strength >= 0  and strength <= 40:     filename += '020'
        elif strength >= 41 and strength <= 60:     filename += '6080'
        elif strength >= 61 and strength <= 100:    filename += '80100'

        return filename + ".png"
