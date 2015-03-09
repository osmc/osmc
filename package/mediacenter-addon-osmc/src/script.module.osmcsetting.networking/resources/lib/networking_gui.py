# Standard Modules
from collections import namedtuple, OrderedDict
import socket
import sys
import os
import os.path
import subprocess
import time


# XBMC Modules
import xbmcaddon
import xbmcgui
import xbmc

__addon__ = xbmcaddon.Addon('script.module.osmcsetting.networking')
DIALOG = xbmcgui.Dialog()


# Custom modules
sys.path.append(xbmc.translatePath(os.path.join(__addon__.getAddonInfo('path'), 'resources', 'lib')))

import osmc_bluetooth
import osmc_network


def log(message):
    xbmc.log(str(message), level=xbmc.LOGDEBUG)


def lang(id):
    san = __addon__.getLocalizedString(id).encode('utf-8', 'ignore')
    return san


gui_ids = { \
 \
    1000: 'Header Group list',
    101: 'Wired Network',
    102: 'Wireless Network',
    103: 'Bluetooth',
    10111: 'Wired - Manual/ DHCP toggle',
    10112: 'Wired - IP Address',
    910112: 'Wired - IP Address VALUE',
    10113: 'Wired - Subnet Mask',
    910113: 'Wired - Subnet Mask VALUE',
    10114: 'Wired - Default Gateway',
    910114: 'Wired - Default Gateway VALUE',
    10115: 'Wired - Primary DNS',
    910115: 'Wired - Primary DNS VALUE',
    10116: 'Wired - Secondary DNS',
    910116: 'Wired - Secondary DNS VALUE',
    10118: 'Wired - Apply',
    10119: 'Wired - Reset',
    10120: 'Wired - Enable Adapter',
    10220: 'Wireless - Scan for connections',
    10211: 'Wireless - Automatically configure the network toggle',
    10212: 'Wireless - IP Address',
    910212: 'Wireless - IP Address VALUE',
    10213: 'Wireless - Subnet Mask',
    910213: 'Wireless - Subnet Mask VALUE',
    10214: 'Wireless - Default Gateway',
    910214: 'Wireless - Default Gateway VALUE',
    10215: 'Wireless - Primary DNS',
    910215: 'Wireless - Primary DNS VALUE',
    10216: 'Wireless - Secondary DNS',
    910216: 'Wireless - Secondary DNS VALUE',
    10217: 'Wireless - Enable Adapter',
    10218: 'Wireless - Apply',
    10219: 'Wireless - Reset',
    10300: 'Bluetooth - Refresh',
    10301: 'Bluetooth - Toggle Bluetooth Adapter',
    10303: 'Bluetooth - Toggle Discovery',
    5000: 'WiFi panel',
    6000: 'Bluetooth paired devices panel',
    7000: 'Bluetooth discoverd devices panel'

}

ip_controls = [10112, 10113, 10114, 10115, 10116, 910112, 910113, 910114, 910115, 910116, 10212, 10213, 10214, 10215
    , 10216, 910212, 910213, 910214, 910215, 910216, ]


WIRED_NETWORK_SELECTOR = 101
WIRELESS_NETWORK_SELECTOR = 102
BLUETOOTH_SELECTOR = 103
MAIN_MENU = [WIRED_NETWORK_SELECTOR, WIRELESS_NETWORK_SELECTOR, BLUETOOTH_SELECTOR]

#panel_controls = [1010, 1020, 1030]

STATUS_LABEL = 81000

BLUETOOTH_CONTROLS = [10300, 10301, 10303, 6000, 7000]

BLUETOOTH_ENABLE_TOGGLE = 10301

ALL_WIRED_CONTROLS = [10111, 10112, 10113, 10114, 10115, 10116, 10118, 10119,  910112, 910113, 910114,
                      910115, 910116]

WIRED_IP_VALUES = [910112, 910113, 910114, 910115, 910116]

WIRED_IP_LABELS = [10112, 10113, 10114, 10115, 10116]


WIRED_APPLY_BUTTON = 10118

WIRED_RESET_BUTTON = 10119

WIRED_DHCP_MANUAL_BUTTON = 10111

WIRED_IP_LABELS = [10112, 10113, 10114, 10115, 10116]

WIRED_ADAPTER_TOGGLE = 10120

WIRED_APPLY_BUTTON = 10118

WIRED_RESET_BUTTON = 10119

WIRED_DHCP_MANUAL_BUTTON = 10111

ALL_WIRELESS_CONTROLS = [5000, 910212, 910213, 910214, 910215, 910216, 10211, 10212, 10213, 10214, 10215, 10216,
                         10218, 10219, 10220]

WIRELESS_IP_VALUES = [910212, 910213, 910214, 910215, 910216]

WIRELESS_IP_LABELS = [10212, 10213, 10214, 10215, 10216]

WIRELESS_SCAN_BUTTON = 10220

WIRELESS_ADAPTER_TOGGLE = 10217

WIRELESS_APPLY_BUTTON = 10218

WIRELESS_RESET_BUTTON = 10219

WIRELESS_DHCP_MANUAL_BUTTON = 10211

WIRELESS_NETWORKS = 5000

password = None


class networking_gui(xbmcgui.WindowXMLDialog):
    current_network_config = {}  # holds the current network config

    reboot_required_file = '/tmp/.reboot-needed'

    use_preseed = False

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
        self.wifis = []

        # connected SSID, the ssid we are currently connected to
        self.conn_ssid = None

        # list containing list items of all paired bluetooth devices
        self.paired_bluetooths = []

        # list containing list items of all discovered bluetooth devices
        self.discovered_bluetooths = []

        self.preseed_data = None

        self.status_label = None

    def onInit(self):
         # Wired Network Label
        self.status_label = self.getControl(STATUS_LABEL);

        # wifi panel (WFP)
        self.WFP = self.getControl(5000)

        # bluetooth paired device panel (BTP)
        self.BTP = self.getControl(6000)

        # bluetooth discovered device panel (BTD)
        self.BTD = self.getControl(7000)

        panel_to_show = WIRED_NETWORK_SELECTOR
        if self.use_preseed and not osmc_network.get_nfs_ip_cmdline_value():
            self.preseed_data = osmc_network.parse_preseed()
            if self.preseed_data:
                if self.preseed_data['Interface'].startswith('wlan') and osmc_network.is_wifi_available():
                    panel_to_show = WIRELESS_NETWORK_SELECTOR
                else:
                    panel_to_show = WIRED_NETWORK_SELECTOR

        # set all the panels to invisible except the first one
        for ctl in MAIN_MENU:
                self.getControl(ctl * 10).setVisible(True if ctl == panel_to_show else False)
        if panel_to_show == WIRED_NETWORK_SELECTOR:
            self.populate_wired_panel()
        if panel_to_show == WIRELESS_NETWORK_SELECTOR:
            self.populate_wifi_panel(False)

        if self.use_preseed and not osmc_network.get_nfs_ip_cmdline_value():
            self.setup_networking_from_preseed()

    def setup_networking_from_preseed(self):
        wired = False
        connected = False

        if self.preseed_data['Interface'].startswith('wlan') and osmc_network.is_wifi_available():
            if not osmc_network.is_wifi_enabled():
                self.toggle_wifi()
            ssid = self.preseed_data['SSID']
            encrypted = False
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
                self.current_network_config['Method'] = 'dhcp'
            else:
                self.current_network_config['Method'] = 'manual'
                self.current_network_config['Address'] = self.preseed_data['Address']
                self.current_network_config['Netmask'] = self.preseed_data['Netmask']
                if 'Gateway' in self.preseed_data:
                    self.current_network_config['Gateway'] = self.preseed_data['Gateway']
                if 'DNS_1' in self.preseed_data:
                    self.current_network_config['DNS_1'] = self.preseed_data['DNS_1']
                if 'DNS_2' in self.preseed_data:
                    self.current_network_config['DNS_2'] = self.preseed_data['DNS_2']

            osmc_network.apply_network_changes(self.current_network_config)


        if wired:
            self.populate_wired_panel()
        else:
            self.populate_wifi_panel(False)

    def onClick(self, controlID):
        if controlID in ip_controls:
            self.edit_ip_address(controlID)

    def onAction(self, action):
        actionID = action.getId()
        focused_control = self.getFocusId()

        log('actionID = ' + str(actionID))
        log('focused_control = %s,   %s' % (type(focused_control), focused_control))

        if actionID in (10, 92):
            self.close()

        if focused_control in MAIN_MENU:
            # change to the required settings panel

            for ctl in MAIN_MENU:
                self.getControl(ctl * 10).setVisible(True if ctl == focused_control else False) 

            if focused_control != self.current_panel:

                self.current_panel = focused_control

                if focused_control == 101:
                    self.populate_wired_panel()

                elif focused_control == 102:
                    self.populate_wifi_panel()

                elif focused_control == 103:
                 self.populate_bluetooth_panel()

        if actionID == 7:  # Selected
            if focused_control in BLUETOOTH_CONTROLS:
                self.handle_bluetooth_selection(focused_control)
                self.populate_bluetooth_panel()

            if focused_control in ALL_WIRED_CONTROLS:
                self.handle_wired_selection(focused_control)
            if focused_control == WIRED_ADAPTER_TOGGLE:
                self.handle_wired_selection(focused_control)

            if focused_control in ALL_WIRELESS_CONTROLS:
                self.handle_wireless_selection(focused_control)
            if focused_control == WIRELESS_ADAPTER_TOGGLE:
                self.handle_wireless_selection(focused_control)


    def edit_ip_address(self, controlID):
        relevant_label_control = self.getControl(900000 + controlID)
        current_label = relevant_label_control.getLabel()

        if current_label == '___ : ___ : ___ : ___':
            current_label = ''

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

            #ip_string = ' : '.join(str(user_input).split('.'))
            relevant_label_control.setLabel(user_input)

            return

    def toggle_controls(self, enabled, control_ids):
        for control_id in control_ids:
            control = self.getControl(control_id)
            control.setEnabled(enabled)

    def get_wired_config(self):
        return osmc_network.get_ethernet_settings()

    def populate_wired_panel(self):
        if os.path.isfile(self.reboot_required_file):
            # 'NFS Network Settings'
            # 'The displayed network configuration may be out dated - A reboot is recommended before proceeding'
            DIALOG.ok(lang(32036), lang(32038))
        # Clear wired network Panel
        if osmc_network.is_ethernet_enabled():
            self.current_network_config = self.get_wired_config()
            if self.current_network_config:
                self.toggle_controls(True, ALL_WIRED_CONTROLS)

                interface = self.current_network_config['Interface']

                if self.current_network_config['State'] in ('online'):
                    #         'Status'                               'Connected'
                    status = lang(32044) + ': ' + interface + ' (' + lang(32046) + ')'
                elif self.current_network_config['State'] in ('ready'):
                    #         'Status'                               'No internet'
                    status = lang(32044) + ': ' + interface + ' (' + lang(32047) + ')'
                else:
                    #                 'Status'
                    status = status = lang(32044) + ' ' + interface
                self.status_label.setLabel(status)
                self.update_manual_DHCP_button(WIRED_DHCP_MANUAL_BUTTON, WIRED_IP_VALUES, WIRED_IP_LABELS)

                self.populate_ip_controls(self.current_network_config, WIRED_IP_VALUES)

                # enable reset and apply button
                self.update_apply_reset_button('WIRED')

            else:  # no wired connection
                self.toggle_controls(False, ALL_WIRED_CONTROLS)
                #                 'Status'     'no wired connection'
                status = lang(32044) + ': ' + lang(32049)
                self.status_label.setLabel(status)
        else:  # Disabled
                self.toggle_controls(False, ALL_WIRED_CONTROLS)
                #                 'Status'     'disabled'
                status = lang(32044) + ': ' + lang(32048)
                self.status_label.setLabel(status)
                self.update_apply_reset_button('WIRED')

        adapterRadioButton = self.getControl(WIRED_ADAPTER_TOGGLE)
        adapterRadioButton.setSelected(osmc_network.is_ethernet_enabled())
        adapterRadioButton.setEnabled(True)

    def update_manual_DHCP_button(self, button_id, ip_values, ip_labels):
        manualDHCPButton = self.getControl(button_id)
        if 'dhcp' in self.current_network_config['Method']:
            # 'Configure Network Using DHCP'
            manualDHCPButton.setLabel(lang(32006))
            # if configuration is by DHCP disable controls
            self.toggle_controls(False, ip_values)
            self.toggle_controls(False, ip_labels)
        else:
            # 'Configure Network Manually'
            manualDHCPButton.setLabel(lang(32033))
            self.toggle_controls(True, ip_values)
            self.toggle_controls(True, ip_labels)

    def populate_ip_controls(self, settings_dict, controls):
        ip_address = self.getControl(controls[0])
        ip_address.setLabel(settings_dict['Address'])
        subnet = self.getControl(controls[1])
        subnet.setLabel(settings_dict['Netmask'])
        defaultGateway = self.getControl(controls[2])
        if settings_dict.has_key('Gateway'):
            defaultGateway.setLabel(settings_dict['Gateway'])
        else:
            defaultGateway.setLabel('')
            defaultGateway.setEnabled(False)
        primaryDNS = self.getControl(controls[3])
        primaryDNS.setLabel(settings_dict['DNS_1'])
        secondaryDNS = self.getControl(controls[4])
        if settings_dict.has_key('DNS_2'):
            secondaryDNS.setLabel(settings_dict['DNS_2'])
        else:
            secondaryDNS.setLabel('')

    def clear_ip_controls(self, controls):
        ip_address = self.getControl(controls[0])
        ip_address.setLabel('')
        subnet = self.getControl(controls[1])
        subnet.setLabel('')
        defaultGateway = self.getControl(controls[2])
        defaultGateway.setLabel('')
        primaryDNS = self.getControl(controls[3])
        primaryDNS.setLabel('')
        secondaryDNS = self.getControl(controls[4])
        secondaryDNS.setLabel('')
        self.toggle_controls(False, controls)

    def update_current_ip_settings(self, controls):
        ip_address = self.getControl(controls[0])
        self.current_network_config['Address'] = ip_address.getLabel()
        subnet = self.getControl(controls[1])
        self.current_network_config['Netmask'] = subnet.getLabel()
        defaultGateway = self.getControl(controls[2])
        self.current_network_config['Gateway'] = defaultGateway.getLabel()
        primaryDNS = self.getControl(controls[3])
        self.current_network_config['DNS_1'] = primaryDNS.getLabel()
        secondaryDNS = self.getControl(controls[4])
        if secondaryDNS.getLabel():
            self.current_network_config['DNS_2'] = secondaryDNS.getLabel()
        log(self.current_network_config)

    def handle_wired_selection(self, control_id):
        if control_id == WIRED_DHCP_MANUAL_BUTTON:
            if self.current_network_config['Method'] == 'dhcp':
                self.current_network_config['Method'] = 'manual'
            elif self.current_network_config['Method'] == 'manual':
                self.current_network_config['Method'] = 'dhcp'
            elif self.current_network_config['Method'] == 'nfs_dhcp':
                self.current_network_config['Method'] = 'nfs_manual'
            elif self.current_network_config['Method'] == 'nfs_manual':
                self.current_network_config['Method'] = 'nfs_dhcp'
            self.update_manual_DHCP_button(WIRED_DHCP_MANUAL_BUTTON, WIRED_IP_VALUES, WIRED_IP_LABELS)

        if control_id == WIRED_RESET_BUTTON:
            self.current_network_config = self.get_wired_config()
            self.populate_ip_controls(self.current_network_config, WIRED_IP_VALUES)
            self.update_manual_DHCP_button(WIRED_DHCP_MANUAL_BUTTON, WIRED_IP_VALUES, WIRED_IP_LABELS)
            self.setFocusId(WIRED_DHCP_MANUAL_BUTTON)

        if control_id == WIRED_APPLY_BUTTON:
            osmc_network.apply_network_changes(self.current_network_config)
            if self.current_network_config['Method'] in ['nfs_dhcp', 'nfs_manual']:
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

        if control_id in WIRED_IP_LABELS:
            self.update_current_ip_settings(WIRED_IP_VALUES)
        self.update_apply_reset_button('WIRED')

    def toggle_ethernet(self):
        self.toggle_controls(False, ALL_WIRED_CONTROLS)
        #                          'Status'              'Configuring...'
        self.status_label.setLabel(lang(32044) + ' : ' + lang(32016))
        osmc_network.toggle_ethernet_state(not osmc_network.is_ethernet_enabled())
        # 10 second wait to allow connman to make the changes before refreshing
        time.sleep(10)

    def update_apply_reset_button(self, type):
        if type == 'WIRED':
            if cmp(self.get_wired_config(), self.current_network_config) == 0 and osmc_network.is_ethernet_enabled():
                self.toggle_controls(False, [WIRED_RESET_BUTTON, WIRED_APPLY_BUTTON])
            else:
                self.toggle_controls(True, [WIRED_RESET_BUTTON, WIRED_APPLY_BUTTON])
        if type == 'WIRELESS':
            wireless_config = self.get_wireless_config(self.conn_ssid)
            if cmp(wireless_config, self.current_network_config) == 0 and osmc_network.is_wifi_enabled():
                self.toggle_controls(False, [WIRELESS_RESET_BUTTON, WIRELESS_APPLY_BUTTON])
            else:
                self.toggle_controls(True, [WIRELESS_RESET_BUTTON, WIRELESS_APPLY_BUTTON])

    def get_wireless_config(self, ssid):
        if ssid is not None and ssid in osmc_network.get_wifi_networks():
            config = osmc_network.get_wifi_networks()[ssid]
            if self.wireless_password:
                config['Password'] = self.wireless_password
            return config
        return {}

    def populate_wifi_panel(self, scan=False):
        # remove everything from the existing panel
        self.WFP.reset()
        if osmc_network.is_wifi_available():
            if osmc_network.is_wifi_enabled():
                self.populate_wifi_networks(scan)
                if self.conn_ssid:
                    self.update_manual_DHCP_button(WIRELESS_DHCP_MANUAL_BUTTON, WIRELESS_IP_VALUES, WIRELESS_IP_LABELS)
                    self.populate_ip_controls(self.current_network_config, WIRELESS_IP_VALUES)
                    self.toggle_controls(True, [WIRELESS_ADAPTER_TOGGLE, WIRELESS_SCAN_BUTTON, WIRELESS_NETWORKS,
                                                WIRELESS_DHCP_MANUAL_BUTTON ])
                else:# not connected to a network
                    self.toggle_controls(False, ALL_WIRELESS_CONTROLS)
                    self.toggle_controls(True, [WIRELESS_ADAPTER_TOGGLE, WIRELESS_SCAN_BUTTON, WIRELESS_NETWORKS])
                    self.clear_ip_controls(WIRELESS_IP_VALUES)

            else:# Wifi disabled
                 self.toggle_controls(False, ALL_WIRELESS_CONTROLS)

            adapterRadioButton = self.getControl(WIRELESS_ADAPTER_TOGGLE)
            adapterRadioButton.setSelected(osmc_network.is_wifi_enabled())
            adapterRadioButton.setEnabled(True)

        else:# Wifi not available
            self.toggle_controls(False, ALL_WIRELESS_CONTROLS)

    def populate_wifi_networks(self, scan):
        self.wifis = []
        self.conn_ssid = None
        # remove everything from the existing panel
        self.WFP.reset()
        # add a item ro show we are scanning
        if scan:
            scanningListItem = xbmcgui.ListItem(lang(32040))
            self.WFP.addItems([scanningListItem])
            osmc_network.scan_wifi()

        wifi_networks = osmc_network.get_wifi_networks()
        for ssid in wifi_networks.keys():
            info = wifi_networks[ssid]
            itm = xbmcgui.ListItem(ssid)
            strength = info['Strength']
            # icon_tuple = (connected, encrypted, strength)
            if info['Security'] == 'none':
                encrypted = False
            else:
                encrypted = True
            connected = False
            if not info['State'] == 'idle':
                connected = True
                self.conn_ssid = ssid
                self.current_network_config = self.get_wireless_config(ssid)
                log(self.current_network_config)
                self.populate_ip_controls(info, WIRELESS_IP_VALUES)
            icon_image = self.get_wifi_icon(encrypted, strength / 25, connected)
            itm.setIconImage(icon_image)
            itm.setProperty('Strength', str(strength))
            itm.setProperty('Encrypted', str(encrypted))
            itm.setProperty('Path', info['path'])
            itm.setProperty('Connected', str(connected))
            itm.setProperty('SSID', ssid)
            self.wifis.append(itm)

        # sort the list of wifis based on signal strength
        self.wifis.sort(key=self.sort_strength, reverse=True)

        # remove everything from the existing panel
        self.WFP.reset()

        # add the to the panel
        self.WFP.addItems(self.wifis)

        # set the current connection as selected
        for i, wifi in enumerate(self.wifis):
            if wifi.getLabel() == self.conn_ssid:
                self.WFP.selectItem(i)

    def handle_wireless_selection(self, control_id):
        if control_id == 5000:  # wireless network
            self.handle_selected_wireless_network()

        if control_id == WIRELESS_SCAN_BUTTON:
            self.populate_wifi_panel(True)

        if control_id == WIRELESS_DHCP_MANUAL_BUTTON:
            if self.current_network_config['Method'] == 'dhcp':
                self.current_network_config['Method'] = 'manual'
            elif self.current_network_config['Method'] == 'manual':
                self.current_network_config['Method'] = 'dhcp'
            self.update_manual_DHCP_button(WIRELESS_DHCP_MANUAL_BUTTON, WIRELESS_IP_VALUES, WIRELESS_IP_LABELS)

        if control_id == WIRELESS_RESET_BUTTON:
            self.current_network_config = self.get_wireless_config(self.conn_ssid)
            self.update_manual_DHCP_button(WIRELESS_DHCP_MANUAL_BUTTON, WIRELESS_IP_VALUES, WIRELESS_IP_LABELS)
            self.populate_wifi_networks(False)
            self.setFocusId(WIRELESS_DHCP_MANUAL_BUTTON)

        if control_id == WIRELESS_APPLY_BUTTON:
            print self.current_network_config
            osmc_network.apply_network_changes(self.current_network_config)
            self.populate_wifi_panel()
            self.setFocusId(WIRELESS_DHCP_MANUAL_BUTTON)

        if control_id == WIRELESS_ADAPTER_TOGGLE:
            self.toggle_wifi()
            self.populate_wifi_panel()

        if control_id in WIRELESS_IP_LABELS:
            self.update_current_ip_settings(WIRELESS_IP_VALUES)

        self.update_apply_reset_button('WIRELESS')

    def toggle_wifi(self):
        self.WFP.reset()
        item = xbmcgui.ListItem(lang(32016))
        self.toggle_controls(False, ALL_WIRELESS_CONTROLS)
        self.WFP.addItem(item)
        osmc_network.toggle_wifi_state(not osmc_network.is_wifi_enabled())
        # 5 second wait to allow connman to make the changes before refreshing
        time.sleep(5)
        if not osmc_network.is_wifi_enabled():
            self.current_network_config = {}

    def handle_selected_wireless_network(self):
        item = self.WFP.getSelectedItem()
        path = item.getProperty('Path')
        encrypted = item.getProperty('Encrypted')
        connected = item.getProperty('Connected')
        ssid = item.getProperty('SSID')
        if connected == 'False' and not self.conn_ssid:
            self.connect_to_wifi(ssid, encrypted)
        else:
            if ssid == self.conn_ssid:
                # 'Wireless' , 'Disconnect from'
                if DIALOG.yesno(lang(32041), lang(32042) + ' ' + self.conn_ssid + '?'):
                    self.conn_ssid = None
                    self.wireless_password = None
                    self.current_network_config = {}
                    osmc_network.wifi_disconnect(path)
                    osmc_network.wifi_remove(path)
                    self.current_network_config = {'Method' : 'dhcp'} # force manual/dhcp button back to auto
                    self.update_manual_DHCP_button(WIRELESS_DHCP_MANUAL_BUTTON, WIRELESS_IP_VALUES, WIRELESS_IP_LABELS)
                    self.current_network_config = {}
                    self.clear_ip_controls(WIRELESS_IP_VALUES)
                    self.toggle_controls(False, [WIRED_DHCP_MANUAL_BUTTON])
        osmc_network.update_preseed_file(self.current_network_config)
        self.populate_wifi_networks(False)

    def connect_to_wifi(self, ssid, encrypted, password=None, scan=False):
        self.WFP.reset()
        if scan:
            scanningListItem = xbmcgui.ListItem(lang(32040))
            self.WFP.addItems([scanningListItem])
            osmc_network.scan_wifi()
        if ssid in osmc_network.get_wifi_networks():
            self.WFP.reset()
            # 'Configuring'
            item = xbmcgui.ListItem(lang(32016))
            self.WFP.addItem(item)
            path = osmc_network.get_wifi_networks()[ssid]['path']
            connection_status = False
            if encrypted == 'False':
                if DIALOG.yesno(lang(32041), lang(32042) + ' ' + ssid + '?'):
                    connection_status = osmc_network.wifi_connect(path)
            else:
                if password is None:
                    password = DIALOG.input(lang(32013), type=xbmcgui.INPUT_ALPHANUM, option=xbmcgui.ALPHANUM_HIDE_INPUT)
                if password:
                    self.wireless_password = password
                    connection_status = osmc_network.wifi_connect(path, password)
            if not connection_status:
                #      'Connection to '                  'failed'
                message = lang(32043) + ' ' + ssid + ' ' + lang(32025)
                #                                                   'Wireless'
                xbmc.executebuiltin("XBMC.Notification(%s,%s,%s)" % (lang(32041), message, "2500"))
                osmc_network.wifi_remove(path)
                self.current_network_config = {}
                self.clear_ip_controls(WIRELESS_IP_VALUES)
                self.toggle_controls(False, [WIRELESS_DHCP_MANUAL_BUTTON])
                return False
            else:
                self.current_network_config = self.get_wireless_config(ssid)
                self.toggle_controls(True, ALL_WIRELESS_CONTROLS)
                self.update_manual_DHCP_button(WIRELESS_DHCP_MANUAL_BUTTON, WIRELESS_IP_VALUES, WIRELESS_IP_LABELS)
                self.populate_ip_controls(self.current_network_config, WIRELESS_IP_VALUES)
                self.toggle_controls(True, [WIRELESS_ADAPTER_TOGGLE, WIRELESS_SCAN_BUTTON, WIRELESS_NETWORKS,
                                            WIRELESS_DHCP_MANUAL_BUTTON ])
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

    def get_wifi_icon(self, encrypted, strength, connected):
        icon_tuple = (connected, encrypted, strength)
        icons = {
            (True, True, 0): 'bar0_ce.png',
            (True, True, 1): 'bar1_ce.png',
            (True, True, 2): 'bar2_ce.png',
            (True, True, 3): 'bar3_ce.png',
            (True, True, 4): 'bar4_ce.png',
            (True, False, 0): 'bar0_cx.png',
            (True, False, 1): 'bar1_cx.png',
            (True, False, 2): 'bar2_cx.png',
            (True, False, 3): 'bar3_cx.png',
            (True, False, 4): 'bar4_cx.png',
            (False, True, 0): 'bar0_xe.png',
            (False, True, 1): 'bar1_xe.png',
            (False, True, 2): 'bar2_xe.png',
            (False, True, 3): 'bar3_xe.png',
            (False, True, 4): 'bar4_xe.png',
            (False, False, 0): 'bar0_xx.png',
            (False, False, 1): 'bar1_xx.png',
            (False, False, 2): 'bar2_xx.png',
            (False, False, 3): 'bar3_xx.png',
            (False, False, 4): 'bar4_xx.png',
        }

        return icons.get(icon_tuple, 'bar0_xx.png')

    def populate_bluetooth_panel(self):
        """
                Populates the bluetooth panel
        """
        # clear discovered and paired lists
        self.BTD.reset()
        self.BTP.reset()

        # enable all controls
        self.toggle_controls(True, BLUETOOTH_CONTROLS)
        # disable all if bluetooth is not detected
        if not osmc_bluetooth.is_bluetooth_available():
            self.toggle_controls(False, BLUETOOTH_CONTROLS)
            return
        else:
            if not osmc_bluetooth.is_bluetooth_enabled():
                controls_to_disable = list(BLUETOOTH_CONTROLS)
                controls_to_disable.remove(BLUETOOTH_ENABLE_TOGGLE)
                self.toggle_controls(False, controls_to_disable)
                return

        bluetoothRadioButton = self.getControl(10301)
        bluetoothRadioButton.setSelected(osmc_bluetooth.is_bluetooth_enabled())
        discoveryRadioButton = self.getControl(10303)
        discoveryRadioButton.setSelected(osmc_bluetooth.is_discovering())

        bluetooth_paired_dict = self.populate_bluetooth_dict(True)
        bluetooth_discovered_dict = self.populate_bluetooth_dict(False)

        # add paired devices to the list
        self.paired_bluetooths = list(self.create_bluetooth_items(bluetooth_paired_dict))
        # sort the list of paired based on alias
        self.paired_bluetooths.sort(key=self.sort_alias)
        # remove everything from the existing panel
        self.BTP.reset()
        # add the paired devices
        self.BTP.addItems(self.paired_bluetooths)

        # add discovered devices to the list
        self.discovered_bluetooths = list(self.create_bluetooth_items(bluetooth_discovered_dict))
        # sort the list of discovered based on alias
        self.discovered_bluetooths.sort(key=self.sort_alias)
        # remove everything from the existing panel
        self.BTD.reset()
        # add the paired devices
        self.BTD.addItems(self.discovered_bluetooths)

    def handle_bluetooth_selection(self, control_id):
        # 10300 - No Action Here - Refresh - populate_bluetooth_panel() will be called by calling code
        if control_id == 10301:  # Enable Bluetooth
            osmc_bluetooth.toggle_bluetooth_state(not osmc_bluetooth.is_bluetooth_enabled())

        if control_id == 10303:  # Discovery
            if not osmc_bluetooth.is_discovering():
                osmc_bluetooth.start_discovery()
            else:
                osmc_bluetooth.stop_discovery()

        if control_id == 6000:  # paired devices
            item = self.BTP.getSelectedItem()
            address = item.getProperty('address')
            alias = item.getProperty('alias')
            # 'Bluetooth' , 'Remove Device'
            if DIALOG.yesno(lang(32020), lang(32021) + ' ' + alias + '?'):
                osmc_bluetooth.remove_device(address)

        if control_id == 7000:  # Discovered devices
            item = self.BTD.getSelectedItem()
            address = item.getProperty('address')
            alias = item.getProperty('alias')
            # 'Bluetooth' , 'Pair with Device'
            if DIALOG.yesno(lang(32020), lang(32022) + ' ' + alias + '?'):
                script_base_path = os.path.join(__addon__.getAddonInfo('path'), 'resources', 'lib') + '/'
                result = osmc_bluetooth.pair_device(address, script_base_path)
                if result:
                    # 'Paired Sucessfully with'
                    message = lang(32023) + ' ' + alias
                else:
                    # 'Pairing with'         'failed'
                    message = lang(32024) + alias + lang(320025)
                # 'Bluetooth'
                xbmc.executebuiltin("XBMC.Notification(%s,%s,%s)" % (lang(32020), message, "2500"))

    def create_bluetooth_items(self, bluetooth_dict):
        items = []
        for address, info in bluetooth_dict.iteritems():
            label = address
            if info['alias']:
                label = info['alias']
            itm = xbmcgui.ListItem(label)
            icon_image = 'disconnected.png'
            if info['connected']:
                icon_image = 'connected.png'
            itm.setIconImage(icon_image)
            itm.setProperty('address', address)
            itm.setProperty('alias', info['alias'])
            items.append(itm)
        return items

    def populate_bluetooth_dict(self, paired):
        devices = {}
        if paired:
            devices = osmc_bluetooth.list_paired_devices()
        else:
            devices = osmc_bluetooth.list_discovered_devices()
        bluetooth_dict = {}
        for address in devices.keys():
            alias = str(osmc_bluetooth.get_device_property(address, 'Alias'))
            paired = osmc_bluetooth.get_device_property(address, 'Paired')
            connected = osmc_bluetooth.get_device_property(address, 'Connected')
            bluetooth_dict[address] = {'alias': alias, 'paired': paired, 'connected': connected}
            log(bluetooth_dict[address])
        return bluetooth_dict

    def sort_alias(self, itm):
        try:
            metric = int(itm.getProperty('alias'))
        except:
            metric = 0

        return metric

