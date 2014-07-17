/*
   Copyright (C) 2010 by Ronnie Sahlberg <ronniesahlberg@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/
/*
 * This is the lowlevel interface to access NFS resources.
 * Through this interface you have access to the full gamut of nfs and nfs related
 * protocol as well as the XDR encoded/decoded structures.
 */
#include <stdint.h>
#include <rpc/rpc.h>
#include <rpc/auth.h>

struct rpc_data {
       int size;
       unsigned char *data;
};

struct rpc_context;
struct rpc_context *rpc_init_context(void);
void rpc_destroy_context(struct rpc_context *rpc);

void rpc_set_auth(struct rpc_context *rpc, AUTH *auth);

int rpc_get_fd(struct rpc_context *rpc);
int rpc_which_events(struct rpc_context *rpc);
int rpc_service(struct rpc_context *rpc, int revents);
char *rpc_get_error(struct rpc_context *rpc);
int rpc_queue_length(struct rpc_context *rpc);


#define RPC_STATUS_SUCCESS	   	0
#define RPC_STATUS_ERROR		1
#define RPC_STATUS_CANCEL		2

/*
 * Async connection to the tcp port at server:port.
 * Function returns
 *  0 : The connection was initiated. Once the connection establish finishes, the callback will be invoked.
 * <0 : An error occured when trying to set up the connection. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : The tcp connection was successfully established.
 *                      data is NULL.
 * RPC_STATUS_ERROR   : The connection failed to establish.
 *                      data is the erro string.
 * RPC_STATUS_CANCEL  : The connection attempt was aborted before it could complete.
 *                    : data is NULL.
 */
int rpc_connect_async(struct rpc_context *rpc, const char *server, int port, rpc_cb cb, void *private_data);
/*
 * When disconnecting a connection in flight. All commands in flight will be called with the callback
 * and status RPC_STATUS_ERROR. Data will be the error string for the disconnection.
 */
int rpc_disconnect(struct rpc_context *rpc, char *error);


/* 
 * PORTMAP FUNCTIONS
 */

/*
 * Call PORTMAPPER/NULL
 * Function returns
 *  0 : The connection was initiated. Once the connection establish finishes, the callback will be invoked.
 * <0 : An error occured when trying to set up the connection. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the portmapper daemon.
 *                      data is NULL.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the portmapper.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_pmap_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data);


/*
 * Call PORTMAPPER/GETPORT.
 * Function returns
 *  0 : The connection was initiated. Once the connection establish finishes, the callback will be invoked.
 * <0 : An error occured when trying to set up the connection. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the portmapper daemon.
 *                      data is a (uint32_t *), containing the port returned.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the portmapper.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_pmap_getport_async(struct rpc_context *rpc, int program, int version, int protocol, rpc_cb cb, void *private_data);

/*
 * Call PORTMAPPER/SET
 * Function returns
 *  0 : The connection was initiated. Once the connection establish finishes, the callback will be invoked.
 * <0 : An error occured when trying to set up the connection. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the portmapper daemon.
 *                      data is a (uint32_t *), containing status
 * RPC_STATUS_ERROR   : An error occured when trying to contact the portmapper.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_pmap_set_async(struct rpc_context *rpc, int program, int version, int protocol, int port, rpc_cb cb, void *private_data);

/*
 * Call PORTMAPPER/UNSET
 * Function returns
 *  0 : The connection was initiated. Once the connection establish finishes, the callback will be invoked.
 * <0 : An error occured when trying to set up the connection. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the portmapper daemon.
 *                      data is a (uint32_t *), containing status
 * RPC_STATUS_ERROR   : An error occured when trying to contact the portmapper.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_pmap_unset_async(struct rpc_context *rpc, int program, int version, int protocol, int port, rpc_cb cb, void *private_data);

/*
 * Call PORTMAPPER/CALLIT.
 * Function returns
 *  0 : The connection was initiated. Once the connection establish finishes, the callback will be invoked.
 * <0 : An error occured when trying to set up the connection. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the portmapper daemon
 *                      data is a 'pmap_call_result' pointer.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the portmapper.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_pmap_callit_async(struct rpc_context *rpc, int program, int version, int procedure, const char *data, int datalen, rpc_cb cb, void *private_data);

/* 
 * MOUNT FUNCTIONS
 */
char *mountstat3_to_str(int stat);
int mountstat3_to_errno(int error);

/*
 * Call MOUNT/NULL
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the mount daemon.
 *                      data is NULL.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the mount daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_mount_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data);

/*
 * Call MOUNT/MNT
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the mount daemon.
 *                      data is union mountres3.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the mount daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_mount_mnt_async(struct rpc_context *rpc, rpc_cb cb, char *export, void *private_data);

/*
 * Call MOUNT/DUMP
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the mount daemon.
 *                      data is a mountlist.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the mount daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_mount_dump_async(struct rpc_context *rpc, rpc_cb cb, void *private_data);

/*
 * Call MOUNT/UMNT
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the mount daemon.
 *                      data NULL.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the mount daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_mount_umnt_async(struct rpc_context *rpc, rpc_cb cb, char *export, void *private_data);

/*
 * Call MOUNT/UMNTALL
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the mount daemon.
 *                      data NULL.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the mount daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_mount_umntall_async(struct rpc_context *rpc, rpc_cb cb, void *private_data);

/*
 * Call MOUNT/EXPORT
 * NOTE: You must include 'libnfs-raw-mount.h' to get the definitions of the
 * returned structures.
 *
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the mount daemon.
 *                      data is a pointer to an exports pointer:
 *                      exports export = *(exports *)data;
 * RPC_STATUS_ERROR   : An error occured when trying to contact the mount daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_mount_export_async(struct rpc_context *rpc, rpc_cb cb, void *private_data);




/* 
 * NFS FUNCTIONS
 */
struct nfs_fh3;
char *nfsstat3_to_str(int error);
int nfsstat3_to_errno(int error);

/*
 * Call NFS/NULL
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is NULL.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data);

/*
 * Call NFS/GETATTR
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is GETATTR3res
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_getattr_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, void *private_data);

/*
 * Call NFS/LOOKUP
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is LOOKUP3res
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_lookup_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, char *name, void *private_data);

/*
 * Call NFS/ACCESS
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is ACCESS3res
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_access_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, int access, void *private_data);

/*
 * Call NFS/READ
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is READ3res
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_read_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, uint64_t offset, uint64_t count, void *private_data);

/*
 * Call NFS/WRITE
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is WRITE3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_write_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, char *buf, uint64_t offset, uint64_t count, int stable_how, void *private_data);

/*
 * Call NFS/COMMIT
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is COMMIT3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_commit_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, void *private_data);


/*
 * Call NFS/SETATTR
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is SETATTR3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
struct SETATTR3args;
int rpc_nfs_setattr_async(struct rpc_context *rpc, rpc_cb cb, struct SETATTR3args *args, void *private_data);



/*
 * Call NFS/MKDIR
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is MKDIR3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
struct MKDIR3args;
int rpc_nfs_mkdir_async(struct rpc_context *rpc, rpc_cb cb, struct MKDIR3args *args, void *private_data);





/*
 * Call NFS/RMDIR
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is RMDIR3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_rmdir_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, char *dir, void *private_data);




/*
 * Call NFS/CREATE
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is CREATE3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
struct CREATE3args;
int rpc_nfs_create_async(struct rpc_context *rpc, rpc_cb cb, struct CREATE3args *args, void *private_data);


/*
 * Call NFS/MKNOD
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is MKNOD3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_mknod_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, char *file, int mode, int major, int minor, void *private_data);


/*
 * Call NFS/REMOVE
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is REMOVE3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_remove_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, char *name, void *private_data);



/*
 * Call NFS/READDIR
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is READDIR3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_readdir_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, uint64_t cookie, char *cookieverf, int count, void *private_data);

/*
 * Call NFS/READDIRPLUS
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is READDIRPLUS3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_readdirplus_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, uint64_t cookie, char *cookieverf, int count, void *private_data);

/*
 * Call NFS/FSSTAT
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is FSSTAT3res
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_fsstat_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, void *private_data);



/*
 * Call NFS/FSINFO
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is FSINFO3res
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_fsinfo_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, void *private_data);



/*
 * Call NFS/READLINK
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is READLINK3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
struct READLINK3args;
int rpc_nfs_readlink_async(struct rpc_context *rpc, rpc_cb cb, struct READLINK3args *args, void *private_data);



/*
 * Call NFS/SYMLINK
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is SYMLINK3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
struct SYMLINK3args;
int rpc_nfs_symlink_async(struct rpc_context *rpc, rpc_cb cb, struct SYMLINK3args *args, void *private_data);


/*
 * Call NFS/RENAME
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is RENAME3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_rename_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *olddir, char *oldname, struct nfs_fh3 *newdir, char *newname, void *private_data);



/*
 * Call NFS/LINK
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the nfs daemon.
 *                      data is LINK3res *
 * RPC_STATUS_ERROR   : An error occured when trying to contact the nfs daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfs_link_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *file, struct nfs_fh3 *newdir, char *newname, void *private_data);




/* 
 * RQUOTA FUNCTIONS
 */
char *rquotastat_to_str(int error);
int rquotastat_to_errno(int error);

/*
 * Call RQUOTA1/NULL
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the rquota daemon.
 *                      data is NULL.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the rquota daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_rquota1_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data);

/*
 * Call RQUOTA1/GETQUOTA
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the rquota daemon.
 *                      data is a RQUOTA1res structure.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the rquota daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_rquota1_getquota_async(struct rpc_context *rpc, rpc_cb cb, char *export, int uid, void *private_data);

/*
 * Call RQUOTA1/GETACTIVEQUOTA
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the rquota daemon.
 *                      data is a RQUOTA1res structure.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the rquota daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_rquota1_getactivequota_async(struct rpc_context *rpc, rpc_cb cb, char *export, int uid, void *private_data);




/*
 * Call RQUOTA2/NULL
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the rquota daemon.
 *                      data is NULL.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the rquota daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_rquota2_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data);

/*
 * Call RQUOTA2/GETQUOTA
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the rquota daemon.
 *                      data is a RQUOTA1res structure.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the rquota daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_rquota2_getquota_async(struct rpc_context *rpc, rpc_cb cb, char *export, int type, int uid, void *private_data);

/*
 * Call RQUOTA2/GETACTIVEQUOTA
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the rquota daemon.
 *                      data is a RQUOTA1res structure.
 * RPC_STATUS_ERROR   : An error occured when trying to contact the rquota daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_rquota2_getactivequota_async(struct rpc_context *rpc, rpc_cb cb, char *export, int type, int uid, void *private_data);



/*
 * Call NFSACL/NULL
 * Call the NULL procedure for the NFSACL
 *
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the rquota daemon.
 *                      data is NULL
 * RPC_STATUS_ERROR   : An error occured when trying to contact the rquota daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfsacl_null_async(struct rpc_context *rpc, rpc_cb cb, void *private_data);

/*
 * Call NFSACL/GETACL
 *
 * Function returns
 *  0 : The call was initiated. The callback will be invoked when the call completes.
 * <0 : An error occured when trying to set up the call. The callback will not be invoked.
 *
 * When the callback is invoked, status indicates the result:
 * RPC_STATUS_SUCCESS : We got a successful response from the rquota daemon.
 *                      data is a GETACL3res pointer
 * RPC_STATUS_ERROR   : An error occured when trying to contact the rquota daemon.
 *                      data is the error string.
 * RPC_STATUS_CANCEL : The connection attempt was aborted before it could complete.
 *                     data is NULL.
 */
int rpc_nfsacl_getacl_async(struct rpc_context *rpc, rpc_cb cb, struct nfs_fh3 *fh, uint32_t mask, void *private_data);

