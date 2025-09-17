// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2007-2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

// Create a FERS antenna description file from 2 sets of CSV antenna data

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void processCsv(FILE* fPin, FILE* fPOut, const char* encTag, const char* d1Tag, const char* d2Tag)
{
	char* buff = malloc(2048);
	char* line = fgets(buff, 2048, fPin);
	while (line != NULL)
	{
		char* comma = strchr(line, ',');
		if (!comma)
		{
			fprintf(stderr, "[ERROR] Malformed CSV line in input file %s\n\n", line);
			exit(1);
		}
		*comma = 0;
		comma++;
		comma[strlen(comma) - 1] = 0;
		fprintf(stderr, "%s\n", line);
		fprintf(stderr, "%s\n", comma + 1);
		fprintf(fPOut, "\t<%s>\n\t\t<%s>%s</%s><%s>%s</%s>\n\t</%s>\n", encTag, d1Tag, line, d1Tag, d2Tag, comma,
		        d2Tag, encTag);
		line = fgets(line, 2048, fPin);
	}

	free(buff);
}

int main(const int argc, char* argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "Usage: csv2antenna <outfile> <elevation gains> <azimuth gains>\n");
		exit(2);
	}
	FILE* f_pin1 = fopen(argv[2], "r");
	if (!f_pin1)
	{
		perror("Could not open input file");
		exit(2);
	}
	FILE* f_pin2 = fopen(argv[3], "r");
	if (!f_pin2)
	{
		perror("Could not open input file");
		exit(2);
	}
	FILE* f_p_out = fopen(argv[1], "w");
	if (!f_p_out)
	{
		perror("Could not open input file");
		exit(2);
	}
	fprintf(f_p_out, "<antenna>\n<elevation>\n");
	processCsv(f_pin1, f_p_out, "gainsample", "angle", "gain");
	fprintf(f_p_out, "</elevation>\n<azimuth>\n");
	processCsv(f_pin2, f_p_out, "gainsample", "angle", "gain");
	fprintf(f_p_out, "</azimuth>\n</antenna>\n");
	fclose(f_pin1);
	fclose(f_pin2);
	fclose(f_p_out);
}
