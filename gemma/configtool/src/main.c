/* The gemma library config program
 */

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

# include <string.h>
# include "../../include/gemma/gemma.h"

# include "gemma.h"

# ifndef ulong
#  define ulong unsigned long
# endif
# ifndef ushort
#  define ushort unsigned short
# endif
# ifndef uchar
#  define uchar unsigned char
# endif

static long
asc2bin(char *in)
{
	short x = 0;
	long number = 0;

	while ((in[x] >= '0') && (in[x] <= '9'))
	{
		number *= 10;
		number += (in[x] & 0x0f);
		x++;
	}

	return number;
}

static void
bin2asc(long number, char *out)
{
	char decout[10];
	ushort x, quot, remd;

	if (number)
	{
		if (number < 0)
		{
			*out++ = '-';
			number = ~number;
			number++;
		}

		for (x = 0; number && (x < 10);)
		{
			quot = (ushort)(number / 10);
			remd = (ushort)(number % 10);
			number = (ulong)quot;
			decout[x++] = remd | 0x30;
		}
		while(x > 0)
			*out++ = decout[--x];
	}
	else
		*out++ = '0';

	*out = 0;
}

static void
deselect(WINDIAL *wd, short obj)
{
	OBJECT *ob = wd->wb_treeptr;

	if ((!(ob[obj].ob_state & OS_WHITEBAK) && !(ob[obj].ob_flags & OF_RBUTTON)))
		objc_xchange(wd, obj, ob[obj].ob_state & ~OS_SELECTED, 1);
}

long
newkey(WINDIAL *wd, short vec, short ks, short kc)
{
	if ((kc & 0x007f) == 0x0011)
		windial_longjmp(wd, vec);
	return 0;
}

long
newmsg(WINDIAL *wd, short vec, short *msg)
{
	if (msg[0] == AP_TERM)
		windial_longjmp(wd, vec);

	return 0;
}

static SLB *kern;
static char *libbuf;
static long liblen;
static ushort libmode;
static char fullname[1024];

static long *
load_library(void)
{
	char *dir;
	long r;

	dir = env_get("SLBPATH=");
	if (!dir)
		dir = "./";
	strcpy(fullname, dir);
	strcat(fullname, "/gemma.slb");

	r = (kern->exec)(kern->handle, 517L, 4, (char *)fullname, &libbuf, &liblen, &libmode);

	if (r == 0)
	{
		uchar *buf = libbuf;
		long x = liblen;

		do
		{
# ifdef __M68020__
			if (*(ulong *)buf == 0xdeadefeb)
# else
			if ((buf[0] == 0xde) && (buf[1] == 0xad) && (buf[2] == 0xef) && (buf[3] == 0xeb))
# endif
				return (long *)buf;
			else
				buf++;
		} while (--x);
	}

	return 0;
}

static const short buttons[] =
{
	0, 0, CHECKPOINT, DRAWZOOM, MOVEBOXES,
 	COMPENSATION, FORMALERT, 0, DEBUGGING,
 	FSELECTSLB
};

# define NUMSET	10

static void
do_window(WINDIAL *wd)
{
	OBJECT *ob = wd->wb_treeptr;
	TEDINFO *ted;
	long settings[NUMSET], original[NUMSET], *prefs = load_library();
	short x;

	if (!prefs || (prefs[1] < 17))
	{
		windial_alert(1, (char *)CANNOTLOAD);
		return;
	}

	for (x = 0; x < NUMSET; x++)
	{
		short obj;

		settings[x] = prefs[x + 7];
		original[x] = settings[x];
		obj = buttons[x];
		if (obj)
		{
			if (settings[x])
				ob[obj].ob_state |= OS_SELECTED;
			else
				ob[obj].ob_state &= ~OS_SELECTED;
		}
	}

	ted = ob[BUTTONRELEASE].ob_spec.tedinfo;
	bin2asc(settings[0], ted->te_ptext);
	ted = ob[AVBUFRELEASE].ob_spec.tedinfo;
	bin2asc(settings[1], ted->te_ptext);

	windial_open(wd);

	if (windial_setjmp(wd, WD_MSGVEC, newmsg))
		return;
	if (windial_setjmp(wd, WD_KEYVEC, newkey))
		return;

	for (;;)
	{
		short m;

		m = windial_formdo(wd);

		if (m == -1)
			return;
		else
		{
			m &= 0x7fff;
			deselect(wd, m);
		}

		switch(m)
		{
			case	CHECKPOINT:
				settings[2] = (ob[m].ob_state & OS_SELECTED) ? 1 : 0;
				break;
			case	DRAWZOOM:
				settings[3] = (ob[m].ob_state & OS_SELECTED) ? 1 : 0;
				break;
			case	MOVEBOXES:
				settings[4] = (ob[m].ob_state & OS_SELECTED) ? 1 : 0;
				break;
			case	COMPENSATION:
				settings[5] = (ob[m].ob_state & OS_SELECTED) ? 1 : 0;
				break;
			case	FORMALERT:
				settings[6] = (ob[m].ob_state & OS_SELECTED) ? 1 : 0;
				break;
			case	DEBUGGING:
				settings[8] = (ob[m].ob_state & OS_SELECTED) ? 1 : 0;
				break;
			case	FSELECTSLB:
				settings[9] = (ob[m].ob_state & OS_SELECTED) ? 1 : 0;
				break;
			case	OK:
			{
				short users;
				long a;

				a = asc2bin(ob[BUTTONRELEASE].ob_spec.tedinfo->te_ptext);
				if (!a)
					a = 50;
				settings[0] = a;
				a = asc2bin(ob[AVBUFRELEASE].ob_spec.tedinfo->te_ptext);
				if (!a)
					a = 200;
				settings[1] = a;

				a = 0;
				for (x = 0; x < NUMSET; x++)
					a |= (settings[x] - original[x]);
				if (!a)
					return;

				for (x = 0; x < NUMSET; x++)
					prefs[x + 7] = settings[x];
				a = (kern->exec)(kern->handle, 518L, 4, fullname, libbuf, liblen, libmode);	/* save */
				if (a < 0)
				{
					windial_error(a, (char *)-1);
					return;
				}

				users = (gemma.exec)(gemma.handle, (long)43, (short)0);
				if (users > 1)
				{
					a = windial_alert(1, (char *)IMMEDIATELY);
					if (a == 1)
					{
						a = 0;
						for (x = 0; x < NUMSET; x++)
							a |= (gemma.exec)(gemma.handle, (long)34, (short)2, (short)x+0x4000, (long)settings[x]);
						if (a < 0)
							windial_error(a, (char *)-1);
					}
				}
			}
			case	CANCEL:
				return;
		}
	}
}

int
main(void)
{
	WINDIAL *wd;
	long r;

	r = appl_open("gemma.rsc", 0, (char *)PNAME);
	if (r < 0)
		return r;

	kern = (SLB *)(gemma.exec)(gemma.handle, 34L, 2, 0x0001, 0L);

	wd = (WINDIAL *)windial_create(0, WINDOW, 0, BUTTONRELEASE, (char *)WNAME);

	do_window(wd);

	windial_close(wd);
	windial_delete(wd);

	return appl_close();
}

/* EOF */
