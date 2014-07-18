#include <string.h>
#include "afp.h"
#include "map_def.h"

static char *afp_map_strings[] = {
	"Unknown",
	"Common user directory",
	"Login ids",
	"Name mapped",
	"",
};

char * get_mapping_name(struct afp_volume * volume)
{
        return afp_map_strings[volume->mapping];

}



unsigned int map_string_to_num(char * name)
{
	int i;
	for (i=0;strlen(afp_map_strings[i])>0;i++) {
		if (strcasecmp(name,afp_map_strings[i])==0)
		return i;
	}
	return 0;
}

