/* FIXME: handle ; delimited commands? */
#include <string.h>
#include <ctype.h>

#include "common/rom.h"
#include "common/readline.h"
#include "common/strbuf.h"
#include "common/alloc.h"

/* raw line input functions */
static char *raw_read(void);
static int raw_putc(char c);
static void raw_backspace(void);
static int raw_arrow(int which);
static int raw_cursor(int off, int whence);
#define CURSOR_SET	0
#define CURSOR_CUR	1
#define CURSOR_END	2

/* a member of the command history */
struct histent {
	int h_num;
	char *h_line;
	struct histent *h_next;
	struct histent *h_prev;
};

/* how long is the command history?  must be > 0 */
#define HISTSZ	128
static struct histent history[HISTSZ];
static struct histent *histn = &history[0];
static struct histent *hist0 = &history[0];
static int histnum;

static void readline_init(void);
static int saveline(char *line);
static char *histlookup_num(int num);
static char *histlookup_str(char *str);
static int readline_init_done;

/* global raw input buffer */
static int rawpos = 0;
static int overwrite = 0;
static struct strbuf *rawbuf = NULL;
static struct strbuf _rawbuf;
static int ups = 0;
static char *prevs[HISTSZ];

#ifdef INROM	
# define PUTC(c)	putc(c)
# define GETC()		getc()
#else
# define PUTC(c)	putc(c, stdout)
# define GETC()		getc(stdin)
#endif

#define ESC		'\033'
#define ESC2		'['
#define ARROW_UP	'A'
#define ARROW_DOWN	'B'
#define ARROW_RIGHT	'C'
#define ARROW_LEFT	'D'
#define HOME		'1'
#define INSERT		'2'
#define END		'4'
#define CTRL_PAD	'~'
#define DEL		'\177'
#define CTRL(c)		(1 + ((c) - 'A'))
#define beep()		PUTC('\a')

/* 
 * read a raw command - no history expansion yet 
 * make sure not to free the memory returned by this!
 */
static char *
raw_read(void)
{
	int c;
	int done = 0;

	/* init */
	if (!rawbuf) {
		rawbuf = &_rawbuf;
	} else if (rawbuf->buf) {
		free(rawbuf->buf);
	}
	strbuf_init(rawbuf, NULL, STRBUF_GROW);
	rawpos = 0;
	memset(prevs, 0, sizeof(prevs));
	ups = 0;

	while (!done) {
		c = GETC();

		switch(c) {
		  case '\r':
		  case '\n':
		  	/* return or enter ends line */
			done = 1;
			break;

		  case '\b':
		  case DEL:
		  	/* BS and DEL go back one char */
			raw_backspace();
			break;

		  case '\t':
			/* command completion */
			//FIXME:
			break;

		  case ESC: {
			/* ESC codes we can handle */
			int c2; 
			c2 = GETC();
			if (c2 == ESC2) {
				c2 = GETC();
				switch (c2) {
				  case ARROW_UP: 
				  case ARROW_DOWN: 
				  case ARROW_RIGHT:
				  case ARROW_LEFT:
					raw_arrow(c2);
					break;
				  case INSERT:
					if (GETC() == CTRL_PAD) {
						overwrite ^= 1;
					} else {
						beep();
					}
					break;
				  case HOME:
					if (GETC() == CTRL_PAD) {
						raw_cursor(0, CURSOR_SET);
					} else {
						beep();
					}
					break;
				  case END:
					if (GETC() == CTRL_PAD) {
						raw_cursor(0, CURSOR_END);
					} else {
						beep();
					}
					break;
				  default:
					beep();
				}
			} else if (c2 == 'Z' && GETC() == 'Z') {
				/* for fun */
				raw_putc('\n');
				done = 1;
			} else {
				beep();
			}
			break;
		  }

		  case CTRL('A'):
			/* go to BOL */
			raw_cursor(0, CURSOR_SET);
			break;
		
		  case CTRL('E'):
			/* go to EOL */
			raw_cursor(0, CURSOR_END);
			break;
		
		  case CTRL('U'):
		  case CTRL('W'):
			/* clear to BOL */
			while (rawpos) {
				raw_backspace();
			}
			break;

		  case CTRL('K'): {
		  	/* clear to EOL */
		  	int i = rawpos;
			raw_cursor(0, CURSOR_END);
			while (rawpos > i) {
				raw_backspace();
			}
			break;
		  }

		  case CTRL('D'):
			/* delete char under cursor */
			if (rawpos < rawbuf->len) {
				raw_arrow(ARROW_RIGHT);
				raw_backspace();
			}
			break;

		  case CTRL('P'):
			/* same as up arrow */
			raw_arrow(ARROW_UP);
			break;

		  case CTRL('N'):
			/* same as down arrow */
			raw_arrow(ARROW_DOWN);
			break;

		  case CTRL('B'):
			/* same as left arrow */
			raw_arrow(ARROW_LEFT);
			break;

		  case CTRL('F'):
			/* same as right arrow */
			raw_arrow(ARROW_RIGHT);
			break;

		  case CTRL('C'):
			/* cancel this input buffer */
			free(rawbuf->buf);
			rawbuf->buf = NULL;
			done = 1;
			break;

		  case CTRL('L'):
		  	/* clears vt100 screens */
			putc(c); /* this does not go into the buffer */
			/* FIXME: redraw the line */
			break;

		  default: {
			/* print it */
			int i = rawpos;
			if (overwrite && rawbuf->buf) {
				/* overwrite existing character */
				strbuf_deletec(rawbuf, rawpos);
			}
			/* print and slide the rest right */
			strbuf_insertc(rawbuf, rawpos, c);
			raw_cursor(0, CURSOR_END);
			raw_cursor(i+1, CURSOR_SET);
		  }
		}
	}

	/* we're done - next line */
	raw_putc('\n');
	
	/* cleanup */
	c = 0;
	while (prevs[c] && c < HISTSZ) {
		if (prevs[c] != rawbuf->buf) {
			free(prevs[c]);
		}
		c++;
	}

	if (!rawbuf->buf) {
		rawbuf->buf = strdup("");
	}

	return rawbuf->buf;
}

static int
raw_cursor(int off, int whence)
{
	int newpos;

	if (whence == CURSOR_SET) {
		newpos = off;
	} else if (whence == CURSOR_CUR) {
		newpos = rawpos + off;
	} else if (whence == CURSOR_END) {
		newpos = rawbuf->len - off;
	} else {
		return -1;
	}

	if (newpos < 0 || newpos > rawbuf->len) {
		return -1;
	}

	while (newpos > rawpos) {
		raw_putc(rawbuf->buf[rawpos]);
	}

	while (newpos < rawpos) {
		raw_putc('\b');
	}
		
	return rawpos;
}

static int
raw_putc(char c)
{
	PUTC(c);

	if (c == '\b') {
		rawpos--;
	} else if (c == '\n' || c== '\r') {
		rawpos = 0;
	} else {
		rawpos++;
	}

	return c;
}

static void
raw_backspace(void)
{
	if (rawpos) {
		int i = rawpos-1;

		raw_cursor(-1, CURSOR_CUR);
		strbuf_deletec(rawbuf, rawpos);

		if (rawpos == rawbuf->len) {
			raw_putc(' ');
			raw_putc('\b');
		} else {
			raw_cursor(0, CURSOR_END);
			raw_putc(' ');
			raw_putc('\b');
			raw_cursor(i, CURSOR_SET);
		}
	} else {
		beep();
	}
}

static int
raw_arrow(int which)
{
	if (which == ARROW_UP) {
		char *p;
	
		p = histlookup_num(histnum-(ups+1));
		if (!p) {
			beep();
			return 0;
		}

		/* save current buffer */
		prevs[ups] = rawbuf->buf;
		ups++;
		if (prevs[ups]) {
			p = prevs[ups];
		} else {
			p = strdup(p);
		}
	
		/* clear the old */
		raw_cursor(0, CURSOR_SET);
		while (rawpos <= rawbuf->len) {
			raw_putc(' ');
		}
		raw_cursor(0, CURSOR_SET);

		/* reveal the new */
		rawbuf->buf = p;
		rawbuf->len = strlen(p);
		raw_cursor(0, CURSOR_END);
	} else if (which == ARROW_DOWN) {
		if (!ups) {
			beep();
			return 0;
		}

		/* save current buffer */
		prevs[ups] = rawbuf->buf;
		ups--;

		/* clear the old */
		raw_cursor(0, CURSOR_SET);
		while (rawpos <= rawbuf->len) {
			raw_putc(' ');
		}
		raw_cursor(0, CURSOR_SET);

		/* reveal the new */
		rawbuf->buf = prevs[ups];
		rawbuf->len = prevs[ups] ?
			strlen(prevs[ups]) : 0;
		raw_cursor(0, CURSOR_END);
	} else if (which == ARROW_LEFT) {
		if (raw_cursor(-1, CURSOR_CUR) < 0) {
			beep();
		}
	} else if (which == ARROW_RIGHT) {
		if (raw_cursor(1, CURSOR_CUR) < 0) {
			beep();
		}
	} else {
		return -1;
	}

	return 0;
}


/*
 * readline code
 */

typedef enum {
	ST_FAIL = -1,
	ST_SUCCESS = 0,
	ST_LINE,
	ST_BANG,
	ST_BANGNUM,
	ST_BANGSTR,
	ST_SLASHESC,
	ST_SQSTR,
	ST_DQSTR,
} readline_state_t;

#define iseol(c)		((c) == '\0')
char *
readline(void)
{
	int bangflag = 0;
	readline_state_t state = ST_LINE;
	readline_state_t pushed_state = ST_FAIL;
	char *ptr;
	struct strbuf buf;
	char bbuf[128];
	int bi = 0;

	if (!readline_init_done) {
		readline_init();
	}

	/* get the raw command line */
	raw_read();
	ptr = rawbuf->buf;

	strbuf_init(&buf, strdup(""), STRBUF_GROW);

	while (state != ST_SUCCESS && state != ST_FAIL) {
		int c = *ptr++;

		switch (state) {
		  case ST_LINE: {
		  	if (iseol(c)) {
				state = ST_SUCCESS;
			} else if (c == '!') {
				/* history expansion */
				pushed_state = state;
				state = ST_BANG;
			} else if (c == '\\') {
				/* backslash-escaped character */
				state = ST_SLASHESC;
			} else if (c == '\'') {
				/* single quoted string */
				pushed_state = state;
				strbuf_appendc(&buf, c);
				state = ST_SQSTR;
			} else if (c == '"') {
				/* double quoted string */
				strbuf_appendc(&buf, c);
				state = ST_DQSTR;
			} else {
				/* save the character */
				strbuf_appendc(&buf, c);
			}
			break;
		  }
		  case ST_BANG: {
			if (c == '!') {
				/* handle !! (previous cmd in history) */
				char *p;
				
		  		bangflag = 1;
				p = histlookup_num(histnum - 1);
				if (p) {
					strbuf_append(&buf, p);
					state = ST_LINE;
				} else {
					printf("!!: no such history entry\n");
					state = ST_FAIL;
				}
			} else if (isdigit(c)) {
		  		bangflag = 1;
				ptr--;
				state = ST_BANGNUM;
			} else if (isalnum(c)) {
		  		bangflag = 1;
				ptr--;
				state = ST_BANGSTR;
			} else {
				printf("!: syntax error\n");
				state = ST_FAIL;
			}
			break;
		  }
		  case ST_BANGNUM: {
		  	if (isdigit(c)) {
				bbuf[bi++] = (char)c;
				bbuf[bi] = '\0';
			} else {
				char *p;
				long n = 0;
#ifdef INROM
				stoli(bbuf, &n);
#else
				n = strtol(bbuf, NULL, 0);
#endif
				p = histlookup_num(n);
				if (p) {
					strbuf_append(&buf, p);
					ptr--;
					state = pushed_state;
				} else {
					printf("!%ld: no such history entry\n", 
						n);
					state = ST_FAIL;
				}
				bi = 0;
			}
		  	break;
		  }
		  case ST_BANGSTR: {
		  	if (!iseol(c) && !isspace(c)) {
				bbuf[bi++] = (char)c;
				bbuf[bi] = '\0';
			} else {
				char *p;
				p = histlookup_str(bbuf);
				if (p) {
					strbuf_append(&buf, p);
					ptr--;
					state = pushed_state;
				} else {
					printf("!%s: no such history entry\n",
						bbuf);
					state = ST_FAIL;
				}
				bi = 0;
			}
		  	break;
		  }
		  case ST_SLASHESC: {
		  	strbuf_appendc(&buf, c);
		  	state = ST_LINE;
		  	break;
		  }
		  case ST_SQSTR: {
		  	if (c == '\'') {
		  		strbuf_appendc(&buf, c);
		  		state = pushed_state;
			} else if (iseol(c)) {
				printf("unterminated '\n");
				state = ST_FAIL;
			} else {
		  		strbuf_appendc(&buf, c);
			}
		  	break;
		  }
		  case ST_DQSTR: {
			if (c == '\"') {
				strbuf_appendc(&buf, c);
				state = ST_LINE;
		  	} else if (iseol(c)) {
				printf("unterminated \"\n");
				state = ST_FAIL;
			} else if (c == '!') {
				/* history expansion */
				pushed_state = state;
				state = ST_BANG;
			} else if (c == '\'') {
				/* single quoted string */
				pushed_state = state;
				strbuf_appendc(&buf, c);
				state = ST_SQSTR;
			} else {
				/* save the character */
				strbuf_appendc(&buf, c);
			}
		  	break;
		  }
		  default:
		  	printf("readline: invalid state %d\n", state);
			state = ST_FAIL;
			break;
		}
	}

	if (state == ST_SUCCESS && rawbuf->buf[0]) {
		saveline(buf.buf);
		if (bangflag) {
			printf("%s\n", buf.buf);
		}
	}

	/* free raw version, save final version */
	free(rawbuf->buf);
	rawbuf->buf = buf.buf;

	return rawbuf->buf;
}

void
readline_history(void)
{
	struct histent *h = hist0;
	int i = 0;

	while (h->h_line && i++ < HISTSZ) {
		printf("%d: %s\n", h->h_num, h->h_line);
		h = h->h_next;
	}
}

void
readline_flush(void)
{
	struct histent *h = hist0;
	int i;

	for (i = 0; i < HISTSZ; i++) {
		if (h->h_line) {
			free(h->h_line);
		}
		h = h->h_next;
	}
}

static void 
readline_init(void)
{
	int i;

	/* make it into a circular list */
	for (i = 0; i < HISTSZ-1; i++) {
		history[i].h_next = &history[i+1];
	}
	history[i].h_next = &history[0];

	for (i = HISTSZ-1; i > 0; i--) {
		history[i].h_prev = &history[i-1];
	}
	history[i].h_prev = &history[HISTSZ-1];

	readline_init_done++;
}

static int
saveline(char *line)
{
	static int sync = 0;

	if (histn->h_line) {
		free(histn->h_line);
	}
	histn->h_line = strdup(line);
	histn->h_num = histnum;
	histn = histn->h_next;

	if (sync) {
		hist0 = histn;
	}

	if (histn == hist0) {
		sync = 1;
	}

	return histnum++;
}

static char *
histlookup_num(int num)
{
	struct histent *h = hist0;
	
	do {
		if (h->h_num == num) {
			return h->h_line;
		}
		h = h->h_next;
	} while (h != histn);

	return NULL;
}

static char *
histlookup_str(char *str)
{
	struct histent *h = histn->h_prev;
	
	do {
		if (h->h_line && !strncmp(h->h_line, str, strlen(str))) {
			return h->h_line;
		}
		h = h->h_prev;
	} while (h != histn->h_prev);

	return NULL;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
