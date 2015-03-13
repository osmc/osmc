import os
import subprocess
import xbmcgui
import xbmc
from collections import OrderedDict
import glob
from CompLogger import comprehensive_logger as clog

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
    def process_user_changes(self, initiants, finitiants):
        ''' User selection is a list of tuples (s_name::str, service_name::str, enable::bool) '''

        for clean_name, s_entry in initiants:
            self.enable_service(s_entry)

        for clean_name, s_entry in finitiants:
            self.disable_service(s_entry)


class service_selection(xbmcgui.WindowXMLDialog):


    def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):

        # self.service_list = kwargs.get('service_list', [])

        # the lists to hold the services to start, and to finish
        self.initiants  = []
        self.finitiants = []

        self.garcon = MaitreD()
        self.service_list = self.garcon.discover_services()


    def dummy(self):
        print('logger not provided')


    def onInit(self):

        # Populate the list frame
        self.name_list = self.getControl(500)
        self.name_list.setEnabled(True)

        item_pos = 0

        for s_name, service_tup in self.service_list.iteritems():
            # populate the list
            s_entry, service_name, running, enabled = service_tup

            if running != " (running)":
                if enabled == True:
                    sundry = " (enabled)"
                else:
                    sundry = " (stopped)"

            else:
                sundry = " (running)"

            self.tmp = xbmcgui.ListItem(label=s_name + sundry, label2=s_entry)
            self.name_list.addItem(self.tmp)

            # highlight the already selection randos
            # if enabled:
            #     self.name_list.getListItem(item_pos).select(True)

            item_pos += 1

        self.setFocus(self.name_list)


    def onAction(self, action):
        actionID = action.getId()
        if (actionID in (ACTION_PREVIOUS_MENU, ACTION_NAV_BACK)):
            self.close()


    def onClick(self, controlID):

        if controlID == 7:
            self.close()

        elif controlID == 6:
            self.process()
            self.close()

        elif controlID == 500:

            selItem = self.name_list.getSelectedItem().getLabel()
            s_entry = self.name_list.getSelectedItem().getLabel2()
            clean_name = selItem.replace(' (running)','').replace(' (enabled)','').replace(' (stopped)','').replace('\n','')

            item_tup = (clean_name, s_entry)
            
            if selItem.endswith(' (running)') or selItem.endswith(' (enabled)'):

                # if the item is currently enabled or running

                if item_tup in self.finitiants:
                    self.finitiants.remove(item_tup)
                else:
                    self.finitiants.append(item_tup)

            else:

                # if the item is not enabled

                if item_tup in self.initiants:
                    self.initiants.remove(item_tup)
                else:
                    self.initiants.append(item_tup)

            xbmc.sleep(10)

            self.update_checklist()


    def update_checklist(self):

        if not self.initiants and not self.finitiants:
            self.getControl(1102).setText(' ')
            return

        todo_list = '-= Pending Actions =-\n\n'

        if self.initiants:
            todo_list += "Enable:\n   - " + '\n   - '.join([x[0] for x in self.initiants])

            if self.finitiants:
                todo_list += '\n\n'

        if self.finitiants:
            todo_list += "Disable:\n   - " + '\n   - '.join([x[0] for x in self.finitiants])

        self.getControl(1102).setText(todo_list)


    def process(self):

        self.garcon.process_user_changes(self.initiants, self.finitiants)
        


