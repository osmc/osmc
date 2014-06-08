/*


    Copyright (C) 2006 Alex deVries <alexthepuffin@gmail.com>

*/
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>

#include "afp.h"
#include "afp_protocol.h"

#undef DID_CACHE_DISABLE

static unsigned short timeout=10;

struct did_cache_entry {
                                     /* For the example /foo/bar/baz */
	char dirname[AFP_MAX_PATH];  /* full name, eg. /foo/bar/ */
	unsigned int did;            /*            eg  2323        */
	struct timeval time;
	struct did_cache_entry * next;
} ;

int free_entire_did_cache(struct afp_volume * volume) 
{

	struct did_cache_entry * d, * p, *p2;

	pthread_mutex_lock(&volume->did_cache_mutex);

	p=volume->did_cache_base;

	for (d=volume->did_cache_base;d;d=p)
	{
		p2=p;
		p=d->next;
		free(p2);
	}
	pthread_mutex_unlock(&volume->did_cache_mutex);

	return 0;
}

int remove_did_entry(struct afp_volume * volume, const char * name) 
{
	struct did_cache_entry * d, * p=NULL;

	pthread_mutex_lock(&volume->did_cache_mutex);

	for (d=volume->did_cache_base;d;d=d->next)
	{
		if (strcmp(d->dirname,name)==0) {
			if (p) 
				p->next=d->next;
			else 
				volume->did_cache_base=d->next;
			volume->did_cache_stats.force_removed++;
			free(d);
			break;
		} else 
			p=d;
	}
	pthread_mutex_unlock(&volume->did_cache_mutex);
	return 0;
}

	
static int add_did_cache_entry(struct afp_volume * volume, 
	unsigned int new_did, char * path)
{

	struct did_cache_entry * new, *old_base;

	#ifdef DID_CACHE_DISABLE
	return 0;
	#endif

	if ((new=malloc(sizeof(* new)))==NULL) return -1;


	memset(new,0,sizeof(*new));

	new->did=new_did;
	memcpy(new->dirname,path,AFP_MAX_PATH);
	gettimeofday(&new->time,NULL);

	pthread_mutex_lock(&volume->did_cache_mutex);
	old_base=volume->did_cache_base;
	volume->did_cache_base=new;
	new->next=old_base;
	pthread_mutex_unlock(&volume->did_cache_mutex);

	return 0;

}

unsigned char is_dir(struct afp_volume * volume, 
	unsigned int parentdid, const char * path)
{
	int ret;
	unsigned int filebitmap=0;
	unsigned int dirbitmap=0;
	struct afp_file_info fi;
#if 0
	struct did_cache_entry * p;

	if ((p=find_did_cache_entry(volume,parentdid,path,strlen(path)))) 
		return p->isdir;
#endif
	ret =afp_getfiledirparms(volume,parentdid,
		filebitmap,dirbitmap,path,&fi);

	if (ret) return 0;


	return fi.isdir;
}

static unsigned int find_dirid_by_fullname(struct afp_volume * volume,
	char * path)
{
	struct did_cache_entry * p, *prev=volume->did_cache_base;
	struct timeval time;
	unsigned int found_did=0;
	unsigned char breakearly=0;

	#ifdef DID_CACHE_DISABLE
	goto out;
	#endif

	gettimeofday(&time,NULL);

	pthread_mutex_lock(&volume->did_cache_mutex);
	for (p=volume->did_cache_base;p;p=p->next) {
		if (time.tv_sec > (p->time.tv_sec+timeout)) {
			volume->did_cache_stats.expired++;
			if (prev==volume->did_cache_base) {
				if (strcmp(p->dirname,path)==0) breakearly=1;
				volume->did_cache_base=p->next;
				free(p);
				if (breakearly) goto out;
				p=volume->did_cache_base;
				if (!p) goto out;
				prev=volume->did_cache_base;
				continue;
			} else {
				prev->next=p->next;
				free(p);
				p=prev;
			}
		}
		if (strcmp(p->dirname,path)==0) {
			found_did=p->did;
			volume->did_cache_stats.hits++;
			goto out;
		}
		prev=p;
	}
out:
	pthread_mutex_unlock(&volume->did_cache_mutex);
	return found_did;
}


/* This calculates the dirid and basename.  It *always* gets the parent did. */

int get_dirid(struct afp_volume * volume, const char * path, 
	char * basename, unsigned int * dirid)
{
	char * p, *p2;
	int ret;
	struct afp_file_info fi;
	unsigned int filebitmap,dirbitmap;
	unsigned int newdid;
	unsigned int parent_did;
	char copy[AFP_MAX_PATH];

	if (((p=strrchr(path,'/')))==NULL) return -1; 

	/* Calculate the basename, leave copy with just the parent */
	if (basename) {
		memset(basename,0,AFP_MAX_PATH);
		memcpy(basename,p+1,strlen(path)-(p-path)-1);
	}

	/* p now points to the last '/' */

	if (p-path==0) {
		*dirid=AFP_ROOT_DID;
		goto out;
	}

	memcpy(copy,path,p-path+1);

	if (copy[p-path]=='/') copy[p-path]='\0'; /* Lop off the last '/' */

	/* See if the parent's fullname is in the cache */

	if ((newdid=find_dirid_by_fullname(volume,copy))) {
		*dirid=newdid;
		goto out;
	}
	
	/* No?  Work your way back to the start from the end looking
	   for a parent */

	while ((p=strrchr(copy,'/'))) {
		if (p==copy) {
			/* Okay, we're done since we're at the start*/
			parent_did=AFP_ROOT_DID;
			break;
		}
		*p='\0';
		if ((parent_did=find_dirid_by_fullname(volume,copy))) {
			break;
		}
	}

	/* Okay, now we have the topmost cached parent */
	/* Move forward now from the last parentid */
	filebitmap=kFPNodeIDBit ;
	dirbitmap=kFPNodeIDBit ;


	/* Go to the end of last known entry */
	p=path+(p-copy);
	p2=p;

	while ((p=strchr(p+1,'/'))) {

		memset(copy,0,AFP_MAX_PATH);
		memcpy(copy,p2,p-p2);

		volume->did_cache_stats.misses++;

		ret =afp_getfiledirparms(volume,parent_did,
			filebitmap,dirbitmap,copy,&fi);

		if (fi.isdir) {
			/* Add it to the cache */
			memset(copy,0,AFP_MAX_PATH);
			memcpy(copy,path,p-path);
			add_did_cache_entry(volume, fi.fileid,copy);

		} else {
			break;
		}
		parent_did=fi.fileid;
		p2=p;
	}
	*dirid=parent_did;

out:
	return 0;
}

