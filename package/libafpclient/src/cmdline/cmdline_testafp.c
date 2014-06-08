/* 
	Copyright (C) 2008 Alex deVries
	
*/

#include "afp.h"
#include "midlevel.h"

#include "cmdline_main.h"

#include <stdlib.h>
#include <stdio.h>

extern struct afp_volume * vol;

static int test_one_url(char * url_string, 
	enum {TCPIP,AT} protocol, 
	char * username, 
	char * uamname,
	char * password,
	char * servername, 	
	int port,
	char * volumename, 
	char * path)
{
	struct afp_url valid_url;
	afp_default_url(&valid_url);
	valid_url.protocol=protocol;
	sprintf(valid_url.servername,servername);
	sprintf(valid_url.volumename,volumename);
	sprintf(valid_url.path,path);
	sprintf(valid_url.username,username);
	sprintf(valid_url.password,password);
	sprintf(valid_url.uamname,uamname);
	valid_url.port=port;

	if (afp_url_validate(url_string,&valid_url)) 
		printf("* Could not parse %s\n",url_string);
	else
		printf("* Parsed %s correctly\n",url_string);

	return 0;
}

int test_urls(void)
{

	printf("Testing URL parsing\n");
	
	test_one_url("afp://user::name;AUTH=authtype:pa@@sword@server/volume/path",
		TCPIP,"user:name","authtype","pa@sword","server",548,"volume","path");

	test_one_url("afp://username;AUTH=authtype:password@server/volume/path",
		TCPIP,"username","authtype","password","server",548,"volume","path");
	test_one_url("afp://username;AUTH=authtype:password@server:548/volume/path",
		TCPIP,"username","authtype","password","server",548,"volume","path");
	test_one_url("afp://username:password@server/volume/path",
		TCPIP,"username","","password","server",548,"volume","path");
	test_one_url("afp://username@server/volume/path",
		TCPIP,"username","","","server",548,"volume","path");
	test_one_url("afp://server/volume/path",
		TCPIP,"","","","server",548,"volume","path");
	test_one_url("afp://server/",
		TCPIP,"","","","server",548,"","");
	test_one_url("afp://server:22/",
		TCPIP,"","","","server",22,"","");
	test_one_url("afp://server:22",
		TCPIP,"","","","server",22,"","");
	test_one_url("afp://server:22/volume/",
		TCPIP,"","","","server",22,"volume","");
	return 0;
}

int com_testafp(char * arg)
{
	char * data = malloc(200);
	int i;

	if (!arg)
		arg = "";

for (i=0;i<6;i++) {
	data[0]=0x00;
	data[1]=0x00;
	data[2]=i;
	data[3]=4;
	data[4]=0x00;
	data[5]=0x00;
	data[6]=0x00;
	data[7]=0x01;
	data[8]=0x00;
	data[9]=0x00;
	data[10]=0x00;
	data[11]=0x00;

	sprintf(data+12,"%s","mymountpoint");
	afp_newcommand76(vol,31,data);
}
	return 0;


	data[0]=0x00;
	data[1]=0x00;
	data[2]=0x00;
	data[3]=0x04;
	data[4]=0x00;
	data[5]=0x00;
	data[6]=0x00;
	data[7]=0x02;
	data[8]=0x00;
	data[9]=0x00;
	data[10]=0x00;
	data[11]=0x00;

afp_newcommand76(vol,12,data);


	return 0;
}

