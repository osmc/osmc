
import os
import requests
from xml.etree import ElementTree as ET

URL_BASE = 'http://lirc.sourceforge.net/remotes/'

def get_base():
	'''
		Returns a list of subpages for remotes
	'''
	r = requests.get(URL_BASE)

	t = r.text

	return [ x[:x.index('/"')] for x in t.split('<a href="') if '/"' in x]


def get_subpages(stub):
	'''
		Returns a dictionary of conf files (keys) found in the sub-menu along with images (values) if there are any
	'''

	ignore_chars = ['/', '?']
	image_chars  = ['.jpg', '.png']

	confs = {}
	images = []

	subs = []

	r = requests.get(URL_BASE)

	t = r.text

	# create a list of links from the sub page
	subs_raw = [ x[:x.index('"')] for x in t.split('<a href="') if '"' in x ]

	# remove the links with those specific characters
	for sub in subs_raw:
		for ig in ignore_chars:
			if ig in sub:
				break
		else:
			subs.append(sub)

	# add the confs to the confs dict and push the images off to the images list
	for sub in subs:
		for img in image_chars:
			if img in sub:
				images.append(sub)
				break
		else:
			confs[sub] = None

	# cycle through the images and match them to the conf of the same name
	for image in images:
		cnf = image.replace('.png','').replace('.jpg')
		if cnf in confs:
			confs[cnf] = image

	return confs


def download_file(stub, itm):

	r = requests.get(URL_BASE + stub + '/' + itm)

	with open("code3.zip", "wb") as f:

		f.write(r.content)

	return local_filename