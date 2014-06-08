#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>

#include "afp.h"
#include "map_def.h"

/* How mapping works
 *
 * There are 3 kinds of user/group mapping:
 *
 * 1. Common user directory
 *
 * This is where the client and server share the same user/group database.  
 * An example of this is where NIS is used.
 *
 * 2. Login ids 
 *
 * This is where all the files appear as the user that logged in.  This would
 * typically be used where the databases are separate, and one user expects to
 * be able to read/write all the files he sees.  This can get confusing, since
 * any files that aren't his will appear to be owned by him, but writing to
 * them will result in an EPERM.
 *
 * 3. Name mapped
 *
 * All the uid/gid translation is done based on usernames.  
 *
 * This is currently unsupported.
 *
 */

int translate_uidgid_to_server(struct afp_volume * volume,
	unsigned int * newuid, unsigned int *newgid) 
{
	switch(volume->mapping) {
		case AFP_MAPPING_COMMON:
			/* The user databases are the same, don't do 
			 * any translaion. */
		break;
		case AFP_MAPPING_LOGINIDS:
			/* Return the value of what we used when we 
 			  logged in.*/

			*newuid=volume->server->server_uid;
			*newgid=volume->server->server_gid;
			break;
		default:
		case AFP_MAPPING_UNKNOWN:
			return -1;
		break;
	}
	return 0;
}

int translate_uidgid_to_client(struct afp_volume * volume,
	unsigned int * newuid, unsigned int *newgid) 
{
	switch(volume->mapping) {
		case AFP_MAPPING_COMMON:
			/* The user databases are the same, don't do 
			 * any translaion. */
		break;
		case AFP_MAPPING_LOGINIDS:
			/* This is the case where we always return the uid/gid
			 * of the user who ran afpfsd.
			 */
			*newuid=volume->server->passwd.pw_uid;
			*newgid=volume->server->passwd.pw_gid;
			break;
		default:
		case AFP_MAPPING_UNKNOWN:
			return -1;
		break;
	}
	return 0;
}

int afp_detect_mapping(struct afp_volume * volume)
{
	char name[255];
	struct passwd * passwd_entry;
	unsigned int dummy;
	unsigned int tmpgid;

	/* See if it is already set.  This is typically when the client
         * requested a specific mapping. */

	if (volume->mapping != AFP_MAPPING_UNKNOWN)
		return 0;

	if ((volume->attributes & kNoNetworkUserIDs) ) {
		volume->mapping = AFP_MAPPING_LOGINIDS;
		return 0;
	}
	/* Check to see if the user databases appear to be synchronous.
         * This is done based on the algorithm specified on p.20.
         */

	/* By default, we're in AFP_MAPPING_LOGINIDS. */

	volume->mapping = AFP_MAPPING_LOGINIDS;
	volume->server->server_gid_valid=0;

	/* 1. call getuid */

	passwd_entry=&volume->server->passwd;

	/* 2. Send FPGetUserInfo to get the user's uid */

	if (afp_getuserinfo(volume->server, 1, /* this user */
		0, /* Irrelevant since we're getting the info for ThisUser */
		kFPGetUserInfo_USER_ID, 
		&volume->server->server_uid,
		&dummy))
		return 0;

	/* In the past, this has not worked for some versions of Mac OS, but	
	 * this hasn't been fully verified */
	if (afp_getuserinfo(volume->server, 1, /* this user */
		0, 
		kFPGetUserInfo_PRI_GROUPID, &dummy,
		&tmpgid)==0) {
			volume->server->server_gid_valid=1;
			volume->server->server_gid=tmpgid;
	}
		
	/* 3. If they match, call getpwuid to get the local user name */

	if (volume->server->server_uid!=passwd_entry->pw_uid) 
		return 0;

	/* 4. send an FPMapID to get the user's username. */

	afp_mapid(volume->server,
		(volume->attributes & kSupportsUTF8Names) 
			? kUserIDToUTF8Name : kUserIDToName,
		passwd_entry->pw_uid,name);


	/* 5. If they match, we're in AFP_MAPPING_COMMON mode. */

	if (strcmp(name,passwd_entry->pw_name)==0) 
		volume->mapping = AFP_MAPPING_COMMON;


	return 0;

}



