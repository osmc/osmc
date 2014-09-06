all:
	echo -e "Please specify a platform\nmake linux\nmake win\nmake osx"

linux:
	bash make_host_linux.sh

osx:
	bash make_host_osx.sh

win:
	bash make_host_win.sh "C:/MinGW/qt/qt-everywhere-opensource-src-4.8.6/bin" "C:/MinGW/bin"

clean:
	rm -f osmc-installer.deb >/dev/null 2>&1
	rm -f osmc-installer.exe >/dev/null 2>&1	
