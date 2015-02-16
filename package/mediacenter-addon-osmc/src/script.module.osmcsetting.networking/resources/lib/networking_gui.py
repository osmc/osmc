# Standard Modules
from collections import namedtuple, OrderedDict
import socket
import sys
import os
import os.path
import subprocess
import time

import random  # farts, this is needed for testing only
import string  # farts, this is needed for testing only

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
    1: 'Headings -- Wired Network - Wireless Network - Bluetooth - Tethering(X) - VPN(X)',
    1010: 'Panel Wired Network',
    1020: 'Panel Wireless Network',
    1030: 'Panel Bluetooth',
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
    10117: 'Wired - Network pannl',
    10118: 'Wired - Apply',
    10119: 'Wired - Reset',
    10200: 'Wireless - Scan for connections',
    10211: 'Wireless - Automatically configure the network toggle',
    10214: 'Wireless - Default Gateway',
    910214: 'Wireless - Default Gateway VALUE',
    10212: 'Wireless - IP Address',
    910212: 'Wireless - IP Address VALUE',
    10215: 'Wireless - Primary DNS',
    910215: 'Wireless - Primary DNS VALUE',
    10216: 'Wireless - Secondary DNS',
    910216: 'Wireless - Secondary DNS VALUE',
    10213: 'Wireless - Subnet Mask',
    910213: 'Wireless - Subnet Mask VALUE',
    10300: 'Bluetooth - Refresh',
    10301: 'Bluetooth - Toggle Bluetooth Adapter',
    10302: 'Bluetooth - Toggle Bluetooth Service',
    10303: 'Bluetooth - Toogle Discovery',
    5000: 'WiFi panel',
    6000: 'Bluetooth paired devices panel',
    7000: 'Bluetooth discoverd devices panel'

}

ip_controls = [10112, 10113, 10114, 10115, 10116, 910112, 910113, 910114, 910115, 910116, 10212, 10213, 10214, 10215
    , 10216, 910212, 910213, 910214, 910215, 910216, ]

hdg = namedtuple('hdg', ['name', 'lang_id', 'panel_id'])
heading_controls = [
    hdg('wired', 32001, 1010),
    hdg('wireless', 32002, 1020),
    hdg('bluetooth', 32003, 1030),
]

panel_controls = [1010, 1020, 1030]

password_buttons = [10217]

bluetooth_controls = [10300, 10301, 10302, 10303, 6000, 7000]

bluetooth_service_button = 10302

all_wired_controls = [10111, 10112, 10113, 10114, 10115, 10116, 10117, 10118, 10119, 910112, 910113, 910114, 910115, 910116]

wired_ip_controls = [910112, 910113, 910114, 910115, 910116]

wired_ip_labels = [10112, 10113, 10114, 10115, 10116]

wired_apply_button = 10118

wired_reset_button = 10119

wired_dhcp_manual_button = 10111

wired_button_controls = [wired_dhcp_manual_button, wired_apply_button, wired_reset_button]


class networking_gui(xbmcgui.WindowXMLDialog):
    current_network_config = {}  # holds the current network config

    setting_changed = False

    reboot_required_file = '/tmp/.reboot-needed'

    def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):
        self.setting_values = kwargs.get('setting_values', {})

        # this stores the wifi password for sending to connman (or equivalent)
        # [ wired password, wireless password]
        self.password = ['', '']

        # heading control list (HCL)
        self.HCL = None

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
        self.conn_ssid = ''

        # list containing list items of all paired bluetooth devices
        self.paired_bluetooths = []

        # list containing list items of all discovered bluetooth devices
        self.discovered_bluetooths = []

    def onInit(self):

        # heading control list (HCL)
        self.HCL = self.getControl(1)

        # Wired Network List (of one a way of showing connected icon )
        self.WDP = self.getControl(10117)

        # wifi panel (WFP)
        self.WFP = self.getControl(5000)

        # bluetooth paired device panel (BTP)
        self.BTP = self.getControl(6000)

        # bluetooth discovered device panel (BTD)
        self.BTD = self.getControl(7000)

        # populate the heading control list (HCL)
        for heading in heading_controls:
            # farts insert conditional checking of heading necessity here

            tmp = xbmcgui.ListItem(lang(heading.lang_id))
            tmp.setProperty('panel_id', str(heading.panel_id))

            self.HCL.addItem(tmp)

        # set all the panels to invisible except the first one
        self.toggle_panel_visibility(0)

        # set focus on the heading control list
        self.setFocusId(1)

        # Populate the first panel
        self.populate_wired_panel()


    def onClick(self, controlID):
        if controlID in ip_controls:
            self.edit_ip_address(controlID)
        elif controlID in password_buttons:
            # ask the user to enter the password, keyboard pops up, entry is hidden
            pwd = DIALOG.input(lang(32013), type=xbmcgui.INPUT_ALPHANUM, option=xbmcgui.ALPHANUM_HIDE_INPUT)
            if pwd:
                # save the password in memory for sending to connman
                self.password[password_buttons.index(controlID)] = pwd
                # change the password label control to display asterisks
                self.getControl(90000 + controlID).setLabel('*********')

    def onAction(self, action):
        actionID = action.getId()
        focused_control = self.getFocusId()

        log('actionID = ' + str(actionID))
        log('focused_control = %s,   %s' % (type(focused_control), focused_control))

        if actionID in (10, 92):
            self.close()

        if focused_control == 1:
            # change to the required settings panel

            try:
                focused_position = int(xbmc.getInfoLabel('Container(1).Position'))
            except:
                focused_position = 0

            log('focused_position = %s' % focused_position)

            self.toggle_panel_visibility(focused_position)

            if focused_position == 0:
                self.populate_wired_panel()

            if focused_position == 1:
                self.populate_wifi_panel()

            if focused_position == 2:
                self.populate_bluetooth_panel()

        if actionID == 7:  # Selected
            if focused_control in bluetooth_controls:
                self.handle_bluetooth_selection(focused_control)
                self.populate_bluetooth_panel()

            if focused_control in all_wired_controls:
                self.handle_wired_selection(focused_control)


    def toggle_panel_visibility(self, focused_position):
        ''' Takes the focused position in the Heading List Control and sets only the required panel to visible. '''
        target_panel = 0

        # get the required panel
        try:
            target_panel = int(self.HCL.getListItem(focused_position).getProperty('panel_id'))

        except:
            target_panel = 0

        log('target_panel = %s' % target_panel)

        for panel_id in panel_controls:
            self.getControl(panel_id).setVisible(True if target_panel == panel_id else False)


    def edit_ip_address(self, controlID):
        relevant_label_control = self.getControl(900000 + controlID)
        current_label = relevant_label_control.getLabel()

        if current_label == '___.___.___.___':
            current_label = ''

        user_input = DIALOG.input(lang(32004), current_label, type=xbmcgui.INPUT_IPADDRESS)

        if not user_input or  user_input  == '0.0.0.0':

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
            relevant_label_control.setLabel(user_input)

            return


    def toggle_controls(self, enabled, control_ids):
        for control_id in control_ids:
            control = self.getControl(control_id)
            control.setEnabled(enabled)

    def get_wired_config(self):
        # here is where we could parse preseed.cfg if we wanted to
        return osmc_network.get_network_settings()


    def populate_wired_panel(self):
        if os.path.isfile(self.reboot_required_file):
            # 'NFS Network Settings'
            # 'The displayed network configuration may be out dated - A reboot is recommended before proceeding'
            DIALOG.ok(lang(32036), lang(32038))
        self.current_network_config = self.get_wired_config()
        log(self.current_network_config)
        if self.current_network_config: 
            self.toggle_controls(True, all_wired_controls)
            itm = xbmcgui.ListItem(self.current_network_config['Interface'])
            icon_image = 'disconnected.png'
            if self.current_network_config['State'] in ('online'):
                icon_image = 'connected.png'
            if self.current_network_config['State'] in ('ready'):
                icon_image = 'no_internet.png'
            itm.setIconImage(icon_image)
            self.update_manual_DHCP_button(wired_dhcp_manual_button)
            self.populate_ip_controls(self.current_network_config, wired_ip_controls)
            # enable reset and apply button
            self.toggle_controls(False, [wired_reset_button, wired_apply_button])
        else: # no wired connection
            self.setFocusId(1)
            self.toggle_controls(False, all_wired_controls)
            itm = xbmcgui.ListItem('eth0')
            icon_image = 'disconnected.png'
            itm.setIconImage(icon_image)

        # Clear wired network Panel
        self.WDP.reset()
        self.WDP.addItem(itm)


    def update_manual_DHCP_button(self, button_id):
        manualDHCPButton = self.getControl(button_id)
        if 'dhcp' in self.current_network_config['Method']:
            # 'Configure Network Using DHCP'
            manualDHCPButton.setLabel(lang(32006))
            # if configuration is by DHCP disable controls
            self.toggle_controls(False, wired_ip_controls)
            self.toggle_controls(False, wired_ip_labels)

        else:
            # 'Configure Network Manually'
            manualDHCPButton.setLabel(lang(32033))
            self.toggle_controls(True, wired_ip_controls)
            self.toggle_controls(True, wired_ip_labels)


    def populate_ip_controls(self, settings_dict, ip_controls):
        ip_address = self.getControl(ip_controls[0])
        ip_address.setLabel(settings_dict['Address'])
        subnet = self.getControl(ip_controls[1])
        subnet.setLabel(settings_dict['Netmask'])
        defaultGateway = self.getControl(ip_controls[2])
        defaultGateway.setLabel(settings_dict['Gateway'])
        primaryDNS = self.getControl(ip_controls[3])
        primaryDNS.setLabel(settings_dict['DNS_1'])
        secondaryDNS = self.getControl(ip_controls[4])
        if settings_dict.has_key('DNS_2'):
            secondaryDNS.setLabel(settings_dict['DNS_2'])

    def update_current_ip_settings(self, ip_controls):
        ip_address = self.getControl(ip_controls[0])
        self.current_network_config['Address'] = ip_address.getLabel()
        subnet = self.getControl(ip_controls[1])
        self.current_network_config['Netmask'] = subnet.getLabel()
        defaultGateway = self.getControl(ip_controls[2])
        self.current_network_config['Gateway'] = defaultGateway.getLabel()
        primaryDNS = self.getControl(ip_controls[3])
        self.current_network_config['DNS_1'] = primaryDNS.getLabel()
        secondaryDNS = self.getControl(ip_controls[4])
        if secondaryDNS.getLabel():
            self.current_network_config['DNS_2'] = secondaryDNS.getLabel()
        log(self.current_network_config)

    def handle_wired_selection(self, control_id):
        if control_id == wired_dhcp_manual_button: # Manual / DHCP Button
            if self.current_network_config['Method'] == 'dhcp':
                self.current_network_config['Method'] = 'manual'
            elif self.current_network_config['Method'] == 'manual':
                self.current_network_config['Method'] = 'dhcp'
            elif self.current_network_config['Method'] == 'nfs_dhcp':
                self.current_network_config['Method'] = 'nfs_manual'
            elif self.current_network_config['Method'] == 'nfs_manual':
                self.current_network_config['Method'] = 'nfs_dhcp'
            self.update_manual_DHCP_button(wired_dhcp_manual_button)
            self.setting_changed = True

        if control_id == wired_reset_button:  # Reset
            self.current_network_config = self.get_wired_config()
            self.populate_ip_controls(self.current_network_config, wired_ip_controls)
            self.update_manual_DHCP_button(wired_dhcp_manual_button)
            self.setFocusId(wired_dhcp_manual_button)

        if control_id == wired_apply_button:
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

        if control_id in wired_ip_labels:
            self.update_current_ip_settings(wired_ip_controls)

        self.update_apply_reset_button()



    def update_apply_reset_button(self):
        if cmp(self.get_wired_config(), self.current_network_config) == 0:
            self.toggle_controls(False, [wired_reset_button, wired_apply_button])
        else:
            self.toggle_controls(True, [wired_reset_button, wired_apply_button])


    def populate_wifi_panel(self):
        """ Populates the wifi panel with the information provided in the wifi_dict.
                wifi_dict = {
                            ssid' : {'encryption': True|False, 'strength': percent::int }
                            }
            """
        wifi_dict = {}
        for x in range(22):
            word = self.randomword()
            encryption = random.choice([True, False])
            strength = random.randint(0, 85)
            wifi_dict[word] = {'encryption': encryption, 'strength': strength}

        self.wifis = []

        for ssid, info in wifi_dict.iteritems():

            # hidden networks are ignored

            if ssid:
                itm = xbmcgui.ListItem(ssid)

                st = info['strength']

                # icon_tuple = (connected, encrypted, strength)
                icon_image = self.get_wifi_icon(info['encryption'], (int(st) / 25 ) + 1, False)

                itm.setIconImage(icon_image)

                itm.setProperty('strength', str(st))

                self.wifis.append(itm)

        # sort the list of wifis based on signal strength
        self.wifis.sort(key=self.sort_strength, reverse=True)

        # remove everything from the existing panel
        self.WFP.reset()

        # add the top 20 new items to the panel
        self.WFP.addItems(self.wifis[:20])

        # set the current connection as selected
        for i, wifi in enumerate(self.wifis):
            if wifi.getLabel() == self.conn_ssid:
                self.WFP.selectItem(i)

        self.WFP.getListItem(random.randint(0, 10)).select(True)


    def populate_bluetooth_panel(self):
        """
                Populates the bluetooth panel
        """
        # clear discovered and paired lists
        self.BTD.reset()
        self.BTP.reset()

        # enable all controls
        self.toggle_controls(True, bluetooth_controls)
        # disable all if bluetooth is not detected
        if not osmc_bluetooth.is_bluetooth_available():
            self.toggle_controls(False, bluetooth_controls)
            return
        else:
            # disable all but the enable service button
            if not self.is_bluetooth_running():
                controls_to_disable = list(bluetooth_controls)
                controls_to_disable.remove(bluetooth_service_button)
                self.toggle_controls(False, controls_to_disable)
                return

        adapterRadioButton = self.getControl(10301)
        adapterRadioButton.setSelected(osmc_bluetooth.is_bluetooth_enabled())
        serviceRadioButton = self.getControl(10302)
        serviceRadioButton.setSelected(self.is_bluetooth_running())
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
        if control_id == 10301:  # Enable Adapter
            osmc_bluetooth.toggle_bluetooth_state(not osmc_bluetooth.is_bluetooth_enabled())

        if control_id == 10302:  # Enable Service
            self.toggle_bluetooth_service(not self.is_bluetooth_running())

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

        if control_id == 7000:  # Discovred devices
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


    def get_wifi_icon(self, encrypted, strength, connected=False):
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


    def sort_strength(self, itm):
        try:
            metric = int(itm.getProperty('strength'))
        except:
            metric = 0

        return metric


    def sort_alias(self, itm):
        try:
            metric = int(itm.getProperty('alias'))
        except:
            metric = 0

        return metric


    def randomword(self):
        length = random.randint(0, 25)

        return ''.join(random.choice(string.letters + string.digits) for i in range(length))

        # NOTE - Maybe this should come out into a new module so we use the same code for
        # Services addon and Bluetooth?


    def is_bluetooth_running(self):
        p = subprocess.call(['sudo', '/bin/systemctl', 'is-enabled', 'bluetooth.service'])
        if p == 0:
            enabled = True
        else:
            enabled = False
        p = subprocess.call(['sudo', '/bin/systemctl', 'is-active', 'bluetooth.service'])
        if p == 0:
            active = True
        else:
            active = False
        return enabled and active


    def toggle_bluetooth_service(self, enable):
        message = ''
        if enable:
            subprocess.call(['sudo', '/bin/systemctl', 'enable', 'bluetooth.service'])
            subprocess.call(['sudo', '/bin/systemctl', 'start', 'bluetooth.service'])
            message = 'Bluetooth Service Enabled'
        else:
            subprocess.call(['sudo', '/bin/systemctl', 'disable', 'bluetooth.service'])
            subprocess.call(['sudo', '/bin/systemctl', 'stop', 'bluetooth.service'])
            message = 'Bluetooth Service Disabled'
        xbmc.executebuiltin("XBMC.Notification(%s,%s,%s)" % ('Bluetoooth', message, "2500"))
