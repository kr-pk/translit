/* Include file for translit.c program
 * Copyright (c) 1993 Jan Labanowski and JKL Enterprises, Inc.
 *   Jan Labanowski, jkl@ccl.net, JKL@OHSTPY.BITNET Jan. 5, 1993
 * You must modify this file before you try to compile the program
 */


/* these are "standard" include files. Some may have different names on
   your system. If program complains here, you need to check it */

#include <stdio.h>
#include <string.h>     /* some older machines have <strings.h> !!! */
#include <stdlib.h>     /* some machines use <unix.h> here or some/no-thing */
#include <ctype.h>


/* These are some defines which relate to the system and compiler
 * you are running:
 *     1 (one) means YES, TRUE,
 *     0 (zero) means NO, FALSE
 */
#define GETOPT  1   /* does your C compiler have getopt routine? Most compilers
                       do, but for example VAX C does not. Some PCs do not,
                       UN*X usually has.  Enter 1 if you have
                       getopt and 0 if you do not. */

#define GETENV  1   /* does your C compiler and system have getenv routine?
                       To my knowledge all UN*X, VAX-VMS and MS-DOS have.
                       Enter 1 if you have it, and 0 if you do not. If you do
                       not have GETENV, you do not have environment. */

#define STRCHR  1   /* the routine finding a position of a character in 
                       a string is called strchr. If you do not have this
                       routine, say 0, if you have it, say 1 */

#define STRCSPN 1   /* some compiler libaries do not have the strcspn routine.
                       If you have it enter 1, if you do not, enter 0 */

#define STATICFUN   1   /* if your compiler supports declarations of 
                           static functions:
                                static int boo(foo)
                           enter 1. If it chokes on it, change to 0.  */


/* if transliteration table file is not found in the current directory
 * program looks for this file in directory pointed by PATH. For UNIX
 * it might be something like "/usr/local/lib/", and for MS-DOS it maybe
 * something like: "C:\\INCLUDE\\". Remember to put a slash(backslash)
 * after last subdirectory name. For DOS remember that backslashes have
 * to be quoted, i.e., you enter the backslash twice. I did not check it
 * for the VAX.
 */
                 /* search path for transliteration rules files */
#define TPATH    "/usr/local/lib/translit/"

/* DECONVNAME --- default file name for conversion table (no path, just
 * file name.
 */
#define DEFCONVNAME "koi8-tex.rus"


/* if environment variables TRANSP and TRANSF are defined, the
   TPATH and DECONVNAME are taken from them, not from the above defs.
   If these environment variables are booked you need to put here
   something else.  */

/* name of envoronment variable with TPATH */
#define TRANSPATH "TRANSP"

/* name of environment variable with DEFCONVNAME */
#define DEFNAME   "TRANSF"

