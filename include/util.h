/*
 * Copyright (c) 2025 Joris Vink <joris@sanctorum.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __H_LITANY_UTIL_H
#define __H_LITANY_UTIL_H

#include <sys/queue.h>

/* Handy macros. */
#define PRECOND(x)							\
	do {								\
		if (!(x)) {						\
			fatal("precondition failed in %s:%s:%d\n",	\
			    __FILE__, __func__, __LINE__);		\
		}							\
	} while (0)

#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define htobe16(x)		OSSwapHostToBigInt16(x)
#define htobe32(x)		OSSwapHostToBigInt32(x)
#define htobe64(x)		OSSwapHostToBigInt64(x)
#define be16toh(x)		OSSwapBigToHostInt16(x)
#define be32toh(x)		OSSwapBigToHostInt32(x)
#define be64toh(x)		OSSwapBigToHostInt64(x)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* System messages have an id of 0. */
#define LITANY_MESSAGE_SYSTEM_ID	0

/* The maximum number of bytes per message we can submit. */
#define LITANY_MESSAGE_MAX_SIZE		512

/* The different types of messages. */
#define LITANY_MESSAGE_TYPE_TEXT	1
#define LITANY_MESSAGE_TYPE_ACK		2
#define LITANY_MESSAGE_TYPE_HEARTBEAT	3

/*
 * A message containing some data that we are sending to the other
 * side every few seconds until the side ACKs its transfer.
 */
struct litany_msg_data {
	u_int64_t		id;
	u_int16_t		len;
	u_int8_t		type;
	u_int8_t		data[LITANY_MESSAGE_MAX_SIZE];
} __attribute__((packed));

struct litany_msg {
	u_int64_t		id;
	time_t			age;
	struct litany_msg_data	data;
	TAILQ_ENTRY(litany_msg)	list;
};

TAILQ_HEAD(litany_msg_list, litany_msg);

/* src/main.cc */
extern const char	*config_file;
void			fatal(const char *, ...) __attribute__((noreturn));

/* src/msg.c */
void	litany_msg_number_reset(u_int8_t);
void	litany_msg_ack(struct litany_msg_list *, u_int64_t);

struct litany_msg	*litany_msg_register(struct litany_msg_list *,
			    const void *, size_t);

#if defined(__cplusplus)
}
#endif

#endif
