#!/usr/bin/env python

'''
   Copyright 2010 Filia Dova, Georgios Migdos

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
'''

# Standard Modules
import subprocess
import threading
import time



class Utilities:
	def getWirelessInterfacesList(self):
		networkInterfaces=[]		
		command = ["iwconfig"]
		process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		process.wait()
		(stdoutdata, stderrdata) = process.communicate();
		output = stdoutdata
		lines = output.splitlines()
		for line in lines:
			if(line.find("IEEE 802.11")!=-1):
				networkInterfaces.append(line.split()[0])
		return networkInterfaces
		

#======================================================================================================================

#--- Class that calls iwlist periodically
#--- and parses its output - also gets GPS data from gpsd:

class wifiScanner(threading.Thread):

	def __init__(self, wifiNetworks, interval):
		self.stopThread = threading.Event()
		self.wifiNetworks = wifiNetworks
		self.interval = interval
		self.setWirelessInterface(None)
		self.scanning = False
		threading.Thread.__init__(self)
		
	def run(self):
		self.stopThread.clear()
		self.scanning = True
		
		while( not self.stopThread.isSet() ):
			self.scanForWifiNetworks()			
			time.sleep(self.interval)
		
		self.scanning = False
		
	
	def stop(self):
		self.stopThread.set()
		
	def isScanning(self):
		return self.scanning
			
	def getWifiNetworksList(self):
		result = []
		for k,v in self.wifiNetworks.iteritems():
			result.append(v)				
		return result
		
	def setWirelessInterface(self, iface):
		self.wIface = iface
		
	def getWirelessInterface(self):
		return self.wIface
		
		
	def scanForWifiNetworks(self):
		networkInterface = self.wIface
		output = ""
		if(networkInterface!=None):		
			command = ["iwlist", networkInterface, "scanning"]
			process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			process.wait()
			(stdoutdata, stderrdata) = process.communicate();
			output =  stdoutdata
			self.parseIwlistOutput(output)
	
	
	def cutFrom(self, s, pattern):
		index = s.find(pattern)
		if(index>-1):
			return s[index+len(pattern):]
		else:
			return ""
		
	def cutTo(self, s, pattern):
		index = s.find(pattern)
		if(index>-1):
			return s[:index]
		else:
			return s
		
	def parseIwlistOutput(self, output):
		output = self.cutFrom(output, "Address:")
		while (output!=""):	
			entry = self.cutTo(output, "Address:")
	
			address = ""
			essid = ""
			mode = ""
			channel = ""
			frequency = ""
			quality = ""
			signal = ""
			noise = ""
			encryption = ""
	
			address = entry[1:18]
	
			startIndex = entry.find("ESSID:\"")
			if(startIndex > -1):		
				endIndex = entry.find("\"\n", startIndex)
				essid = entry[startIndex+7:endIndex]
	
			startIndex = entry.find("Mode:")
			if(startIndex > -1):
				endIndex = entry.find("\n", startIndex)
				mode = entry[startIndex+5:endIndex]
	
			startIndex = entry.find("Channel:")
			if(startIndex > -1):
				endIndex = entry.find("\n", startIndex)
				channel = entry[startIndex+8:endIndex]
		
			startIndex = entry.find("Frequency:")
			if(startIndex > -1):
				endIndex = entry.find("\n", startIndex)
				frequency = entry[startIndex+10:endIndex]
		
			startIndex = entry.find("Quality=")
			if(startIndex > -1):
				endIndex = entry.find("Signal", startIndex) -2
				qual = eval(entry[startIndex+8:endIndex]+".0")
				if(qual > 1.0):
					qual = 1.0
				quality = str(qual)
	
			startIndex = entry.find("Signal level:")
			if(startIndex > -1):
				endIndex = entry.find("dBm", startIndex) -1
				signal = entry[startIndex+13:endIndex]
	
			startIndex = entry.find("Noise level=")
			if(startIndex > -1):
				endIndex = entry.find("dBm", startIndex) -1
				noise = entry[startIndex+12:endIndex]
	
			startIndex = entry.find("Encryption key:")
			if(startIndex > -1):
				endIndex = entry.find("\n", startIndex)
				encryption = entry[startIndex+15:endIndex]
			
			key = (address, essid)
			value = [address, essid, mode, channel, frequency, quality, signal, noise, encryption]
			try:
				oldValue = self.wifiNetworks[key]				
				qualityN = eval(quality)
				oldQualityN = eval(oldValue[5])	
				if qualityN >  oldQualityN:
					self.wifiNetworks[key] = value
			except KeyError:
				self.wifiNetworks[key] = value
			
			output = self.cutFrom(output, "Address:")


	def exportXML(self, filename):
		out = open(filename, 'w')
		out.write('<?xml version="1.0" encoding="UTF-8"?>\n')
		out.write("<networkslist>\n")
		
		lst = self.getWifiNetworksList()
		for l in lst:
			out.write("\t<network>\n")
			
			out.write("\t\t<address>"+ l[0] +"</address>\n")
			out.write("\t\t<essid>"+ l[1] +"</essid>\n")
			out.write("\t\t<mode>"+ l[2] +"</mode>\n")
			out.write("\t\t<channel>"+ l[3] +"</channel>\n")
			out.write("\t\t<frequency>"+ l[4] +"</frequency>\n")
			out.write("\t\t<quality>"+ l[5] +"</quality>\n")
			out.write("\t\t<signal>"+ l[6] +"</signal>\n")
			out.write("\t\t<noise>"+ l[7] +"</noise>\n")
			out.write("\t\t<security>"+ l[8] +"</security>\n")
			
			out.write("\t</network>\n")
			
		out.write("</networkslist>\n")
		out.close()



# If there is only one wireless interface then just use that.
# If there is more than one then create window to select the wireless interface.
wirelessInterfaces = Utilities().getWirelessInterfacesList()

print wirelessInterfaces

iface = wirelessInterfaces[0]

wifiNetworks={}
SCANNER_INTERVAL = 5
UPDATE_INTERVAL = 2

scanner = wifiScanner(wifiNetworks, SCANNER_INTERVAL)

scanner.setWirelessInterface(iface)
scanner.start()


for x in range(20):
	time.sleep(2)
	print len(scanner.wifiNetworks)

print 'scan over'
print scanner.wifiNetworks

for k, connection in scanner.wifiNetworks.iteritems():
	print "Name: %s   Strength: %s" % (connection[1], (int(float(connection[5]) * 1000.0)) / 10.0)

print 'Ended'

# # set timer to scan for 10 seconds then display results

# # allow the user to select a rescan, hold rescan until they say stop

# # if there are no results, show 'No Wireless shit found'

# # if there is then display it 
