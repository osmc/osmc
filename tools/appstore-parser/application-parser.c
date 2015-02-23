/*
 * application-parser.c
 *
 * (c) 2014-2015 Sam Nazarko
 * email@samnazarko.co.uk
 *
 */

#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		printf("%s\n%s\n", "OSMC application-parser", "usage: pass two or more JSON files for merging");
		return -1;
	}
	char *json_buffer;
	asprintf(&json_buffer, "%s\n\t%s\n\t%s\n", "{", "\"application\":","[");
	int json_count = argc - 1;
	int i;
	for (i = 0; i < json_count; i++)
	{
		FILE *fp;
		int line_count = 0;
		int lines_processed = 1;
		fp = fopen(argv[i+1], "r");
		if (fp)
		{
			asprintf(&json_buffer, "%s\t%s\n", json_buffer, "{");
			char line[512];
			while (!feof(fp))
				if (fgets(line, sizeof(line), fp) != NULL)
					line_count +=1;
			fseek(fp, 0, SEEK_SET);
			while (!feof(fp))
			{
				if (fgets(line, sizeof(line), fp) != NULL)
				{
					if (lines_processed != 1 && lines_processed != 2 && lines_processed != 3 && lines_processed != (line_count - 2) && lines_processed != (line_count - 1) && lines_processed != (line_count - 0))
						asprintf(&json_buffer, "%s\t\t%s", json_buffer, line);
					lines_processed++;
				}
			}
			if (i != (json_count - 1))
				asprintf(&json_buffer, "%s\t%s\n", json_buffer, "},");
			else
				asprintf(&json_buffer, "%s\t%s\n", json_buffer, "}");
			fclose(fp);
		}
		else
		{
			/* We are getting non-existent files. Bail */
			return -1;
		}
	}
	asprintf(&json_buffer, "%s\t\n%s\n", json_buffer, "]");
	printf("%s\n%s", json_buffer, "}");
	return 0;
}
