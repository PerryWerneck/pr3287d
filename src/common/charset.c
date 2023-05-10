/*
 * Copyright (c) 2001-2009, Paul Mattes.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of Paul Mattes nor the names of his contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PAUL MATTES "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL PAUL MATTES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	charset.c
 *		Limited character set support.
 */

#include "globals.h"

#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#if !defined(_WIN32) /*[*/
#include <locale.h>
#include <langinfo.h>
#else
#include <winnls.h>
#endif /*]*/

#if defined(__CYGWIN__) /*[*/
#include <w32api/windows.h>
#undef _WIN32
#endif /*]*/

#include "3270ds.h"
#include "charsetc.h"
#include "unicodec.h"
#include "unicode_dbcsc.h"
#include "utf8c.h"

unsigned long cgcsgid = 0x02b90025;
unsigned long cgcsgid_dbcs = 0x02b90025;
int dbcs = 0;

char *encoding = CN;
char *converters = CN;

/*
 * Change character sets.
 * Returns 0 if the new character set was found, -1 otherwise.
 */
enum cs_result
charset_init(char *csname)
{

#if defined(_WIN32) 
	{
		char codeset_name[64];
		memset(codeset_name,0,64);
		snprintf(codeset_name, 63, "CP%d", GetACP());
		set_codeset(codeset_name);
	}

#else
	{
		setlocale(LC_ALL, "");
		const char *codeset_name = nl_langinfo(CODESET);
		set_codeset(codeset_name);
	}


#endif // _WIN32

	const char *host_codepage;
    const char *cgcsgid_str;
	const char *display_charsets;

	if (set_uni(csname, &host_codepage, &cgcsgid_str, &display_charsets) < 0)
		return CS_NOTFOUND;

	cgcsgid = strtoul(cgcsgid_str, NULL, 0);
	if (!(cgcsgid & ~0xffff))
		cgcsgid |= 0x02b90000;

#if defined(X3270_DBCS) /*[*/
	if (set_uni_dbcs(csname, &cgcsgid_str, &display_charsets) == 0) {
	    dbcs = 1;
		cgcsgid_dbcs = strtoul(cgcsgid_str, NULL, 0);
	}
#endif /*]*/

	return CS_OKAY;
}
