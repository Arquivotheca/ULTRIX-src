/* @(#)nfs.h	4.1  (ULTRIX)        7/2/90     */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any  other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University    of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or  reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/*
 *	Portions of this software have been licensed to 
 *	Digital Equipment Company, Maynard, MA.
 *	Copyright (c) 1987 Sun Microsystems, Inc.  ALL RIGHTS RESERVED.
 */
/*
 *	Modification History:
 *
 * 	14 Jun 89 -- condylis
 *		Added copyright header.
 *
 */


#define NFS_PORT           2049
#define NFS_MAXDATA        8192
#define NFS_MAXPATHLEN     1024
#define NFS_MAXNAMLEN	 255
#define NFS_FHSIZE	 32
#define NFS_COOKIESIZE	 4
#define NFS_FIFO_DEV	 -1	/* size kludge for named pipes */

/*
 * File types
 */
#define NFSMODE_FMT   0170000	/* type of file */
#define NFSMODE_DIR   0040000	/* directory */
#define NFSMODE_CHR   0020000	/* character special */
#define NFSMODE_BLK   0060000	/* block special */
#define NFSMODE_REG   0100000	/* regular */
#define NFSMODE_LNK   0120000	/* symbolic link */
#define NFSMODE_SOCK  0140000	/* socket */
#define NFSMODE_FIFO  0010000	/* fifo */

/*
 * Error status
 */
enum nfsstat {
	NFS_OK= 0,		/* no error */
	NFSERR_PERM=1,		/* Not owner */
	NFSERR_NOENT=2,		/* No such file or directory */
	NFSERR_IO=5,		/* I/O error */
	NFSERR_NXIO=6,		/* No such device or address */
	NFSERR_ACCES=13,	/* Permission denied */
	NFSERR_EXIST=17,	/* File exists */
	NFSERR_NODEV=19,	/* No such device */
	NFSERR_NOTDIR=20,	/* Not a directory*/
	NFSERR_ISDIR=21,	/* Is a directory */
	NFSERR_FBIG=27,		/* File too large */
	NFSERR_NOSPC=28,	/* No space left on device */
	NFSERR_ROFS=30,		/* Read-only file system */
	NFSERR_NAMETOOLONG=63,	/* File name too long */
	NFSERR_NOTEMPTY=66,	/* Directory not empty */
	NFSERR_DQUOT=69,	/* Disc quota exceeded */
	NFSERR_STALE=70,	/* Stale NFS file handle */
	NFSERR_WFLUSH=99	/* write cache flushed */
};

/*
 * File types
 */
enum ftype {
	NFNON = 0,	/* non-file */
	NFREG = 1,	/* regular file */
	NFDIR = 2,	/* directory */
	NFBLK = 3,	/* block special */
	NFCHR = 4,	/* character special */
	NFLNK = 5,	/* symbolic link */
	NFSOCK = 6,	/* unix domain sockets */
	NFBAD = 7,	/* unused */
	NFFIFO = 8 	/* named pipe */
};

/*
 * File access handle
 */
struct nfs_fh {
	opaque data[NFS_FHSIZE];
};

/* 
 * Timeval
 */
struct nfstime {
	unsigned seconds;
	unsigned useconds;
};


/*
 * File attributes
 */
struct fattr {
	ftype type;		/* file type */
	unsigned mode;		/* protection mode bits */
	unsigned nlink;		/* # hard links */
	unsigned uid;		/* owner user id */
	unsigned gid;		/* owner group id */
	unsigned size;		/* file size in bytes */
	unsigned blocksize;	/* prefered block size */
	unsigned rdev;		/* special device # */
	unsigned blocks;	/* Kb of disk used by file */
	unsigned fsid;		/* device # */
	unsigned fileid;	/* inode # */
	nfstime	atime;		/* time of last access */
	nfstime	mtime;		/* time of last modification */
	nfstime	ctime;		/* time of last change */
};

/*
 * File attributes which can be set
 */
struct sattr {
	unsigned mode;	/* protection mode bits */
	unsigned uid;	/* owner user id */
	unsigned gid;	/* owner group id */
	unsigned size;	/* file size in bytes */
	nfstime	atime;	/* time of last access */
	nfstime	mtime;	/* time of last modification */
};


typedef string filename<NFS_MAXNAMLEN>; 
typedef string nfspath<NFS_MAXPATHLEN>;

/*
 * Reply status with file attributes
 */
union attrstat switch (nfsstat status) {
case NFS_OK:
	fattr attributes;
default:
	void;
};

struct sattrargs {
	nfs_fh file;
	sattr attributes;
};

/*
 * Arguments for directory operations
 */
struct diropargs {
	nfs_fh	dir;	/* directory file handle */
	filename name;		/* name (up to NFS_MAXNAMLEN bytes) */
};

struct diropokres {
	nfs_fh file;
	fattr attributes;
};

/*
 * Results from directory operation
 */
union diropres switch (nfsstat status) {
case NFS_OK:
	diropokres diropres;
default:
	void;
};

union readlinkres switch (nfsstat status) {
case NFS_OK:
	nfspath data;
default:
	void;
};

/*
 * Arguments to remote read
 */
struct readargs {
	nfs_fh file;		/* handle for file */
	unsigned offset;	/* byte offset in file */
	unsigned count;		/* immediate read count */
	unsigned totalcount;	/* total read count (from this offset)*/
};

/*
 * Status OK portion of remote read reply
 */
struct readokres {
	fattr	attributes;	/* attributes, need for pagin*/
	opaque data<NFS_MAXDATA>;
};

union readres switch (nfsstat status) {
case NFS_OK:
	readokres reply;
default:
	void;
};

/*
 * Arguments to remote write 
 */
struct writeargs {
	nfs_fh	file;		/* handle for file */
	unsigned beginoffset;	/* beginning byte offset in file */
	unsigned offset;	/* current byte offset in file */
	unsigned totalcount;	/* total write count (to this offset)*/
	opaque data<NFS_MAXDATA>;
};

struct createargs {
	diropargs where;
	sattr attributes;
};

struct renameargs {
	diropargs from;
	diropargs to;
};

struct linkargs {
	nfs_fh from;
	diropargs to;
};

struct symlinkargs {
	diropargs from;
	nfspath to;
	sattr attributes;
};


typedef opaque nfscookie[NFS_COOKIESIZE];

/*
 * Arguments to readdir
 */
struct readdirargs {
	nfs_fh dir;		/* directory handle */
	nfscookie cookie;
	unsigned count;		/* number of directory bytes to read */
};

struct entry {
	unsigned fileid;
	filename name;
	nfscookie cookie;
	entry *nextentry;
};

struct dirlist {
	entry *entries;
	bool eof;
};

union readdirres switch (nfsstat status) {
case NFS_OK:
	dirlist reply;
default:
	void;
};

struct statfsokres {
	unsigned tsize;	/* preferred transfer size in bytes */
	unsigned bsize;	/* fundamental file system block size */
	unsigned blocks;	/* total blocks in file system */
	unsigned bfree;	/* free blocks in fs */
	unsigned bavail;	/* free blocks avail to non-superuser */
};

union statfsres switch (nfsstat status) {
case NFS_OK:
	statfsokres reply;
default:
	void;
};

/*
 * Remote file service routines
 */
/*
program NFS_PROGRAM {
	version NFS_VERSION {
		void 
		NFSPROC_NULL(void) = 0;

		attrstat 
		NFSPROC_GETATTR(nfs_fh) =	1;

		attrstat 
		NFSPROC_SETATTR(sattrargs) = 2;

		void 
		NFSPROC_ROOT(void) = 3;

		diropres 
		NFSPROC_LOOKUP(diropargs) = 4;

		readlinkres 
		NFSPROC_READLINK(nfs_fh) = 5;

		readres 
		NFSPROC_READ(readargs) = 6;

		void 
		NFSPROC_WRITECACHE(void) = 7;

		attrstat
		NFSPROC_WRITE(writeargs) = 8;

		diropres
		NFSPROC_CREATE(createargs) = 9;

		nfsstat
		NFSPROC_REMOVE(diropargs) = 10;

		nfsstat
		NFSPROC_RENAME(renameargs) = 11;

		nfsstat
		NFSPROC_LINK(linkargs) = 12;

		nfsstat
		NFSPROC_SYMLINK(symlinkargs) = 13;

		diropres
		NFSPROC_MKDIR(createargs) = 14;

		nfsstat
		NFSPROC_RMDIR(diropargs) = 15;

		readdirres
		NFSPROC_READDIR(readdirargs) = 16;

		statfsres
		NFSPROC_STATFS(nfs_fh) = 17;
	} = 2;
} = 100003;
*/
