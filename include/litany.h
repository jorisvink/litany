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

#ifndef __H_LITANY_H
#define __H_LITANY_H

#include <sys/types.h>

#include <QJsonObject>
#include <QApplication>

#include "util.h"
#include "litany.h"
#include "tunnel.h"
#include "liturgy.h"
#include "peer.h"
#include "peer_chat.h"
#include "group_chat.h"
#include "window.h"

/* src/main.cc */
extern QApplication	*app;

char		*litany_json_string(QJsonObject *, const char *);
u_int64_t	litany_json_number(QJsonObject *, const char *, u_int64_t);

#endif
