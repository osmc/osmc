#!/usr/bin/env osascript -l JavaScript
//Based script from Balena: https://raw.githubusercontent.com/balena-io/etcher/master/lib/shared/catalina-sudo/sudo-askpass.osascript.js
ObjC.import('stdlib')

const app = Application.currentApplication()
app.includeStandardAdditions = true

const result = app.displayDialog('OSMC installer needs privileged access in order to flash disks.\n\nType your password to allow this.', {
  defaultAnswer: '',
  withIcon: 'caution',
  buttons: ['Cancel', 'Ok'],
  defaultButton: 'Ok',
  hiddenAnswer: true,
})

if (result.buttonReturned === 'Ok') {
  result.textReturned
} else {
  $.exit(255)
}
