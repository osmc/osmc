import os
import subprocess
import xbmcgui
import xbmc
from collections import OrderedDict
import glob
from comprehensive_function_logger import comprehensive_logger as clog

ACTION_PREVIOUS_MENU = 10
ACTION_NAV_BACK      = 92
SAVE                 = 5
HEADING              = 1
ACTION_SELECT_ITEM   = 7


def KodiLogger(message):
    xbmc.log('OSMC SERVICES ' + str(message), level=xbmc.LOGDEBUG)


class MaitreD(object):

    def __init__(self):
        self.services = {}


    @clog(KodiLogger)
    def enable_service(self, s_entry):
        
        os.system("sudo /bin/systemctl enable " + s_entry)
        os.system("sudo /bin/systemctl start " + s_entry)
    

    @clog(KodiLogger)
    def disable_service(self, s_entry):
        
        os.system("sudo /bin/systemctl disable " + s_entry)
        os.system("sudo /bin/systemctl stop " + s_entry)


    @clog(KodiLogger)
    def is_running(self, s_entry):

        p = subprocess.call(["sudo", "/bin/systemctl", "is-active", s_entry])

        return True if p == 0 else False


    @clog(KodiLogger)
    def is_enabled(self, s_entry):

        p = subprocess.call(["sudo", "/bin/systemctl", "is-enabled", s_entry])

        return True if p == 0 else False


    @clog(KodiLogger, maxlength=500)
    def discover_services(self):
        ''' Returns a dict of service tuples. {s_name: (entry, service_name, running, enabled)} '''

        svcs = {}

        for service_name in os.listdir("/etc/osmc/apps.d"):
            service_name = service_name.replace('\n','')
            if os.path.isfile("/etc/osmc/apps.d/" + service_name):
                with open ("/etc/osmc/apps.d/" + service_name) as f:
                    s_name = f.readline().replace('\n','')
                    s_entry = f.readline().replace('\n','')

                    KodiLogger("MaitreD: Service Friendly Name: " + s_name)
                    KodiLogger("MaitreD: Service Entry Point: " + s_entry)

                    enabled     = self.is_enabled(s_entry)
                    runcheck    = self.is_running(s_entry)
                    if runcheck:
                        running = " (running)"
                    else:
                        running = " (stopped)"

                    svcs[s_name] = ( s_entry, service_name, running, enabled )
                                    

        # this last part creates a dictionary ordered by the services friendly name
        self.services = OrderedDict()
        for key in sorted(svcs.keys()):
            self.services[key] = svcs[key]
        
        return self.services


    @clog(KodiLogger, maxlength=250)
    def process_user_changes(self, user_selection):
        ''' User selection is a list of tuples (s_name::str, service_name::str, enable::bool) '''

        for s_name, s_entry, status in user_selection:
            if status != self.services[s_name][-1]:
                if status == True:
                    self.enable_service(s_entry)
                else:
                    self.disable_service(s_entry)



class service_selection(xbmcgui.WindowXMLDialog):


    def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):

        self.service_list = kwargs.get('service_list', [])

        self.garcon = MaitreD()
        self.service_list = self.garcon.discover_services()


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
            s_entry, service_name, running, enabled = service_tup
            self.tmp = xbmcgui.ListItem(label=s_name + running, label2=s_entry)
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
            s_name = line.getLabel().replace(' (running)','').replace(' (stopped)','').replace('\n','')
            s_entry = line.getLabel2().replace('\n','')
            issel = True if line.isSelected() == 1 else False
            processing_list.append((s_name, s_entry, line.isSelected()))

        self.garcon.process_user_changes(processing_list)

