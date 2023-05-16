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

#include <config.h>
#include <windows.h>
#include <winspool.h>
#include <stdio.h>
#include <time.h>
#include "localdefs.h"
#include "wsc.h"
#include <fcntl.h>
#include "trace_dsc.h"

#ifdef HAVE_PDFGEN
	#include <pdfgen.h>
#endif // HAVE_PDFGEN

#define PRINTER_BUFSIZE	16384

static enum {
    PRINTER_IDLE,	/* not doing anything */
    PRINTER_OPEN,	/* open, but no pending print job */
    PRINTER_JOB,	/* print job pending */
    PRINTER_PAGE	/* printer page open */
} printer_state = PRINTER_IDLE;

static enum {
	PRINTER_MODE_DEFAULT,	/* Use default windows spool */
	PRINTER_MODE_FILE,		/* Print to file */
#ifdef HAVE_PDFGEN
	PRINTER_MODE_PDF,		/* Print to pdf */
#endif // HAVE_PDFGEN
} printer_mode = PRINTER_MODE_DEFAULT;

static HANDLE printer_handle;
static FILE *printer_file = NULL;

static char printer_buf[PRINTER_BUFSIZE];
static char output_path[PATH_MAX+1] = "\0";
static int pbcnt = 0;

#ifdef HAVE_PDFGEN
struct {
	struct pdf_doc *document;
	struct pdf_object *page;
	float row;
	struct {
		float size;
		char * name;
	} font;
} pdf = {
	.document = NULL,
	.page = NULL,
	.row = 0
};
#endif // HAVE_PDFGEN

/*
 * This is not means a general-purpose interface to the Win32 Print Spooler,
 * but rather the minimum subset needed by wpr3287.
 *
 * The functions generally return 0 for success, and -1 for failure.
 * If a failure occurs, they issue an error message via the 'errmsg' call.
 */

void ws_set_pdf_output() {
#ifdef HAVE_PDFGEN
	printer_mode = PRINTER_MODE_PDF;
	if(!output_path[0]) {
		strncpy(output_path,".",PATH_MAX);
	}
#endif // HAVE_PDFGEN
}

void ws_set_output_path(const char *path) {
	strncpy(output_path,path,PATH_MAX);
	if(printer_mode == PRINTER_MODE_DEFAULT) {
		printer_mode = PRINTER_MODE_FILE;
	}
}

/*
 * Start talking to the named printer.
 * If printer_name is NULL, uses the default printer.
 * This call should should only be made once.
 */
int
ws_start(char *printer_name)
{
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

	case PRINTER_MODE_PDF:
		if(pdf.document) {
			pdf_destroy(pdf.document);
			pdf.document = NULL;
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
	case PRINTER_PAGE:
	    break;
    }

	int rv = 0;

	if (pbcnt != 0) {

		switch(printer_mode) {
		case PRINTER_MODE_DEFAULT:
			{
				DWORD wrote;

				if (WritePrinter(printer_handle, printer_buf, pbcnt, &wrote) == 0) {
					errmsg("ws_flush: WritePrinter failed, Win32 error %d", GetLastError());
					rv = -1;
				}

			}
			break;

		case PRINTER_MODE_FILE:
			fwrite(printer_buf,pbcnt,1,printer_file);
			break;

		case PRINTER_MODE_PDF:
			{
				printer_buf[pbcnt] = 0;
				char *line = printer_buf;
				while(*line) {
					char *ptr = strchr(line,'\n');

					if(ptr) {
						*(ptr++) = 0;
					} else {
						ptr = line+strlen(line);
					}

					// Remove control chars
					while(*line && *line < ' ') {
						line++;
					}

					// Compute font size.
					if(*line) {

						float document_width = pdf_page_width(pdf.page);
						float text_width = 0.0;

						if(pdf_get_font_text_width(pdf.document, pdf.font.name, line, pdf.font.size, &text_width) < 0) {
							errmsg("ws_flush: Cant get text width for '%s'",line);
							return -1;
						}
						while(text_width > document_width) {

							if(pdf.font.size < 0.1) {
								errmsg("ws_flush: Text line '%s' too big for font size",line);
								return -1;
							}

							pdf.font.size -= 0.1;
							if(pdf_get_font_text_width(pdf.document, pdf.font.name, line, pdf.font.size, &text_width) < 0) {
							errmsg("ws_flush: Cant get text width for '%s'",line);
								return -1;
							}
						}
					}

					// Compute text heigth.
					{
						pdf.row -= pdf.font.size + 2;
					}

					pdf_add_text(pdf.document, NULL, line, pdf.font.size, 10, pdf.row, PDF_BLACK);
					line = ptr;
				}


			}
			break;

		}
		pbcnt = 0;

	}

    return rv;
}

static void build_output_filename(char *filename,const char *ext) {
	static unsigned int sequencial = 0;
	char timestamp[20];
	char seq[10];
	char *ptr;

	{
		time_t t;
		struct tm *tmp;

		t = time(NULL);
		tmp = localtime(&t);
		if (tmp == NULL) {
			perror("localtime");
			exit(EXIT_FAILURE);
		}


		if (strftime(timestamp, sizeof(timestamp), "%y%m%d", tmp) == 0) {
		   fprintf(stderr, "strftime returned 0");
		   exit(EXIT_FAILURE);
	   }
	}

	do {
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wstringop-truncation"

		strncpy(filename,output_path,sizeof(filename)-1);

		if(filename[strlen(filename)-1] != '/') {
			strncat(filename,"/",sizeof(filename)-1);
		}
		strncat(filename,timestamp,sizeof(filename)-1);

		snprintf(seq,sizeof(seq),"%08d",(++sequencial));
		strncat(filename,seq,sizeof(filename)-1);

		strncat(filename,".",sizeof(filename)-1);
		strncat(filename,ext,sizeof(filename)-1);

		#pragma GCC diagnostic pop

		for(ptr = filename;*ptr;ptr++) {
			if(*ptr == '\\') {
				*ptr = '/';
			}
		}

	} while(access(filename,F_OK) == 0);

	#ifdef DEBUG
		printf("Writing to %s\n",filename);
	#endif // DEBUG

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
			doc_info.pDocName = PACKAGE_NAME " print job";
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
			char filename[PATH_MAX+1];
			build_output_filename(filename,"txt");
			printer_file = fopen(filename,"w");
		}
		break;

	case PRINTER_MODE_PDF:
		{
			struct pdf_info info = {
				.creator = PACKAGE_NAME,
				.producer = PACKAGE_NAME,
				.title = PACKAGE_NAME " print job",
				.author = PACKAGE_NAME,
				.subject = PACKAGE_NAME,
				.date = "Today"
			};

			pdf.document = pdf_create(PDF_A4_WIDTH, PDF_A4_HEIGHT, &info);
			pdf.page = NULL;
			pdf.font.size = 10;

			if(!pdf.font.name)
				pdf.font.name = "Courier";

			pdf_set_font(pdf.document, pdf.font.name);

		}
		break;

	}

	return 0;
}


/*
 * Write a byte to the current print job.
 */
int
ws_putc(char c)
{

	if(printer_state == PRINTER_IDLE) {
	    errmsg("ws_putc: printer not open");
	    return -1;
	}

	if(printer_state == PRINTER_OPEN) {
		if(ws_open()) {
			return -1;
		}
	    printer_state = PRINTER_JOB;
	    pbcnt = 0;
	}

	if(printer_state == PRINTER_JOB) {

		if(printer_mode == PRINTER_MODE_DEFAULT) {

			if(c == '\f') {
				return 0;
			}

			if(StartPagePrinter(printer_handle) == 0) {
				errmsg("%s: StartPagePrinter failed, Win32 error %d", __FUNCTION__, GetLastError());
				return -1;
			}

		} else if(printer_mode == PRINTER_MODE_PDF) {

			if(c == '\f') {
				return 0;
			}

			pdf.page = pdf_append_page(pdf.document);
			pdf.row = pdf_page_height(pdf.page);

		}

		printer_state = PRINTER_PAGE;

	}

	if(printer_state != PRINTER_PAGE) {
	    errmsg("ws_putc: Unexpected printer state");
	    return -1;
	}

	if(c == '\f') {

		if((ws_flush() < 0)) {
			return -1;
		}

		if(printer_mode == PRINTER_MODE_DEFAULT) {

			if(EndPagePrinter(printer_handle) == 0) {
				errmsg("%s: EndPagePrinter failed, Win32 error %d", __FUNCTION__, GetLastError());
				return -1;
			}

			printer_state = PRINTER_JOB;
			return 0;

		} else if(printer_mode == PRINTER_MODE_PDF) {

			printer_state = PRINTER_JOB;
			pdf.page = NULL;
			return 0;

		}
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
	int rv = 0;

	if(printer_state == PRINTER_IDLE) {
	    errmsg("ws_endjob: printer not open");
	    return -1;
	}

    /* Flush whatever's pending. */
    if (ws_flush() < 0)
		rv = 1;

    /* Close out the job. */

	switch(printer_mode) {
	case PRINTER_MODE_DEFAULT:
		{

			if(printer_state == PRINTER_PAGE && EndPagePrinter(printer_handle) == 0) {
				errmsg("%s: EndPagePrinter failed, Win32 error %d", __FUNCTION__, GetLastError());
				rv = 1;
			}

			if (EndDocPrinter(printer_handle) == 0) {
				errmsg("ws_endjob: EndDocPrinter failed, Win32 error %d", GetLastError());
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

	case PRINTER_MODE_PDF:
		{
			char filename[PATH_MAX+1];
			build_output_filename(filename,"pdf");

			trace_ds("Saving %s\n",filename);

			pdf_save(pdf.document, filename);
			pdf_destroy(pdf.document);
			pdf.document = NULL;
			pdf.page = NULL;

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
