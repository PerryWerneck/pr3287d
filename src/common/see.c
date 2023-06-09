/*
 * Copyright (c) 1993-2009, Paul Mattes.
 * Copyright (c) 2004, Don Russell.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of Paul Mattes, Don Russell, Jeff Sparkes, GTRC nor
 *       the names of their contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PAUL MATTES AND DON RUSSELL "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PAUL MATTES OR DON RUSSELL BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	see.c
 *		3270 data stream decode functions.
 *
 */

#include "globals.h"

#if defined(X3270_TRACE) /*[*/

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include "3270ds.h"

#include "tablesc.h"
#include "utf8c.h"
#include "seec.h"
#include "unicodec.h"

const char *
unknown(unsigned char value)
{
	static char buf[64];

	(void) sprintf(buf, "unknown[0x%x]", value);
	return buf;
}

const char *
see_ebc(unsigned char ch)
{
	static char buf[8];
	char mb[16];
	ucs4_t uc;

	switch (ch) {
	    case FCORDER_NULL:
		return "NULL";
	    case FCORDER_SUB:
		return "SUB";
	    case FCORDER_DUP:
		return "DUP";
	    case FCORDER_FM:
		return "FM";
	    case FCORDER_FF:
		return "FF";
	    case FCORDER_CR:
		return "CR";
	    case FCORDER_NL:
		return "NL";
	    case FCORDER_EM:
		return "EM";
	    case FCORDER_EO:
		return "EO";
	    case FCORDER_SI:
		return "SI";
	    case FCORDER_SO:
		return "SO";
	}
	if (ebcdic_to_multibyte_x(ch, CS_BASE, mb, sizeof(mb), EUO_NONE, &uc)
		    && (mb[0] != ' ' || ch == 0x40))
		strcpy(buf, mb);
	else
	    	(void) sprintf(buf, "X'%02X'", ch);
	return buf;
}

const char *
see_aid(unsigned char code)
{
	switch (code) {
	case AID_NO: 
		return "NoAID";
	case AID_ENTER: 
		return "Enter";
	case AID_PF1: 
		return "PF1";
	case AID_PF2: 
		return "PF2";
	case AID_PF3: 
		return "PF3";
	case AID_PF4: 
		return "PF4";
	case AID_PF5: 
		return "PF5";
	case AID_PF6: 
		return "PF6";
	case AID_PF7: 
		return "PF7";
	case AID_PF8: 
		return "PF8";
	case AID_PF9: 
		return "PF9";
	case AID_PF10: 
		return "PF10";
	case AID_PF11: 
		return "PF11";
	case AID_PF12: 
		return "PF12";
	case AID_PF13: 
		return "PF13";
	case AID_PF14: 
		return "PF14";
	case AID_PF15: 
		return "PF15";
	case AID_PF16: 
		return "PF16";
	case AID_PF17: 
		return "PF17";
	case AID_PF18: 
		return "PF18";
	case AID_PF19: 
		return "PF19";
	case AID_PF20: 
		return "PF20";
	case AID_PF21: 
		return "PF21";
	case AID_PF22: 
		return "PF22";
	case AID_PF23: 
		return "PF23";
	case AID_PF24: 
		return "PF24";
	case AID_OICR: 
		return "OICR";
	case AID_MSR_MHS: 
		return "MSR_MHS";
	case AID_SELECT: 
		return "Select";
	case AID_PA1: 
		return "PA1";
	case AID_PA2: 
		return "PA2";
	case AID_PA3: 
		return "PA3";
	case AID_CLEAR: 
		return "Clear";
	case AID_SYSREQ: 
		return "SysReq";
	case AID_QREPLY:
		return "QueryReplyAID";
	default: 
		return unknown(code);
	}
}

const char *
see_attr(unsigned char fa)
{
	static char buf[256];
	const char *paren = "(";

	buf[0] = '\0';

	if (fa & FA_PROTECT) {
		(void) strcat(buf, paren);
		(void) strcat(buf, "protected");
		paren = ",";
		if (fa & FA_NUMERIC) {
			(void) strcat(buf, paren);
			(void) strcat(buf, "skip");
			paren = ",";
		}
	} else if (fa & FA_NUMERIC) {
		(void) strcat(buf, paren);
		(void) strcat(buf, "numeric");
		paren = ",";
	}
	switch (fa & FA_INTENSITY) {
	case FA_INT_NORM_NSEL:
		break;
	case FA_INT_NORM_SEL:
		(void) strcat(buf, paren);
		(void) strcat(buf, "detectable");
		paren = ",";
		break;
	case FA_INT_HIGH_SEL:
		(void) strcat(buf, paren);
		(void) strcat(buf, "intensified");
		paren = ",";
		break;
	case FA_INT_ZERO_NSEL:
		(void) strcat(buf, paren);
		(void) strcat(buf, "nondisplay");
		paren = ",";
		break;
	}
	if (fa & FA_MODIFY) {
		(void) strcat(buf, paren);
		(void) strcat(buf, "modified");
		paren = ",";
	}
	if (strcmp(paren, "("))
		(void) strcat(buf, ")");
	else
		(void) strcpy(buf, "(default)");

	return buf;
}

static const char *
see_highlight(unsigned char setting)
{
	switch (setting) {
	    case XAH_DEFAULT:
		return "default";
	    case XAH_NORMAL:
		return "normal";
	    case XAH_BLINK:
		return "blink";
	    case XAH_REVERSE:
		return "reverse";
	    case XAH_UNDERSCORE:
		return "underscore";
	    case XAH_INTENSIFY:
		return "intensify";
	    default:
		return unknown(setting);
	}
}

const char *
see_color(unsigned char setting)
{
	static const char *color_name[] = {
	    "neutralBlack",
	    "blue",
	    "red",
	    "pink",
	    "green",
	    "turquoise",
	    "yellow",
	    "neutralWhite",
	    "black",
	    "deepBlue",
	    "orange",
	    "purple",
	    "paleGreen",
	    "paleTurquoise",
	    "grey",
	    "white"
	};

	if (setting == XAC_DEFAULT)
		return "default";
	else if (setting < 0xf0)
		return unknown(setting);
	else
		return color_name[setting - 0xf0];
}

static const char *
see_transparency(unsigned char setting)
{
	switch (setting) {
	    case XAT_DEFAULT:
		return "default";
	    case XAT_OR:
		return "or";
	    case XAT_XOR:
		return "xor";
	    case XAT_OPAQUE:
		return "opaque";
	    default:
		return unknown(setting);
	}
}

static const char *
see_validation(unsigned char setting)
{
	static char buf[64];
	const char *paren = "(";

	(void) strcpy(buf, "");
	if (setting & XAV_FILL) {
		(void) strcat(buf, paren);
		(void) strcat(buf, "fill");
		paren = ",";
	}
	if (setting & XAV_ENTRY) {
		(void) strcat(buf, paren);
		(void) strcat(buf, "entry");
		paren = ",";
	}
	if (setting & XAV_TRIGGER) {
		(void) strcat(buf, paren);
		(void) strcat(buf, "trigger");
		paren = ",";
	}
	if (strcmp(paren, "("))
		(void) strcat(buf, ")");
	else
		(void) strcpy(buf, "(none)");
	return buf;
}

static const char *
see_outline(unsigned char setting)
{
	static char buf[64];
	const char *paren = "(";

	(void) strcpy(buf, "");
	if (setting & XAO_UNDERLINE) {
		(void) strcat(buf, paren);
		(void) strcat(buf, "underline");
		paren = ",";
	}
	if (setting & XAO_RIGHT) {
		(void) strcat(buf, paren);
		(void) strcat(buf, "right");
		paren = ",";
	}
	if (setting & XAO_OVERLINE) {
		(void) strcat(buf, paren);
		(void) strcat(buf, "overline");
		paren = ",";
	}
	if (setting & XAO_LEFT) {
		(void) strcat(buf, paren);
		(void) strcat(buf, "left");
		paren = ",";
	}
	if (strcmp(paren, "("))
		(void) strcat(buf, ")");
	else
		(void) strcpy(buf, "(none)");
	return buf;
}

static const char *
see_input_control(unsigned char setting)
{
	switch (setting) {
	    case XAI_DISABLED:
		return "disabled";
	    case XAI_ENABLED:
		return "enabled";
	    default:
		return unknown(setting);
	}
}

const char *
see_efa(unsigned char efa, unsigned char value)
{
	static char buf[64];

	switch (efa) {
	    case XA_ALL:
		(void) sprintf(buf, " all(%x)", value);
		break;
	    case XA_3270:
		(void) sprintf(buf, " 3270%s", see_attr(value));
		break;
	    case XA_VALIDATION:
		(void) sprintf(buf, " validation%s", see_validation(value));
		break;
	    case XA_OUTLINING:
		(void) sprintf(buf, " outlining(%s)", see_outline(value));
		break;
	    case XA_HIGHLIGHTING:
		(void) sprintf(buf, " highlighting(%s)", see_highlight(value));
		break;
	    case XA_FOREGROUND:
		(void) sprintf(buf, " foreground(%s)", see_color(value));
		break;
	    case XA_CHARSET:
		(void) sprintf(buf, " charset(%x)", value);
		break;
	    case XA_BACKGROUND:
		(void) sprintf(buf, " background(%s)", see_color(value));
		break;
	    case XA_TRANSPARENCY:
		(void) sprintf(buf, " transparency(%s)",
		    see_transparency(value));
		break;
	    case XA_INPUT_CONTROL:
		(void) sprintf(buf, " input-control(%s)",
		    see_input_control(value));
		break;
	    default:
		(void) sprintf(buf, " %s[0x%x]", unknown(efa), value);
		break;
	}
	return buf;
}

const char *
see_efa_only(unsigned char efa)
{
	switch (efa) {
	    case XA_ALL:
		return "all";
	    case XA_3270:
		return "3270";
	    case XA_VALIDATION:
		return "validation";
	    case XA_OUTLINING:
		return "outlining";
	    case XA_HIGHLIGHTING:
		return "highlighting";
	    case XA_FOREGROUND:
		return "foreground";
	    case XA_CHARSET:
		return "charset";
	    case XA_BACKGROUND:
		return "background";
	    case XA_TRANSPARENCY:
		return "transparency";
	    default:
		return unknown(efa);
	}
}

const char *
see_qcode(unsigned char id)
{
	static char buf[64];

	switch (id) {
	    case QR_CHARSETS:
		return "CharacterSets";
	    case QR_IMP_PART:
		return "ImplicitPartition";
	    case QR_SUMMARY:
		return "Summary";
	    case QR_USABLE_AREA:
		return "UsableArea";
	    case QR_COLOR:
		return "Color";
	    case QR_HIGHLIGHTING:
		return "Highlighting";
	    case QR_REPLY_MODES:
		return "ReplyModes";
	    case QR_DBCS_ASIA:
		return "DbcsAsia";
	    case QR_ALPHA_PART:
		return "AlphanumericPartitions";
	    case QR_DDM:
		return "DistributedDataManagement";
	    case QR_RPQNAMES:
		return "RPQNames";
	    default:
		(void) sprintf(buf, "unknown[0x%x]", id);
		return buf;
	}
}

#endif /*]*/
