
/* This is a program fom transliterating files from one character set to
   another. 
   TRANSLIT --- Version 1.02, April 4, 1993.
   Copyright (c) by Jan Labanowski, 1993 and JKL Enterprises, Inc.
   Permission is given to disribute this program freely in accordance with
   the rules and conditions spelled out in the program documentation. If you
   got this program without the documentation, or if some files were missing,
   somebody must have violated the rules. In this case, please delete the the
   program and obtain the information on how to get the complete distribution
   from the author. The rules require that the whole package is distributed 
   (i.e., the source code, the transliteration tables, and the documentation).

   3/29/93 Jan Labanowski. Thanks to Leonid A. Broukhis (leo@ipmce.su)
   I noticed that translit will not work correctly if SHIFT-IN and SHIFT-OUT
   is the same sequence. The modification was made in which code detecting
   SHIFT-IN and SHIFT-OUT exchanged places (i.e., program looks for SHIFT-IN
   before it looks for SHIFT-OUT) 

   Author: Jan Labanowski, P.O.Box 21821, Columbus, OH 43221-0821, USA
   E-mail: jkl@ccl.net, JKL@OHSTPY.BITNET
 */

#include "paths.h"              /* for local definitions */
#include "reg_exp.h"             /* for regexp package */


#define OPTIONS       "i:o:t:d"   /* allowed options on command line */
#define MAXPAIRS      1000        /* maximum number of conversion pairs */
#define MAXSETS       10          /* maximum number of shift in/out sets */
#define MAXLEVEL      10         /* maximum set nesting level */
#define MAXBUFF       1000        /* maximum size of the buffer */
#define MAXMATCH      100         /* maximum length of match to regular exp. */


/* define all local functions as static if compiler likes it */
#if STATICFUN
#define STATIC static
#else
#define STATIC
#endif

/* types to hold the translation maps for single chars. If inp_maps is of
 * type IMAPP, then character of code c in set k will correspond to
 * a string pointed by (*inp_maps[k])[c]; 
 */

/*  ========== now include definitions for paths and regexp */


typedef char*   IMAP[256];      /* type IMAP is a 256 element array
                                             of pointers to string */
typedef IMAP*   IMAPP[MAXSETS]; /* array of pointers to IMAP */

/* types to hold output set number for a single char. if out_sets is of type
 *  OSETP and c is a code, and k is the set of input character, then output
 *  set number is (*out_sets[k])[c]; */
typedef int        OSET[256];
typedef OSET*      OSETP[MAXSETS];


typedef union  {
                char   *seq;   /* pointer to a string */
                reg_exp  *re;   /* pointer to a regular expression "program */
               } ADDR;


typedef struct {
                 int   typ;    /* type of pointer in ADDR union:
                                  0-string (seq), 1-input regexp (re),
                                  2-output regexp (seq) (output regexp
                                  is a string !)*/
                 int   len;    /* length of a string if present */
                 int   set;    /* character set number for the string */
                 ADDR  ad;     /* string or regexp program */
                }   SDATA;


/* Some compiler represent character codes > 127 as negative numbers, i.e.,
 * character 255 is -1, char 254 is -2, etc.
 * The flag SIGNED_CHAR_TYPE is set by the program, (program checks which
 * convention is used. It is set to 1, if characters have sign (i.e., 255=-1)
 * and is set to 0 if characters are unsigned (i.e., 255=255). Do not touch
 * this declaration, unless you know what you are doing.
 */
int  SIGNED_CHAR_TYPE;

 char tabline[MAXBUFF]; /* line of text from conversion table file */
 char last_tab_line[MAXBUFF];
 char *lineptr;         /* pointer to the first unread character of tabline */
 int   n_line_chars;    /* number of characters in tabline buffer */
 int  chars_left;       /* no. of chars left in input buffer */

 int  memleft;          /* tells how much memory is left in allocated area */
 char *memptr;          /* pointer for memory allocation area */
 char regerrstr[100];   /* string to hold error message from regular exp */
 reg_exp *regauxptr;     /* aux pointer for regular expresion structure */
 int debug_flg=0;       /* if 1, then additional info sent to stderr */

 FILE *inpf;            /* input file pointer */
 FILE *outf;            /* output file pointer */
 FILE *tabl;            /* file with translation table */

 int n_conv_seq;             /* number of conversion sequences */
 SDATA inp_data[MAXPAIRS];   /* structure to hold types, lengths and pointers
                                for input sequences */
 SDATA out_data[MAXPAIRS];   /* structure to hold types lengths and pointers
                                for output sequences */
 SDATA inp_SO_data[MAXSETS]; /* structure with types, lens, ptrs for inp_SO*/
 SDATA inp_SO_subs[MAXSETS]; /* holds substitution string/regexp for inp_SO*/
 SDATA inp_SI_data[MAXSETS]; /* structure with types, lens, ptrs for inp_SI*/
 SDATA inp_SI_subs[MAXSETS]; /* holds substitution string/regexp for inp SI*/
 SDATA inp_nest_open[MAXSETS]; /* for sick transliteration cases, like TeX */
 SDATA inp_nest_close[MAXSETS]; /* where you need to count {} pairs */

 SDATA *junky;

 IMAPP inp_maps;             /* maps for single character sequences, 
                    array of pointers. Element of the array is a pointer
                    to the array of pointers which point at strings */
 OSETP out_sets;   /* output set numbers corresponding to inp_maps 
                    array of pointers to integer pointers */
 int  n_inp_sets;            /* number of input sets */
 int  n_out_sets;            /* number of output sets */
 char *out_SI[MAXSETS];      /* pointer to output shift in sequences */
 char *out_SO[MAXSETS];      /* pointer to output shift out sequences */
 int  out_SI_len[MAXSETS];   /* out SI sequence length */
 int  out_SO_len[MAXSETS];   /* out SO sequence length */

 char *begseq,   /* sequence to be written at the beginning of output*/
       *endseq;  /* sequence to be written at the end of output file */

 int file_version;           /* conversion table version number */
 int strstart, strend,       /* codes delimiting strings */
     liststart, listend,     /* codes delimiting lists */
     regexstart, regexend,   /* codes delimiting expressions */
     curst1, curend1,
     curst2, curend2;        /* auxiliary */

 char  scr1[MAXBUFF], scr2[MAXBUFF],            /* scratch space */
      scr1a[MAXBUFF], scr2a[MAXBUFF];           /* scratch space */
 char *scr1ptr, *scr2ptr, *scr1pt, *scr2pt,
                *scrauxptr, *scrcurptr;         /* aux string pointers */

 int  inp_seq_length;       /* length of input sequence */
 int  out_seq_length;       /* length of output sequence */
 int  out_set_number;       /* number of output set */
 char *out_seq_ptr;        /* pointer to output sequence */

 reg_exp *reg_comp();
 int reg_try();
 void reg_sub();
 void reg_error();

/* fix  if no strchr routine in the libarry */
#if STRCHR
#else
#define strchr indexfun
#endif

/* this is index which is equivalent to strchr */
char *indexfun (s, c)
char	*s;
int 	c;
	{
	while (*s)
		if (c == *s) return (s);
		else s++;
	return (NULL);
	}


/* ============================================================ */

/* include code for getopt() if not known to compiler */

#if GETOPT
#else

/*
       This is a some getopt I took from the net and do not remember
       who actually wrote this
*/

#define	ARGCH    (int)':'
#define BADCH	 (int)'?'
#define EMSG	 ""
#define	ENDARGS  "--"

/*
 * get option letter from argument vector
 */
int	opterr = 1,		/* useless, never set or used */
	optind = 1,		/* index into parent argv vector */
	optopt;			/* character checked for validity */
char	*optarg;		/* argument associated with option */

#define tell(s)	fputs(*nargv,stderr);fputs(s,stderr); \
		fputc(optopt,stderr);fputc('\n',stderr);return(BADCH);


STATIC int getopt(nargc,nargv,ostr)
int	nargc;
char	**nargv,
	*ostr;
{
	static char	*place = EMSG;	/* option letter processing */
	register char	*oli;		/* option letter list index */

	if(!*place) {			/* update scanning pointer */
		if(optind >= nargc || *(place = nargv[optind]) != '-' ||
			 !*++place) return(EOF);
		if (*place == '-') {	/* found "--" */
			++optind;
			return(EOF);
		}
	}				/* option letter okay? */
	if ((optopt = (int)*place++) == ARGCH || !(oli = strchr(ostr,optopt))) {
		if(!*place) ++optind;
		tell(": illegal option -- ");
	}
	if (*++oli != ARGCH) {		/* don't need argument */
		optarg = NULL;
		if (!*place) ++optind;
	}
	else {				/* need an argument */
		if (*place) optarg = place;	/* no white space */
		else if (nargc <= ++optind) {	/* no arg */
			place = EMSG;
			tell(": option requires an argument -- ");
		}
	 	else optarg = nargv[optind];	/* white space */
		place = EMSG;
		++optind;
	}
	return(optopt);			/* dump back option letter */
}

#endif


/* ================= charcode ======================
 * returns the code of the character given its integer code. If global
 * variable SIGNED_CHAR_TYPE flag is 1, then character code is negative
 * for chars >= 128, otherwise they are passed through.
 * ================================================== */
STATIC char charcode (intcde)
int intcde;
{
 if(SIGNED_CHAR_TYPE == 1) {  /* if signed chars used */
   if(intcde >= 128) {        /* if integer code is larger than 128 */
     return((char)(intcde - 256));   /* make it negative complement */
     }
   else {      /* return the original code */
     return((char)intcde);
     }
   }
 else {
   return((char)intcde);
   }
}
/* ================= intcode =====================
 * returns integer code of a character depending on the value of
 * SIGNED_CHAR_TYPE flag
 * =============================================== */
int intcode(charcde)
char charcde;
{
 if(SIGNED_CHAR_TYPE == 1) {  /* if signed chars used */
   if((int)charcde < (int)0) {     /* if negative code */
     return((int)((int)charcde + 256));   /* convert to positive */
     }
   else {      /* return the original code */
     return((int)charcde);
     }
   }
 else {
   return((int)charcde);
   }
}

/* ================= tablerr ======================
 * terminates the program with a message to stderr and contents of
 * the buffer. num - exit status, errmsg - message
 * ================================================ */
STATIC int tablerr(num, errmsg)
int num;
char *errmsg;
{
 fprintf(stderr,"%s\n", errmsg);
 fprintf(stderr,
 "Current contents of the input buffer for conversion table file:\n");
 fprintf(stderr,"%s\n", last_tab_line);
 exit(num);
 return(0);  /* to keep compiler happy that there is return from function */
}
   
/* ================= getnblkline ====================
 * gets a nonblank line from tabl file and resets pointers if clearflg == 1,
 * otherwise, appends the line to the current buffer.
 * Line is stored in the global variable tabline. The global *lineptr
 * is reset to line beginning.
 * If EOF reached, or line too long, returns -1,  else a number of chars in
 * the line. If line starts with # or ! in first column, it is skipped.
 * ==================================================  */
STATIC int getnblkline(fileptr, clearflg)
FILE *fileptr;
int clearflg;
{
 int  l, maxc;
 char *auxptr;
 if(clearflg == 1) {
   n_line_chars = 0;
   lineptr = tabline;
   }
 maxc = MAXBUFF - n_line_chars -2;  /* how much space in the buffer */
 while (fgets (lineptr, maxc, fileptr) != NULL) {
   strcpy(last_tab_line, lineptr);   /* save current line for error messages */
   if((*lineptr == '#') || (*lineptr == '!')) {   /* skip comment lines */
     continue;
     }
   l = strlen(lineptr);  /* how many chars we read ? */
   n_line_chars += l;    /* how many chars in the buffer */

   if(n_line_chars >  MAXBUFF-5) {    /* if line too long */
     return(-1);
     }
   if(clearflg == 1) { /* do it only if first line is fetched */
     auxptr = lineptr;
     while ((isspace(*auxptr) != 0) && (*auxptr != '\0')) {
       auxptr++;   /* skip front spaces */
       }

     if(*auxptr == '\0') {   /* if blank line */
       continue;
       }
     }
   return(l);   /* return length of line just read */
   }  /* end while */
 return(-1);   /* end of file found */
}
/* ================= chknblk ==========================
 * returns a code of the first nonblank character at the current position
 * of tabline buffer. The lineptr is left at this char (NOT AT THE NEXT CHAR !
 * If no nonblank  * character, returns -1.
 * ==================================================== */
STATIC int chknblk(fileptr)
FILE *fileptr;
{
 int ch;

 Fetch_next:
 while (*lineptr != '\0') {
   if(isspace(*lineptr) == 0) {
     ch = intcode(*lineptr);
     return(ch);
     }
   lineptr++;
   }
 if(getnblkline(fileptr, 0) > 0) {
   goto Fetch_next;
   }
 else {
   return(-1);
   }
}

/* ================= getnumber ===========================
 * retrieves integer nonnegative decimal number from a the current
 * line (tabline).
 * Returns the number, or -9999 if no good number in the line 
 * only number < 1000 allowed
 * ========================================================== */
STATIC int getnumber(fileptr)
FILE *fileptr;
{
 int num, flg, sign;
 num = 0;
 flg = 0; 
 sign = 0;

Next_line:
 while (*lineptr != '\0') {
   if(flg == 0) {   /* only spaces found till now */
     if(isspace(*lineptr) != 0) {
       lineptr++;
       continue;
       }
     else {
       flg = 1;   /* the nonblank char found */
       }
     }

   if(flg == 1) {  /* the nonblank char was found */
     if(sign == 0) {   /* sign may only be located before the number */
       if(*lineptr == '-') {   
         sign = -1;
         lineptr++;
         }
       else if(*lineptr == '+') {
         sign = 1;
         lineptr++;
         }
       else {  /* set it to +1, so it is checked only once */
         sign = 1;
         }
       }
     if(isdigit(*lineptr) != 0) {
       num = 10*num + *lineptr - '0';
       if(num > 1000) {
         return(-9999);    /* number too large */
         }
       } 
     else if(isspace(*lineptr) != 0) {  /* end of number */
       return(num*sign);
       }
     else {
       return(-9999);  /* some strange character */
       }
     }
   lineptr++;   /* to next character */
   }
 if(flg == 1) {  /* if valid number collected before '\0' */
   return(num*sign);
   }
 else {
   if(getnblkline(fileptr, 0) > 0) {
     goto Next_line;
     }
   else {
     return(-9999);   /* if no number before end of file, error */
     }
   }
}


/* ================= getstring ==========================
 * returns the pointer to the string from the tabline. The pointer is
 * volatile, and will point to garbage after next getnblkline call, so
 * you need to copy it (or use it) immedaitely after the call.
 * Returns a pointer to string if successful, and NULL pointer if not.
 * startcode --- character code which starts the string (it is not
 *               included in the string. If startcode = '\0', the
 *               string is collected from the curent pointer to a buffer.
 * endcode   --- character which ends the string. It is not included in
 *               the string. If endcode = '\0', then string is collected
 *               until first blank or end of string found.
 * If no startcode found or no endcode found, the NULL string is returned.
 * ========================================================= */

STATIC char *getstring(startcode, endcode, fileptr)
int startcode, endcode;
FILE *fileptr;
{
 int flg;
 char *startptr;

 flg = 0; 
Read_next_line:
 while (*lineptr != '\0') {
   if(flg == 0) {   /* if startcode not found yet */
     if(startcode != 0) {
       if(charcode(startcode) == *lineptr) {
         flg = 1;   /* the startcode found */
         lineptr++;
         startptr = lineptr;  /* skip startcode */
         continue;
         }
       else {
         lineptr++;
         continue;
         }
       }
     else {   /* startcode is 0 */
       flg = 1;
       startptr = lineptr;
       lineptr++;
       continue;
       }
     }  /* end flg == 0 */

   if(flg == 1) {  /* the 1st char was found */
     if(endcode == 0) {   /* if stop on blank requested */
       if(isspace(*lineptr) != 0) {  /* if space found */
         *lineptr = '\0';   /* mark string end */
         lineptr++;         /* advance pointer */
         return(startptr);
         }
       else {   /* collect chars */
         lineptr++;
         continue;
         }
       }
     else if(charcode(*lineptr) == endcode) {  /* if stop at endcode */
       *lineptr = '\0';
       lineptr++;
       return(startptr);
       }
     else { /* if not endcode , collect next characters */
       lineptr++;
       continue;
       }
     }   /* end flg == 1 */  
   }   /* end while */

 /* the buffer was exhausted */
 if(endcode == 0) {
   return(startptr);
   }
 else {
   if(getnblkline(fileptr, 0) > 0) {
     goto Read_next_line;
     }
   else {
     return((char *)NULL);
     }
   }
}

/* ============================ convnum ===================
 * returns a nonegative number based on str. Scans the string
 * from position posbeg, and returns first invalid character position
 * in posend. If error, returns -1 (less than 2 characters, num > 255).
 * str - scanned string
 * digits  string of allowed ordered digits in lowercase
 * posbeg - start
 * ======================================================== */
 
STATIC int convnum(buff, digits, posbeg)
char *buff, *digits;
int posbeg;
{
 int num, i, l, d, base;

 base = strlen(digits);
 num = 0;
 i = posbeg;

 while (buff[i] != '\0') {
   d = -1;
   for(l = 0; l < base; l++) {
     if(buff[i] == digits[l]) {
       d = l;
       break;
       }
     }
   if(d >= 0) {
     num = d + num*base;
     if(num > 255) {  /* if code too large */
       return(-1);
       }
     i++;
     }
   else {
     break;
     }
   }
 if((i - posbeg) < 2) { /* if less than two characters in a number */
   return(-1);
   }
 buff[i] = '\0';
 return(num);
}
 

/* ============================ str2code ==================
 * str2code returns a code specified in buff. The valid numbers must have at
 * least 2 digits. Here is a format of the code string
 * (n represents valid  digit for a given base).
 * nnn        (up to 3 decimal digits, first is not zero)
 * 0nnn       (up to 3 octal digits)
 * 0xnn       (up to 3 hex digits)
 * 0onnn      (up to 3 octal digits)
 * 0dnnn      (up to 3 decimal digits)
 * The buff string will have '\0' put at the position after a valid number
 * If no valid number can be parsed, or number is greater than 255, -1
 * is returned.
 * ======================================================== */
STATIC int str2code(buff)
char *buff;
{
 int i, l, num;
 static char decdig[]="0123456789",
              octdig[]="01234567",
              hexdig[]="0123456789abcdef";

 l = strlen(buff);   /* get length */
 if(l < 2) {  /* string too short */
   return(-1);
   }

 for(i = 0; i < l; i++) {    /* convert to lowercase */
   if(isalpha(buff[i]) != 0) {  /* if letter */
     buff[i] = tolower(buff[i]);
     }
   }

 if(isdigit(buff[0]) == 0) {  /* if first char not a digit */
   return(-1);
   }

 if(buff[0] == '0') { /*if starting char is 0, then octal */
   if((num = convnum(buff, octdig, 0)) != -1) { /* check if no base */
     return(num);
     }
   }
 else { /* this has to be a decimal number */
   if((num = convnum(buff, decdig, 0)) != -1) {
     return(num);
     }
   else { /* error in decimal number */
     return(-1);
     }
   }
 /* the base is specified at buff[1] */
 if(buff[1] == 'o') {
   num = convnum(buff, octdig, 2);
   }
 else if(buff[1] == 'd') {
   num = convnum(buff, decdig, 2);
   }
 else if(buff[1] == 'x') {
   num = convnum(buff, hexdig, 2);
   }
 else {  /* no base found */
   return(-1);
   }
 return(num);
}

/* ================= convstr ========================
 * copies inp_string to out_strings, and when codes are given as \xxx
 * converts them to characters. 
 * Returns:
 *  0 if OK
 *  1 if character zero (e.g., \00 or \0x0) is found (it ends the string
 *       processing, since it is string terminator).
 * =================================================== */
STATIC int convstr(inp_string, out_string)
char *inp_string, *out_string;
{
 int ch, ch1, i, l, num, n;
 char buff[8];

 n = 0;
 while ((ch = *inp_string) != '\0') {
   n++;    /* count characters */
   if(ch == '\\')  {
     /* skip blank sequence */
     ch1 = *(inp_string + 1);     /* charcode following "\" */
     if(isspace(ch1) != 0) {  /* if "\" followed by blanks */
       inp_string++;  /* skip over "\" */
       n++;
       while( isspace((*inp_string)) != 0)  { /* skip all spaces */
         n++;
         inp_string++;
         }
       n--;  /* it will be advanced at the top of loop */
       ch = *inp_string;
       if(ch == '\0') {
         *out_string = '\0';
         return(0);
         }
       continue;   /* start new loop turn */
       }  /* ch is space */

     /* now check is \020, etc., i.e., codes */
     for(i = 1; i <= 6; i++) {   /* copy possible number to a buffer */
       buff[i-1] = *(inp_string+i);
       }
     buff[6] = '\0';  /* terminate buff */
     if((num = str2code(buff)) >= 0) {
       *out_string++ = charcode(num);
       /* find how many characters have beed used ( number + \ )  */
       if(num == 0) {
         *out_string = '\0';
         return(1);
         }
       l = strlen(buff) + 1;
       inp_string += l;
       continue;
       }
     }
   *out_string++ = *inp_string;
   inp_string++;
   }   /* end while */
 *out_string = '\0';
 return(0);
}


/* ====================== compstr ========================
 * returns 1 if str1 is located at the beginning of str2 and 0 otherwise
 * ======================================================= */
STATIC int compstr(str1, str2)
char str1[], str2[];
{
 int i;
 if(str1[0] == '\0') { /* empty sequence never matches */
   return(0);
   }
 for(i = 0; str1[i] != '\0'; i++) {
   if(str1[i] != str2[i]) {
     return(0);
     } 
   }
 return(1);
}
 
/* ====================== chkseqs ============================
 * returns the sequence number if sequence is present at the beginning
 * of buffer and -1 otherwise (first sequence has number 0);
 * If regular expression, then SDATA.len is set to the length of
 * the string which matches the regular expession.
 * ============================================================= */
STATIC int chkseqs(n_seq, seqstruc, buff)
int n_seq;
SDATA *seqstruc;
char *buff;
{
 int i, j, l;
 char *sp, *ep, *str;
 reg_exp *reaux;
 
 if(n_seq == 0) {
   return(-1);
   }
 for (i = 0; i < n_seq; i++) {
   if(seqstruc->typ == 0) {  /* if plain string */
     str = (seqstruc->ad).seq;
     if(*str != '\0') {
       l = 1;
       for(j = 0; *str != '\0'; j++) {
          if(*(buff + j) != *str++) {
            l = 0;
            break;
            }
          }
       if(l == 1) {
         return(i);
         }
       }
     }
   else if(seqstruc->typ == 1) {  /* regexp */
     if(reg_try((seqstruc->ad).re, buff) == 1) {  /* if anchored match found */
       reaux = (seqstruc->ad).re;  /* get address of search program */
       sp = reaux->startp[0];      /* address of 1st char of match */
       ep = reaux->endp[0];        /* next char after match */
       if(sp != buff) {  /* matches are anchored, at the buff beginning ! */
         tablerr(10, "Internal error in regexp package\n");
         }
       l = seqstruc->len = ep - sp;   /* match length */
       if(l <= 0) {
         fprintf(stderr,"Error when matching regular expression %d\n", i+1);
         exit(10);
         }
       return(i);
       }
     }
   seqstruc++;
   }   /* end for */
 return(-1);
}

/* =================== rdelim ==================
 * read delimiters from tabl file
 * ============================================= */
STATIC int rdelim(startd, endd)
int *startd, *endd;
{
 if(getnblkline(tabl, 1) < 0) {
   tablerr(10,  "Could not read left delimiter code");
   }
 if((*startd = chknblk(tabl)) < 0) {
   tablerr(10,  "Could not read left delimiter code");
   }
 lineptr++;  /* point at next char */
 if(isspace(*lineptr) == 0) {
   tablerr(10,
   "(Left Delimiter):Delimiters should be single chars separated by spaces");
   }
 if((*endd = chknblk(tabl)) < 0) {
   tablerr(10,  "Could not read right delimiter code");
   }
 lineptr++;  /* point at next char */
 if(isspace(*lineptr) == 0) {
   tablerr(10,
   "(Right Delimiter):Delimiters should be single chars separated by spaces");
   }
 return(0);
}

/* ================== beseq ==============
 * read starting or ending sequence for output
 * and return pointer
 * ======================================= */
STATIC char* beseq()
{
 char *scr1pt, *scr2pt;
 int l;

 if((getnblkline(tabl, 1) < 0) ||
    ((scr1pt = getstring(strstart,strend,tabl)) == (char*)NULL)) {
   tablerr(10, "Error when reading starting/ending sequence");
   }
 l = strlen(scr1pt) + 1;
 if((scr2pt = (char*)malloc(l*sizeof(char))) == NULL) {
   tablerr(10, "Out of memory");
   }
 convstr(scr1pt,scr2pt);
 return(scr2pt);
}

/* ================= allomaps ===============
 * Allocate space for maps
 * ============================================ */
STATIC int allomaps(n)
int n;
{
 int i;
 /* Allocate space for inp_maps and out_sets for input set 0 */
 if((inp_maps[n] = (IMAP*)malloc(256*sizeof(char*))) == NULL) {
    /*if failed */
   tablerr(10, "Out of memory for storing sequences");
   }

 if((out_sets[n] = (OSET*)malloc(256*sizeof(int))) == NULL) { /* if failed */
   tablerr(10, "Out of memory for storing sequences");
   }

 for(i = 0; i < 256; i++) { /* zero allocated memory */
   (*inp_maps[n])[i] = (char*)NULL;
   (*out_sets[n])[i] = 0;
   }
 return(0);
}

/* =================== savestring ================= 
 * saves string in the allocated storage and returns pointer to it
 * does all the housekeeping
 * ================================================== */
STATIC char *savestring(str)
char *str;
{
 int l;
 char *retptr;

 l = strlen(str)+1;
 if(memleft < l) {
    memleft = 5*MAXPAIRS;
    if((memptr = (char*)malloc(memleft*sizeof(char)))
                         == NULL) {
      tablerr(10,"Out of memory for allocation");
      }
    }
 strcpy(memptr, str);
 retptr = memptr;
 memptr += l;
 memleft -= l;
 return(retptr);
}

/* ================= splitlist =================
 * unfolds the list [] to a list of characters (i.e. [a-d] = [abcd])
 * =============================================== */
STATIC int splitlist(inlist, unflist)
char *inlist, *unflist;
{
 int ch, ch1, ch2, i, len;
 
 convstr(inlist, inlist);   /* convert codes */
 len = strlen(inlist);
 if(len == 0) {
   tablerr(10, "Empty list specified");
   }

 *unflist++ = *inlist++;   /* save first character */
 while ( *inlist != '\0') {
   ch = *inlist;
   if((ch != '-') || (*(inlist+1) == '\0')) {
     *unflist++ = ch;
     }
   else { /* the minus is inside */
     ch1 = intcode(*(inlist-1));
     ch2 = intcode(*(inlist+1));
     if(ch2 <= ch1) {
       tablerr(10, "The limits in the range within the list are reversed");
       }
     for(i = ch1+1; i < ch2; i++) {
       *unflist++ = charcode(i);
       }
     }
   inlist++;
   }
 *unflist = '\0';
 return(0);
}

/* ======================== regerror ==================
 * regerror --- routine called from within a regexp package. Aborts
 * program with message
 * ====================================================  */
void reg_error(s)
char *s;
{
 strcat(regerrstr,s);
 tablerr(11,regerrstr);
}

/* ========================  rdinshift ==================
 * reads in a shift sequence, assuming that the getnblkline was called
 * Fills in structure SDATA. If typ = 1, it is assumed that it is data
 * for matching, if typ = 2, this is data to be output 
 * If OK, returns 0, else dies
 * ======================================================= */
STATIC int rdinshift(sdstr, sttyp)
SDATA *sdstr;
int sttyp;
{
 int mode1;
 ADDR ads;
 curst1 = chknblk(tabl);  /* check what type delimiter */
 if(curst1 == strstart) {
   mode1 = 1;
   curend1 = strend;
   }
 else if(curst1 == liststart) {
   mode1 = 2;
   tablerr(10, "Lists not allowed for input SHIFT sequences");
   }
 else if(curst1 == regexstart) {
   mode1 = 3;
   curend1 = regexend;
   }
 else {
   tablerr(10, "Error when reading SHIFT input sequences");
   }

 if((scr1pt = getstring(curst1, curend1, tabl)) == (char*)NULL) {
   tablerr(10, "Error when reading input SHIFT sequences");
   }

 convstr(scr1pt, scr1);   /* convert codes in the sequence  */
 scr1pt = savestring(scr1);   /* save sequence in memory */
 strcpy(regerrstr, "Error in regexp for input SHIFT sequences:");

 if(mode1 == 1) {
   sdstr->typ = 0;   /* common string */
   sdstr->len = strlen(scr1pt);
   ads.seq = scr1pt;  /* save string address */
   sdstr->ad = ads;
   }
 else if(mode1 == 3) {
   if(sttyp == 1) {
     sdstr->typ = 1;
     regauxptr = reg_comp(scr1pt);
     if(regauxptr == (reg_exp *)NULL) {
       tablerr(10, "Error in regular expression");
       }
     ads.re = regauxptr;
     sdstr->ad = ads;
     }
   else {
     sdstr->typ = 2;
     sdstr->len = strlen(scr1pt);
     ads.seq = scr1pt;
     sdstr->ad = ads;
     }
   }
 return(0);
}

/* =========================  match_subs ==========================
 * match_subs matches the match_data sequence description to the
 * current position of the input file string (scrcurptr) and if match
 * is found, finds the replacement string and puts it in scr1 buffer.
 * it sets the global variables out_seq_length, out_seq_ptr, inp_seq_length
 * out_set_number. Returns 1 on success, and 0 if match was not found.
 * ================================================================== */
STATIC int match_subs(match_data, repl_data)
SDATA *match_data, *repl_data;
{
 if(chkseqs(1, match_data, scrcurptr) >= 0) {
   inp_seq_length = match_data->len; /*chkseqs sets it for inp.typ 1 */
   out_set_number = repl_data->set;
   if(repl_data->typ == 2) {  /* if regexp substitution */
     /* find a substitution string */
     regauxptr = (match_data->ad).re; /* pointer to regexp prog */
     /* scr contains the substitution string */
     reg_sub(regauxptr, (repl_data->ad).seq, scr1);
     out_seq_length = strlen(scr1);  /*number of chars in substitute */
     out_seq_ptr = scr1;  /* pointer to substitute string */
     }
   else { /* if plain string (type = 0) */
     out_seq_length = repl_data->len;
     out_seq_ptr = (repl_data->ad).seq;
     }
   if(out_seq_length > MAXMATCH) {
     fprintf(stderr,
     "The substitution string is too long (%d chararacters):\n%s\n",
     out_seq_length, out_seq_ptr);
     exit(1);
     }
   return(1);
   }
 else {
   return(0);
   }
}

/* =================== repl_inp =============================
 * replaces matching portion of an input text with a substitute string.
 * ========================================================== */
int repl_inp()
{
 int k, l, i;

 if(out_seq_length > MAXMATCH) {
   fprintf(stderr,
   "The output substitution sequence is too long (%d characters):\n%s\n",
                                             out_seq_length, out_seq_ptr);
   exit(1);
   }
 if(inp_seq_length >= out_seq_length) { /* do not have to copy strings */
   k = inp_seq_length - out_seq_length;  /* diff in lengths */
   scrcurptr += k;    /* move forwarde by the diff */
   chars_left -= k;
   for (i = 0; i < out_seq_length; i++) {  /* copy chars */
     *(scrcurptr + i) = *(out_seq_ptr + i);
     }
   }
 else  {  /* have to push remaining chars to the right to make space */
   k = out_seq_length - inp_seq_length;  /* diff in lengths */
   l = strlen(scrcurptr);                /* length of input text */
   /* memmove could be used, but it is not in all libaries */
   for (i = l; i >= 0; i--) { /* move to right, start with terminating '\0' */
     *(scrcurptr + i + k) = *(scrcurptr + i);
     }
   for (i = 0; i < out_seq_length; i++) { /* place the output string */
     *(scrcurptr + i) = *(out_seq_ptr + i);
     }
   chars_left += k;     /* update chars_left, scrcurptr not changed */
   }
 return(0);
}

/*======================== main ================================== */

int main(argc, argv)
int argc;
char **argv;
{
 char *tabl_file;           /* name of file with conversion table */
 static char deftablfile[200]=
       DEFCONVNAME;         /* default conversion file name */
 static char deftablpath[200]=
       TPATH;               /* default conversion file path */
 char table_name[300];      /* working array for conversion table */

 int  level;                /* input set nesting level */
 int  inp_cur_set[MAXLEVEL];/* set input number being processed */
 int  inp_cur_nest[MAXLEVEL]; /* current nesting count for input set */
 int  cur_inp_set;          /* current input set, same as inp_cur_set[level] */
 int  out_cur_set;          /* output set level being processed */
 int  buffer_size;          /* size of input buffer string */
 int opt;                   /* option letter */
 int mode1, mode2; /* type of string (1=str, 2=list, 3=regex) */

 int flg, ch, i, j, k, l, n;          /* aux variables */
 

#if GETOPT     
 extern char *optarg;       /* option argument from getopt */
 extern int optind, opterr; /* needed for getopt */
#endif

 static char usage[]=
     "Usage: translit [-i inpfil] [-o outfil] [-t convtabfil] [convtabfil]\n";


 inpf = stdin;              /* initialize input to standard input */   /*UNIX*/
 outf = stdout;             /* initialize output to standard output */ /*UNIX*/

 /* set SIGNED_CHAR_TYPE flag */
 scr1[0] = '\372';
 if((int)scr1[0] < 0) {
   SIGNED_CHAR_TYPE = 1;
   }
 else {
   SIGNED_CHAR_TYPE = 0;
   }

/* if environment is supported */
#if GETENV
 /* if TRANSPATH variable defined, take its contents */
 if((scr1pt = getenv(TRANSPATH)) != (char *)NULL) {
   strcpy(deftablpath, scr1pt);
   }
 if((scr1pt = getenv(DEFNAME)) != (char *)NULL) {
   strcpy(deftablfile, scr1pt);
   }
#endif

 tabl_file = deftablfile;   /* default table file name */
 
 flg = 0;    /* set to no conv table given as an argument */
 i = j = k = 0;   /* flags, for files specified: i-inp, j-out, k-tabl */
 while ((opt = getopt(argc, argv, OPTIONS)) != EOF) {
   switch (opt) {
     case 'd':
       debug_flg = 1;
       break;
     case 'i':
       if(i != 0) {
         fprintf(stderr, "You specified option -i twice\n");
         return(1);
         }
       if((inpf = fopen(optarg, "r")) == NULL)  {
         fprintf(stderr,"Error: Could not find input file: %s\n", optarg);
         return(1);
         }
       i = 1;
       break;
     case 'o':
       if(j != 0) {
         fprintf(stderr, "You specified option -o twice\n");
         return(1);
         }
       if((outf = fopen(optarg, "r")) != NULL)  {
         fprintf(stderr,
         "Error: Output file: %s already exists! Delete it first.\n", optarg);
         exit(3);
         }
       if((outf = fopen(optarg, "w")) == NULL)  {
         fprintf(stderr,"Error: Could not open output file: %s\n", optarg);
         exit(2);
         }
       j = 1;
       break;

     case 't':
       if(k != 0) {
         fprintf(stderr, "You specified option -t twice\n");
         return(1);
         }
       tabl_file = optarg;
       flg = 1;
       k = 1;
       break;
     case '?':
       fprintf(stderr,"Error: %s\n", usage);
       exit(3);
     }  /* end switch */
   }  /* end while */

 if(optind < argc) {    /* check if translation table given w/o option -t */
   if(flg == 1) {
     fprintf (stderr,"Error: You specified conversion table file twice\n");
     exit(4);
     }
   tabl_file = argv[optind];
   if(argc > optind + 1) {
     fprintf (stderr,"Error: %s\n", usage);
     exit(5);
     }
   }

 if((tabl = fopen(tabl_file, "r")) == NULL) { /* try to open file with table */
   strcpy(table_name, deftablpath);  /* copy path to scratch string */
   strcat(table_name, tabl_file);
   if((tabl = fopen(table_name, "r")) == NULL) {  /* try to open path/file */
     fprintf(stderr,"Could not find the conversion table file: %s\n",
             tabl_file);
     exit(6);
     }
   }

 /* read in file version number */
 if((getnblkline(tabl, 1) < 0) || ((file_version = getnumber(tabl)) < 0)) {
   tablerr(7, "Could not read file format number");
   }
 if(file_version != 1) {
   tablerr(10,  "This format of conversion file is not supported");
   }

 /* read in delimiters */

 rdelim(&strstart, &strend);
 rdelim(&liststart, &listend);
 rdelim(&regexstart, &regexend);

 /* read in starting and ending sequences */
 begseq = beseq();
 endseq = beseq();
   
 /* reserve memory for sequences */
 chars_left = 5*MAXPAIRS;  /* size of allocated block */
 if((scr1ptr = (char*)malloc(chars_left*sizeof(char)))
                   == NULL) {
   tablerr(10, "Out of memory for storing sequences");
   }
 
 /* Allocate space for inp_maps and out_sets for input set 0 */

 allomaps(0);

 /* Read number of input sets */
 if((getnblkline(tabl, 1) <= 0) || ((n_inp_sets = getnumber(tabl)) < 0)) {
   tablerr(10, "Error when reading input set count");
   }

 if(n_inp_sets >= MAXSETS) {
   tablerr(10, "Too many input shift sequences");
   }

 /* read input SI/SO sequences */
 for (i = 0; i < n_inp_sets; i++) { 
   /* Allocate space for inp_maps and out_sets for input set i+1 */
   allomaps(i+1);

   /* read in input SHIFTs seq */
   if(getnblkline(tabl, 1) <= 0) {  
     tablerr(10, "Error when reading output shift sequences");
     }
   rdinshift(&inp_SO_data[i], 1);
   rdinshift(&inp_SO_subs[i], 2);
   if((inp_SO_subs[i].typ == 2) && (inp_SO_data[i].typ == 0)) {
     tablerr(10,
     "Plain string type for matching and substitution expression for output");
     }
   rdinshift(&inp_nest_open[i], 1);
   rdinshift(&inp_nest_close[i], 1);
   rdinshift(&inp_SI_data[i], 1);
   rdinshift(&inp_SI_subs[i], 2);
   if((inp_SI_subs[i].typ == 2) && (inp_SI_data[i].typ == 0)) {
     tablerr(10,
     "Plain string type for matching and substitution expression for output");
     }

   if(debug_flg == 1) {
     if(inp_SO_data[i].typ == 0) {
       fprintf(stderr,"%2d) inp_SO =|%s|     ", i, (inp_SO_data[i].ad).seq);
       }
     else {
       fprintf(stderr,"%2d) inp_SO =%d     ", i, inp_SO_data[i].typ);
       }
     if((inp_SO_subs[i].typ == 0) || (inp_SO_subs[i].typ == 2)) {
       fprintf(stderr,"%2d) inp_SOsub =|%s|     ", i, (inp_SO_subs[i].ad).seq);
       }
     else {
       fprintf(stderr,"%2d) inp_SOsub =%d     ", i, inp_SO_subs[i].typ);
       }
     if(inp_nest_open[i].typ == 0) {
       fprintf(stderr,"nest_open =|%s|     ",  (inp_nest_open[i].ad).seq);
       }
     else {
       fprintf(stderr,"nest_open =%d     ", inp_nest_open[i].typ);
       }
     if(inp_nest_close[i].typ == 0) {
       fprintf(stderr,"nest_close =|%s|     ", (inp_nest_close[i].ad).seq);
       }
     else {
       fprintf(stderr,"nest_close =%d     ", inp_nest_close[i].typ);
       }
     if(inp_SI_data[i].typ == 0) {
       fprintf(stderr,"inp_SI =|%s|\n", (inp_SI_data[i].ad).seq);
       }
     else {
       fprintf(stderr,"inp_SI =%d\n", inp_SI_data[i].typ);
       }
     if((inp_SI_subs[i].typ == 0) || (inp_SI_subs[i].typ == 2)) {
       fprintf(stderr,"%2d) inp_SIsub =|%s|     ", i, (inp_SI_subs[i].ad).seq);
       }
     else {
       fprintf(stderr,"%2d) inp_SIsub =%d     ", i, inp_SI_subs[i].typ);
       }
     }  /* end debug_flg */
   }

 if((getnblkline(tabl, 1) <= 0) || ((n_out_sets = getnumber(tabl)) < 0)) {
  /* read in out SHIFTs count */
   tablerr(10, "Error when reading output set count");
   }
 if(n_out_sets > MAXSETS) {
   tablerr(10, "Too many output SHIFT sequences requested");
   }

 for (i = 0; i < n_out_sets; i++) {
   /* read in out SHIFTs seq */
   if((getnblkline(tabl, 1) <= 0) ||
      ((scr1pt = getstring(strstart, strend, tabl)) == (char*)NULL) ||
      ((scr2pt = getstring(strstart, strend, tabl)) == (char*)NULL)) {
     tablerr(10, "Error when reading output shift sequences");
     }
   convstr(scr1pt, scr1a);
   out_SO_len[i]= strlen(scr1a);
   out_SO[i] = savestring(scr1a);   

   convstr(scr2pt, scr2a);
   out_SI_len[i] = strlen(scr2a);
   out_SI[i] = savestring(scr2a);
   if(debug_flg == 1) {
     fprintf(stderr,"%2d) out_SO string=|%s|   out_SI string=|%s|\n",
     i, out_SO[i], out_SI[i]);
     }
   }  /* end for */

 i = 0;
 while (getnblkline(tabl, 1) > 0)  {
   if((inp_data[i].set = getnumber(tabl)) < 0) {  /* get inp set number */
     tablerr(10, "Set number for input sequences is wrong");
     }
   if(((k = inp_data[i].set) > n_inp_sets) || (k < 0) ) {
     tablerr(10,"Input set number for a sequence wrong");
     }
   curst1 = chknblk(tabl);  /* check what type of string follows */
   if(curst1 == strstart) {
     mode1 = 1;
     curend1 = strend;
     }
   else if(curst1 == liststart) {
     mode1 = 2;
     curend1 = listend;
     }
   else if(curst1 == regexstart) {
     mode1 = 3;
     curend1 = regexend;
     }
   else {
     tablerr(10, "Delimiter wrong when reading input sequences");
     }
   /* get input sequence */
   if((scr1pt = getstring(curst1, curend1, tabl)) == (char*)NULL) {
     tablerr(10, "Error reading input sequence");
     }
   scr1pt = savestring(scr1pt);  /* Save the string */

   if((out_data[i].set = getnumber(tabl)) < -3) {  /* get inp set number */
     tablerr(10, "Wrong code for the output set number");
     }

   if(out_data[i].set > n_out_sets) {
     tablerr(10, "Output set number for a sequence is wrong");
     }

   curst2 = chknblk(tabl);  /* check what type of string follows */
   if(curst2 == strstart) {
     mode2 = 1;
     curend2 = strend;
     }
   else if(curst2 == liststart) {
     mode2 = 2;
     curend2 = listend;
     }
   else if(curst2 == regexstart) {
     mode2 = 3;
     curend2 = regexend;
     }
   else {
     tablerr(10, "Delimiter wrong when reading sequences");
     }
  
   if((scr2pt = getstring(curst2, curend2, tabl)) == (char*)NULL) {
     tablerr(10, "Error reading input sequence");
     }
   scr2pt = savestring(scr2pt);

   /* check if acceptable types for sequences */
   if((mode2 == 3) && (mode1 != 3)) {  /* no regular expressions for output */
     tablerr(10,
   "Regular expression as output sequence and input not a regular expression");
     }
   else if((mode1 == 1) && (mode2 == 2)) { /* inp string, out list */
     tablerr(10, "You specified list for output and string for input");
     }
   else if((mode1 == 3) && (mode2 == 2)) { /* inp regex, out list */
     tablerr(10, "You specified string for input and list for output");
     }
   else if((mode1 == 2) && (out_data[i].set < 0)) {
     tablerr(10,
    "Input LIST and output set code -1/-2/-3 is not supported at this moment");
     }

   if(mode1 == 2) {  /* if list for input expression */
     /* split string at - sign */
     splitlist(scr1pt, scr1);
     if(mode2 == 2) {
       splitlist(scr2pt, scr2); 
       if(strlen(scr1) != strlen(scr2)) {
         tablerr(10,
         "The number of codes in the input and output list is different");
         }
       }
     }  /* end mode 2 */
   else {  /* for all other modes, convert the codes */
     convstr(scr1pt, scr1a); /* convert codes in input string */
     }

   if((mode1 == 1) && (strlen(scr1a) == 1)) { /* single inp char */
     /* it is like list with a single character, so cheat */
     if(out_data[i].set >= 0) {
       mode1 = 2;
       strcpy(scr1, scr1a);  /* make it a list */ 
       }
     else {
       tablerr(10,
  "One-character input strings and output codes -1/-2/-3 are not supported\n");
       }
     }
     
   if(mode1 == 2) {  /* fill the lists for mode 2 */     
     if(mode2 == 1) { /* if normal string as output sequence */
       convstr(scr2pt, scr2);
       scr2pt = savestring(scr2);
       }
     else { /* if mode2 = 2 */
       scr2pt = scr2;
       }
     /* now fill the maps */
     k = inp_data[i].set;
     l = out_data[i].set;
     scr1pt = scr1;  /* points at input list */
     scr2ptr = scr2pt;  /* points at output list or string */
     while (*scr1pt != '\0') {
       if(mode2 == 2) {  /* prepare string with code */
         scr1a[0] = *scr2ptr++;
         scr1a[1] = '\0';
         scr2pt = savestring(scr1a);
         }
       ch = intcode(*scr1pt);
       if((*inp_maps[k])[ch] != (char *)NULL) {
         fprintf(stderr,
"You have entered the character |%c| with code \\0d%d for input set %d\n",
         charcode(ch), ch, k);
         tablerr(10, "Delete previous references if not needed");
         }
 
       (*inp_maps[k])[ch] = scr2pt;  /* save output sequence */
       (*out_sets[k])[ch] = l;       /* save output set number */
       scr1pt++;  /* next code for output */
       }
     i--;  /* do not save this line in inp_str and out */
     } /* end if mode1 = 2*/
   else if(mode1 == 1) { /* if multicharacter input string */
     scr1pt = savestring(scr1a);
     convstr(scr2pt, scr2a);
     scr2pt = savestring(scr2a);
     inp_data[i].typ = 0;
     inp_data[i].len =  strlen(scr1pt);
     (inp_data[i].ad).seq = scr1pt;
     out_data[i].typ = 0;
     out_data[i].len =  strlen(scr2pt);
     (out_data[i].ad).seq = scr2pt;
     }
   else if(mode1 == 3) { /* if regular expression for input */
     inp_data[i].typ = 1;
     l = strlen(scr1a);    /* length of converted input expression */
     if(scr1a[0] == '^') {
       tablerr(10,
       "The ^ (beginning anchor) is not supported");
       }
     if((scr1a[l-1] == '$') && (scr1a[l-1] != '\\')) {
       tablerr(10, "The $ (end anchor) is not supported");
       }

     strcpy(regerrstr, "Error in input regular expression sequence: ");

     if((regauxptr = reg_comp(scr1a)) == NULL) {
       tablerr(10, "Error in the input regular expression sequence");
       }
     (inp_data[i].ad).re = regauxptr;
     convstr(scr2pt, scr1a);   /* convert codes in out string */
     scr2pt = savestring(scr1a);
     if(mode2 == 3) {  /* mark type of expression plan(0)/substit string(2) */
       out_data[i].typ = 2;
       }
     else  {
       out_data[i].typ = 0;
       }
     out_data[i].len = strlen(scr2pt);
     (out_data[i].ad).seq = scr2pt;
     }

   /* advance pointers */

   n_conv_seq = ++i;
   if(n_conv_seq >= (MAXPAIRS-1)) {
     tablerr(10, 
 "Too many transliteration sequences. Recompile program with larger MAXPAIRS");
     }
   }  /* end while getnblkline */

 if(debug_flg == 1) {
   fprintf(stderr,"Multicharacter input sequences \n");
   for(i=0; i < n_conv_seq; i++) {
     fprintf(stderr,"%2d) inp.type=%2d inp.set=%2d out.type=%2d out.set=%2d\n",
        i, inp_data[i].typ, inp_data[i].set, out_data[i].typ, out_data[i].set);
      if(inp_data[i].typ == 0) {
        fprintf(stderr,"       Inp.str=|%s|   ", (inp_data[i].ad).seq);
        }
      fprintf(stderr, "Out.str=|%s|\n", (out_data[i].ad).seq);
      }
   fprintf(stderr,
        "input_set charcode input_character  --> output_set output_string/\n");
   for(i = 0; i <= n_inp_sets; i++) {
     for(k = 0; k < 256; k++) {
       if((*inp_maps[i])[k] != (char *)NULL) {
         fprintf(stderr," %2d  \\%04o  %c   -->  %2d  %s\n",
         i, k,  charcode(k), (*out_sets[i])[k], (*inp_maps[i])[k]);
         }
       }
     }
   }


 fprintf(outf,"%s",begseq);   /* output starting sequence */

 /* transliterate input file to output file */

 level = 0;
 if(n_inp_sets > 0) {
   cur_inp_set = 1;
   }
 else {
   cur_inp_set = 0;
   }
 inp_cur_set[level] = cur_inp_set;   /* 1st input set is a default */
 inp_cur_nest[level] = 0;
 scr1ptr = scr1a;
 scr1ptr[0] = '\0';
 scr2ptr = scr2a;
 scr2ptr[0] = '\0';
 scrcurptr = scr2ptr;
 chars_left = 0;
 buffer_size = MAXBUFF/2;   /* will be set to 0 if EOF */
 out_cur_set = 1;  /* no output set yet */


 while ( buffer_size > 0) {
   /* swap input buffer pointers */
   scrauxptr = scr2ptr;
   scr2ptr = scr1ptr;
   scr1ptr = scrauxptr;

   scrauxptr = scrcurptr;   /* old buffer last pointer */
   scrcurptr = scr1ptr;     /* new buffer start */

   /* copy remains of old buffer to new one */
   strcpy(scrcurptr, scrauxptr);

   l = chars_left;
   for(i = 0; i < buffer_size; i++) {  /* append input chars to scr1a */
     if((k = fgetc(inpf)) == EOF) {
       buffer_size = 0;   /* end of file */
       break;
       }
     else if(k == '\0') { /* skip zero characters */
       i--;
       continue;
       }
     else { /* if normal character */
       *(scrcurptr + l++) = k;
       }
     }
   *(scrcurptr + l) = '\0';    /* terminate buffer with 0 */
   chars_left = l;    ;   /* length of combined string */
   if(buffer_size == 0) {  /* if EOF */
     chars_left += MAXMATCH+1; /* fool the program that there is more */
     }

   while (chars_left > MAXMATCH) {
     /* check if end of scrcurptr --- it means end of input file, since only
        then it can get to the end of the string, otherwise it stops
        MAXMATCH before */
     if(*scrcurptr == '\0') {  /* end of file */
       if(n_out_sets > 0) {  /* if multiple output sets */
         l = out_SI_len[out_cur_set - 1];
         for(i = 0; i < l; i++) {
           k = out_SI[out_cur_set-1][i];
           fputc(k, outf);
           }
         break;
         }
       }

Backstep2:       

     /* check if SHIFT IN sequence for current input set */
     if(n_inp_sets > 0) {
       /* check SI sequence only when nesting count is 0 */
       if(inp_cur_nest[level] == 0) {
         if(match_subs(&inp_SI_data[cur_inp_set-1], 
                       &inp_SO_subs[cur_inp_set-1]) > 0) {  /* is SI */
           repl_inp();
           level--;
           if(level < 0) {
             level = 0;
             fprintf(stderr,
"More SHIFT_IN sequences than corresponding SHIFT_OUT sequences in text\n");
             }
           cur_inp_set = inp_cur_set[level];  /* set previous inp set number */
           continue;
           }
         }
       }

     /* check if new set of input chars started */
     l = -1;
     for (i = 0; i < n_inp_sets; i++) {
       if(match_subs(&inp_SO_data[i], &inp_SO_subs[i]) > 0) {
         l = i;
         break;
         }
       }
     if(l >= 0) {   /* is SO matched */
       repl_inp();    /* substitute SO_data with SO_seqs */
       if((inp_SI_data[l].len > 0) || (inp_SI_data[l].typ == 1)){
       /* increase level only is SHIFT IN present */
         level++;  /* increase number of "opened" input sets */
         inp_cur_nest[level] = 0;  /* It is new level,zero nesting sequences */
         if(level > MAXLEVEL) {
           fprintf(stderr,
           "Too many nested input character sets in input file\n");
           exit(39);
           }
         }
       l++;   /* sets in arrays are saved starting from 0, 
               i.e., set nr 1 corresponds to element [0], 2 --> [1], etc. */
       inp_cur_set[level] = l;   /* save set number at current nesting level */
       cur_inp_set = l;
       continue;
       }

     /* Now check if the input sequence corresponding to cur_inp_set
        matches the string */

Backstep1:                        /* if output set number is -2, start again */
     flg = -1;
     for(i = 0; i < n_conv_seq; i++) {
       k = inp_data[i].set; /*get set number for current transliteration seq */
       if((k == cur_inp_set) || (k == 0)) { /* if equal to current or 0 */
         if(match_subs(&inp_data[i], &out_data[i]) > 0) {
           if(out_set_number < 0) { /* if backsteping */
             repl_inp();  /* replace */
             if (out_set_number == -1) {
               flg = -1;
               }
             else if (out_set_number == -2) {
               goto Backstep1;
               }
             else if (out_set_number == -3) {
               goto Backstep2;
               }
             }
           else {  /* if set number >= 0 */
             flg = i;
             break;
             }
           }  
         }
       }

     if(flg < 0) {  /* if no matching input multichar sequence found */
       ch = intcode(*scrcurptr);  /* current input character */
       if((out_seq_ptr = (*inp_maps[cur_inp_set])[ch]) != NULL) { 
            /* if out_seq exists for current input set */
         out_set_number = (*out_sets[cur_inp_set])[ch];
         flg = 1;
         }
       else if((out_seq_ptr = (*inp_maps[0])[ch]) != NULL) {
          /* if out_seq exitst for set number 0 */
         flg = 1;
         out_set_number = (*out_sets[0])[ch];
         }
       if(flg >= 0) {  /* set other things */
         out_seq_length = strlen(out_seq_ptr);
         inp_seq_length = 1;
         }         
       }

     if(flg < 0) {  /* if no match found, copy the input char to output */
       scr1[0] = *scrcurptr;
       scr1[1] = '\0';
       if(*scrcurptr != '\0') {
         out_seq_length = 1;
         }
       else {
         out_seq_length = 0;
         }

       inp_seq_length = 1;
       out_set_number = 0;
       out_seq_ptr = scr1;
       }

     /* At this point all matches and substitutuions have been done */

     /* check if nesting sequences found for a given set and increase or
       decrease nesting if needed */
     if((n_inp_sets > 0) && (out_cur_set > 0)) {
       for(i = 0; i < inp_seq_length; i++) {
         if(chkseqs(1, &inp_nest_close[cur_inp_set-1], scrcurptr+i) >= 0) {
           inp_cur_nest[level]--;
           }
         if(chkseqs(1, &inp_nest_open[cur_inp_set-1], scrcurptr+i) >= 0) {
           inp_cur_nest[level]++;
           }
         }
       }

     /* output  the SI/SO sequences if output set changed */
     if((n_out_sets > 0) &&
        (out_set_number > 0)) {  /* check if multiple output sets */
       if(out_cur_set !=  out_set_number) { /* if new set starts */
         if(out_cur_set > 0) { /* put SHIFT IN for a previous set */
           l = out_SI_len[out_cur_set-1];  /* old SHIFT IN seq length */
           for(i = 0; i < l; i++) { /* output old SHIFT IN */
             k = out_SI[out_cur_set-1][i];
             fputc(k, outf);
             }
           }
         out_cur_set = out_set_number;  /* make it current now */
         if(out_cur_set > 0) {
           l = out_SO_len[out_cur_set-1]; /* length of SHIFT OUT sequence */
           for(i = 0; i < l; i++) {  /* output SHIFT OUT seq for this set */
             k = out_SO[out_cur_set-1][i];
             fputc(k, outf);
             }
           }
         }  /* end out_set changes */
       }  /* if multiple output sets specified */

     /* now output the corresponding sequence */
     for(i = 0; i < out_seq_length; i++) { 
       k = *(out_seq_ptr+i);
       fputc(k,outf);
       }

     /* move past processed input text */
     scrcurptr += inp_seq_length;
     chars_left -= inp_seq_length;


     }  /* while scanning input characters */

   }  /* end while reading input file */
 fprintf(outf,"%s",endseq);   /* output ending sequence */
 fclose(inpf);
 fclose(outf);
 exit(0);
}

