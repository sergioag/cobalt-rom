#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "common/alloc.h"
#include "common/rom.h"
#include "common/strbuf.h"

/* 
 * strbuf code 
 */

int 
strbuf_init(struct strbuf *buf, char *str, strbuf_max_t max)
{
	buf->len = 0;
	buf->buf = str;
	buf->max = max;

	/* NULL str is only OK for STRBUF_GROW or max >= 0 */
	if (!str && STRBUF_HAS_MAX(buf)) {
		buf->buf = malloc(max + 1);
		if (!buf->buf) {
			return -1;
		}
	}

	/* empty string */
	if (buf->buf) {
		buf->buf[0] = '\0';
	} 

	return 0;
}

int 
strbuf_append(struct strbuf *buf, char *str)
{
	char *p;
	char *q;
	int n;

	/* grow the buffer if we need to */
	if (buf->max == STRBUF_GROW) {
		char *new;
		new = realloc(buf->buf, buf->len + strlen(str) + 1);
		if (!new) {
			return -1;
		}

		buf->buf = new;
	}

	p = &(buf->buf[buf->len]);
	q = str;
	n = 0;

	/* append from str until we hit max or '\0' */
	while (*q && ((buf->max < 0) || (buf->len < buf->max))) {
		*p++ = *q++;
		buf->len++;
		n++;
	}
	*p = '\0';

	return n;
}

int 
strbuf_appendc(struct strbuf *buf, char c)
{
	char ar[2] = {c, '\0'};
	return strbuf_append(buf, ar);
}

int
strbuf_insert(struct strbuf *buf, int idx, char *str)
{
	char *dest;
	char *src;
	int len;

	if (idx > buf->len || idx < 0) {
		return 0;
	}

	/* grow the buffer if we need to */
	if (buf->max == STRBUF_GROW) {
		char *new;
		new = realloc(buf->buf, buf->len + 1 + strlen(str));
		if (!new) {
			return -1;
		}

		if (!buf->buf) {	
			/* first allocation */
			new[0] = '\0';
		}
		buf->buf = new;
	} 
	
	src = buf->buf + idx;
	dest = src + strlen(str);

	if (STRBUF_HAS_MAX(buf) && (buf->len + strlen(str)) > buf->max) {
		/* shift last chars right off the end, saving '\0' */
		len = (buf->len - idx) - strlen(str);
		if (len < 0) {
			len = 0;
		}
	} else {
		/* shift it all right one space, including '\0' */
		len = (buf->len - idx) + 1;
		buf->len += strlen(str);
	}
	memmove(dest, src, len);
	memmove(src, str, strlen(str));

	return strlen(str);
}

int
strbuf_insertc(struct strbuf *buf, int idx, char c)
{
	char ar[2] = {c, '\0'};
	return strbuf_insert(buf, idx, ar);
}

int
strbuf_delete(struct strbuf *buf, int idx, int nchars)
{
	char *dest;
	char *src;
	int len;

	if (idx >= buf->len || idx < 0) {
		return 0;
	}

	/* slide it first */
	dest = buf->buf + idx;
	src = dest + nchars;
	len = ((buf->len - idx) - nchars) + 1; /* add one for the '\0' */

	/* shift it all left nchars spaces */
	memmove(dest, src, len);

	/* shrink the buffer if we need to */
	if (buf->max == STRBUF_GROW) {
		char *new;
		new = realloc(buf->buf, (buf->len + 1) - nchars);
		if (!new) {
			return -1;
		}

		buf->buf = new;
	}
	buf->len -= nchars;
	
	return nchars;
}

int
strbuf_deletec(struct strbuf *buf, int idx)
{
	return strbuf_delete(buf, idx, 1);
}

/* self test */
int
strbuf_bist(void)
{
#undef STRBUF_TEST
#ifdef STRBUF_TEST
	struct strbuf sb;
	char buf[32]; 

	/* dynamic buffer */
	printf("Testing STRBUF_GROW:\n");
	strbuf_init(&sb, NULL, STRBUF_GROW);

	printf("   testing append:\n");
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_append(&sb, "len should be 16");
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_append(&sb, ", now 24");
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_appendc(&sb, '!');
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);

	printf("   testing insert:\n");
	strbuf_insertc(&sb, 14, '>');
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_insert(&sb, 0, "***");
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);

	printf("   testing delete:\n");
	strbuf_deletec(&sb, 17);
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_delete(&sb, 0, 3);
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);

	free(sb.buf);
	printf("Done\n\n");

	/* fixed size buffer */
	printf("Testing STRBUF_FIXED:\n");
	strbuf_init(&sb, buf, STRBUF_FIXED);

	printf("   testing append:\n");
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_append(&sb, "len should be 16");
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_appendc(&sb, '!');
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);

	printf("   testing insert:\n");
	strbuf_insertc(&sb, 14, '>');
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_insert(&sb, 0, "***");
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);

	printf("   testing delete:\n");
	strbuf_deletec(&sb, 17);
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_delete(&sb, 0, 3);
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);

	printf("Done\n\n");
	
	/* fixed buffer of maximum length */
	printf("Testing max = 15:\n");
	strbuf_init(&sb, NULL, 15);

	printf("   testing append:\n");
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_append(&sb, "len should be 16");
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_appendc(&sb, '!');
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);

	printf("   testing insert:\n");
	strbuf_insertc(&sb, 14, '>');
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_insert(&sb, 0, "***");
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);

	printf("   testing delete:\n");
	strbuf_deletec(&sb, 17);
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);
	strbuf_delete(&sb, 0, 3);
	printf("      buf = \"%s\" (%p), len = %d\n", 
		sb.buf, sb.buf, sb.len);

	free(sb.buf);
	printf("Done\n\n");
#endif

	return 0;
}
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
