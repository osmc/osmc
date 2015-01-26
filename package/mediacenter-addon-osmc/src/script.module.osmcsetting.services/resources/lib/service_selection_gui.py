import xbmcgui


ACTION_PREVIOUS_MENU = 10
ACTION_NAV_BACK      = 92
SAVE                 = 5
HEADING              = 1
ACTION_SELECT_ITEM   = 7



class service_selection(xbmcgui.WindowXMLDialog):


    def __init__(self, strXMLname, strFallbackPath, strDefaultName, **kwargs):

        self.service_list = kwargs.get('service_list', [])


    def onInit(self):

        # Save button
        self.ok = self.getControl(SAVE)
        self.ok.setLabel('Exit')

        # Heading
        self.hdg = self.getControl(HEADING)
        self.hdg.setLabel('OSMC Service Management')
        self.hdg.setVisible(True)

        # Hide unused list frame
        self.x = self.getControl(3)
        self.x.setVisible(False)

        # Populate the list frame
        self.name_list      = self.getControl(6)
        sself.name_list.setEnabled(True)

        # Set action when clicking right from the Save button
        self.ok.controlRight(self.name_list)
        self.ok.controlLeft(self.name_list)

        for i, service_tup in enumerate(self.service_list):
            # populate the random list
            service, status = service_tup
            self.tmp = xbmcgui.ListItem(service)
            self.name_list.addItem(self.tmp)

            # highlight the already selection randos
            if status == 'active':
                self.name_list.getListItem(i).select(True)

        self.setFocus(self.name_list)


    def onAction(self, action):
        actionID = action.getId()
        if (actionID in (ACTION_PREVIOUS_MENU, ACTION_NAV_BACK)):
            self.close()


    def onClick(self, controlID):

        if controlID == SAVE:
            self.close()

        else:
            selItem = self.name_list.getSelectedPosition()
            if self.name_list.getSelectedItem().isSelected():
                self.name_list.getSelectedItem().select(False)
            else:
                self.name_list.getSelectedItem().select(True)


