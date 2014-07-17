/* NFS part from rfc 1813, NFSACL part is from wireshark sources */

const NFS3_FHSIZE    = 64;    /* Maximum bytes in a V3 file handle */
const NFS3_WRITEVERFSIZE = 8;
const NFS3_CREATEVERFSIZE = 8;
const NFS3_COOKIEVERFSIZE = 8;

typedef opaque cookieverf3[NFS3_COOKIEVERFSIZE];


/*unsigned hyper can be overridden by giving rpcgen -DU_INT64_PLATTFORM_TYPE="foo" - for plattforms
  where rpcgen doesn't know anything about hyper
  default to unsigned hyper as of rfc 1813 */
#ifndef U_INT64_PLATTFORM_TYPE
#define U_INT64_PLATTFORM_TYPE unsigned hyper
#endif/*U_INT64_PLATTFORM_TYPE*/

typedef U_INT64_PLATTFORM_TYPE uint64;
typedef uint64 cookie3;

struct nfs_fh3 {
	opaque       data<NFS3_FHSIZE>;
};

typedef string filename3<>;

struct diropargs3 {
	nfs_fh3     dir;
	filename3   name;
};

enum ftype3 {
	NF3REG    = 1,
	NF3DIR    = 2,
	NF3BLK    = 3,
	NF3CHR    = 4,
	NF3LNK    = 5,
	NF3SOCK   = 6,
	NF3FIFO   = 7
};

typedef unsigned long uint32;

typedef long int32;

typedef uint32 mode3;

typedef uint32 uid3;

typedef uint32 gid3;

typedef uint64 size3;

typedef uint64 fileid3;

struct specdata3 {
	uint32     specdata1;
	uint32     specdata2;
};

struct nfstime3 {
	uint32   seconds;
	uint32   nseconds;
};

struct fattr3 {
	ftype3     type;
	mode3      mode;
	uint32     nlink;
	uid3       uid;
	gid3       gid;
	size3      size;
	size3      used;
	specdata3  rdev;
	uint64     fsid;
	fileid3    fileid;
	nfstime3   atime;
	nfstime3   mtime;
	nfstime3   ctime;
};

union post_op_attr switch (bool attributes_follow) {
	case TRUE:
		fattr3   attributes;
	case FALSE:
		void;
};


enum nfsstat3 {
	NFS3_OK             = 0,
	NFS3ERR_PERM        = 1,
	NFS3ERR_NOENT       = 2,
	NFS3ERR_IO          = 5,
	NFS3ERR_NXIO        = 6,
	NFS3ERR_ACCES       = 13,
	NFS3ERR_EXIST       = 17,
	NFS3ERR_XDEV        = 18,
	NFS3ERR_NODEV       = 19,
	NFS3ERR_NOTDIR      = 20,
	NFS3ERR_ISDIR       = 21,
	NFS3ERR_INVAL       = 22,
	NFS3ERR_FBIG        = 27,
	NFS3ERR_NOSPC       = 28,
	NFS3ERR_ROFS        = 30,
	NFS3ERR_MLINK       = 31,
	NFS3ERR_NAMETOOLONG = 63,
	NFS3ERR_NOTEMPTY    = 66,
	NFS3ERR_DQUOT       = 69,
	NFS3ERR_STALE       = 70,
	NFS3ERR_REMOTE      = 71,
	NFS3ERR_BADHANDLE   = 10001,
	NFS3ERR_NOT_SYNC    = 10002,
	NFS3ERR_BAD_COOKIE  = 10003,
	NFS3ERR_NOTSUPP     = 10004,
	NFS3ERR_TOOSMALL    = 10005,
	NFS3ERR_SERVERFAULT = 10006,
	NFS3ERR_BADTYPE     = 10007,
	NFS3ERR_JUKEBOX     = 10008
};	

enum stable_how {
	UNSTABLE  = 0,
	DATA_SYNC = 1,
	FILE_SYNC = 2
};

typedef uint64 offset3;

typedef uint32 count3;

struct wcc_attr {
	size3       size;
	nfstime3    mtime;
	nfstime3    ctime;
};

union pre_op_attr switch (bool attributes_follow) {
	case TRUE:
		wcc_attr  attributes;
	case FALSE:
		void;
};

struct wcc_data {
	pre_op_attr    before;
	post_op_attr   after;
};

struct WRITE3args {
	nfs_fh3     file;
	offset3     offset;
	count3      count;
	stable_how  stable;
	opaque      data<>;
};

typedef opaque writeverf3[NFS3_WRITEVERFSIZE];

struct WRITE3resok {
	wcc_data    file_wcc;
	count3      count;
	stable_how  committed;
	writeverf3  verf;
};

struct WRITE3resfail {
	wcc_data    file_wcc;
};

union WRITE3res switch (nfsstat3 status) {
	case NFS3_OK:
		WRITE3resok    resok;
	default:
		WRITE3resfail  resfail;
};

struct LOOKUP3args {
	diropargs3  what;
};

struct LOOKUP3resok {
	nfs_fh3      object;
	post_op_attr obj_attributes;
	post_op_attr dir_attributes;
};

struct LOOKUP3resfail {
	post_op_attr dir_attributes;
};



union LOOKUP3res switch (nfsstat3 status) {
	case NFS3_OK:
		LOOKUP3resok    resok;
	default:
		LOOKUP3resfail  resfail;
};

struct COMMIT3args {
	nfs_fh3    file;
	offset3    offset;
	count3     count;
};

struct COMMIT3resok {
	wcc_data   file_wcc;
	writeverf3 verf;
};

struct COMMIT3resfail {
	wcc_data   file_wcc;
};

union COMMIT3res switch (nfsstat3 status) {
	case NFS3_OK:
		COMMIT3resok   resok;
	default:
		COMMIT3resfail resfail;
};

const ACCESS3_READ    = 0x0001;
const ACCESS3_LOOKUP  = 0x0002;
const ACCESS3_MODIFY  = 0x0004;
const ACCESS3_EXTEND  = 0x0008;
const ACCESS3_DELETE  = 0x0010;
const ACCESS3_EXECUTE = 0x0020;

struct ACCESS3args {
     nfs_fh3  object;
     uint32   access;
};

struct ACCESS3resok {
     post_op_attr   obj_attributes;
     uint32         access;
};

struct ACCESS3resfail {
     post_op_attr   obj_attributes;
};

union ACCESS3res switch (nfsstat3 status) {
case NFS3_OK:
     ACCESS3resok   resok;
default:
     ACCESS3resfail resfail;
};

struct GETATTR3args {
	nfs_fh3  object;
};

struct GETATTR3resok {
        fattr3   obj_attributes;
};

union GETATTR3res switch (nfsstat3 status) {
	case NFS3_OK:
		GETATTR3resok  resok;
	default:
		void;
};



enum time_how {
	DONT_CHANGE        = 0,
	SET_TO_SERVER_TIME = 1,
	SET_TO_CLIENT_TIME = 2
};

union set_mode3 switch (bool set_it) {
	case TRUE:
		mode3    mode;
	default:
	void;
};

union set_uid3 switch (bool set_it) {
	case TRUE:
		uid3     uid;
	default:
		void;
};

union set_gid3 switch (bool set_it) {
	case TRUE:
		gid3     gid;
	default:
		void;
};

union set_size3 switch (bool set_it) {
	case TRUE:
		size3    size;
	default:
		void;
};

union set_atime switch (time_how set_it) {
	case SET_TO_CLIENT_TIME:
		nfstime3  atime;
	default:
		void;
};

union set_mtime switch (time_how set_it) {
	case SET_TO_CLIENT_TIME:
		nfstime3  mtime;
	default:
		void;
};

struct sattr3 {
	set_mode3   mode;
	set_uid3    uid;
	set_gid3    gid;
	set_size3   size;
	set_atime   atime;
	set_mtime   mtime;
};

enum createmode3 {
	UNCHECKED = 0,
	GUARDED   = 1,
	EXCLUSIVE = 2
};


typedef opaque createverf3[NFS3_CREATEVERFSIZE];

union createhow3 switch (createmode3 mode) {
	case UNCHECKED:
		sattr3       obj_attributes;
	case GUARDED:
		sattr3       g_obj_attributes;
	case EXCLUSIVE:
		createverf3  verf;
};

struct CREATE3args {
	diropargs3   where;
	createhow3   how;
};

union post_op_fh3 switch (bool handle_follows) {
	case TRUE:
		nfs_fh3  handle;
	case FALSE:
		void;
};

struct CREATE3resok {
	post_op_fh3   obj;
	post_op_attr  obj_attributes;
	wcc_data      dir_wcc;
};

struct CREATE3resfail {
	wcc_data      dir_wcc;
	};

union CREATE3res switch (nfsstat3 status) {
	case NFS3_OK:
		CREATE3resok    resok;
	default:
		CREATE3resfail  resfail;
};

struct REMOVE3args {
	diropargs3  object;
};

struct REMOVE3resok {
	wcc_data    dir_wcc;
};

struct REMOVE3resfail {
	wcc_data    dir_wcc;
};

union REMOVE3res switch (nfsstat3 status) {
	case NFS3_OK:
		REMOVE3resok   resok;
	default:
	REMOVE3resfail resfail;
};

struct READ3args {
	nfs_fh3  file;
	offset3  offset;
	count3   count;
};

struct READ3resok {
	post_op_attr   file_attributes;
	count3         count;
	bool           eof;
	opaque         data<>;
};

struct READ3resfail {
	post_op_attr   file_attributes;
};

union READ3res switch (nfsstat3 status) {
	case NFS3_OK:
		READ3resok   resok;
	default:
		READ3resfail resfail;
};


const FSF3_LINK        = 0x0001;
const FSF3_SYMLINK     = 0x0002;
const FSF3_HOMOGENEOUS = 0x0008;
const FSF3_CANSETTIME  = 0x0010;

struct FSINFO3args {
	nfs_fh3   fsroot;
};

struct FSINFO3resok {
	post_op_attr obj_attributes;
	uint32       rtmax;
	uint32       rtpref;
	uint32       rtmult;
	uint32       wtmax;
	uint32       wtpref;
	uint32       wtmult;
	uint32       dtpref;
	size3        maxfilesize;
	nfstime3     time_delta;
	uint32       properties;
};

struct FSINFO3resfail {
	post_op_attr obj_attributes;
};

union FSINFO3res switch (nfsstat3 status) {
	case NFS3_OK:
		FSINFO3resok   resok;
	default:
		FSINFO3resfail resfail;
};


struct FSSTAT3args {
	nfs_fh3   fsroot;
};

struct FSSTAT3resok {
	post_op_attr obj_attributes;
	size3        tbytes;
	size3        fbytes;
	size3        abytes;
	size3        tfiles;
	size3        ffiles;
	size3        afiles;
	uint32       invarsec;
};

struct FSSTAT3resfail {
	post_op_attr obj_attributes;
};

union FSSTAT3res switch (nfsstat3 status) {
	case NFS3_OK:
		FSSTAT3resok   resok;
	default:
		FSSTAT3resfail resfail;
};

struct PATHCONF3args {
	nfs_fh3   object;
};

struct PATHCONF3resok {
	post_op_attr obj_attributes;
	uint32       linkmax;
	uint32       name_max;
	bool         no_trunc;
	bool         chown_restricted;
	bool         case_insensitive;
	bool         case_preserving;
};

struct PATHCONF3resfail {
	post_op_attr obj_attributes;
};

union PATHCONF3res switch (nfsstat3 status) {
	case NFS3_OK:
		PATHCONF3resok   resok;
	default:
		PATHCONF3resfail resfail;
};

typedef string nfspath3<>;

struct symlinkdata3 {
	sattr3    symlink_attributes;
	nfspath3  symlink_data;
};

struct SYMLINK3args {
	diropargs3    where;
	symlinkdata3  symlink;
};

struct SYMLINK3resok {
	post_op_fh3   obj;
	post_op_attr  obj_attributes;
	wcc_data      dir_wcc;
};

struct SYMLINK3resfail {
	wcc_data      dir_wcc;
};

union SYMLINK3res switch (nfsstat3 status) {
	case NFS3_OK:
		SYMLINK3resok   resok;
	default:
		SYMLINK3resfail resfail;
};


struct READLINK3args {
	nfs_fh3  symlink;
};

struct READLINK3resok {
	post_op_attr   symlink_attributes;
	nfspath3       data;
};

struct READLINK3resfail {
	post_op_attr   symlink_attributes;
};

union READLINK3res switch (nfsstat3 status) {
	case NFS3_OK:
		READLINK3resok   resok;
	default:
	READLINK3resfail resfail;
};


struct devicedata3 {
	sattr3     dev_attributes;
	specdata3  spec;
};

union mknoddata3 switch (ftype3 type) {
	case NF3CHR:
		devicedata3  chr_device;
	case NF3BLK:
		devicedata3  blk_device;
	case NF3SOCK:
		sattr3       sock_attributes;
	case NF3FIFO:
		sattr3       pipe_attributes;
	default:
		void;
};

struct MKNOD3args {
	diropargs3   where;
	mknoddata3   what;
};

struct MKNOD3resok {
	post_op_fh3   obj;
	post_op_attr  obj_attributes;
	wcc_data      dir_wcc;
};

struct MKNOD3resfail {
	wcc_data      dir_wcc;
};

union MKNOD3res switch (nfsstat3 status) {
	case NFS3_OK:
		MKNOD3resok   resok;
	default:
		MKNOD3resfail resfail;
};


struct MKDIR3args {
	diropargs3   where;
	sattr3       attributes;
};

struct MKDIR3resok {
	post_op_fh3   obj;
	post_op_attr  obj_attributes;
	wcc_data      dir_wcc;
};

struct MKDIR3resfail {
	wcc_data      dir_wcc;
};

union MKDIR3res switch (nfsstat3 status) {
	case NFS3_OK:
		MKDIR3resok   resok;
	default:
		MKDIR3resfail resfail;
};

struct RMDIR3args {
	diropargs3  object;
};

struct RMDIR3resok {
	wcc_data    dir_wcc;
};

struct RMDIR3resfail {
	wcc_data    dir_wcc;
};

union RMDIR3res switch (nfsstat3 status) {
	case NFS3_OK:
		RMDIR3resok   resok;
	default:
		RMDIR3resfail resfail;
};

struct RENAME3args {
	diropargs3   from;
	diropargs3   to;
};

struct RENAME3resok {
	wcc_data     fromdir_wcc;
	wcc_data     todir_wcc;
};

struct RENAME3resfail {
	wcc_data     fromdir_wcc;
	wcc_data     todir_wcc;
};

union RENAME3res switch (nfsstat3 status) {
	case NFS3_OK:
		RENAME3resok   resok;
	default:
		RENAME3resfail resfail;
};

struct READDIRPLUS3args {
	nfs_fh3      dir;
	cookie3      cookie;
	cookieverf3  cookieverf;
	count3       dircount;
	count3       maxcount;
};

struct entryplus3 {
	fileid3      fileid;
	filename3    name;
	cookie3      cookie;
	post_op_attr name_attributes;
	post_op_fh3  name_handle;
	entryplus3   *nextentry;
};

struct dirlistplus3 {
	entryplus3   *entries;
	bool         eof;
};

struct READDIRPLUS3resok {
	post_op_attr dir_attributes;
	cookieverf3  cookieverf;
	dirlistplus3 reply;
};


struct READDIRPLUS3resfail {
	post_op_attr dir_attributes;
};

union READDIRPLUS3res switch (nfsstat3 status) {
	case NFS3_OK:
		READDIRPLUS3resok   resok;
	default:
		READDIRPLUS3resfail resfail;
};

struct READDIR3args {
	nfs_fh3      dir;
	cookie3      cookie;
	cookieverf3  cookieverf;
	count3       count;
};


struct entry3 {
	fileid3      fileid;
	filename3    name;
	cookie3      cookie;
	entry3       *nextentry;
};

struct dirlist3 {
	entry3	*entries;
	bool    eof;
};

struct READDIR3resok {
	post_op_attr dir_attributes;
	cookieverf3  cookieverf;
	dirlist3     reply;
};

struct READDIR3resfail {
	post_op_attr dir_attributes;
};

union READDIR3res switch (nfsstat3 status) {
	case NFS3_OK:
		READDIR3resok   resok;
	default:
		READDIR3resfail resfail;
};

struct LINK3args {
	nfs_fh3     file;
	diropargs3  link;
};

struct LINK3resok {
	post_op_attr   file_attributes;
	wcc_data       linkdir_wcc;
};

struct LINK3resfail {
	post_op_attr   file_attributes;
	wcc_data       linkdir_wcc;
};

union LINK3res switch (nfsstat3 status) {
	case NFS3_OK:
		LINK3resok    resok;
	default:
		LINK3resfail  resfail;
};

union sattrguard3 switch (bool check) {
	case TRUE:
		nfstime3  obj_ctime;
	case FALSE:
		void;
};

struct SETATTR3args {
	nfs_fh3      object;
	sattr3       new_attributes;
	sattrguard3  guard;
};

struct SETATTR3resok {
	wcc_data  obj_wcc;
};

struct SETATTR3resfail {
	wcc_data  obj_wcc;
};

union SETATTR3res switch (nfsstat3 status) {
	case NFS3_OK:
		SETATTR3resok   resok;
	default:
		SETATTR3resfail resfail;
};

program NFS_PROGRAM {
	version NFS_V3 {
		void
		NFS3_NULL(void)                    = 0;

		GETATTR3res
		NFS3_GETATTR(GETATTR3args)         = 1;

		SETATTR3res
		NFS3_SETATTR(SETATTR3args)         = 2;

		LOOKUP3res
		NFS3_LOOKUP(LOOKUP3args)           = 3;

		ACCESS3res
		NFS3_ACCESS(ACCESS3args)           = 4;

		READLINK3res
		NFS3_READLINK(READLINK3args)       = 5;

		READ3res
		NFS3_READ(READ3args)               = 6;

		WRITE3res
		NFS3_WRITE(WRITE3args)             = 7;

		CREATE3res
		NFS3_CREATE(CREATE3args)           = 8;

		MKDIR3res
		NFS3_MKDIR(MKDIR3args)             = 9;

		SYMLINK3res
		NFS3_SYMLINK(SYMLINK3args)         = 10;

		MKNOD3res
		NFS3_MKNOD(MKNOD3args)             = 11;

		REMOVE3res
		NFS3_REMOVE(REMOVE3args)           = 12;

		RMDIR3res
		NFS3_RMDIR(RMDIR3args)             = 13;

		RENAME3res
		NFS3_RENAME(RENAME3args)           = 14;

		LINK3res
		NFS3_LINK(LINK3args)               = 15;

		READDIR3res
		NFS3_READDIR(READDIR3args)         = 16;

		READDIRPLUS3res
		NFS3_READDIRPLUS(READDIRPLUS3args) = 17;

		FSSTAT3res
		NFS3_FSSTAT(FSSTAT3args)           = 18;

		FSINFO3res
		NFS3_FSINFO(FSINFO3args)           = 19;

		PATHCONF3res
		NFS3_PATHCONF(PATHCONF3args)       = 20;

		COMMIT3res
		NFS3_COMMIT(COMMIT3args)           = 21;
	} = 3;
} = 100003;



/* NFS ACL definitions based on wireshark souces and network traces */
/* NFSACL interface. Uses same port/process as NFS */

enum nfsacl_type {
     NFSACL_TYPE_USER_OBJ	   = 0x0001,
     NFSACL_TYPE_USER		   = 0x0002,
     NFSACL_TYPE_GROUP_OBJ	   = 0x0004,
     NFSACL_TYPE_GROUP		   = 0x0008,
     NFSACL_TYPE_CLASS_OBJ	   = 0x0010,
     NFSACL_TYPE_CLASS		   = 0x0020,
     NFSACL_TYPE_DEFAULT	   = 0x1000,
     NFSACL_TYPE_DEFAULT_USER_OBJ  = 0x1001, 
     NFSACL_TYPE_DEFAULT_USER      = 0x1002,
     NFSACL_TYPE_DEFAULT_GROUP_OBJ = 0x1004,
     NFSACL_TYPE_DEFAULT_GROUP     = 0x1008,
     NFSACL_TYPE_DEFAULT_CLASS_OBJ = 0x1010,
     NFSACL_TYPE_DEFAULT_OTHER_OBJ = 0x1020
};

const NFSACL_PERM_READ  = 0x04;
const NFSACL_PERM_WRITE = 0x02;
const NFSACL_PERM_EXEC  = 0x01;

struct nfsacl_ace {
       enum nfsacl_type type;
       uint32_t id;
       uint32_t perm;
};

const NFSACL_MASK_ACL_ENTRY         = 0x0001;
const NFSACL_MASK_ACL_COUNT         = 0x0002;
const NFSACL_MASK_ACL_DEFAULT_ENTRY = 0x0004;
const NFSACL_MASK_ACL_DEFAULT_COUNT = 0x0008;

struct GETACL3args {
	nfs_fh3     dir;
	uint32	    mask;
};

struct GETACL3resok {
	post_op_attr   attr;
	uint32_t       mask;
	uint32_t       ace_count;
	struct nfsacl_ace ace<>;
	uint32_t       default_ace_count;
	struct nfsacl_ace default_ace<>;
};

union GETACL3res switch (nfsstat3 status) {
case NFS3_OK:
     GETACL3resok   resok;
default:
     void;
};

program NFSACL_PROGRAM {
	version NFSACL_V3 {
		void
		NFSACL3_NULL(void)                    = 0;

		GETACL3res
		NFSACL3_GETACL(GETACL3args)           = 1;
	} = 3;
} = 100227;
