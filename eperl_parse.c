/*
**        ____           _ 
**    ___|  _ \ ___ _ __| |
**   / _ \ |_) / _ \ '__| |
**  |  __/  __/  __/ |  | |
**   \___|_|   \___|_|  |_|
** 
**  ePerl -- Embedded Perl 5 Language
**
**  ePerl interprets an ASCII file bristled with Perl 5 program statements
**  by evaluating the Perl 5 code while passing through the plain ASCII
**  data. It can operate both as a standard Unix filter for general file
**  generation tasks and as a powerful Webserver scripting language for
**  dynamic HTML page programming. 
**
**  ======================================================================
**
**  Copyright (c) 1996,1997,1998 Ralf S. Engelschall <rse@engelschall.com>
**
**  This program is free software; it may be redistributed and/or modified
**  only under the terms of either the Artistic License or the GNU General
**  Public License, which may be found in the ePerl source distribution.
**  Look at the files ARTISTIC and COPYING or run ``eperl -l'' to receive
**  a built-in copy of both license files.
**
**  This program is distributed in the hope that it will be useful, but
**  WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See either the
**  Artistic License or the GNU General Public License for more details.
**
**  ======================================================================
**
**  eperl_parse.c -- ePerl parser stuff
*/


#include "eperl_config.h"
#include "eperl_global.h"
#include "eperl_proto.h"


/*
**
**  Static Data
**
*/

char       *ePerl_begin_delimiter           = NULL;
char       *ePerl_end_delimiter             = NULL;
int         ePerl_case_sensitive_delimiters = TRUE;
int         ePerl_convert_entities          = FALSE;
int         ePerl_line_continuation         = FALSE;
static char ePerl_ErrorString[1024]         = "";

/*
**
**  Functions
**
*/

/*
**  set ePerl error string
*/
void ePerl_SetError(char *str, ...)
{
    va_list ap;

    va_start(ap, str);
    vsprintf(ePerl_ErrorString, str, ap);
    va_end(ap);
    return;
}

/*
**  get ePerl error string
*/
char *ePerl_GetError(void)
{
    return ePerl_ErrorString;
}

/*
**  fprintf for internal buffer 
*/
char *ePerl_fprintf(char *cpOut, char *str, ...)
{
    va_list ap;

    va_start(ap, str);
    vsprintf(cpOut, str, ap);
    va_end(ap);
    return cpOut+strlen(cpOut);
}

/*
**  fwrite for internal buffer 
*/
char *ePerl_fwrite(char *cpBuf, int nBuf, int cNum, char *cpOut)
{
    char *cp;
    int n;

    n = nBuf*cNum;
    (void)strncpy(cpOut, cpBuf, n);
    cp = cpOut + n;
    *cp = NUL;
    return cp;
}

/*
**  fwrite for internal buffer WITH character escaping
*/
char *ePerl_Efwrite(char *cpBuf, int nBuf, int cNum, char *cpOut)
{
    char *cpI;
    char *cpO;

    for (cpI = cpBuf, cpO = cpOut; cpI < (cpBuf+(nBuf*cNum)); ) {
        switch (*cpI) {
            case '"':  *cpO++ = '\\'; *cpO++ = *cpI++;     break;
            case '@':  *cpO++ = '\\'; *cpO++ = *cpI++;     break;
            case '$':  *cpO++ = '\\'; *cpO++ = *cpI++;     break;
            case '\\': *cpO++ = '\\'; *cpO++ = *cpI++;     break;
            case '\t': *cpO++ = '\\'; *cpO++ = 't'; cpI++; break;
            case '\n': *cpO++ = '\\'; *cpO++ = 'n'; cpI++; break;
            default: *cpO++ = *cpI++;
        }
    }
    *cpO = NUL;
    return cpO;
}


/*
**  fwrite for internal buffer WITH HTML entity conversion
*/

struct html2char {
    char *h;
    char c;
};

static struct html2char html2char[] = {
    { "copy",   '?' },    /* Copyright */
    { "die",    '?' },    /* Di?resis / Umlaut */
    { "laquo",  '?' },    /* Left angle quote, guillemot left */
    { "not",    '?' },    /* Not sign */
    { "ordf",   '?' },    /* Feminine ordinal */
    { "sect",   '?' },    /* Section sign */
    { "um",     '?' },    /* Di?resis / Umlaut */
    { "AElig",  '?' },    /* Capital AE ligature */
    { "Aacute", '?' },    /* Capital A, acute accent */
    { "Acirc",  '?' },    /* Capital A, circumflex */
    { "Agrave", '?' },    /* Capital A, grave accent */
    { "Aring",  '?' },    /* Capital A, ring */
    { "Atilde", '?' },    /* Capital A, tilde */
    { "Auml",   '?' },    /* Capital A, di?resis / umlaut */
    { "Ccedil", '?' },    /* Capital C, cedilla */
    { "ETH",    '?' },    /* Capital Eth, Icelandic */
    { "Eacute", '?' },    /* Capital E, acute accent */
    { "Ecirc",  '?' },    /* Capital E, circumflex */
    { "Egrave", '?' },    /* Capital E, grave accent */
    { "Euml",   '?' },    /* Capital E, di?resis / umlaut */
    { "Iacute", '?' },    /* Capital I, acute accent */
    { "Icirc",  '?' },    /* Capital I, circumflex */
    { "Igrave", '?' },    /* Capital I, grave accent */
    { "Iuml",   '?' },    /* Capital I, di?resis / umlaut */
    { "Ntilde", '?' },    /* Capital N, tilde */
    { "Oacute", '?' },    /* Capital O, acute accent */
    { "Ocirc",  '?' },    /* Capital O, circumflex */
    { "Ograve", '?' },    /* Capital O, grave accent */
    { "Oslash", '?' },    /* Capital O, slash */
    { "Otilde", '?' },    /* Capital O, tilde */
    { "Ouml",   '?' },    /* Capital O, di?resis / umlaut */
    { "THORN",  '?' },    /* Capital Thorn, Icelandic */
    { "Uacute", '?' },    /* Capital U, acute accent */
    { "Ucirc",  '?' },    /* Capital U, circumflex */
    { "Ugrave", '?' },    /* Capital U, grave accent */
    { "Uuml",   '?' },    /* Capital U, di?resis / umlaut */
    { "Yacute", '?' },    /* Capital Y, acute accent */
    { "aacute", '?' },    /* Small a, acute accent */
    { "acirc",  '?' },    /* Small a, circumflex */
    { "acute",  '?' },    /* Acute accent */
    { "aelig",  '?' },    /* Small ae ligature */
    { "agrave", '?' },    /* Small a, grave accent */
    { "amp",    '&' },    /* Ampersand */
    { "aring",  '?' },    /* Small a, ring */
    { "atilde", '?' },    /* Small a, tilde */
    { "auml",   '?' },    /* Small a, di?resis / umlaut */
    { "brkbar", '?' },    /* Broken vertical bar */
    { "brvbar", '?' },    /* Broken vertical bar */
    { "ccedil", '?' },    /* Small c, cedilla */
    { "cedil",  '?' },    /* Cedilla */
    { "cent",   '?' },    /* Cent sign */
    { "curren", '?' },    /* General currency sign */
    { "deg",    '?' },    /* Degree sign */
    { "divide", '?' },    /* Division sign */
    { "eacute", '?' },    /* Small e, acute accent */
    { "ecirc",  '?' },    /* Small e, circumflex */
    { "egrave", '?' },    /* Small e, grave accent */
    { "eth",    '?' },    /* Small eth, Icelandic */
    { "euml",   '?' },    /* Small e, di?resis / umlaut */
    { "frac12", '?' },    /* Fraction one-half */
    { "frac14", '?' },    /* Fraction one-fourth */
    { "frac34", '?' },    /* Fraction three-fourths */
    { "gt",     '>' },    /* Greater than */
    { "hibar",  '?' },    /* Macron accent */
    { "iacute", '?' },    /* Small i, acute accent */
    { "icirc",  '?' },    /* Small i, circumflex */
    { "iexcl",  '?' },    /* Inverted exclamation */
    { "igrave", '?' },    /* Small i, grave accent */
    { "iquest", '?' },    /* Inverted question mark */
    { "iuml",   '?' },    /* Small i, di?resis / umlaut */
    { "lt",     '<' },    /* Less than */
    { "macr",   '?' },    /* Macron accent */
    { "micro",  '?' },    /* Micro sign */
    { "middot", '?' },    /* Middle dot */
    { "nbsp",   ' ' },    /* Non-breaking Space */
    { "ntilde", '?' },    /* Small n, tilde */
    { "oacute", '?' },    /* Small o, acute accent */
    { "ocirc",  '?' },    /* Small o, circumflex */
    { "ograve", '?' },    /* Small o, grave accent */
    { "ordm",   '?' },    /* Masculine ordinal */
    { "oslash", '?' },    /* Small o, slash */
    { "otilde", '?' },    /* Small o, tilde */
    { "ouml",   '?' },    /* Small o, di?resis / umlaut */
    { "para",   '?' },    /* Paragraph sign */
    { "plusmn", '?' },    /* Plus or minus */
    { "pound",  '?' },    /* Pound sterling */
    { "quot",   '"' },    /* Quotation mark */
    { "raquo",  '?' },    /* Right angle quote, guillemot right */
    { "reg",    '?' },    /* Registered trademark */
    { "shy",    '?' },    /* Soft hyphen */
    { "sup1",   '?' },    /* Superscript one */
    { "sup2",   '?' },    /* Superscript two */
    { "sup3",   '?' },    /* Superscript three */
    { "szlig",  '?' },    /* Small sharp s, German sz */
    { "thorn",  '?' },    /* Small thorn, Icelandic */
    { "times",  '?' },    /* Multiply sign */
    { "uacute", '?' },    /* Small u, acute accent */
    { "ucirc",  '?' },    /* Small u, circumflex */
    { "ugrave", '?' },    /* Small u, grave accent */
    { "uuml",   '?' },    /* Small u, di?resis / umlaut */
    { "yacute", '?' },    /* Small y, acute accent */
    { "yen",    '?' },    /* Yen sign */
    { "yuml",'\255' },    /* Small y, di?resis / umlaut */
    { NULL, NUL }
};

char *ePerl_Cfwrite(char *cpBuf, int nBuf, int cNum, char *cpOut)
{
    char *cpI;
    char *cpO;
    int i;
    int n;
    char *cpE;

    cpI = cpBuf;
    cpO = cpOut;
    cpE = cpBuf+(nBuf*cNum);
    while (cpI < cpE) {
        if (*cpI == '&') {
            for (i = 0; html2char[i].h != NULL; i++) {
                n = strlen(html2char[i].h);
                if (cpI+1+n+1 < cpE) {
                    if (*(cpI+1+n) == ';') {
                        if (strncmp(cpI+1, html2char[i].h, n) == 0) {
                            *cpO++ = html2char[i].c;
                            cpI += 1+n+1;
                            continue;
                        }
                    }
                }
            }
        }
        *cpO++ = *cpI++;
    }
    *cpO = NUL;
    return cpO;
}


/*
**
**  Own string functions with maximum length (n) support 
**
*/

char *strnchr(char *buf, char chr, int n)
{
    char *cp;
    char *cpe;

    for (cp = buf, cpe = buf+n-1; cp <= cpe; cp++) {
        if (*cp == chr)
            return cp;
    }
    return NULL;
}

char *strnstr(char *buf, char *str, int n)
{
    char *cp;
    char *cpe;
    int len;
    
    len = strlen(str);
    for (cp = buf, cpe = buf+n-len; cp <= cpe; cp++) {
        if (strncmp(cp, str, len) == 0)
            return cp;
    }
    return NULL;
}

char *strncasestr(char *buf, char *str, int n)
{
    char *cp;
    char *cpe;
    int len;
    
    len = strlen(str);
    for (cp = buf, cpe = buf+n-len; cp <= cpe; cp++) {
        if (strncasecmp(cp, str, len) == 0)
            return cp;
    }
    return NULL;
}

char *strndup(char *buf, int n)
{
    char *cp;

    cp = (char *)malloc(n+1);
    strncpy(cp, buf, n);
    return cp;
}


/*
**  convert buffer from bristled format to plain format
*/
char *ePerl_Bristled2Plain(char *cpBuf)
{
    char *rc;
    char *cpOutBuf = NULL;
    char *cpOut = NULL;
    char *cps, *cpe;
    char *cps2, *cpe2;
    int nBuf;
    char *cpEND;
    int n;

    if (strlen(cpBuf) == 0) {
        /* make sure we return a buffer which the caller can free() */
        cpOutBuf = (char *)malloc(sizeof(char) * 1);
        *cpOutBuf = NUL;
        return cpOutBuf;
    }

    nBuf = strlen(cpBuf);
    cpEND = cpBuf+nBuf;

    /* allocate memory for the Perl code */
    n = sizeof(char) * nBuf * 10;
    if (nBuf < 1024)
        n = 16384;
    if ((cpOutBuf = (char *)malloc(n)) == NULL) {
        ePerl_SetError("Cannot allocate %d bytes of memory", n);
        CU(NULL);
    }
    cpOut = cpOutBuf;

    /* now step through the file and convert it to legal Perl code.
       This is a bit complicated because we have to make sure that
       we parse the correct delimiters while the delimiter
       characters could also occur inside the Perl code! */
    cps = cpBuf;
    while (cps < cpEND) {

        if (ePerl_case_sensitive_delimiters)
            cpe = strnstr(cps, ePerl_begin_delimiter, cpEND-cps);
        else
            cpe = strncasestr(cps, ePerl_begin_delimiter, cpEND-cps);
        if (cpe == NULL) {

            /* there are no more ePerl blocks, so
               just encapsulate the remaining contents into
               Perl print constructs */

            if (cps < cpEND) {
                cps2 = cps;
                /* first, do all complete lines */
                while (cps2 < cpEND && (cpe2 = strnchr(cps2, '\n', cpEND-cps2)) != NULL) {
                    if (ePerl_line_continuation && cps < cpe2 && *(cpe2-1) == '\\') {
                        if (cpe2-1-cps2 > 0) {
                            cpOut = ePerl_fprintf(cpOut, "print \"");
                            cpOut = ePerl_Efwrite(cps2, cpe2-1-cps2, 1, cpOut);
                            cpOut = ePerl_fprintf(cpOut, "\";");
                        }
                        cpOut = ePerl_fprintf(cpOut, "\n");
                    }
                    else {
                        cpOut = ePerl_fprintf(cpOut, "print \"");
                        cpOut = ePerl_Efwrite(cps2, cpe2-cps2, 1, cpOut);
                        cpOut = ePerl_fprintf(cpOut, "\\n\";\n");
                    }
                    cps2 = cpe2+1;
                }
                /* then do the remainder which is not
                   finished by a newline */
                if (cpEND > cps2) {
                    cpOut = ePerl_fprintf(cpOut, "print \"");
                    cpOut = ePerl_Efwrite(cps2, cpEND-cps2, 1, cpOut);
                    cpOut = ePerl_fprintf(cpOut, "\";");
                }
            }
            break; /* and break the whole processing step */

        }
        else {

            /* Ok, there is at least one more ePerl block */

            /* first, encapsulate the content from current pos
               up to the begin of the ePerl block as print statements */
            if (cps < cpe) {
                cps2 = cps;
                while ((cpe2 = strnchr(cps2, '\n', cpe-cps2)) != NULL) {
                    if (ePerl_line_continuation && cps < cpe2 && *(cpe2-1) == '\\') {
                        if (cpe2-1-cps2 > 0) {
                            cpOut = ePerl_fprintf(cpOut, "print \"");
                            cpOut = ePerl_Efwrite(cps2, cpe2-1-cps2, 1, cpOut);
                            cpOut = ePerl_fprintf(cpOut, "\";");
                        }
                        cpOut = ePerl_fprintf(cpOut, "\n");
                    }
                    else {
                        cpOut = ePerl_fprintf(cpOut, "print \"");
                        cpOut = ePerl_Efwrite(cps2, cpe2-cps2, 1, cpOut);
                        cpOut = ePerl_fprintf(cpOut, "\\n\";\n");
                    }
                    cps2 = cpe2+1;
                }
                if (cpe > cps2) {
                    cpOut = ePerl_fprintf(cpOut, "print \"");
                    cpOut = ePerl_Efwrite(cps2, cpe-cps2, 1, cpOut);
                    cpOut = ePerl_fprintf(cpOut, "\";");
                }
            }

            /* just output a leading space to make
               the -x display more readable. */
            if (cpOut > cpOutBuf && *(cpOut-1) != '\n') 
                cpOut = ePerl_fprintf(cpOut, " ");

            /* skip the start delimiter */
            cps = cpe+strlen(ePerl_begin_delimiter);

            /* recognize the 'print' shortcut with '=',
             * e.g. <:=$var:>
             */
            if (*cps == '=') {
                cpOut = ePerl_fprintf(cpOut, "print ");
                cps++;
            }

            /* skip all following whitespaces.
               Be careful: we could skip newlines too, but then the
               error output will give wrong line numbers!!! */
            while (cps < cpEND) {
                if (*cps != ' ' && *cps != '\t')
                    break;
                cps++;
            }
            cpe = cps;

            /* move forward to end of ePerl block. */
            if (ePerl_case_sensitive_delimiters)
                cpe = strnstr(cpe, ePerl_end_delimiter, cpEND-cpe);
            else
                cpe = strncasestr(cpe, ePerl_end_delimiter, cpEND-cpe);
            if (cpe == NULL) {
                ePerl_SetError("Missing end delimiter");
                CU(NULL);
            }

            /* step again backward over whitespaces */
            for (cpe2 = cpe; 
                 cpe2 > cps && (*(cpe2-1) == ' ' || *(cpe2-1) == '\t' || *(cpe2-1) == '\n');
                 cpe2--)
                ;
            
            /* pass through the ePerl block without changes! */
            if (cpe2 > cps) { 
                if (ePerl_convert_entities == TRUE)
                    cpOut = ePerl_Cfwrite(cps, cpe2-cps, 1, cpOut);
                else
                    cpOut = ePerl_fwrite(cps, cpe2-cps, 1, cpOut);

                /* be smart and automatically add a semicolon
                   if not provided at the end of the ePerl block.
                   But know the continuation indicator "_". */
                if ((*(cpe2-1) != ';') &&
                    (*(cpe2-1) != '_')   )
                    cpOut = ePerl_fprintf(cpOut, ";");
                if (*(cpe2-1) == '_') 
                    cpOut = cpOut - 1;
            }

            /* end preserve newlines for correct line numbers */
            for ( ; cpe2 <= cpe; cpe2++)
                if (*cpe2 == '\n')
                    cpOut = ePerl_fprintf(cpOut, "\n");

            /* output a trailing space to make
               the -x display more readable when
               no newlines have finished the block. */
            if (cpOut > cpOutBuf && *(cpOut-1) != '\n') 
                cpOut = ePerl_fprintf(cpOut, " ");

            /* and adjust the current position to the first character
               after the end delimiter */
            cps = cpe+strlen(ePerl_end_delimiter);

            /* finally just one more feature: when an end delimiter
               is directly followed by ``//'' this discards all
               data up to and including the following newline */
            if (cps < cpEND-2 && *cps == '/' && *(cps+1) == '/') {
                /* skip characters */
                cps += 2;
                for ( ; cps < cpEND && *cps != '\n'; cps++)
                    ;
                if (cps < cpEND)
                    cps++;
                /* but preserve the newline in the script */
                cpOut = ePerl_fprintf(cpOut, "\n");
            }
        }
    }
    RETURN_WVAL(cpOutBuf);

    CUS:
    if (cpOutBuf) 
        free(cpOutBuf);
    RETURN_EXRC;
}

/*EOF*/
