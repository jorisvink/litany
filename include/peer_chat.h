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

#ifndef __H_LITANY_PEER_CHAT_H
#define __H_LITANY_PEER_CHAT_H

#include <QObject>
#include <QListView>
#include <QLineEdit>
#include <QMainWindow>
#include <QStandardItemModel>

#include "tunnel.h"

/*
 * A litany chat, a new window that maintains a tunnel to the peer
 * on the other side and allows end-to-end encrypted communication.
 */
class PeerChat: public QMainWindow {
	Q_OBJECT

public:
	PeerChat(QJsonObject *, const char *);
	~PeerChat(void);

	void message_show(const char *, u_int64_t, Qt::GlobalColor);

private slots:
	void create_message(void);

private:
	/* GUI stuff. */
	QListView			*view;
	QLineEdit			*input;
	QStandardItemModel		*model;

	/* Our own id in the flock (kek-id). */
	QString				kek_id;

	/* * The libkyrka tunnel object handling our encrypted transport. */
	Tunnel				*tunnel;
};

#endif
