all:
	@echo "Please specify a platform\nmake obs\nmake win\nmake osx"

obs:
	bash make_host_obs.sh

osx:
	bash make_host_osx.sh

win:
	bash make_host_win.sh

clean:
	rm -f osmc-installer.dmg >/dev/null 2>&1
	rm -f osmc-installer.exe >/dev/null 2>&1
	rm -rf obs >/dev/null 2>&1	
