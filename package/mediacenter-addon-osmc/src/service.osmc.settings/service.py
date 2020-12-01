#!/usr/bin/python
# -*- coding: utf-8 -*-
'''
 Copyright (C) 2014 KodeKarnage

 This Program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This Program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with XBMC; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 http://www.gnu.org/copyleft/gpl.html
'''

# KODI modules
import xbmc

# Custom modules
import resources.lib.osmc_main as m


def log(message):

	try:
		message = str(message)
	except UnicodeEncodeError:
		message = message.encode('utf-8', 'ignore' )

	xbmc.log('OSMC ADDON MAIN ' + str(message), level=xbmc.LOGDEBUG)


if __name__ == "__main__":

	m.set_version()

	Main_Service = m.Main()

	log('Exiting OSMC Settings')



