

class LangRetriever(object):
	''' Used to retrieve localised strings from the addons po files.

		Requires the parent addon object. This takes the form in that parent script of:
			__addon__ = xbmcaddon.Addon()

		Best usage:
			from osmc_language import LangRetriever
			LangRet = LangRetriever(__addon__)
			lang    = LangRet.lang

		'''

	def __init__(self, addon):

		self.__addon__ = addon


	def lang(self, id):

		if self.__addon__ is not None:
			san = self.__addon__.getLocalizedString(id).encode( 'utf-8', 'ignore' )
		else:
			san = ''

		return san 
