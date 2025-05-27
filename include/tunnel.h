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

#ifndef __H_LITANY_TUNNEL_H
#define __H_LITANY_TUNNEL_H

#include <QTimer>
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>

#include <libkyrka/libkyrka.h>

#include "util.h"

/*
 * The interface objects wanting to use tunnels must adhere too.
 */
class TunnelInterface {
public:
	virtual void message_show(const char *, u_int64_t, Qt::GlobalColor);
};

/*
 * A tunnel object, responsible for maintaing a single peer-to-peer
 * and end-to-end encrypted tunnel to a peer using libkyrka.
 */
class Tunnel: public QObject {
	Q_OBJECT

public:
	Tunnel(TunnelInterface *, QJsonObject *, u_int8_t);
	~Tunnel(void);

	void send_heartbeat(void);
	void send_text(const void *, size_t);
	void send_msg(struct litany_msg_data *);

	void socket_send(const void *, size_t, int, int);

	void send_ack(u_int64_t);
	void recv_ack(u_int64_t);
	void recv_msg(Qt::GlobalColor, u_int64_t, const char *, ...);

	void system_msg(const char *, ...);

	void peer_alive(void);
	void peer_update(struct kyrka_event_peer *);

	/* The peer_id we are talking too. */
	u_int8_t		peer_id;

private slots:
	void manage(void);
	void packet_read(void);
	void resend_pending(void);

private:
	QUdpSocket		socket;

	/* The ip:port of our peer. */
	quint16			peer_port;
	QHostAddress		peer_address;

	/* The ip:port of our cathedral. */
	quint16			cathedral_port;
	QHostAddress		cathedral_address;

	/* The interface that manages us. */
	TunnelInterface		*owner;

	/* The underlying libkyrka context to maintain tunnel state. */
	KYRKA			*kyrka;

	QTimer			manager;
	time_t			last_update;
	time_t			last_notify;
	time_t			last_heartbeat;

	/* Timer to periodically flush messages. */
	QTimer			flush;

	/* List of non-ack'd messages. */
	struct litany_msg_list	msgs;
};

#endif
