#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "afp.h"
#include "resource.h"
#include "lowlevel.h"
#include "did.h"
#include "midlevel.h"

#define appledouble ".AppleDouble"
#define finderinfo_string ".finderinfo"
#define comment_string ".comment"
#define servericon_string "/.servericon"

#define min(a,b) (((a)<(b)) ? (a) : (b))

static unsigned int extra_translate(
	struct afp_volume * volume, const char * path,
	char ** newpath_p)
{

	char * p;
	char * newpath;
	*newpath_p=NULL;
	if (~volume->extra_flags & VOLUME_EXTRA_FLAGS_SHOW_APPLEDOUBLE) 
		return 0;

	if (strcmp(path,servericon_string)==0)
		return AFP_META_SERVER_ICON;

	if (strlen(path)<(strlen(appledouble)+1))
		return 0;

	if ((p=strrchr(path,'/'))==NULL) 
		return 0;

	p+=1;

	if (strcmp(p,appledouble)==0) {

		/* Okay, all we have is /foo/.AppleDouble */
		/* Make a copy, but not the last .AppleDouble */
		newpath = malloc(strlen(path));
		memset(newpath,0,strlen(path));
		if (strlen(path)==strlen(appledouble)+1) 
			newpath[0]='/';
		else 
			memcpy(newpath,path,strlen(path)-(strlen(appledouble)+1));
		*newpath_p=newpath;

		return AFP_META_APPLEDOUBLE;
	}

	if ((p=strstr(path,appledouble))) {
		/* So we have /foo/.AppleDouble/blah.  */
		newpath = malloc(strlen(path));
		memset(newpath,0,strlen(path));
		memcpy(newpath,path,p-path);
		p+=strlen(appledouble);
		strcat(newpath,p+1);
		*newpath_p=newpath;

		/* If the ending is .finderinfo, tag it */


		if (strlen(newpath)>strlen(finderinfo_string)) {
			p=newpath+strlen(newpath)-strlen(finderinfo_string);
			if (strcmp(p,finderinfo_string)==0) {
				*p='\0';
				return AFP_META_FINDERINFO;
			}
		}

		if (strlen(newpath)>strlen(comment_string)) {
			p=newpath+strlen(newpath)-strlen(comment_string);
			if (strcmp(p,comment_string)==0) {
				*p='\0';
				return AFP_META_COMMENT;
			}
		}

		return AFP_META_RESOURCE;
	}

	return 0;
}

static int ensure_dt_opened(struct afp_volume * volume)
{

	if (volume->dtrefnum>0) return 0;

	return (afp_opendt(volume,&volume->dtrefnum));

}

static int get_comment_size(struct afp_volume * volume, const char * basename,
	unsigned int did)
{
	struct afp_comment comment;
	int ret=0;
	int size=1024;

	if ((comment.data = malloc(size))==NULL)
		return -1;
	comment.maxsize=size;
	comment.size=0;
	if (volume->dtrefnum==0) 
		if (afp_opendt(volume,&volume->dtrefnum) <0) {
			ret=-EIO;
			goto out;
		}

	ret=(afp_getcomment(volume,did, basename, &comment));
	switch(ret) {
		case kFPAccessDenied:
			ret=-EACCES;
			break;
		case kFPMiscErr:
		case kFPParamErr:
			ret=-EIO;
			break;
		case kFPItemNotFound:
		case kFPObjectNotFound:
			ret=-ENOENT;
			break;
		case kFPNoErr:
			ret=comment.size;
		default:
		break;
	}
out:
	free(comment.data);
	return ret;
}

int appledouble_creat(struct afp_volume * volume, const char * path, mode_t mode)
{
	int resource;
	char * newpath;
	resource = extra_translate(volume, path, &newpath);
	switch(resource) {
		case AFP_META_RESOURCE:
			free(newpath);
			return 1;
		case AFP_META_APPLEDOUBLE:
			free(newpath);
			return -EBADF;
		case AFP_META_SERVER_ICON:
			return -EPERM;
			return 1;
		case AFP_META_FINDERINFO:
			free(newpath);
			return 1;
	}
	return 0;
}

int appledouble_chmod(struct afp_volume * volume, const char * path, mode_t mode)
{
	int resource;
	char * newpath;

	/* There's no resource you can chmod */
	resource = extra_translate(volume, path, &newpath);
	free(newpath);
	if (resource==0) return 0;
	return -EPERM;
}

int appledouble_unlink(struct afp_volume * volume, const char *path)
{
	int resource;
	char * newpath;

	/* There's no resource you can unlink */
	resource = extra_translate(volume, path, &newpath);
	free(newpath);
	if (resource==0) return 0;
	return -EPERM;
}

int appledouble_close(struct afp_volume * volume, struct afp_file_info * fp)
{
	switch(fp->resource) {
		case AFP_META_RESOURCE:
			switch(afp_closefork(volume,fp->forkid)) {
			case kFPNoErr:
				break;
			default:
			case kFPParamErr:
			case kFPMiscErr:
				return -EIO;
			}

			return 0;
		case AFP_META_APPLEDOUBLE:
			return -EBADF;
		case AFP_META_SERVER_ICON:
			return 1;
		case AFP_META_FINDERINFO:
			return 0;
	}
	return 0;
}

int appledouble_write(struct afp_volume * volume, struct afp_file_info *fp,
                const char *data, size_t size, off_t offset, size_t *totalwritten)
{
	int ret;
	int towrite=size;
	unsigned int did;
	struct afp_file_info fp2;

	switch(fp->resource) {
		case AFP_META_RESOURCE:
			ret=ll_write(volume,data,size,offset,fp,totalwritten);
			return ret;
		case AFP_META_APPLEDOUBLE:
			return -EBADF;
		case AFP_META_FINDERINFO:
			if (offset>=32) return -EINVAL;
			if (towrite>(32-offset)) towrite=32-offset;

			/* Get the previous finderinfo */
			ret=ll_get_directory_entry(volume,fp->basename,fp->did,
				kFPFinderInfoBit,kFPFinderInfoBit,&fp2);
			if (ret<0) return ret;

			/* Copy in only the parts we got */
			memcpy(fp->finderinfo+offset,data,towrite);

			ret=afp_setfiledirparms(volume,
					fp->did,fp->basename,
					kFPFinderInfoBit, fp);
			switch(ret) {
				case kFPNoErr:
					break;
				case kFPAccessDenied:
					return -EACCES;
				case kFPObjectNotFound:
					return -ENOENT;
				case kFPBitmapErr:
				case kFPMiscErr:
				case kFPObjectTypeErr:
				case kFPParamErr:
				default:
					break;

			}
			*totalwritten=towrite;

			return 1;
		case AFP_META_COMMENT:
			switch(afp_addcomment(volume, fp->did,fp->basename,
				(char *)data,(uint64_t *) totalwritten)) {
			case kFPAccessDenied:
				return -EACCES;
			case kFPObjectNotFound:
				return -ENOENT;
			case kFPNoErr:
                		*totalwritten=size;
				return 1;
			case kFPMiscErr:
			default:
				return -EIO;
			}
		case AFP_META_SERVER_ICON:
			return -EPERM;

	}
	return 0;


}

int appledouble_read(struct afp_volume * volume, struct afp_file_info *fp,
	char * buf, size_t size, off_t offset, size_t * amount_read,
	int * eof)
{
	int tocopy;
	int ret;
	struct afp_comment comment;
	*amount_read=0;
	*eof=0;

	comment.data=malloc(size);
	comment.maxsize=size;

	switch(fp->resource) {
		case AFP_META_RESOURCE:
			ret=ll_read(volume,buf,size,offset,fp,eof);
			return ret;
		case AFP_META_APPLEDOUBLE:
			return -EBADF;
		case AFP_META_FINDERINFO:
			if (offset>32) return -EFAULT;
			ret=ll_get_directory_entry(volume,fp->basename,fp->did,
				kFPFinderInfoBit,kFPFinderInfoBit,fp);
			if (ret<0) return ret;
			tocopy=min(size,(32-offset));
			memcpy(buf+offset,fp->finderinfo,tocopy);
			if (offset+tocopy==32) *eof=1;
			*amount_read=tocopy;
		case AFP_META_COMMENT:
			if (fp->eof)  ret=1;  else 
			switch(afp_getcomment(volume,fp->did, fp->basename, &comment)) {
				case kFPAccessDenied:
					ret=-EACCES;
					break;
				case kFPMiscErr:
				case kFPParamErr:
					ret=-EIO;
					break;
				case kFPItemNotFound:
				case kFPObjectNotFound:
					ret=-ENOENT;
					break;
				case kFPNoErr:
					memcpy(buf,comment.data,comment.size);
					*amount_read =comment.size;
					ret=1;
					*eof=1;
					fp->eof=1;
				default:
				break;
			}
			free(comment.data);
			return ret;
		case AFP_META_SERVER_ICON:
			if (offset>256) return -EFAULT;
			tocopy=min(size,(256-offset));
			memcpy(buf+offset,volume->server->icon,tocopy);
			*eof=1;
			fp->eof=1;
			*amount_read=tocopy;
			return 1;
	}
	return 0;
}

int appledouble_truncate(struct afp_volume * volume, const char * path, int offset)
{

	char * newpath;
	int resource = extra_translate(volume, path, &newpath);
	struct afp_file_info fp;
	int ret;
	int dirid;
	char basename[AFP_MAX_PATH];

	switch(resource) {
		case AFP_META_RESOURCE:

			get_dirid(volume,newpath,basename,&dirid);

			ret=afp_openfork(volume,1,dirid,O_WRONLY,
				basename,&fp);

			ret=ll_zero_file(volume,fp.forkid,0);
			if (ret<0) {
				afp_closefork(volume,fp.forkid);
				remove_opened_fork(volume,fp);
				free(newpath);
				return ret;
			}
			afp_closefork(volume,fp.forkid);
			remove_opened_fork(volume,fp);

			return 1;
		case AFP_META_APPLEDOUBLE:
			free(newpath);
			return -EISDIR;
		case AFP_META_FINDERINFO:
			free(newpath);
			return 1;
		case AFP_META_COMMENT:
			free(newpath);
			return 1;
		case AFP_META_SERVER_ICON:
			free(newpath);
			return -EPERM;
	}
	return 0;
}

int appledouble_open(struct afp_volume * volume, const char * path, int flags, 
	struct afp_file_info *fp)
{
	char * newpath;
	int ret;

	switch((fp->resource = extra_translate(volume, path, &newpath))) {
		case AFP_META_RESOURCE:
			if (get_dirid(volume,newpath,fp->basename,&fp->did)<0) {
				ret=-ENOENT;
				goto error;
			}
			ret=ll_open(volume,newpath,flags,fp);
			free(newpath);
			if (ret<0) return ret;
			return 1;
		case AFP_META_APPLEDOUBLE:
			free(newpath);
			return -EISDIR;
		case AFP_META_FINDERINFO:
			if (get_dirid(volume,newpath,fp->basename,&fp->did)<0)
				return -ENOENT;
			free(newpath);
			return 1;
		case AFP_META_COMMENT:
			if (get_dirid(volume,newpath,fp->basename,&fp->did)<0) {
				ret=-ENOENT;
				goto error;
			}
			if (volume->dtrefnum==0) {
				switch(afp_opendt(volume,&volume->dtrefnum)) {
				case kFPParamErr:
				case kFPMiscErr:
					free(newpath);
					return -EIO;
				case kFPNoErr:
				default:
					break;
				}
			} 
			free(newpath);
			return 1;
		case AFP_META_SERVER_ICON:
			free(newpath);
			return 1;
	}
	return 0;
error:
	free(newpath);
	return ret;

}

int appledouble_getattr(struct afp_volume * volume, 
	const char * path, struct stat *stbuf)
{
	unsigned int resource;
	char * newpath;
	int ret;

	resource = extra_translate(volume, path, &newpath);
	switch(resource) {
		case AFP_META_RESOURCE:
			ll_getattr(volume,newpath,stbuf,1);
			goto okay;
		case AFP_META_APPLEDOUBLE:
			stbuf->st_mode = 0700 | S_IFDIR;
			goto okay;
		case AFP_META_FINDERINFO:
			ll_getattr(volume,newpath,stbuf,0);
			stbuf->st_mode |= S_IFREG;
			stbuf->st_size=32;
			goto okay;
		case AFP_META_COMMENT: {
			unsigned int did;
			char basename[AFP_MAX_PATH];

			ret=ll_getattr(volume,newpath,stbuf,0);
			if (ret<0) 
				goto error;

			ret=get_dirid(volume,newpath,basename,&did);
			if (ret<0) 
				goto error;

			ret=get_comment_size(volume,basename,did);
			if (ret<0) 
				goto error;

			stbuf->st_mode |= S_IFREG;
			stbuf->st_size=ret;
			goto okay;
		}
		case AFP_META_SERVER_ICON:
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_size=256;
			goto okay;
	}
	return 0;
okay:
	free(newpath);
	return 1;
error:
	free(newpath);
	return ret;

}

static void remove_fp(struct afp_file_info **base,struct afp_file_info * toremove)
{
	struct afp_file_info *fp, *prev=NULL;

	for (fp=*base;fp;fp=fp->next) 
		if (fp==toremove) {
			if (prev==NULL) {
				*base=fp->next;;
				free(fp);
				prev=NULL;
			} else {
				prev->next=fp->next;
				prev=fp;
				free(fp);
			}
		}

}

static int add_fp(struct afp_file_info **newchain, struct afp_file_info *fp,
		char * suffix, unsigned int size)
{
	struct afp_file_info * newfp;
	newfp=malloc(sizeof(struct afp_file_info));
	memcpy(newfp,fp,sizeof(struct afp_file_info));
	strcat(newfp->name,suffix);
	newfp->resourcesize=size;
	newfp->unixprivs.permissions|=S_IFREG;
	newfp->next=*newchain;
	*newchain=newfp;
}

int appledouble_readdir(struct afp_volume * volume, 
	const char *path, struct afp_file_info **base)
{
	unsigned int resource;
	char * newpath;

	resource = extra_translate(volume, path, &newpath);

	switch(resource) {
		case 0:
			return 0;
		case AFP_META_APPLEDOUBLE: {
			struct afp_file_info *fp, *prev=NULL, 
				*newfp=NULL, *newchain=NULL, *last=NULL;
			ll_readdir(volume, newpath,base,1);

			/* Add .finderinfo files */
			for (fp=*base;fp;fp=fp->next) {
				add_fp(&newchain,fp,finderinfo_string,32);

				/* Add comments if it has a size > 0 */
				if (ensure_dt_opened(volume)==0) {
					int size=get_comment_size(volume,
						fp->name,fp->did);

					if (size>0) 
					add_fp(&newchain,fp,comment_string,32);
				}

				if (fp->unixprivs.permissions & S_IFREG) {
					if (fp->resourcesize==0) {
						remove_fp(base,fp);
					}
				} else {
					remove_fp(base,fp);
				}
				last=fp;
			}
			if ((newchain) && (last)) {
				last->next=newchain;
			}

			free(newpath);
			return 1;
		case AFP_META_RESOURCE:
		case AFP_META_SERVER_ICON:
		case AFP_META_COMMENT:
			free(newpath);
			return -ENOTDIR;
		}
	}

	return 0;
}

int appledouble_mkdir(struct afp_volume * volume, const char * path, mode_t mode)
{
	int resource;
	char * newpath;

	/* You can't mkdir */
	resource = extra_translate(volume, path, &newpath);
	free(newpath);
	if (resource==0) return 0;
	return -EPERM;

}

int appledouble_readlink(struct afp_volume * volume, const char * path, 
	char * buf, size_t size)
{
	int resource;
	char * newpath;

	/* You can't readlink*/
	resource = extra_translate(volume, path, &newpath);
	free(newpath);
	if (resource==0) return 0;
	return -EPERM;

}

int appledouble_rmdir(struct afp_volume * volume, const char * path)
{
	int resource;
	char * newpath;

	/* You can't rmdir*/
	resource = extra_translate(volume, path, &newpath);
	free(newpath);
	if (resource==0) return 0;
	return -EPERM;

}

int appledouble_chown(struct afp_volume * volume, const char * path,
		uid_t uid, gid_t gid)
{
	int resource;
	char * newpath;

	/* You can't chown*/
	resource = extra_translate(volume, path, &newpath);
	free(newpath);
	if (resource==0) return 0;
	return -EPERM;

}

int appledouble_utime(struct afp_volume * volume, const char * path,
	struct utimbuf * timebuf)
{
	int resource;
	char * newpath;

	/* You can't utime*/
	resource = extra_translate(volume, path, &newpath);
	free(newpath);
	if (resource==0) return 0;
	return -EPERM;
}

int appledouble_symlink(struct afp_volume *vol, const char *path1, const char *path2)
{
	int resource;
	char * newpath;

	/* You can't symlink*/
	resource = extra_translate(vol, path1, &newpath);
	free(newpath);
	if (resource==0) return 0;
	return -EPERM;
}

int appledouble_rename(struct afp_volume * volume, const char * path_from, 
	const char * path_to)
{
	int resource;
	char * newpath;

	/* You can't rename*/
	resource = extra_translate(volume, path_to, &newpath);
	free(newpath);
	if (resource==0) return 0;
	return -EPERM;
}

