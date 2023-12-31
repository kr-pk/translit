/*
 * regsub
 *
 *	Copyright (c) 1986 by University of Toronto.
 *	Written by Henry Spencer.  Not derived from licensed software.
 *
 *	Permission is granted to anyone to use this software for any
 *	purpose on any computer system, and to redistribute it freely,
 *	subject to the following restrictions:
 *
 *	1. The author is not responsible for the consequences of use of
 *		this software, no matter how awful, even if they arise
 *		from defects in it.
 *
 *	2. The origin of this software must not be misrepresented, either
 *		by explicit claim or by omission.
 *
 *	3. Altered versions must be plainly marked as such, and must not
 *		be misrepresented as being the original software.
 */

#include "paths.h"
#include "reg_exp.h"         /* modified by jkl */
/* #include "reg_magic.h" reg_magic included in reg_exp.h , jkl */

/*  substituted with my charcode routine 
   #ifndef CHARBITS
   #define	UCHARAT(p)	((int)*(unsigned char *)(p))
   #else
   #define	UCHARAT(p)	((int)*(p)&CHARBITS)
   #endif
*/

#define UCHARAT(p)     (intcode(*(p)))
extern int intcode();

/*
 - reg_sub - perform substitutions after a regexp match
 */
void
reg_sub(prog, source, dest)
reg_exp *prog;
char *source;
char *dest;
{
	register char *src;
	register char *dst;
	register char c;
	register int no;
	register int len;
/*	extern char *strncpy();   jkl */

	if (prog == NULL || source == NULL || dest == NULL) {
		reg_error("NULL parm to regsub");
		return;
	}
	if (UCHARAT(prog->program) != MAGIC) {
		reg_error("damaged regexp fed to regsub");
		return;
	}

	src = source;
	dst = dest;
	while ((c = *src++) != '\0') {
		if (c == '&')
			no = 0;
		else if (c == '\\' && '0' <= *src && *src <= '9')
			no = *src++ - '0';
		else
			no = -1;

		if (no < 0) {	/* Ordinary character. */
			if (c == '\\' && (*src == '\\' || *src == '&'))
				c = *src++;
			*dst++ = c;
		} else if (prog->startp[no] != NULL && prog->endp[no] != NULL) {
			len = prog->endp[no] - prog->startp[no];
			(void) strncpy(dst, prog->startp[no], len);
			dst += len;
			if (len != 0 && *(dst-1) == '\0') {/* strncpy hit NUL. */
				reg_error("damaged match string");
				return;
			}
		}
	}
	*dst++ = '\0';
}
