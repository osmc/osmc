
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "afp.h"

void afp_default_url(struct afp_url *url)
{
	memset(url,0,sizeof(*url));
	url->protocol=TCPIP;
	url->port=548;
}

static int check_servername (char * servername) 
{
	if (strchr(servername,':')) return -1;
	if (strchr(servername,'/')) return -1;
	return 0;
}

static int check_port(char * port) 
{
	long long ret = strtol(port,NULL,10);
	if ((ret<0) || (ret>32767)) return -1;
	if (errno) {
		printf("port error\n");
		return -1;
	}
	return 0;
}

static int check_uamname(const char * uam) 
{
	char * p;
	for (p=uam;*p;p++) {
		if (*p==' ') continue;
		if ((*p<'A') || (*p>'z')) return -1;
	}
	return 0;
}

static int check_username(const char * user) 
{
	return 0;
}

static int check_password(const char * pass) 
{
	return 0;
}

static void escape_string(char * string, char c) 
{
	int i; char d;
	int inescape=0;
	char tmpstring[1024];
	char * p = tmpstring;
	memset(tmpstring,0,1024);

	for (i=0;i<strlen(string);i++) {

		d=string[i]; /* convenience */

		if ((inescape) && (d==c)){
			inescape=0;
			continue;
		}
		*p=d;
		p++;
		if (d==c) inescape=1;
	}
	strcpy(string,tmpstring);

}

static void escape_url(struct afp_url * url)
{
	escape_string(url->password,'@');
	escape_string(url->username,':');
}


void afp_print_url(struct afp_url * url)
{

	printf("servername: %s\n"
	"volumename: %s\n"
	"path: %s\n"
	"username: %s\n"
	"password: %s\n"
	"port: %d\n"
	"uam name: %s\n",
	url->servername,
	url->volumename,
	url->path,
	url->username,
	url->password,
	url->port,url->uamname);

}

static char * escape_strrchr(const char * haystack, int c, const char *toescape) 
{
	char * p;
	if (strchr(toescape,c)==NULL)
		return strrchr(haystack,c);

	if ((p=strrchr(haystack,c))==NULL)
		return NULL;

	if (p==haystack) 
		return p;

	if (*(p-1)!=c) 
		return p;

	p-=2;

	return escape_strrchr(p,c,toescape);
}

static char * escape_strchr(const char * haystack, int c, const char * toescape)
{
	char * p;
	if (strchr(toescape,c)==NULL)
		return strchr(haystack,c);

	if ((p=strchr(haystack,c))==NULL)
		return NULL;

	if (p-haystack==strlen(haystack))
		return p;

	if (*(p+1)!=c) 
		return p;

	p+=2;

	return escape_strchr(p,c,toescape);
}

int afp_parse_url(struct afp_url * url, const char * toparse, int verbose)
{
	char firstpart[255],secondpart[2048];
	char *p, *q;
	int firstpartlen;
	int skip_earliestpart=0;
	int skip_secondpart=0;
	char * lastchar;

	if (verbose) printf("Parsing %s\n",toparse);

	url->username[0]='\0';
	url->servername[0]='\0';
	url->uamname[0]='\0';
	url->password[0]='\0';
	url->volumename[0]='\0';
	url->path[0]='\0';

	/* The most complex URL is:
 
	afp://user;AUTH=authType:password@server-name:port/volume-name/path 

	where the optional parms are user, password, AUTH and port, so the
	simplest is:

	afp://server-name/volume-name/path 

	*/

	/* if there is a ://, make sure it is preceeded by afp */

	if ((p=strstr(toparse,"://"))!=NULL) {
		q=p-3;
		if (p<toparse) {
			if (verbose) printf("URL does not start with afp://\n");
			return -1;
		}

		if (strncmp(q,"afp",3)!=0) {
			if (verbose) printf("URL does not start with afp://\n");
			return -1;
		}
		p+=3;
	} else {
		if (verbose) printf("This isn't a URL at all.\n");
		return -1;

	}
	if (p==NULL) p=toparse;

	/* Now split on the first / */
	if (sscanf(p,"%[^/]/%[^$]",

		firstpart, secondpart)!=2) {
		/* Okay, so there's no volume. */
		skip_secondpart=1;
	}

	firstpartlen=strlen(firstpart);

	lastchar=firstpart+firstpartlen-1;

	/* First part could be something like:
		user;AUTH=authType:password

	   We'll assume that the breakout is:
                user;  optional user name
	        AUTH=authtype:
	*/

	/* Let's see if there's a ';'.  q is the end of the username */

	if ((p=escape_strchr(firstpart,'@',"@"))) {
		*p='\0';
		p++; 
	} else {
		skip_earliestpart=1;
		p=firstpart;
	}
	/* p now points to the start of the server name*/

	/* see if we have a port number */

	if ((q=strchr(p,':'))) {
		*q='\0';
		q++;
		if (check_port(q)) return -1;
		if ((url->port=atoi(q))==0) {
			if (verbose) printf("Port appears to be zero\n");
			return -1;
		}
	}

	snprintf(url->servername,strlen(p)+1,p);
	if (check_servername(url->servername)) {
			if (verbose) printf("This isn't a valid servername\n");
			return -1;
	}

	if ((p==NULL) || ((strlen(p)+p-1)==lastchar)) {
		/* afp://server */
	}

	if ((q) && ((strlen(q)+q-1)==lastchar)) {
		/* afp://server:port */
	}


	/* Earliest part */

	if (skip_earliestpart) {
		p+=strlen(p);
		goto parse_secondpart;
	}
	p=firstpart;

	/* Now we're left with something like user[;AUTH=uamname][:password] */

	/* Look for :password */

	if ((q=escape_strrchr(p,':',":"))) {
		*q='\0';
		q++;
		snprintf(url->password,strlen(q)+1,q);
		if (check_password(url->password)) {
			if (verbose) printf("This isn't a valid passwd\n");
			return -1;
		}
	}

	/* Now we're down to user[;AUTH=uamname] */
	p=firstpart;

	if ((q=strstr(p,";AUTH="))) {
		*q='\0';
		q+=6;
		snprintf(url->uamname,strlen(q)+1,q);
		if (check_uamname(url->uamname)) {
			if (verbose) printf("This isn't a valid uamname\n");
			return -1;
		}
	}

	if (strlen(p)>0) {
		snprintf(url->username,strlen(p)+1,p);
		if (check_username(url->username)) {
			if (verbose) printf("This isn't a valid username\n");
			return -1;;
		}
	}


parse_secondpart:
	if (skip_secondpart) goto done;
	if (strlen(secondpart)==0) goto done;

	if (secondpart[strlen(secondpart)]=='/') 
		secondpart[strlen(secondpart)]='\0';

	p=secondpart;
	if ((q=strchr(p,'/'))) {
		*q='\0';
		q++;
	}
	snprintf(url->volumename,strlen(p)+1,p);


	if (q) {
		url->path[0]='/';
		snprintf(url->path+1,strlen(q)+1,q);
	}

done:
	escape_url(url);
	if (verbose) printf("Successful parsing of URL\n");
	return 0;
}


int afp_url_validate(char * url_string, struct afp_url * valid_url)
{
	struct afp_url new_url;

	if (afp_parse_url(&new_url, url_string,0)) {
		printf("url doesn't parse\n");
		goto error;
	}

#if BROKEN

	if (new_url.protocol!=valid_url->protocol) {
		printf("protocol doesn't match, I got %d when I expected %d\n",
			new_url.protocol,valid_url->protocol);
		goto error;
	}
#endif

	if (strcmp(new_url.username, valid_url->username)!=0) {
		printf("username doesn't match, I got %s when I should have received %s\n",new_url.username, valid_url->username);
		goto error;
	}
	if (strcmp(new_url.uamname, valid_url->uamname)!=0) {
		printf("uamname doesn't match, I got %s when I should have received %s\n",new_url.uamname, valid_url->uamname);
		goto error;
	}

	if (strcmp(new_url.password, valid_url->password)!=0) {
		printf("password doesn't match, I got %s when I should have received %s\n",new_url.password, valid_url->password);
	goto error;
	}

	if (strcmp(new_url.servername, valid_url->servername)!=0) {
		printf("servername doesn't match, I got %s when I should have received %s\n",new_url.servername, valid_url->servername);
		goto error;
	}

	if (strcmp(new_url.volumename, valid_url->volumename)!=0) {
		printf("volumename doesn't match, I got %s when I should have received %s\n",new_url.volumename, valid_url->volumename);
		goto error;
	}
	return 0;
error:
	return -1;
}
	

