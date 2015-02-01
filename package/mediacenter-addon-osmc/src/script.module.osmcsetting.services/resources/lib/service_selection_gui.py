import os
import subprocess
import xbmcgui
from collections import OrderedDict
import glob


ACTION_PREVIOUS_MENU = 10
ACTION_NAV_BACK      = 92
SAVE                 = 5
HEADING              = 1
ACTION_SELECT_ITEM   = 7


class MaitreD(object):

    def __init__(self, logger):
        self.log = logger
        self.services = {}
        self.poll_services()

    def poll_services(self):
        full_paths = glob.glob('/etc/systemd/system/*.wants/*')
        self.active_services = [os.path.basename(x) for x in full_paths]

    def enable_service(self, service_name):
        self.log("MaitreD: Enabling " + service_name)
        os.system("sudo /bin/systemctl enable " + service_name)
        os.system("sudo /bin/systemctl start " + service_name)
        
    def disable_service(self, service_name):
        self.log("MaitreD: Disabling " + service_name)
        os.system("sudo /bin/systemctl disable " + service_name)
        os.system("sudo /bin/systemctl stop " + service_name)

    def is_running(self, service_name):
        p = subprocess.Popen(["sudo", "/bin/systemctl", "status", service_name], stdout=subprocess.PIPE)
        out, err = p.communicate()

        return True if 'running' in out else False

    def is_enabled(self, service_name):

        if service_name in self.active_services:
            self.log("%s is currently enabled" % service_name)  
            return True
        else:
            self.log("%s is currently enabled" % service_name)
            return False

    def all_services(self):
        ''' Returns a dict of service tuples. {s_name: (entry, service_name, running, enabled)} '''

        svcs = OrderedDict()

        for service_name in os.listdir("/etc/osmc/apps.d"):
            if os.path.isfile("/etc/osmc/apps.d/" + service_name):
                with open ("/etc/osmc/apps.d/" + service_name) as f:
                    s_name = f.readline()
                    s_entry = f.readline()

                    self.log("MaitreD: Service Friendly Name: " + s_name)
                    self.log("MaitreD: Service Entry Point: " + s_entry)

                    enabled = self.is_enabled(service_name)
                    running = ''
                    if enabled:
                        r = self.is_running(service_name)
                        if r:
                            running = " (running)"
                        else:
                            running = " (stopped)"

                    svcs[s_name] = ( s_entry, service_name, running, enabled )
                                    

        # this last part creates a dictionary ordered by the services friendly name
        self.services = OrderedDict([(k, svcs[k]) for k in sorted(svcs.keys())])
        self.log('MaitreD: service list = %s' % self.services)
        return self.services

    def process_services(self, user_selection):
        ''' User selection is a list of tuples (s_name::str, enable::bool) '''

        self.log('MaireD: process_services = %s' % user_selection)

        for service_name, status in user_selection:
            if status != self.services[service_name][-1]:
                if status == True:
                    self.enable_service(service_name)
                else:
                    self.disable_service(service_name)



class service_selection(xbmcgui.WindowXMLDialog):


    def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):

        self.service_list = kwargs.get('service_list', [])
        self.log = kwargs.get('logger', self.dummy)

        self.garcon = MaitreD(self.log)
        self.service_list = self.garcon.all_services()


    def dummy(self):
        print('logger not provided')

    def onInit(self):

        # Save button
        self.ok = self.getControl(SAVE)
        self.ok.setLabel('Apply')

        # Heading
        self.hdg = self.getControl(HEADING)
        self.hdg.setLabel('OSMC Service Management')
        self.hdg.setVisible(True)

        # Hide unused list frame
        self.x = self.getControl(3)
        self.x.setVisible(False)

        # Populate the list frame
        self.name_list = self.getControl(6)
        self.name_list.setEnabled(True)

        # Set action when clicking right from the Save button
        self.ok.controlRight(self.name_list)
        self.ok.controlLeft(self.name_list)

        item_pos = 0

        for s_name, service_tup in self.service_list.iteritems():
            # populate the list
            entry, service_name, running, enabled = service_tup
            self.tmp = xbmcgui.ListItem(label=s_name + running, label2=str(enabled))
            self.name_list.addItem(self.tmp)

            # highlight the already selection randos
            if enabled:
                self.name_list.getListItem(item_pos).select(True)

            item_pos += 1

        self.setFocus(self.name_list)


    def onAction(self, action):
        actionID = action.getId()
        if (actionID in (ACTION_PREVIOUS_MENU, ACTION_NAV_BACK)):
            self.close()


    def onClick(self, controlID):

        if controlID == SAVE:
            self.process()
            self.close()

        else:
            selItem = self.name_list.getSelectedPosition()
            if self.name_list.getSelectedItem().isSelected():
                self.name_list.getSelectedItem().select(False)
            else:
                self.name_list.getSelectedItem().select(True)

    def process(self):

        sz = self.name_list.size()

        processing_list = []

        for x in range(sz):
            line = self.name_list.getListItem(x)
            processing_list.append((line.getLabel().replace(' (running)','').replace(' (stopped)',''), line.isSelected()))

        self.garcon.process_services(processing_list)

