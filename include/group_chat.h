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

#ifndef __H_LITANY_GROUP_CHAT_H
#define __H_LITANY_GROUP_CHAT_H

#include <QList>
#include <QObject>
#include <QListView>
#include <QLineEdit>
#include <QJsonObject>
#include <QMainWindow>
#include <QStandardItemModel>

#include "tunnel.h"
#include "liturgy.h"

/*
 * A group chat with tunnels to many peers.
 */
class GroupChat: public QMainWindow,
    public LiturgyInterface, public TunnelInterface {
	Q_OBJECT

public:
	GroupChat(QJsonObject *, const char *);
	~GroupChat(void);

	void peer_set_state(u_int8_t, int) override;
	void message_show(const char *, u_int64_t, Qt::GlobalColor) override;

private slots:
	void create_message(void);

private:
	/* GUI stuff. */
	QListView			*view;
	QLineEdit			*input;
	QStandardItemModel		*model;

	/* Our own id in the flock (kek-id). */
	QString				kek_id;

	/* The configuration. */
	QJsonObject			*tunnel_config;

	/* The discovery liturgy. */
	Liturgy				*discovery;

	/* A tunnel per participant. */
	Tunnel				*tunnels[KYRKA_PEERS_PER_FLOCK];
};

#endif
