/*
 * printf.c
 *
 * Copyright (c) 2000 Tim Hockin
 * $Id: printf.c,v 1.2 2003/12/11 00:57:09 thockin Exp $
 */

#include <printf.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "common/rom.h"
#include "common/alloc.h"
#include "common/strbuf.h"
#include "common/serial.h"

/* used for tracking the state of a conversion */
struct conv {
	int alt;
	int width;
	int precis;
	int mod;
};
#define NEW_CONV 		{0, -1, -1, 0}

/* flags for alternate form specifiers */
#define CONV_ALT_HASH		1
#define CONV_ALT_ZERO		2
#define CONV_ALT_MINUS		4
#define CONV_ALT_PLUS		8
#define CONV_ALT_SPACE		16
#define CONV_ALT_UPPER		128
/* flags for length modifiers */
#define CONV_MOD_SHORT		1
#define CONV_MOD_LONG		2
#define CONV_MOD_LONGLONG	4
#define CONV_MOD_UNSIGNED	128

/* types */
typedef unsigned long long ulonglong;

/* local functions */
static int vprintf(const char *fmt, va_list args);
static int vsnprintf(char *str, int maxlen, const char *fmt, va_list args);
static int conv_alt_val(char c);

static int sprintf_core(struct strbuf *buf, const char *fmt, va_list args);
static int sprintf_num(struct strbuf *buf, struct conv *c, ulong num, 
	int base);
static int sprintf_ll_num(struct strbuf *buf, struct conv *c, 
	ulonglong num, int base);
static int sprintf_string(struct strbuf *buf, struct conv *c, char *str);

/*
 * the externally called functions
 */
int 
printf(const char *format, ...)
{
	int len;
	va_list args;

	/* create the varargs list */
	va_start(args, format);

	/* call va_list aware form */
	len = vprintf(format, args);

	/* clean up the varargs list */
	va_end(args);

	return len;
}

int 
sprintf(char *str, const char *format, ...)
{
	int len;
	va_list args;

	/* create the varargs list */
	va_start(args, format);

	/* call va_list aware form */
	len = vsnprintf(str, -1, format, args);

	/* clean up the varargs list */
	va_end(args);

	return len;
}

int 
snprintf(char *str, int maxlen, const char *format, ...)
{
	int len;
	va_list args;

	/* create the varargs list */
	va_start(args, format);

	/* call va_list aware form */
	len = vsnprintf(str, maxlen, format, args);

	/* clean up the varargs list */
	va_end(args);

	return len;
}

int
getc(void)
{
	unsigned char c;

	serial_inb(&c);
	return c;
}

int
putc(char c)
{
	serial_outb(c);
	if (c == '\n') {
		serial_outb('\r');
	}
	return (int)c;
}

int
puts(char *str)
{
	char *p;
	int len = 0;

	/* just be safe */
	if (str) {
		p = str;
	} else {
		p = "(NULL)";
	}
	
	/* print string */
	while (*p) {
		putc(*p);
		len++;
		p++;
	}

	return len;
}

static const struct units_struct {
	uint32 divisor;
	const char symbol;
} units_table[] = {
	{ 0x40000000, 'G'  },
	{ 0x00100000, 'M'  },
	{ 0x00000400, 'K'  },
	{ 0x00000000, '\0' }
};

const char *
print_bytes(uint32 num)
{
	const struct units_struct *table = units_table;
	static char *buf;

	buf = malloc(32);
	if (!buf) {
		printf("malloc failed at %s:%d\n", 
		       __FILE__, __LINE__);
		return NULL;
	}
	memset(buf, 0, 32);

	while (table->symbol) {
		if ((num >= table->divisor) &&
		   ((num % table->divisor) == 0)) {
			sprintf(buf, "%d%c",
				num / table->divisor,
				table->symbol);
			return (const char *)buf;
		}
		table++;
	}

	sprintf(buf, "%d", num);
	return (const char *)buf;
}

/*
 * internal functions
 */

static int
vprintf(const char *fmt, va_list args)
{
	int len;
	struct strbuf buf;

	/* initialize the strbuf to grow as needed */
	strbuf_init(&buf, NULL, STRBUF_GROW);

	/* build up the target string */
	len = sprintf_core(&buf, fmt, args);
	
	/* dump it to console */
	puts(buf.buf);

	/* cleanup */
	free(buf.buf);

	return len;
}

static int
vsnprintf(char *str, int maxlen, const char *fmt, va_list args)
{
	int len;
	struct strbuf buf;

	/* 
	 * initialize the strbuf to use str
	 * set max to be fixed if maxlen < 0
	 * set max to maxlen-1, otherwise
	 *   snprintf 'n' includes '\0', subtract 1 here, deal with it later
	 */
	strbuf_init(&buf, str, (maxlen > 0) ? maxlen-1 : STRBUF_FIXED);

	/* build up the target string */
	len = sprintf_core(&buf, fmt, args);
	
	return len;
}

static int
sprintf_core(struct strbuf *buf, const char *fmt, va_list args)
{
	const char *p = fmt;

	while (*p) {
		struct conv c = NEW_CONV;

		/* handle a conversion */
		if (*p == '%') {
			int conv_err = 0;
 
 			p++;

			/* first, look for alternate form flags */
			while (*p == '#' 
			    || *p == '0' 
			    || *p == '-' 
			    || *p == '+' 
			    || *p == ' ') {
				/* flag the alt form */
				c.alt |= conv_alt_val(*p);
				p++;
			}

			/* second, look for a field width */
			if (*p == '*') {
				/* a '*' says get a va_arg value */
				c.width = va_arg(args, int);
				p++;
			} else if (isdigit(*p)) {
				/* read an integer string */
				c.width = 0;
				while (isdigit(*p)) {
					c.width *= 10;
					c.width += (*p) - '0';
					p++;
				}
			}

			/* third, look for precision */
			if (*p == '.') {
				p++;
				/* a '*' means get a va_arg value */
				if (*p == '*') {
					c.precis = va_arg(args, int);
					p++;
				} else {
					/* -ve precision is ignored */
					int neg = 1;
					if (*p && *p == '-') {
						p++;
						neg = -1;
					}
					/* read an integer string */
					c.precis = 0;
					while (*p && isdigit(*p)) {
						c.precis *= 10;
						c.precis += (*p) - '0';
						p++;
					}
					c.precis *= neg;
				}
			}

			/* fourth, look for a length modifier */
			if (*p) {
				if (*p == 'h') {
			 		c.mod = CONV_MOD_SHORT;
					p++;
				} else if (*p == 'L' || *p == 'q') {
					c.mod = CONV_MOD_LONGLONG;
					p++;
				} else if (*p == 'l') {
					if (*(p+1) == 'l') {
						c.mod = CONV_MOD_LONGLONG;
						p+=2;
					} else {
						c.mod = CONV_MOD_LONG;
						p++;
					}
				} 
			}

			/* last - the conversion specifier */
			switch (*p) {
				case 'd':
				case 'i':
					/* signed int */
					if (c.mod & CONV_MOD_LONGLONG) {
						long long n = va_arg(args, long long);
						sprintf_ll_num(buf, &c, n, 10);
					} else {
						long n = va_arg(args, long);
						sprintf_num(buf, &c, n, 10);
					}
					break;
				case 'o':
					/* octal */
					c.mod |= CONV_MOD_UNSIGNED;
					if (c.mod & CONV_MOD_LONGLONG) {
						long long n = va_arg(args, long long);
						sprintf_ll_num(buf, &c, n, 8);
					} else {
						long n = va_arg(args, long);
						sprintf_num(buf, &c, n, 8);
					}
					break;
				case 'X':
					/* hex (upper case) */
					c.alt |= CONV_ALT_UPPER;
				case 'x':
					/* hex (lower case) */
					c.mod |= CONV_MOD_UNSIGNED;
					if (c.mod & CONV_MOD_LONGLONG) {
						long long n = va_arg(args, long long);
						sprintf_ll_num(buf, &c, n, 16);
					} else {
						long n = va_arg(args, long);
						sprintf_num(buf, &c, n, 16);
					}
					break;
				case 'b':
					/* binary - not standard */
					c.mod |= CONV_MOD_UNSIGNED;
					if (c.mod & CONV_MOD_LONGLONG) {
						long long n = va_arg(args, long long);
						sprintf_ll_num(buf, &c, n, 2);
					} else {
						long n = va_arg(args, long);
						sprintf_num(buf, &c, n, 2);
					}
					break;
				case 'u':
					/* unsigned */
					c.mod |= CONV_MOD_UNSIGNED;
					if (c.mod & CONV_MOD_LONGLONG) {
						ulonglong n = va_arg(args, ulonglong);
						sprintf_ll_num(buf, &c, n, 10);
					} else {
						ulong n = va_arg(args, ulong);
						sprintf_num(buf, &c, n, 10);
					}
					break;
				case 'c': {
					char c = va_arg(args, char);
					strbuf_appendc(buf, c);
					break;
				}
				case 's': {
					char *p = va_arg(args, char *);
					sprintf_string(buf, &c, p);
					break;
				}
				case 'p': {
					/* pointer */
					ulong p = va_arg(args, ulong);
					c.alt |= CONV_ALT_HASH | CONV_ALT_ZERO;
					c.mod |= CONV_MOD_UNSIGNED;
					sprintf_num(buf, &c, p, 16);
					break;
				}
#if 0 /* no floating point */
				case 'e':
				case 'E':
				case 'f':
				case 'g':
				case 'G':
					break;
#endif
				case 'n': {
					/* store the # chars printed so far */
					int *p = va_arg(args, int *);
					*p = buf->len;
					break;
				}
				case '%':
					strbuf_appendc(buf, '%');
					break;
				default:
					conv_err = 1;
			}

			/* something screwy - just print the literal */
			if (conv_err) {
				strbuf_appendc(buf, *p);
			}
		} else {
			strbuf_appendc(buf, *p);
		}

		/* next char */
		p++;
	}

	return buf->len;
}

/*
 * conv_alt_val
 *
 * helper routine to convert an alternate form value
 */
static int
conv_alt_val(char c)
{
	switch (c) {
		case '#':
			return CONV_ALT_HASH;
		case '0':
			return CONV_ALT_ZERO;
		case '-':
			return CONV_ALT_MINUS;
		case '+':
			return CONV_ALT_PLUS;
		case ' ':
			return CONV_ALT_SPACE;
		default:
			return 0;
	}
}

/*
 * sprintf_num
 *
 * print a formatted integer
 * returns the number of characters printed
 */
static int
sprintf_num(struct strbuf *buf, struct conv *c, ulong num, int base)
{
	char nbuf[128];
	char *p = NULL;
	int width = c->width;
	int precis = c->precis;
	char pad = ' ';
	char isneg = 0;
	char *chars;
	ulong n;
	int len0 = buf->len;
	
	/* 2 <= base <= 16 */
	if (base < 2 || base > 16) {
		return 0;
	}

	/* pick a character set */
	if (c->alt & CONV_ALT_UPPER) {
		chars = "0123456789ABCDEF";
	} else {
		chars = "0123456789abcdef";
	}

	/* convert to positive, remember that it is negative */
	if (!(c->mod & CONV_MOD_UNSIGNED) && (long)num < 0) {
		isneg = 1;
		num = (long)num * -1;
	}
	
	/* cast to correct data size */
	if (c->mod & CONV_MOD_SHORT) {
		n = (ushort)num;
	} else if (c->mod & CONV_MOD_LONG) {
		n = (ulong)num;
	} else {
		n = (uint)num;
	}

	/* handle signs/leading space if requested */
	if (!(c->mod & CONV_MOD_UNSIGNED)) {
		if (isneg) {
			strbuf_appendc(buf, '-');
			width--;
		} else if (c->alt & CONV_ALT_PLUS) {
			strbuf_appendc(buf, '+');
			width--;
		} else if (c->alt & CONV_ALT_SPACE) {
			strbuf_appendc(buf, ' ');
			width--;
		}
	}

	/* handle '#' altform */
	if (c->alt & CONV_ALT_HASH) {
		if (base == 16 && (p-2 >= nbuf)) {
			strbuf_appendc(buf, '0');
			strbuf_appendc(buf, (c->alt & CONV_ALT_UPPER)?'X':'x');
			width -= 2;
		} else if (base == 8 && (p-1 >= nbuf)) {
			strbuf_appendc(buf, '0');
			width--;
		}
	}

	/* write the number, from the right */
	nbuf[sizeof(nbuf) - 1] = '\0';
	p = &(nbuf[sizeof(nbuf) - 2]);

	/* must have at least one char */
	*p = chars[n % base];
	n /= base;
	width--;
	precis--;
	while (p >= nbuf && n) {
		p--;
		*p = chars[n % base];
		n /= base;
		width--;
		precis--;
	}

	/* fulfill a precison specifier */
	while (p >= nbuf && precis > 0) {
		p--;
		precis--;
		width--;
		*p = '0';
	}
	
	/* see if we are left or right aligned */
	if (!(c->alt & CONV_ALT_MINUS)) {
		/* fill any extra padding from width specifier */
		pad = ((c->alt & CONV_ALT_ZERO) && (c->precis < 0))?'0':' ';
		while (p >= nbuf && width > 0) {
			p--;
			width--;
			*p = pad;
		}
		strbuf_append(buf, p);
	} else {
		strbuf_append(buf, p);

		/* handle a width request */
		while (width > 0) {
			strbuf_appendc(buf, ' ');
			width--;
		}
	}

	return (buf->len - len0);
}

/* 
 * sprintf_ll_num
 *
 * the exact same as printf_num(), but with a long long 
 */
static int
sprintf_ll_num(struct strbuf *buf, struct conv *c, ulonglong num, int base)
{
	char nbuf[128];
	char *p = NULL;
	int width = c->width;
	int precis = c->precis;
	char pad = ' ';
	char isneg = 0;
	char *chars;
	ulonglong n;
	int len0 = buf->len;
	
	/* 2 <= base <= 16 */
	if (base < 2 || base > 16) {
		return 0;
	}

	/* pick a character set */
	if (c->alt & CONV_ALT_UPPER) {
		chars = "0123456789ABCDEF";
	} else {
		chars = "0123456789abcdef";
	}

	/* convert to positive, remember that it is negative */
	if (!(c->mod & CONV_MOD_UNSIGNED) && (long long)num < 0) {
		isneg = 1;
		num = (long long)num * -1;
	}
	
	n = (ulonglong)num;

	/* handle signs/leading space if requested */
	if (!(c->mod & CONV_MOD_UNSIGNED)) {
		if (isneg) {
			strbuf_appendc(buf, '-');
			width--;
		} else if (c->alt & CONV_ALT_PLUS) {
			strbuf_appendc(buf, '+');
			width--;
		} else if (c->alt & CONV_ALT_SPACE) {
			strbuf_appendc(buf, ' ');
			width--;
		}
	}

	/* handle '#' altform */
	if (c->alt & CONV_ALT_HASH) {
		if (base == 16 && (p-2 >= nbuf)) {
			strbuf_appendc(buf, '0');
			strbuf_appendc(buf, (c->alt & CONV_ALT_UPPER)?'X':'x');
			width -= 2;
		} else if (base == 8 && (p-1 >= nbuf)) {
			strbuf_appendc(buf, '0');
			width--;
		}
	}

	/* write the number, from the right */
	nbuf[sizeof(nbuf) - 1] = '\0';
	p = &(nbuf[sizeof(nbuf) - 2]);

	/* must have at least one char */
	*p = chars[n % base];
	n /= base;
	width--;
	precis--;
	while (p >= nbuf && n) {
		p--;
		*p = chars[n % base];
		n /= base;
		width--;
		precis--;
	}

	/* fulfill a precison specifier */
	while (p >= nbuf && precis > 0) {
		p--;
		precis--;
		width--;
		*p = '0';
	}
	
	/* see if we are left or right aligned */
	if (!(c->alt & CONV_ALT_MINUS)) {
		/* fill any extra padding from width specifier */
		pad = ((c->alt & CONV_ALT_ZERO) && (c->precis < 0))?'0':' ';
		while (p >= nbuf && width > 0) {
			p--;
			width--;
			*p = pad;
		}
		strbuf_append(buf, p);
	} else {
		strbuf_append(buf, p);

		/* handle a width request */
		while (width > 0) {
			strbuf_appendc(buf, ' ');
			width--;
		}
	}

	return (buf->len - len0);
}

static int
sprintf_string(struct strbuf *buf, struct conv *c, char *str)
{
	char *p;
	int len0 = buf->len;
	char *precisbuf = NULL;

	/* just be safe */
	if (str) {
		if (c->precis > 0) {
			precisbuf = malloc(c->precis + 1);
			if (!precisbuf) {
				/* FIXME: handle error */
			}
			strncpy(precisbuf, str, c->precis);
			precisbuf[c->precis] = '\0';
			p = precisbuf;
		} else {
			p = str;
		}
	} else {
		p = "(NULL)";
	}

	strbuf_append(buf, p);

	/* cleanup, if needed */
	if (precisbuf) {
		free(precisbuf);
	}

	return (buf->len - len0);
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
