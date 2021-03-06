/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Taught to send *real* morse by Lyndon Nerenberg (VE6BBM)
 * <lyndon@orthanc.ca>
 */

static const char copyright[] =
"@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";

#if 0
static char sccsid[] = "@(#)morse.c	8.1 (Berkeley) 5/31/93";
#endif
static const char rcsid[] =
 "$FreeBSD$";

#include <sys/time.h>
#include <sys/ioctl.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <langinfo.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#ifdef __FreeBSD__
/* Always use the speaker, let the open fail if -p is selected */
#define SPEAKER "/dev/speaker"
#endif

#define WHITESPACE " \t\n"
#define DELIMITERS " \t"

#ifdef SPEAKER
#include <dev/speaker/speaker.h>
#endif

struct morsetab {
	const char      inchar;
	const char     *morse;
};

static const struct morsetab mtab[] = {

	/* letters */

	{'a', ".-"},
	{'b', "-..."},
	{'c', "-.-."},
	{'d', "-.."},
	{'e', "."},
	{'f', "..-."},
	{'g', "--."},
	{'h', "...."},
	{'i', ".."},
	{'j', ".---"},
	{'k', "-.-"},
	{'l', ".-.."},
	{'m', "--"},
	{'n', "-."},
	{'o', "---"},
	{'p', ".--."},
	{'q', "--.-"},
	{'r', ".-."},
	{'s', "..."},
	{'t', "-"},
	{'u', "..-"},
	{'v', "...-"},
	{'w', ".--"},
	{'x', "-..-"},
	{'y', "-.--"},
	{'z', "--.."},

	/* digits */

	{'0', "-----"},
	{'1', ".----"},
	{'2', "..---"},
	{'3', "...--"},
	{'4', "....-"},
	{'5', "....."},
	{'6', "-...."},
	{'7', "--..."},
	{'8', "---.."},
	{'9', "----."},

	/* punctuation */

	{',', "--..--"},
	{'.', ".-.-.-"},
	{'"', ".-..-."},
	{'!', "..--."},
	{'?', "..--.."},
	{'/', "-..-."},
	{'-', "-....-"},
	{'=', "-...-"},		/* BT */
	{':', "---..."},
	{';', "-.-.-."},
	{'(', "-.--."},		/* KN */
	{')', "-.--.-"},
	{'$', "...-..-"},
	{'+', ".-.-."},		/* AR */
	{'@', ".--.-."},	/* AC */
	{'_', "..--.-"},
	{'\'', ".----."},

	/* prosigns without already assigned values */

	{'#', ".-..."},		/* AS */
	{'&', "...-.-"},	/* SK */
	{'*', "...-."},		/* VE */
	{'%', "-...-.-"},	/* BK */

	{'\0', ""}
};

/*
 * Code-points for some Latin1 chars in ISO-8859-1 encoding.
 * UTF-8 encoded chars in the comments.
 */
static const struct morsetab iso8859_1tab[] = {
	{'\340', ".--.-"},	/* ?? */
	{'\341', ".--.-"},	/* ?? */
	{'\342', ".--.-"},	/* ?? */
	{'\344', ".-.-"},	/* ?? */
	{'\347', "-.-.."},	/* ?? */
	{'\350', "..-.."},	/* ?? */
	{'\351', "..-.."},	/* ?? */
	{'\352', "-..-."},	/* ?? */
	{'\361', "--.--"},	/* ?? */
	{'\366', "---."},	/* ?? */
	{'\374', "..--"},	/* ?? */

	{'\0', ""}
};

/*
 * Code-points for some Greek chars in ISO-8859-7 encoding.
 * UTF-8 encoded chars in the comments.
 */
static const struct morsetab iso8859_7tab[] = {
	/*
	 * This table does not implement:
	 * - the special sequences for the seven diphthongs,
	 * - the punctuation differences.
	 * Implementing these features would introduce too many
	 * special-cases in the program's main loop.
	 * The diphthong sequences are:
	 * alpha iota		.-.-
	 * alpha upsilon	..--
	 * epsilon upsilon	---.
	 * eta upsilon		...-
	 * omicron iota		---..
	 * omicron upsilon	..-
	 * upsilon iota		.---
	 * The different punctuation symbols are:
	 * ;	..-.-
	 * !	--..--
	 */
	{'\341', ".-"},		/* ??, alpha */
	{'\334', ".-"},		/* ??, alpha with acute */
	{'\342', "-..."},	/* ??, beta */
	{'\343', "--."},	/* ??, gamma */
	{'\344', "-.."},	/* ??, delta */
	{'\345', "."},		/* ??, epsilon */
	{'\335', "."},		/* ??, epsilon with acute */
	{'\346', "--.."},	/* ??, zeta */
	{'\347', "...."},	/* ??, eta */
	{'\336', "...."},	/* ??, eta with acute */
	{'\350', "-.-."},	/* ??, theta */
	{'\351', ".."},		/* ??, iota */
	{'\337', ".."},		/* ??, iota with acute */
	{'\372', ".."},		/* ??, iota with diaeresis */
	{'\300', ".."},		/* ??, iota with acute and diaeresis */
	{'\352', "-.-"},	/* ??, kappa */
	{'\353', ".-.."},	/* ??, lambda */
	{'\354', "--"},		/* ??, mu */
	{'\355', "-."},		/* ??, nu */
	{'\356', "-..-"},	/* ??, xi */
	{'\357', "---"},	/* ??, omicron */
	{'\374', "---"},	/* ??, omicron with acute */
	{'\360', ".--."},	/* ??, pi */
	{'\361', ".-."},	/* ??, rho */
	{'\363', "..."},	/* ??, sigma */
	{'\362', "..."},	/* ??, final sigma */
	{'\364', "-"},		/* ??, tau */
	{'\365', "-.--"},	/* ??, upsilon */
	{'\375', "-.--"},	/* ??, upsilon with acute */
	{'\373', "-.--"},	/* ??, upsilon and diaeresis */
	{'\340', "-.--"},	/* ??, upsilon with acute and diaeresis */
	{'\366', "..-."},	/* ??, phi */
	{'\367', "----"},	/* ??, chi */
	{'\370', "--.-"},	/* ??, psi */
	{'\371', ".--"},	/* ??, omega */
	{'\376', ".--"},	/* ??, omega with acute */

	{'\0', ""}
};

/*
 * Code-points for the Cyrillic alphabet in KOI8-R encoding.
 * UTF-8 encoded chars in the comments.
 */
static const struct morsetab koi8rtab[] = {
	{'\301', ".-"},		/* ??, a */
	{'\302', "-..."},	/* ??, be */
	{'\327', ".--"},	/* ??, ve */
	{'\307', "--."},	/* ??, ge */
	{'\304', "-.."},	/* ??, de */
	{'\305', "."},		/* ??, ye */
	{'\243', "."},		/* ??, yo, the same as ye */
	{'\326', "...-"},	/* ??, she */
	{'\332', "--.."},	/* ??, ze */
	{'\311', ".."},		/* ??, i */
	{'\312', ".---"},	/* ??, i kratkoye */
	{'\313', "-.-"},	/* ??, ka */
	{'\314', ".-.."},	/* ??, el */
	{'\315', "--"},		/* ??, em */
	{'\316', "-."},		/* ??, en */
	{'\317', "---"},	/* ??, o */
	{'\320', ".--."},	/* ??, pe */
	{'\322', ".-."},	/* ??, er */
	{'\323', "..."},	/* ??, es */
	{'\324', "-"},		/* ??, te */
	{'\325', "..-"},	/* ??, u */
	{'\306', "..-."},	/* ??, ef */
	{'\310', "...."},	/* ??, kha */
	{'\303', "-.-."},	/* ??, ce */
	{'\336', "---."},	/* ??, che */
	{'\333', "----"},	/* ??, sha */
	{'\335', "--.-"},	/* ??, shcha */
	{'\331', "-.--"},	/* ??, yi */
	{'\330', "-..-"},	/* ??, myakhkij znak */
	{'\334', "..-.."},	/* ??, ae */
	{'\300', "..--"},	/* ??, yu */
	{'\321', ".-.-"},	/* ??, ya */

	{'\0', ""}
};

static void	show(const char *), play(const char *), morse(char);
static void	decode (char *), fdecode(FILE *);
static void	ttyout(const char *);
static void	sighandler(int);

static int	pflag, lflag, rflag, sflag, eflag;
static int	wpm = 20;	/* effective words per minute */
static int	cpm;		/* effective words per minute between
				 * characters */
#define FREQUENCY 600
static int	freq = FREQUENCY;
static char	*device;	/* for tty-controlled generator */

#define DASH_LEN 3
#define CHAR_SPACE 3
#define WORD_SPACE (7 - CHAR_SPACE - 1)
static float	dot_clock;
static float	cdot_clock;
static int	spkr, line;
static struct termios otty, ntty;
static int	olflags;

#ifdef SPEAKER
static tone_t	sound;
#define GETOPTOPTS "c:d:ef:lprsw:"
#define USAGE \
"usage: morse [-elprs] [-d device] [-w speed] [-c speed] [-f frequency] [string ...]\n"
#else
#define GETOPTOPTS "c:d:ef:lrsw:"
#define USAGE \
"usage: morse [-elrs] [-d device] [-w speed] [-c speed] [-f frequency] [string ...]\n"

#endif

static const struct morsetab *hightab;

int
main(int argc, char *argv[])
{
	int    ch, lflags;
	char  *p, *codeset;

	while ((ch = getopt(argc, argv, GETOPTOPTS)) != -1)
		switch ((char) ch) {
		case 'c':
			cpm = atoi(optarg);
			break;
		case 'd':
			device = optarg;
			break;
		case 'e':
			eflag = 1;
			setvbuf(stdout, 0, _IONBF, 0);
			break;
		case 'f':
			freq = atoi(optarg);
			break;
		case 'l':
			lflag = 1;
			break;
#ifdef SPEAKER
		case 'p':
			pflag = 1;
			break;
#endif
		case 'r':
			rflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'w':
			wpm = atoi(optarg);
			break;
		case '?':
		default:
			errx(1, USAGE);
		}
	if ((sflag && lflag) || (sflag && rflag) || (lflag && rflag)) {
		errx(1, "morse: only one of -l, -s, and -r allowed\n");
	}
	if ((pflag || device) && (sflag || lflag)) {
		errx(1, "morse: only one of -p, -d and -l, -s allowed\n");
	}
	if (cpm == 0) {
		cpm = wpm;
	}
	if ((pflag || device) && ((wpm < 1) || (wpm > 60) || (cpm < 1) || (cpm > 60))) {
		errx(1, "morse: insane speed\n");
	}
	if ((pflag || device) && (freq == 0)) {
		freq = FREQUENCY;
	}
#ifdef SPEAKER
	if (pflag) {
		if ((spkr = open(SPEAKER, O_WRONLY, 0)) == -1) {
			err(1, SPEAKER);
		}
	} else
#endif
	if (device) {
		if ((line = open(device, O_WRONLY | O_NONBLOCK)) == -1) {
			err(1, "open tty line");
		}
		if (tcgetattr(line, &otty) == -1) {
			err(1, "tcgetattr() failed");
		}
		ntty = otty;
		ntty.c_cflag |= CLOCAL;
		tcsetattr(line, TCSANOW, &ntty);
		lflags = fcntl(line, F_GETFL);
		lflags &= ~O_NONBLOCK;
		fcntl(line, F_SETFL, &lflags);
		ioctl(line, TIOCMGET, &lflags);
		lflags &= ~TIOCM_RTS;
		olflags = lflags;
		ioctl(line, TIOCMSET, &lflags);
		(void)signal(SIGHUP, sighandler);
		(void)signal(SIGINT, sighandler);
		(void)signal(SIGQUIT, sighandler);
		(void)signal(SIGTERM, sighandler);
	}
	if (pflag || device) {
		dot_clock = wpm / 2.4;		/* dots/sec */
		dot_clock = 1 / dot_clock;	/* duration of a dot */
		dot_clock = dot_clock / 2;	/* dot_clock runs at twice */
						/* the dot rate */
		dot_clock = dot_clock * 100;	/* scale for ioctl */

		cdot_clock = cpm / 2.4;		/* dots/sec */
		cdot_clock = 1 / cdot_clock;	/* duration of a dot */
		cdot_clock = cdot_clock / 2;	/* dot_clock runs at twice */
						/* the dot rate */
		cdot_clock = cdot_clock * 100;	/* scale for ioctl */
	}

	argc -= optind;
	argv += optind;

	if (setlocale(LC_CTYPE, "") != NULL &&
	    *(codeset = nl_langinfo(CODESET)) != '\0') {
		if (strcmp(codeset, "KOI8-R") == 0)
			hightab = koi8rtab;
		else if (strcmp(codeset, "ISO8859-1") == 0 ||
			 strcmp(codeset, "ISO8859-15") == 0)
			hightab = iso8859_1tab;
		else if (strcmp(codeset, "ISO8859-7") == 0)
			hightab = iso8859_7tab;
	}

	if (lflag) {
		printf("m");
	}
	if (rflag) {
		if (*argv) {
			do {
				p = strtok(*argv, DELIMITERS);
				if (p == NULL) {
					decode(*argv);
				}
				else {
					while (p) {
						decode(p);
						p = strtok(NULL, DELIMITERS);
					}
				}
			} while (*++argv);
			putchar('\n');
		} else {
			fdecode(stdin);
		}
	}
	else if (*argv) {
		do {
			for (p = *argv; *p; ++p) {
				if (eflag)
					putchar(*p);
				morse(*p);
			}
			if (eflag)
				putchar(' ');
			morse(' ');
		} while (*++argv);
	} else {
		while ((ch = getchar()) != EOF) {
			if (eflag)
				putchar(ch);
			morse(ch);
		}
	}
	if (device)
		tcsetattr(line, TCSANOW, &otty);
	exit(0);
}

static void
morse(char c)
{
	const struct morsetab *m;

	if (isalpha((unsigned char)c))
		c = tolower((unsigned char)c);
	if ((c == '\r') || (c == '\n'))
		c = ' ';
	if (c == ' ') {
		if (pflag)
			play(" ");
		else if (device)
			ttyout(" ");
		else if (lflag)
			printf("\n");
		else
			show("");
		return;
	}
	for (m = ((unsigned char)c < 0x80? mtab: hightab);
	     m != NULL && m->inchar != '\0';
	     m++) {
		if (m->inchar == c) {
			if (pflag) {
				play(m->morse);
			} else if (device) {
				ttyout(m->morse);
			} else
				show(m->morse);
		}
	}
}

static void
show(const char *s)
{
	if (lflag) {
		printf("%s ", s);
	} else if (sflag) {
		printf(" %s\n", s);
	} else {
		for (; *s; ++s)
			printf(" %s", *s == '.' ? *(s + 1) == '\0' ? "dit" :
			    "di" : "dah");
		printf("\n");
	}
}

static void
play(const char *s)
{
#ifdef SPEAKER
	const char *c;

	for (c = s; *c != '\0'; c++) {
		switch (*c) {
		case '.':
			sound.frequency = freq;
			sound.duration = dot_clock;
			break;
		case '-':
			sound.frequency = freq;
			sound.duration = dot_clock * DASH_LEN;
			break;
		case ' ':
			sound.frequency = 0;
			sound.duration = cdot_clock * WORD_SPACE;
			break;
		default:
			sound.duration = 0;
		}
		if (sound.duration) {
			if (ioctl(spkr, SPKRTONE, &sound) == -1) {
				err(1, "ioctl play");
			}
		}
		sound.frequency = 0;
		sound.duration = dot_clock;
		if (ioctl(spkr, SPKRTONE, &sound) == -1) {
			err(1, "ioctl rest");
		}
	}
	sound.frequency = 0;
	sound.duration = cdot_clock * CHAR_SPACE;
	ioctl(spkr, SPKRTONE, &sound);
#endif
}

static void
ttyout(const char *s)
{
	const char *c;
	int duration, on, lflags;

	for (c = s; *c != '\0'; c++) {
		switch (*c) {
		case '.':
			on = 1;
			duration = dot_clock;
			break;
		case '-':
			on = 1;
			duration = dot_clock * DASH_LEN;
			break;
		case ' ':
			on = 0;
			duration = cdot_clock * WORD_SPACE;
			break;
		default:
			on = 0;
			duration = 0;
		}
		if (on) {
			ioctl(line, TIOCMGET, &lflags);
			lflags |= TIOCM_RTS;
			ioctl(line, TIOCMSET, &lflags);
		}
		duration *= 10000;
		if (duration)
			usleep(duration);
		ioctl(line, TIOCMGET, &lflags);
		lflags &= ~TIOCM_RTS;
		ioctl(line, TIOCMSET, &lflags);
		duration = dot_clock * 10000;
		usleep(duration);
	}
	duration = cdot_clock * CHAR_SPACE * 10000;
	usleep(duration);
}

void
fdecode(FILE *stream)
{
	char *n, *p, *s;
	char buf[BUFSIZ];

	s = buf;
	while (fgets(s, BUFSIZ - (s - buf), stream)) {
		p = buf;

		while (*p && isblank(*p)) {
			p++;
		}
		while (*p && isspace(*p)) {
			p++;
			putchar (' ');
		}
		while (*p) {
			n = strpbrk(p, WHITESPACE);
			if (n == NULL) {
				/* The token was interrupted at the end
				 * of the buffer. Shift it to the begin
				 * of the buffer.
				 */
				for (s = buf; *p; *s++ = *p++)
					;
			} else {
				*n = '\0';
				n++;
				decode(p);
				p = n;
			}
		}
	}
	putchar('\n');
}

void
decode(char *p)
{
	char c;
	const struct morsetab *m;

	c = ' ';
	for (m = mtab; m != NULL && m->inchar != '\0'; m++) {
		if (strcmp(m->morse, p) == 0) {
			c = m->inchar;
			break;
		}
	}

	if (c == ' ')
		for (m = hightab; m != NULL && m->inchar != '\0'; m++) {
			if (strcmp(m->morse, p) == 0) {
				c = m->inchar;
				break;
			}
		}

	putchar(c);
}

static void
sighandler(int signo)
{

	ioctl(line, TIOCMSET, &olflags);
	tcsetattr(line, TCSANOW, &otty);

	signal(signo, SIG_DFL);
	(void)kill(getpid(), signo);
}
