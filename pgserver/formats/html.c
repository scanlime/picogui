/* $Id: html.c,v 1.2 2001/10/17 05:25:01 micahjd Exp $
 *
 * html.c - Use the textbox_document inferface to load HTML markup
 *
 * ---------- Notes:
 * This is not a complete HTML implementation- though this may form the core
 * rendering facilities for a web browser, it is _not_ a standalone web
 * browser. What it can do is use a subset of HTML to represent text
 * formatting in a textbox widget.
 *
 * Currently this parser tries to be a correct subset of the HTML 3.2 spec at:
 *    http://www.w3.org/TR/REC-html32
 *
 * Unknown tags are ignored. The following tags should be supported
 * by this implementation. Tags with an asterisk are not yet implemented:
 *
 *   <body> *
 *   <address> *
 *   <p>
 *   <ul> *
 *   <ol> *
 *   <dl> *
 *   <pre> *
 *   <div> *
 *   <center> *
 *   <blockquote> *
 *   <form> *
 *   <hr> *
 *   <table> *
 *   <TT> *
 *   <I> *
 *   <B> *
 *   <U> *
 *   <STRIKE> *
 *   <BIG> *
 *   <SMALL> *
 *   <SUB> *
 *   <SUP> *
 *   <EM> *
 *   <STRONG> *
 *   <DFN> *
 *   <CODE> *
 *   <SAMP> *
 *   <KBD> *
 *   <VAR> *
 *   <CITE> *
 *   <INPUT> *
 *   <SELECT> *
 *   <TEXTAREA> *
 *   <A> *
 *   <IMG> *
 *   <FONT> *
 *   <BASEFONT> *
 *   <BR> *
 *   <MAP> *
 *   ISO Latin-1 character entities
 *   UTF-8 encoding *
 *
 * ----------
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/textbox.h>

/* Information on the parser state */
struct html_parse {
  struct textbox_cursor *c;
};

/* Parameters for HTML tags */
struct html_tag_params {
  /* Raw text of the HTML parameters */
  const u8 *paramtext;
  int paramtext_len;
};

g_error html_dispatch_tag(struct html_parse *hp,
			  const u8 *start,const u8 *end);
g_error html_dispatch_text(struct html_parse *hp,
			   const u8 *start,const u8 *end);
g_error html_dispatch_textfragment(struct html_parse *hp,
				   const u8 *start,const u8 *end);
char html_findchar(const u8 *charname, int namelen);

/************************* Character name table */

/* This table is used to convert character names like &nbsp; to character
 * numbers, as defined by the HTML standard.
 */

struct html_charname {
  const char *name;
  char ch;
} html_chartable[] = {

  { "nbsp",	32  },	/* no-break space */
  { "iexcl",	161 },	/* inverted exclamation mark */
  { "cent",	162 },	/* cent sign */
  { "pound",	163 },	/* pound sterling sign */
  { "curren",	164 },	/* general currency sign */
  { "yen",	165 },	/* yen sign */
  { "brvbar",	166 },	/* broken (vertical) bar */
  { "sect",	167 },	/* section sign */
  { "uml",	168 },	/* umlaut (dieresis) */
  { "copy",	169 },	/* copyright sign */
  { "ordf",	170 },	/* ordinal indicator, feminine */
  { "laquo",	171 },	/* angle quotation mark, left */
  { "not",	172 },	/* not sign */
  { "shy",	173 },	/* soft hyphen */
  { "reg",	174 },	/* registered sign */
  { "macr",	175 },	/* macron */
  { "deg",	176 },	/* degree sign */
  { "plusmn",	177 },	/* plus-or-minus sign */
  { "sup2",	178 },	/* superscript two */
  { "sup3",	179 },	/* superscript three */
  { "acute",	180 },	/* acute accent */
  { "micro",	181 },	/* micro sign */
  { "para",	182 },	/* pilcrow (paragraph sign) */
  { "middot",	183 },	/* middle dot */
  { "cedil",	184 },	/* cedilla */
  { "sup1",	185 },	/* superscript one */
  { "ordm",	186 },	/* ordinal indicator, masculine */
  { "raquo",	187 },	/* angle quotation mark, right */
  { "frac14",	188 },	/* fraction one-quarter */
  { "frac12",	189 },	/* fraction one-half */
  { "frac34",	190 },	/* fraction three-quarters */
  { "iquest",	191 },	/* inverted question mark */
  { "Agrave",	192 },	/* capital A, grave accent */
  { "Aacute",	193 },	/* capital A, acute accent */
  { "Acirc",	194 },	/* capital A, circumflex accent */
  { "Atilde",	195 },	/* capital A, tilde */
  { "Auml",	196 },	/* capital A, dieresis or umlaut mark */
  { "Aring",	197 },	/* capital A, ring */
  { "AElig",	198 },	/* capital AE diphthong (ligature) */
  { "Ccedil",	199 },	/* capital C, cedilla */
  { "Egrave",	200 },	/* capital E, grave accent */
  { "Eacute",	201 },	/* capital E, acute accent */
  { "Ecirc",	202 },	/* capital E, circumflex accent */
  { "Euml",	203 },	/* capital E, dieresis or umlaut mark */
  { "Igrave",	204 },	/* capital I, grave accent */
  { "Iacute",	205 },	/* capital I, acute accent */
  { "Icirc",	206 },	/* capital I, circumflex accent */
  { "Iuml",	207 },	/* capital I, dieresis or umlaut mark */
  { "ETH",	208 },	/* capital Eth, Icelandic */
  { "Ntilde",	209 },	/* capital N, tilde */
  { "Ograve",	210 },	/* capital O, grave accent */
  { "Oacute",	211 },	/* capital O, acute accent */
  { "Ocirc",	212 },	/* capital O, circumflex accent */
  { "Otilde",	213 },	/* capital O, tilde */
  { "Ouml",	214 },	/* capital O, dieresis or umlaut mark */
  { "times",	215 },	/* multiply sign */
  { "Oslash",	216 },	/* capital O, slash */
  { "Ugrave",	217 },	/* capital U, grave accent */
  { "Uacute",	218 },	/* capital U, acute accent */
  { "Ucirc",	219 },	/* capital U, circumflex accent */
  { "Uuml",	220 },	/* capital U, dieresis or umlaut mark */
  { "Yacute",	221 },	/* capital Y, acute accent */
  { "THORN",	222 },	/* capital THORN, Icelandic */
  { "szlig",	223 },	/* small sharp s, German (sz ligature) */
  { "agrave",	224 },	/* small a, grave accent */
  { "aacute",	225 },	/* small a, acute accent */
  { "acirc",	226 },	/* small a, circumflex accent */
  { "atilde",	227 },	/* small a, tilde */
  { "auml",	228 },	/* small a, dieresis or umlaut mark */
  { "aring",	229 },	/* small a, ring */
  { "aelig",	230 },	/* small ae diphthong (ligature) */
  { "ccedil",	231 },	/* small c, cedilla */
  { "egrave",	232 },	/* small e, grave accent */
  { "eacute",	233 },	/* small e, acute accent */
  { "ecirc",	234 },	/* small e, circumflex accent */
  { "euml",	235 },	/* small e, dieresis or umlaut mark */
  { "igrave",	236 },	/* small i, grave accent */
  { "iacute",	237 },	/* small i, acute accent */
  { "icirc",	238 },	/* small i, circumflex accent */
  { "iuml",	239 },	/* small i, dieresis or umlaut mark */
  { "eth",	240 },	/* small eth, Icelandic */
  { "ntilde",	241 },	/* small n, tilde */
  { "ograve",	242 },	/* small o, grave accent */
  { "oacute",	243 },	/* small o, acute accent */
  { "ocirc",	244 },	/* small o, circumflex accent */
  { "otilde",	245 },	/* small o, tilde */
  { "ouml",	246 },	/* small o, dieresis or umlaut mark */
  { "divide",	247 },	/* divide sign */
  { "oslash",	248 },	/* small o, slash */
  { "ugrave",	249 },	/* small u, grave accent */
  { "uacute",	250 },	/* small u, acute accent */
  { "ucirc",	251 },	/* small u, circumflex accent */
  { "uuml",	252 },	/* small u, dieresis or umlaut mark */
  { "yacute",	253 },	/* small y, acute accent */
  { "thorn",	254 },	/* small thorn, Icelandic */
  { "yuml",	255 },	/* small y, dieresis or umlaut mark */

  { NULL, 0 }
};

/************************* HTML tag handlers */

/* Insert a blank line (next paragraph) */
g_error html_tag_p(struct html_parse *hp, struct html_tag_params *tag) {
  g_error e;
  e = text_insert_linebreak(hp->c);
  errorcheck;
  e = text_insert_string(hp->c," ",HFLAG_NFREE);
  e = text_insert_linebreak(hp->c);
  errorcheck;
  return sucess;
}

/************************* HTML tag table */

struct html_taghandler {
  const char *name;
  g_error (*handler)(struct html_parse *hp, struct html_tag_params *tag);
} html_tagtable[] = {
  
  { "p", &html_tag_p },

  { NULL, NULL }
};

/************************* Interface */

/* This function performs the highest level of HTML parsing- separating
 * HTML tags and normal text.
 */
g_error html_load(struct textbox_cursor *c, const u8 *data, u32 datalen) {
  const u8 *text_start = NULL;
  const u8 *tag_start = NULL;
  int in_tag_quote = 0;        /* In a quoted string within a tag? */
  struct html_parse hp;
  g_error e;

  /* Fill in parse structure */
  memset(&hp,0,sizeof(hp));
  hp.c = c;

  while (*data && datalen) {
    
    /* Are we in an HTML tag now? */
    if (tag_start) {
      if (*data == '"')
	in_tag_quote = !in_tag_quote;

      /* Ending the tag? */
      if ((!in_tag_quote) && *data=='>') {
	e = html_dispatch_tag(&hp,tag_start,data);
	errorcheck;
	tag_start = NULL;
      }
    }
    else {
      /* Normal text */

      /* Beginning an HTML tag? */
      if (*data == '<') {

 	/* Ending text? */
	if (text_start) {
	  e = html_dispatch_text(&hp,text_start,data-1);
	  errorcheck;
	  text_start = NULL;
	}

	tag_start = data;
      }
      else if (!text_start)
	text_start = data;
    }

    data++;
    datalen--;
  }

  /* Trailing text string */
  if (text_start) {
    e = html_dispatch_text(&hp,text_start,data-1);
    errorcheck;
  }

  return sucess;
}

/************************* Utilities */

g_error html_dispatch_tag(struct html_parse *hp,
			  const u8 *start, const u8 *end) {
  struct html_taghandler *p;

  /* Find the handler, prepare parameters, and call it */

  return sucess;
}

/* Chop up text into fragments, sending wordbreaks
 * between them when necessary */
g_error html_dispatch_text(struct html_parse *hp,
			   const u8 *start, const u8 *end) {
  const u8 *fragment;
  g_error e;

  for (fragment = NULL;start <= end;start++) {
    if (fragment) {
      /* Look for the end */

      if (isspace(*start)) {
	e = html_dispatch_textfragment(hp,fragment,start-1);
	errorcheck;
	fragment = NULL;
      }
      else {
	e = text_insert_wordbreak(hp->c);
	errorcheck;
      }
    }
    else {
      /* Look for the beginning */

      if (isspace(*start)) {
	e = text_insert_wordbreak(hp->c);
	errorcheck;
      }
      else
	fragment = start;
    }
  }
  if (fragment) {
    e = html_dispatch_textfragment(hp,fragment,start-1);
    errorcheck;
  }
  return sucess;
}

/* Convert characters like &nbsp; to characters, store and send the string
 * to the textbox.
 */
g_error html_dispatch_textfragment(struct html_parse *hp,
				   const u8 *start,const u8 *end) {
  int length;
  const u8 *p,*cname;
  char *str, *q;
  g_error e;

  /* Count the number of characters in the string, treating &foo; sequences
   * as 1 character.
   */
  for (p=start,length=0; p<=end; p++, length++)
    if (*p == '&')
      for (;p<=end && *p!=';';p++);
  
  /* Transcribe it into a new string, converting &foo; using html_findchar() 
   */
  e = g_malloc((void**)&str, length+1);
  errorcheck;
  str[length-1] = 0;
  for (p=start,q=str; p<=end && length; p++, q++, length--)
    if (*p == '&') {
      /* Character name */
      cname = p+1;
      for (;p<=end && *p!=';';p++);
      *q = html_findchar(cname,p-cname);
    }
    else
      /* Normal character */
      *q = *p;
  
  /* Send it to the textbox */
  return text_insert_string(hp->c,str,0);
}

char html_findchar(const u8 *charname, int namelen) {
  struct html_charname *p;

  /* If it's a numeric code, return it */
  if (charname[0]=='#')
    return atoi(charname+1);

  /* Search our table of character names */
  p = html_chartable;
  while (p->name) {
    if (!strncmp(p->name,charname,namelen))
      return p->ch;
    p++;
  }
    
  /* Still not found... */
  return '?';
}

/* The End */



