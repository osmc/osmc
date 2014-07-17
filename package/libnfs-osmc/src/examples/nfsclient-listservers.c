/* 
   Copyright (C) by Ronnie Sahlberg <ronniesahlberg@gmail.com> 2011
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/* Example program showing sync interface to probe for all local servers
 */

#include <stdio.h>
#include <stdlib.h>
#include "libnfs.h"


int main(int argc _U_, char *argv[] _U_)
{
	struct nfs_server_list *srvrs;
	struct nfs_server_list *srv;

	srvrs = nfs_find_local_servers();	
	for (srv=srvrs; srv; srv = srv->next) {
		printf("NFS SERVER @ %s\n", srv->addr);
	}
	free_nfs_srvr_list(srvrs);
	return 0;
}
