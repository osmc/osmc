import xbmc
import xbmcgui
import subprocess

op = subprocess.check_output['dpkg' '-l' '|' 'grep osmc' '|' 'paste-log']

print op