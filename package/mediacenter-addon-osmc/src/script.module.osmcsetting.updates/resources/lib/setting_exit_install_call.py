import subprocess
import os

os.remove('/var/tmp/.dont_install_downloaded_updates')
subprocess.Popen('sudo systemctl start update-manual')

