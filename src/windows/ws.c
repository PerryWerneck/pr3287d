/*
 * Copyright (c) 2007-2009, Paul Mattes.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Paul Mattes nor his contributors may be used
 *       to endorse or promote products derived from this software without
 *       specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PAUL MATTES "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PAUL MATTES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	ws.c
 *		Interactions with the Win32 print spooler (winspool).
 */

#include <windows.h>
#include <winspool.h>
#include <stdio.h>
#include <time.h>
#include "localdefs.h"
#include "wsc.h"
#include <fcntl.h>
#include "trace_dsc.h"

#define PRINTER_BUFSIZE	16384

static enum {
    PRINTER_IDLE,	/* not doing anything */
    PRINTER_OPEN,	/* open, but no pending print job */
    PRINTER_JOB		/* print job pending */
} printer_state = PRINTER_IDLE;

static enum {
	PRINTER_MODE_DEFAULT,	/* Use default windows spool */
	PRINTER_MODE_FILE,		/* Print to file */
} printer_mode = PRINTER_MODE_DEFAULT;

static HANDLE printer_handle;
static FILE *printer_file = NULL;

static char printer_buf[PRINTER_BUFSIZE];
static char output_path[PATH_MAX+1] = "\0";
static int pbcnt = 0;

/*
 * This is not means a general-purpose interface to the Win32 Print Spooler,
 * but rather the minimum subset needed by wpr3287.
 *
 * The functions generally return 0 for success, and -1 for failure.
 * If a failure occurs, they issue an error message via the 'errmsg' call.
 */

void ws_set_output_path(const char *path) {
	strncpy(output_path,path,PATH_MAX);
	printer_mode = PRINTER_MODE_FILE;
}

/*
 * Start talking to the named printer.
 * If printer_name is NULL, uses the default printer.
 * This call should should only be made once.
 */
int
ws_start(char *printer_name)
{
	trace_ds("Start talking to the printer.\n");

	switch(printer_mode) {
	case PRINTER_MODE_DEFAULT:
		{
			PRINTER_DEFAULTS defaults;

			/* If they didn't specify a printer, grab the default. */
			if (printer_name == NULL) {
				printer_name = ws_default_printer();
				if (printer_name == NULL) {
					errmsg("ws_start: No default printer");
					return -1;
				}
			}

			/* Talk to the printer. */
			(void) memset(&defaults, '\0', sizeof(defaults));
			defaults.pDatatype = "RAW";
			defaults.pDevMode = NULL;
			defaults.DesiredAccess = PRINTER_ACCESS_USE;

			if (OpenPrinter(printer_name, &printer_handle, &defaults) == 0) {
				errmsg("ws_start: OpenPrinter failed, "
					"Win32 error %d", GetLastError());
				return -1;
			}
		}
		break;

	case PRINTER_MODE_FILE:
		if(printer_file) {
			fclose(printer_file);
			printer_file = NULL;
		}
		break;

	}

    printer_state = PRINTER_OPEN;
    return 0;
}

/*
 * flush the print buffer.
 */
int
ws_flush(void)
{
	trace_ds("Flushing printer data.\n");

    switch (printer_state) {
	case PRINTER_IDLE:
	    errmsg("ws_endjob: printer not open");
	    return -1;
	case PRINTER_OPEN:
	    return 0;
	case PRINTER_JOB:
	    break;
    }

	int rv = 0;

	if (pbcnt != 0) {

		switch(printer_mode) {
		case PRINTER_MODE_DEFAULT:
			{
				DWORD wrote;

				if (WritePrinter(printer_handle, printer_buf, pbcnt, &wrote) == 0) {
					errmsg("ws_flush: WritePrinter failed, "
						"Win32 error %d", GetLastError());
					rv = -1;
				}

			}
			break;

		case PRINTER_MODE_FILE:
			fwrite(printer_buf,pbcnt,1,printer_file);
			break;
		}
		pbcnt = 0;

	}

    return rv;
}

int
ws_open()
{
	trace_ds("Opening printer.\n");

	switch(printer_mode) {
	case PRINTER_MODE_DEFAULT:
		{
			DOC_INFO_1 doc_info;
			/* Start a new document. */
			doc_info.pDocName = "wpr3287 print job";
			doc_info.pOutputFile = NULL;
			doc_info.pDatatype = "RAW";
			if (StartDocPrinter(printer_handle, 1, (LPBYTE)&doc_info) == 0) {
				errmsg("ws_putc: StartDocPrinter failed, Win32 error %d", GetLastError());
				return -1;
			}

		}
		break;

	case PRINTER_MODE_FILE:
		{
			static unsigned int sequencial = 0;
			char filename[PATH_MAX+1];
			char timestamp[20];
			char seq[10];

			{
				time_t t;
				struct tm *tmp;

				t = time(NULL);
				tmp = localtime(&t);
				if (tmp == NULL) {
					perror("localtime");
					exit(EXIT_FAILURE);
				}


				if (strftime(timestamp, sizeof(timestamp), "/%y%m%d", tmp) == 0) {
				   fprintf(stderr, "strftime returned 0");
				   exit(EXIT_FAILURE);
			   }
			}

			do {
				strncpy(filename,output_path,sizeof(filename)-1);
				strncat(filename,timestamp,sizeof(filename)-1);

				snprintf(seq,sizeof(seq),"%08d",(++sequencial));
				strncat(filename,seq,sizeof(filename)-1);

				strncat(filename,".txt",sizeof(filename)-1);
			} while(access(filename,F_OK) == 0);

			printer_file = fopen(filename,"w");

#ifdef DEBUG
			printf("Writing to %s\n",filename);
#endif // DEBUG

		}
	}

	return 0;
}


/*
 * Write a byte to the current print job.
 */
int
ws_putc(char c)
{
    switch (printer_state) {

	case PRINTER_IDLE:
	    errmsg("ws_putc: printer not open");
	    return -1;

	case PRINTER_OPEN:
		if(ws_open()) {
			return -1;
		}
	    printer_state = PRINTER_JOB;
	    pbcnt = 0;
	    break;

	case PRINTER_JOB:
	    break;
    }

    /* Flush if needed. */
    if ((pbcnt >= PRINTER_BUFSIZE) && (ws_flush() < 0))
		return -1;

    /* Buffer this character. */
    printer_buf[pbcnt++] = c;
    return 0;
}

int ws_putstring(const char *s) {
	return ws_write(s,strlen(s));
}

/*
 * Write multiple bytes to the current print job.
 */
int
ws_write(const char *s, int len)
{
    while (len--) {
	if (ws_putc(*s++) < 0)
	    return -1;
    }
    return 0;
}

/*
 * Complete the current print job.
 * Leaves the connection open for the next job, which is implicitly started
 * by the next call to ws_putc() or ws_write().
 */
int
ws_endjob(void)
{
	trace_ds("Finishing print job.\n");

    switch (printer_state) {
	case PRINTER_IDLE:
	    errmsg("ws_endjob: printer not open");
	    return -1;
	case PRINTER_OPEN:
	    return 0;
	case PRINTER_JOB:
	    break;
    }

    int rv = 0;

    /* Flush whatever's pending. */
    if (ws_flush() < 0)
		rv = 1;

    /* Close out the job. */

	switch(printer_mode) {
	case PRINTER_MODE_DEFAULT:
		{
			if (EndDocPrinter(printer_handle) == 0) {
			errmsg("ws_endjob: EndDocPrinter failed, "
				"Win32 error %d", GetLastError());
			rv = -1;
			}
		}
		break;

	case PRINTER_MODE_FILE:
		if(printer_file) {
			fclose(printer_file);
			printer_file = NULL;
		}
		break;

	}

    /* Done. */
    printer_state = PRINTER_OPEN;
    return rv;
}

/*
 * Antique method for figuring out the default printer.
 * Needed for compatibility with pre-Win2K systems.
 *
 * For Win2K and later, we could just call GetDefaultPrinter(), but that would
 * require delay-loading winspool.dll, which appears to be beyond MinGW and
 * GNU ld's capabilities at the moment.
 */
char *
ws_default_printer(void)
{
    static char pstring[1024];
    char *comma;

    /* Get the default printer, driver and port "from the .ini file". */
    pstring[0] = '\0';
    if (GetProfileString("windows", "device", "", pstring, sizeof(pstring))
	    == 0) {
	return NULL;
    }

    /*
     * Separate the printer name.  Note that commas are illegal in printer
     * names, so this method is safe.
     */
    comma = strchr(pstring, ',');
		if (comma != NULL)
	*comma = '\0';

    /*
     * If there is no default printer, I don't know if GetProfileString()
     * will fail, or if it will return nothing.  Perpare for the latter.
     */
    if (!*pstring)
		return NULL;

    return pstring;
}