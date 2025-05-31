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

#include <sys/types.h>

#if defined(PLATFORM_WINDOWS)
#include <libkyrka/portable_win.h>
#endif

#include <libkyrka/libkyrka.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util.h"
#include "queue.h"

/* We can use libnyfe because its included in libkyrka. */
void	nyfe_random_init(void);
void	nyfe_random_bytes(void *, size_t);

/* The message number for the next registered message. */
static u_int64_t	msgno = 1;

/*
 * Set the message number to include the given peer id in the highest
 * bits and a set of 32 random bits following that.
 *
 * Note that this message number has nothing to do with the encrypted
 * packet its sequence number at all.
 */
void
litany_msg_number_reset(u_int8_t peer)
{
	u_int32_t	rand;

	nyfe_random_init();
	nyfe_random_bytes(&rand, sizeof(rand));

	msgno = ((u_int64_t)peer << 56) | ((u_int64_t)rand << 24) | 1;
}

/*
 * Register a new message on the given list.
 */
struct litany_msg *
litany_msg_register(struct litany_msg_list *list, const void *data, size_t len)
{
	struct timespec		ts;
	struct litany_msg	*msg;

	PRECOND(list != NULL);
	PRECOND(data != NULL);
	PRECOND(len > 0 && len < LITANY_MESSAGE_MAX_SIZE);

	if ((msg = calloc(1, sizeof(*msg))) == NULL)
		fatal("calloc(%zu): %d", sizeof(*msg), errno);

	(void)clock_gettime(CLOCK_MONOTONIC, &ts);

	msg->id = msgno;
	msg->age = ts.tv_sec;
	memcpy(msg->data.data, data, len);

	msg->data.len = htobe16(len);
	msg->data.id = htobe64(msg->id);
	msg->data.type = LITANY_MESSAGE_TYPE_TEXT;

	TAILQ_INSERT_TAIL(list, msg, list);
	msgno++;

	return (msg);
}

/*
 * Remove a message that matches the given ack message number from the list.
 */
void
litany_msg_ack(struct litany_msg_list *list, u_int64_t ack)
{
	struct litany_msg	*msg;

	PRECOND(list != NULL);

	TAILQ_FOREACH(msg, list, list) {
		if (msg->id == ack) {
			TAILQ_REMOVE(list, msg, list);
			free(msg);
			return;
		}
	}
}
