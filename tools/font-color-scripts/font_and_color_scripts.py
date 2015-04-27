import os
import collections


FONT_CONVERSIONS = 		{
						'osmc_addon_OLD_Font72' 	: {'repl': 'osmc_addon_720_XLarge', 	'count': 0},
						'osmc_addon_OLD_Font48' 	: {'repl': 'osmc_addon_XLarge', 		'count': 0},
						'osmc_addon_OLD_Font42' 	: {'repl': 'osmc_addon_Large', 			'count': 0},
						'osmc_addon_OLD_Font36' 	: {'repl': 'osmc_addon_Med', 			'count': 0},
						'osmc_addon_OLD_Font33' 	: {'repl': 'osmc_addon_Small', 			'count': 0},
						'osmc_addon_OLD_Font30' 	: {'repl': 'osmc_addon_XSmall', 		'count': 0},
						'osmc_addon_OLD_Font28' 	: {'repl': 'osmc_addon_XSmall', 		'count': 0},
						'osmc_addon_OLD_Font27' 	: {'repl': 'osmc_addon_XSmall', 		'count': 0},
						'osmc_addon_OLD_Font25' 	: {'repl': 'osmc_addon_XSmall', 		'count': 0},

						'osmc_addon_Font26' 		: {'repl': 'osmc_addon_XSmall', 		'count': 0},
						'osmc_addon_font13' 		: {'repl': 'osmc_addon_font13', 		'count': 0},

						'osmc_addon_720_OLD_Font72' : {'repl': 'osmc_addon_720_XLarge', 	'count': 0},
						'osmc_addon_720_OLD_Font48' : {'repl': 'osmc_addon_720_XLarge', 	'count': 0},
						'osmc_addon_720_OLD_Font42' : {'repl': 'osmc_addon_720_Large', 		'count': 0},
						'osmc_addon_720_OLD_Font36' : {'repl': 'osmc_addon_720_Med', 		'count': 0},
						'osmc_addon_720_OLD_Font33' : {'repl': 'osmc_addon_720_Small', 		'count': 0},
						'osmc_addon_720_OLD_Font30' : {'repl': 'osmc_addon_720_XSmall', 	'count': 0},
						'osmc_addon_720_OLD_Font28' : {'repl': 'osmc_addon_720_XSmall', 	'count': 0},
						'osmc_addon_720_OLD_Font27' : {'repl': 'osmc_addon_720_XSmall', 	'count': 0},
						'osmc_addon_720_OLD_Font25' : {'repl': 'osmc_addon_720_XSmall', 	'count': 0},

						'osmc_addon_720_Font60' 	: {'repl': 'osmc_addon_Med', 			'count': 0},
						'osmc_addon_720_Font40' 	: {'repl': 'osmc_addon_XSmall', 		'count': 0},
						'osmc_addon_720_Font30' 	: {'repl': 'osmc_addon_720_XSmall', 	'count': 0},
						'osmc_addon_720_Font26' 	: {'repl': 'osmc_addon_720_font13', 	'count': 0},
						'osmc_addon_720_font13' 	: {'repl': 'osmc_addon_720_font13', 	'count': 0},

						}


# test = '<font>osmc_addon_720_OLD_Font72</font>'
# key = 'osmc_addon_720_OLD_Font72'

# new_line = test[:test.index(key)] + FONT_CONVERSIONS[key]['repl'] + test[test.index(key)+len(key):] + '<!--%s-->' % key

# print new_line

cnt = []

base_folder = 'C:\\Users\\x\\Desktop\\GitHub\\osmc\\package\\mediacenter-addon-osmc\\src'

for root, dirs, files in os.walk(base_folder, topdown=False):

	for name in files:

		if name.endswith('.xml'):
			new_lines = []
			with open(os.path.join(root, name), 'r') as f:
				lines = f.readlines()
				conversion = False
				for line in lines:

					# ignore the line if it is commented out
					if line.lstrip().startswith('<!--'):
						new_line = line

					else:

						for key in FONT_CONVERSIONS:
							if key in line.replace('<!--%s-->' % key, ''):
								FONT_CONVERSIONS[key]['count'] = FONT_CONVERSIONS[key]['count'] + 1
								conversion = True
								line = line.replace('\n','').replace('\t','')
								new_line = line[:line.index(key)] + FONT_CONVERSIONS[key]['repl'] + line[line.index(key)+len(key):] + '<!--former_line%s-->\n' % line
								break
						else: # if nobreak
							new_line = line

					new_lines.append(new_line)

			if conversion:
				with open(os.path.join(root, name), 'w') as f:		
					f.writelines(new_lines)

for k, v in FONT_CONVERSIONS.iteritems():
	print v['count'], (6 - len(str(v['count'])) ) * ' ', k


# cnt.sort()

# r = ['<!--', '-->','<font>','</font>']

# coll = collections.Counter(cnt)
# print 'start'
# for x,v in coll.iteritems():
# 	for rep in r:
# 		x = x.replace(rep, '')
# 	print x.lstrip()












