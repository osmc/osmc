import os


# Unique Entries							Original Color					Replacement
COLOR_CONVERSIONS = {		
				'<backgroundcolor>%s</backgroundcolor>'		: {'orig': '0xff18394a',	'repl': 'ff203b4c', 	'count': 0},
				'<colordiffuse>%s</colordiffuse>'			: {'orig': '55f1f1f1',		'repl': 'ff0b2430', 	'count': 0},
				'<colordiffuse>%s</colordiffuse>'			: {'orig': 'FFFFFFFF',		'repl': 'ff0b2430', 	'count': 0},
				'<disabledcolor>%s</disabledcolor>'			: {'orig': 'ff000000',		'repl': 'ff000000', 	'count': 0},
				'<focusedcolor>%s</focusedcolor>'			: {'orig': 'ffffffff',		'repl': 'ffe0b074', 	'count': 0},
				'<selectedcolor>%s</selectedcolor>'			: {'orig': 'FF8FF5EE',		'repl': 'fff1f1f1', 	'count': 0},
				'<selectedcolor>%s</selectedcolor>'			: {'orig': 'ffffa500',		'repl': 'fff1f1f1', 	'count': 0},
				'<shadowcolor>%s</shadowcolor>'				: {'orig': '44000000',		'repl': '44000000', 	'count': 0},
				'<shadowcolor>%s</shadowcolor>'				: {'orig': 'ff000000',		'repl': '44000000', 	'count': 0},
				'<textcolor>%s</textcolor>'					: {'orig': '66ffffff',		'repl': 'ff808080', 	'count': 0},
				'<textcolor>%s</textcolor>'					: {'orig': '88f0f0f0',		'repl': 'ff808080', 	'count': 0},
				'<textcolor>%s</textcolor>'					: {'orig': 'ffffffff',		'repl': 'ff808080', 	'count': 0},
					}

GOFFS_COLORS = 	{
				'ColorFO'				: 'ffe0b074',
				'ColorNF'				: 'ff808080',
				'ColorHeading'			: 'ffd6d6d6',
				'ColorDetails'			: 'ff707070',
				'ColorSelected'			: 'fff1f1f1',
				'ColorDisabled'			: 'ff000000',
				'ColorInvalid'			: 'ffa10000',
				'ShadowColor'			: '44000000',
				'WindowBackgroundColor'	: 'ff0b2430',
				'DialogBackgroundColor'	: 'ff203b4c',
				}



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

						for key in COLOR_CONVERSIONS:
							search_key = key % COLOR_CONVERSIONS[key]['orig']
							if search_key in line.replace('<!--%s-->' % search_key, ''):
								COLOR_CONVERSIONS[key]['count'] = COLOR_CONVERSIONS[key]['count'] + 1
								conversion = True
								line = line.replace('\n','').replace('\t','')
								new_line = line.replace(search_key, key % COLOR_CONVERSIONS[key]['repl']) + '<!--former_line%s-->\n' % line
								break
						else: # if nobreak
							new_line = line

					new_lines.append(new_line)

			if conversion:
				with open(os.path.join(root, name), 'w') as f:		
					f.writelines(new_lines)

for k, v in COLOR_CONVERSIONS.iteritems():
	print v['count'], (6 - len(str(v['count'])) ) * ' ', k					