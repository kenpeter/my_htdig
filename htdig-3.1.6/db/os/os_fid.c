/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 1997, 1998
 *	Sleepycat Software.  All rights reserved.
 */

#include "config.h"

#ifndef lint
static const char sccsid[] = "@(#)os_fid.c	10.12 (Sleepycat) 7/21/98";
#endif /* not lint */

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <string.h>
#include <time.h>
#endif

#include "db_int.h"
#include "common_ext.h"

/*
 * __os_fileid --
 *	Return a unique identifier for a file.
 *
 * PUBLIC: int __os_fileid __P((DB_ENV *, const char *, int, u_int8_t *));
 */
int
__os_fileid(dbenv, fname, timestamp, fidp)
	DB_ENV *dbenv;
	const char *fname;
	int timestamp;
	u_int8_t *fidp;
{
	struct stat sb;
	size_t i;
	
	// https://bugzilla.redhat.com/attachment.cgi?id=110009&action=diff
	// Gary
	//time_t now;

	u_int8_t *p;

	// Gary
	u_int32_t ino, dev;

	/* Clear the buffer. */
	memset(fidp, 0, DB_FILE_ID_LEN);

	// Gary
	/* Check for the unthinkable. */
	/*
	if (sizeof(sb.st_ino) +
	    sizeof(sb.st_dev) + sizeof(time_t) > DB_FILE_ID_LEN)
		return (EINVAL);
	*/

	/* On UNIX, use a dev/inode pair. */
	if (stat(fname, &sb)) {
		__db_err(dbenv, "%s: %s", fname, strerror(errno));
		return (errno);
	}

	// Gary
	ino = sb.st_ino;
	dev = sb.st_dev;

	/*
	 * Use the inode first and in reverse order, hopefully putting the
	 * distinguishing information early in the string.
	 */
	/*
	for (p = (u_int8_t *)&sb.st_ino +
	    sizeof(sb.st_ino), i = 0; i < sizeof(sb.st_ino); ++i)
		*fidp++ = *--p;
	for (p = (u_int8_t *)&sb.st_dev +
	    sizeof(sb.st_dev), i = 0; i < sizeof(sb.st_dev); ++i)
		*fidp++ = *--p;
	*/

	// Gary	
	for (p = (u_int8_t *)&ino +
		sizeof(ino), i = 0; i < sizeof(ino); ++i)
 			*fidp++ = *--p;
	
	for (p = (u_int8_t *)&dev +
		sizeof(dev), i = 0; i < sizeof(dev); ++i)
 			*fidp++ = *--p;


	if (timestamp) {
		// Gary
		//(void)time(&now);

		// Gary
		time_t nowt;
		u_int32_t now;

		(void)time(&nowt);
		now = nowt;		

		for (p = (u_int8_t *)&now +
		    sizeof(now), i = 0; i < sizeof(now); ++i)
			*fidp++ = *--p;
	}
	return (0);
}
