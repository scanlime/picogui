/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/outbound.h"
#include "../common/util.h"
#include "../common/fe.h"

#include <picogui.h>
#include "fe-picogui.h"


static GSList *tmr_list;		  /* timer list */
static int tmr_list_count;
static GSList *se_list;			  /* socket event list */
static int se_list_count;
static pghandle pgEmptyString;

static void
send_command (char *cmd)
{
	handle_command (cmd, sess_list->data, TRUE, FALSE);
}

static int
fieldActivate(struct pgEvent *evt)
{
	/* TODO: check if buffer is only read */
	char *cmd;
	pghandle handle;

	handle=pgGetWidget(evt->from, PG_WP_TEXT);
	if(handle)
	{
		cmd=strdup(pgGetString(pgGetWidget(evt->from, PG_WP_TEXT)));
		send_command(cmd);
		free(cmd);
		pgSetWidget(evt->from, PG_WP_TEXT, pgEmptyString, 0);
	}
	return 0;
}

static int done_intro = 0;

void
fe_new_window (struct session *sess)
{
	char buf[512];
	pghandle scroll;
	pghandle top_bar,right_bar,bottom_bar;

	/* App */
	sess->gui = malloc (sizeof(struct fe_pg_gui));
	((struct fe_pg_gui *)sess->gui)->app =
		pgRegisterApp(PG_APP_NORMAL, "PicoGUI X-Chat", 0);

	/* Toolbars */
	top_bar = pgNewWidget(PG_WIDGET_TOOLBAR,PGDEFAULT,PGDEFAULT);
	pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_TOP,0);
	bottom_bar = pgNewWidget(PG_WIDGET_TOOLBAR,PGDEFAULT,PGDEFAULT);
	pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_BOTTOM,0);
	right_bar = pgNewWidget(PG_WIDGET_TOOLBAR,PGDEFAULT,PGDEFAULT);
	pgSetWidget(PGDEFAULT,PG_WP_SIDE,PG_S_RIGHT,0);

	/* Chat area */
	((struct fe_pg_gui *)sess->gui)->terminal =
	  pgNewWidget(PG_WIDGET_TERMINAL, PGDEFAULT, PGDEFAULT);

	/* Don't let the terminal widget get focus */
	pgSetWidget(PGDEFAULT,
		    PG_WP_TRIGGERMASK, 
		    pgGetWidget(PGDEFAULT, PG_WP_TRIGGERMASK) & ~PG_TRIGGER_DOWN,
		    0);
	
	/* Scroll bar */
	scroll=pgNewWidget(PG_WIDGET_SCROLL, PG_DERIVE_BEFORE, PGDEFAULT);
	pgSetWidget(scroll, PG_WP_BIND,
		    ((struct fe_pg_gui *)sess->gui)->terminal, 0);

	/* Inside the bottom toolbar */
	((struct fe_pg_gui *)sess->gui)->nick =
	  pgNewWidget(PG_WIDGET_LABEL, PG_DERIVE_INSIDE, bottom_bar);
	pgSetWidget(PGDEFAULT, PG_WP_SIDE, PG_S_LEFT, 0);

	pgNewWidget(PG_WIDGET_FIELD, PGDEFAULT, PGDEFAULT);
	pgSetWidget(PGDEFAULT, PG_WP_SIDE, PG_S_ALL, 0);
	pgBind(PGDEFAULT, PG_WE_ACTIVATE, fieldActivate, NULL);
	pgFocus(PGDEFAULT);

	if (!sess->server->front_session)
		sess->server->front_session = sess;
	if (!current_tab)
		current_tab = sess;

	if (done_intro)
		return;
	done_intro = 1;

	snprintf (buf, sizeof (buf),	
				"\n"
				" \017xchat \00310"VERSION"\n"
				" \017Running on \00310%s \017glib \00310%d.%d.%d\n"
				" \017This binary compiled \00310"__DATE__"\017\n",
				get_cpu_str(), 
				glib_major_version, glib_minor_version, glib_micro_version);
	fe_print_text (sess, buf);

	strcpy (buf, "\n\nCompiled in Features\0032:\017 ");
#ifdef USE_PERL
	strcat (buf, "Perl ");
#endif
#ifdef USE_PYTHON
	strcat (buf, "Python ");
#endif
#ifdef USE_PLUGIN
	strcat (buf, "Plugin ");
#endif
#ifdef ENABLE_NLS
	strcat (buf, "NLS ");
#endif
#ifdef USE_TRANS
	strcat (buf, "Trans ");
#endif
#ifdef USE_HEBREW
	strcat (buf, "Hebrew ");
#endif
#ifdef USE_OPENSSL
	strcat (buf, "OpenSSL ");
#endif
#ifdef SOCKS
	strcat (buf, "Socks5 ");
#endif
#ifdef HAVE_ICONV
	strcat (buf, "JCode ");
#endif
#ifdef USE_IPV6
	strcat (buf, "IPv6 ");
#endif
	strcat (buf, "\n\n");
	fe_print_text (sess, buf);
}

static int
get_stamp_str (time_t tim, char *dest, int size)
{
	return strftime (dest, size, prefs.stamp_format, localtime (&tim));
}

static int
timecat (char *buf)
{
	char stampbuf[64];

	get_stamp_str (time (0), stampbuf, sizeof (stampbuf));
	strcat (buf, stampbuf);
	return strlen (stampbuf);
}

/*                       0  1  2  3  4  5  6  7   8   9   10 11  12  13  14 15 */
static int colconv[] = { 0, 7, 4, 2, 1, 3, 5, 11, 13, 12, 6, 16, 14, 15, 10, 7 };

void
fe_print_text (struct session *sess, char *text)
{
	int dotime = FALSE;
	char num[8];
	int reverse = 0, under = 0, bold = 0,
		comma, k, i = 0, j = 0, len = strlen (text);
	unsigned char *newtext = malloc (len + 1024);

	if (prefs.timestamp)
	{
		newtext[0] = 0;
		j += timecat (newtext);
	}
	while (i < len)
	{
		if (dotime && text[i] != 0)
		{
			dotime = FALSE;
			newtext[j] = 0;
			j += timecat (newtext);
		}
		switch (text[i])
		{
		case 3:
			i++;
			if (!isdigit (text[i]))
			{
				newtext[j] = 27;
				j++;
				newtext[j] = '[';
				j++;
				newtext[j] = 'm';
				j++;
				i--;
				goto jump2;
			}
			k = 0;
			comma = FALSE;
			while (i < len)
			{
				if (text[i] >= '0' && text[i] <= '9' && k < 2)
				{
					num[k] = text[i];
					k++;
				} else
				{
					int col, mirc;
					num[k] = 0;
					newtext[j] = 27;
					j++;
					newtext[j] = '[';
					j++;
					if (k == 0)
					{
						newtext[j] = 'm';
						j++;
					} else
					{
						if (comma)
							col = 40;
						else
							col = 30;
						mirc = atoi (num);
						mirc = colconv[mirc];
						if (mirc > 9)
						{
							mirc += 50;
							sprintf ((char *) &newtext[j], "%dm", mirc + col);
						} else
						{
							sprintf ((char *) &newtext[j], "%dm", mirc + col);
						}
						j = strlen (newtext);
					}
					switch (text[i])
					{
					case ',':
						comma = TRUE;
						break;
					default:
						goto jump;
					}
					k = 0;
				}
				i++;
			}
			break;
		case '\026':				  /* REVERSE */
			if (reverse)
			{
				reverse = FALSE;
				strcpy (&newtext[j], "\033[27m");
			} else
			{
				reverse = TRUE;
				strcpy (&newtext[j], "\033[7m");
			}
			j = strlen (newtext);
			break;
		case '\037':				  /* underline */
			if (under)
			{
				under = FALSE;
				strcpy (&newtext[j], "\033[24m");
			} else
			{
				under = TRUE;
				strcpy (&newtext[j], "\033[4m");
			}
			j = strlen (newtext);
			break;
		case '\002':				  /* bold */
			if (bold)
			{
				bold = FALSE;
				strcpy (&newtext[j], "\033[22m");
			} else
			{
				bold = TRUE;
				strcpy (&newtext[j], "\033[1m");
			}
			j = strlen (newtext);
			break;
		case '\007':
			if (!prefs.filterbeep)
			{
				newtext[j] = text[i];
				j++;
			}
			break;
		case '\017':				  /* reset all */
			strcpy (&newtext[j], "\033[m");
			j += 3;
			reverse = FALSE;
			bold = FALSE;
			under = FALSE;
			break;
		case '\t':
			newtext[j] = ' ';
			j++;
			break;
		case '\n':
			newtext[j] = '\r';
			j++;
			if (prefs.timestamp)
				dotime = TRUE;
		default:
			newtext[j] = text[i];
			j++;
		}
	 jump2:
		i++;
	 jump:
		i += 0;
	}
	newtext[j] = 0;

	pgWriteData(((struct fe_pg_gui *)sess->gui)->terminal,
		    pgFromTempMemory(newtext,j));
}

void
fe_timeout_remove (int tag)
{
	timerevent *te;
	GSList *list;

	list = tmr_list;
	while (list)
	{
		te = (timerevent *) list->data;
		if (te->tag == tag)
		{
			tmr_list = g_slist_remove (tmr_list, te);
			free (te);
			return;
		}
		list = list->next;
	}
}

int
fe_timeout_add (int interval, void *callback, void *userdata)
{
	struct timeval now;
	timerevent *te = malloc (sizeof (timerevent));

	tmr_list_count++;  /* this overflows at 2.2Billion, who cares!! */

	te->tag = tmr_list_count;
	te->interval = interval;
	te->callback = callback;
	te->userdata = userdata;

	gettimeofday (&now, NULL);
	te->next_call = now.tv_sec * 1000 + (now.tv_usec / 1000) + te->interval;

	tmr_list = g_slist_prepend (tmr_list, te);

	return te->tag;
}

void
fe_input_remove (int tag)
{
	socketevent *se;
	GSList *list;

	list = se_list;
	while (list)
	{
		se = (socketevent *) list->data;
		if (se->tag == tag)
		{
			se_list = g_slist_remove (se_list, se);
			free (se);
			return;
		}
		list = list->next;
	}
}

int
fe_input_add (int sok, int rread, int wwrite, int eexcept, void *func,
				  void *data)
{
	socketevent *se = malloc (sizeof (socketevent));

	se_list_count++;  /* this overflows at 2.2Billion, who cares!! */

	se->tag = se_list_count;
	se->sok = sok;
	se->rread = rread;
	se->wwrite = wwrite;
	se->eexcept = eexcept;
	se->callback = func;
	se->userdata = data;
	se_list = g_slist_prepend (se_list, se);

	return se->tag;
}

int
fe_args (int argc, char *argv[])
{
	pgInit(argc, argv);
	pgEmptyString=pgNewString("");
	if (argc > 1)
	{
		if (!strcasecmp (argv[1], "--version") || !strcasecmp (argv[1], "-v"))
		{
			puts (VERSION);
			return 0;
		}
	}
	return 1;
}

void
fe_init (void)
{
	se_list = 0;
	se_list_count = 0;
	tmr_list = 0;
	tmr_list_count = 0;
	prefs.autosave = 0;
	prefs.use_server_tab = 0;
	prefs.autodialog = 0;
	prefs.lagometer = 0;
	prefs.skipserverlist = 1;
}

static int
selectwrapper (int nfds, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout);
static void
afterselect(int result, fd_set *rfds);

void
fe_main (void)
{
#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);
#endif
	pgCustomizeSelect(selectwrapper, afterselect);
	pgEventLoop();
}

static fd_set *fe_pg_rfds, *fe_pg_wfds, *fe_pg_efds;

static
int selectwrapper (int n, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout)
{
	static fd_set rfds, wfds, efds;
	struct timeval now;
	socketevent *se;
	timerevent *te;
	GSList *list;
	guint64 shortest, delay;

	if(readfds)
		fe_pg_rfds=readfds;
	else
	{
		fe_pg_rfds=&rfds;
		FD_ZERO (&rfds);
	}
	if(writefds)
		fe_pg_wfds=writefds;
	else
	{
		fe_pg_wfds=&wfds;
		FD_ZERO (&wfds);
	}
	if(exceptfds)
		fe_pg_efds=exceptfds;
	else
	{
		fe_pg_efds=&efds;
		FD_ZERO (&efds);
	}

	list = se_list;
	while (list)
	{
		se = (socketevent *) list->data;
		if (se->rread)
		{
			FD_SET (se->sok, fe_pg_rfds);
			if(n<=se->sok)
				n=se->sok+1;
		}
		if (se->wwrite)
		{
			FD_SET (se->sok, fe_pg_wfds);
			if(n<=se->sok)
				n=se->sok+1;
		}
		if (se->eexcept)
		{
			FD_SET (se->sok, fe_pg_efds);
			if(n<=se->sok)
				n=se->sok+1;
		}
		list = list->next;
	}

	/* find the shortest timeout event */
	shortest = 0;
	list = tmr_list;
	while (list)
	{
		te = (timerevent *) list->data;
		if (te->next_call < shortest || shortest == 0)
			shortest = te->next_call;
		list = list->next;
	}
	if(shortest)	/* FIXME this timeout format sucks! */
	{
		gettimeofday (&now, NULL);
		delay = shortest - ((now.tv_sec * 1000) + (now.tv_usec / 1000));
		now.tv_sec = delay / 1000;
		now.tv_usec = (delay % 1000) * 1000;
		if(timeout)
		{
			if(now.tv_sec<timeout->tv_sec ||
					(now.tv_sec==timeout->tv_sec &&
					now.tv_usec<timeout->tv_usec))
				*timeout=now;
		}
		else
			timeout=&now;
	}

	return select (n, fe_pg_rfds, fe_pg_wfds, fe_pg_efds, timeout);
}

static void
afterselect(int result, fd_set *rfds)
{
	GSList *list;
	socketevent *se;
	timerevent *te;
	struct timeval now;

	/* these should already be the same */
	fe_pg_rfds=rfds;

	/* set all checked flags to false */
	list = se_list;
	while (list)
	{
		se = (socketevent *) list->data;
		se->checked = 0;
		list = list->next;
	}

	/* check all the socket callbacks */
	list = se_list;
	while (list)
	{
		se = (socketevent *) list->data;
		se->checked = 1;
		if (se->rread && FD_ISSET (se->sok, fe_pg_rfds))
		{
			se->callback (NULL, 0, se->userdata);
		} else if (se->wwrite && FD_ISSET (se->sok, fe_pg_wfds))
		{
			se->callback (NULL, 0, se->userdata);
		} else if (se->eexcept && FD_ISSET (se->sok, fe_pg_efds))
		{
			se->callback (NULL, 0, se->userdata);
		}
		list = se_list;
		if (list)
		{
			se = (socketevent *) list->data;
			while (se->checked)
			{
				list = list->next;
				if (!list)
					break;
				se = (socketevent *) list->data;
			}
		}
	}

	/* now check our list of timeout events, some might need to be called! */
	gettimeofday (&now, NULL);
	list = tmr_list;
	while (list)
	{
		te = (timerevent *) list->data;
		list = list->next;
		if (now.tv_sec * 1000 + (now.tv_usec / 1000) >= te->next_call)
		{
			/* if the callback returns 0, it must be removed */
			if (te->callback (te->userdata) == 0)
			{
				fe_timeout_remove (te->tag);
			} else
			{
				te->next_call = now.tv_sec * 1000 + (now.tv_usec / 1000) + te->interval;
			}
		}
	}
}

void
fe_exit (void)
{
	pgExitEventLoop();
}

void
fe_new_server (struct server *serv)
{
	serv->gui = malloc (4);
}

void
fe_message (char *msg, int wait)
{
	puts (msg);
}

void
fe_close_window (struct session *sess)
{
	kill_session_callback (sess);
	pgExitEventLoop();
}

void
fe_beep (void)
{
	pgDriverMessage(PGDM_SOUNDFX, PG_SND_BEEP);
}

void
fe_add_rawlog (struct server *serv, char *text, int outbound)
{
}
void
fe_set_topic (struct session *sess, char *topic)
{
}
void
fe_cleanup (void)
{
}
void
fe_set_hilight (struct session *sess)
{
}
void
fe_update_mode_buttons (struct session *sess, char mode, char sign)
{
}
void
fe_update_channel_key (struct session *sess)
{
}
void
fe_update_channel_limit (struct session *sess)
{
}
int
fe_is_chanwindow (struct server *serv)
{
	return 0;
}

void
fe_add_chan_list (struct server *serv, char *chan, char *users, char *topic)
{
}
void
fe_chan_list_end (struct server *serv)
{
}
int
fe_is_banwindow (struct session *sess)
{
	return 0;
}
void
fe_add_ban_list (struct session *sess, char *chan, char *users, char *topic)
{
}               
void
fe_ban_list_end (struct session *sess)
{
}
void
fe_notify_update (char *name)
{
}
void
fe_text_clear (struct session *sess)
{
}
void
fe_progressbar_start (struct session *sess)
{
}
void
fe_progressbar_end (struct session *sess)
{
}
void
fe_userlist_insert (struct session *sess, struct User *newuser, int row)
{
}
void
fe_userlist_remove (struct session *sess, struct User *user)
{
}
void
fe_userlist_move (struct session *sess, struct User *user, int new_row)
{
}
void
fe_userlist_numbers (struct session *sess)
{
}
void
fe_userlist_clear (struct session *sess)
{
}
void
fe_dcc_update_recv_win (void)
{
}
void
fe_dcc_update_send_win (void)
{
}
void
fe_dcc_update_chat_win (void)
{
}
void
fe_dcc_update_send (struct DCC *dcc)
{
}
void
fe_dcc_update_recv (struct DCC *dcc)
{
}
void
fe_clear_channel (struct session *sess)
{
}
void
fe_session_callback (struct session *sess)
{
}
void
fe_server_callback (struct server *serv)
{
}
void
fe_checkurl (char *text)
{
}
void
fe_pluginlist_update (void)
{
}
void
fe_buttons_update (struct session *sess)
{
}
void
fe_dlgbuttons_update (struct session *sess)
{
}
void
fe_dcc_send_filereq (struct session *sess, char *nick)
{
}
void
fe_set_channel (struct session *sess)
{
}
void
fe_set_title (struct session *sess)
{
}
void
fe_set_nonchannel (struct session *sess, int state)
{
}
void
fe_set_nick (struct server *serv, char *newnick)
{
}
void
fe_change_nick (struct server *serv, char *nick, char *newnick)
{
}
void
fe_ignore_update (int level)
{
}
void
fe_dcc_open_recv_win (int passive)
{
}
void
fe_dcc_open_send_win (int passive)
{
}
void
fe_dcc_open_chat_win (int passive)
{
}
int
fe_is_confmode (struct session *sess)
{
	return 0;
}
int
fe_is_beep (struct session *sess)
{
	return 0;
}

void
fe_userlist_hide (session * sess)
{
}
void
fe_lastlog (session * sess, session * lastlog_sess, char *sstr)
{
}
void
fe_set_lag (server * serv, int lag)
{
}
void
fe_set_throttle (server * serv)
{
}
void
fe_set_away (server *serv)
{
}
int
fe_open_serverlist (session *sess, int auto_connect, int dont_show)
{
	return FALSE;
}
