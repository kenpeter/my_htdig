/* include/htconfig.h.  Generated automatically by configure.  */
/* include/htconfig.h.in.  Generated automatically from configure.in by autoheader.  */
/*

 Part of the ht://Dig package   <http://www.htdig.org/>
 Copyright (c) 1999, 2000 The ht://Dig Group
 For copyright details, see the file COPYING in your distribution
 or the GNU General Public License version 2 or later
 <http://www.gnu.org/copyleft/gpl.html>

*/

#ifndef _config_h_
#define	_config_h_

#define VERSION "3.1.6"

#define PACKAGE "htdig"

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you have the ANSI C header files.  */
/* #undef STDC_HEADERS */

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define this to the type of the third argument of getpeername() */
#define GETPEERNAME_LENGTH_T unsigned int

/* Define if you need a prototype for gethostname() */
/* #undef NEED_PROTO_GETHOSTNAME */

/* Define if the included regex doesn't work */
/* #undef HAVE_BROKEN_REGEX */

/* Define if we should use rxposix.h instead of regex.h */
/* #undef USE_RX */

/* Define if you have the localtime_r function.  */
#define HAVE_LOCALTIME_R 1

/* Define if you have the mkstemp function.  */
#define HAVE_MKSTEMP 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the timegm function.  */
#define HAVE_TIMEGM 1

/* Define if you have the <alloca.h> header file.  */
#define HAVE_ALLOCA_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <getopt.h> header file.  */
#define HAVE_GETOPT_H 1

/* Define if you have the <iostream.h> header file.  */
#define HAVE_IOSTREAM_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <ostream.h> header file.  */
#define HAVE_OSTREAM_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/file.h> header file.  */
#define HAVE_SYS_FILE_H 1

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/select.h> header file.  */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/wait.h> header file.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <wait.h> header file.  */
#define HAVE_WAIT_H 1

/* Define if you have the <zlib.h> header file.  */
#define HAVE_ZLIB_H 1

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* Define if you have the z library (-lz).  */
#define HAVE_LIBZ 1

/* Define to the syslog level for htsearch logging. */
#define LOG_LEVEL LOG_INFO

/* Define to the syslog facility for htsearch logging. */
#define LOG_FACILITY LOG_LOCAL5

/* Define this if you're willing to allow htsearch to take -c even as a CGI */
/*  regardless of the security problems with this. */
/* #undef ALLOW_INSECURE_CGI_CONFIG */

/* Define to remove the word count in db and WordRef struct. */
/* #undef NO_WORD_COUNT */

/* Defined in case the compiler doesn't have TRUE and FALSE constants */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif
