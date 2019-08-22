#ifndef COMMON_STRBUF_H
#define COMMON_STRBUF_H

/* flags for strbuf.max */
typedef enum {
	STRBUF_GROW = -2,
	STRBUF_FIXED = -1
} strbuf_max_t;
#define STRBUF_HAS_MAX(sbp)	((sbp)->max >= 0)

/* used for building string buffers */
struct strbuf {
	int len;
	strbuf_max_t max;
	char *buf;
};

/* string buffer functions */
int strbuf_init(struct strbuf *buf, char *str, strbuf_max_t max);
int strbuf_append(struct strbuf *buf, char *str);
int strbuf_appendc(struct strbuf *buf, char c);
int strbuf_insert(struct strbuf *buf, int idx, char *str);
int strbuf_insertc(struct strbuf *buf, int idx, char c);
int strbuf_delete(struct strbuf *buf, int idx, int nchars);
int strbuf_deletec(struct strbuf *buf, int idx);
int strbuf_bist(void);

#endif
// LICENSE:
// This software is subject to the terms of the GNU GENERAL 
// PUBLIC LICENSE Version 2, June 1991
