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

#ifndef __H_LITANY_QT_H
#define __H_LITANY_QT_H

#include <sys/queue.h>

#include <QObject>
#include <QUdpSocket>
#include <QMainWindow>
#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QTimer>
#include <QProcess>
#include <QListView>
#include <QJsonObject>
#include <QStandardItemModel>

#include <libkyrka/libkyrka.h>

#include "litany.h"

/*
 * A tunnel object, responsible for maintaing a single peer-to-peer
 * and end-to-end encrypted tunnel to a peer using libkyrka.
 */
class Tunnel: public QObject {
	Q_OBJECT

public:
	Tunnel(QJsonObject *, const char *, QObject *);
	~Tunnel(void);

	void send_text(const void *, size_t);
	void send_msg(struct litany_msg_data *);

	void socket_send(const void *, size_t);

	void send_ack(u_int64_t);
	void recv_ack(u_int64_t);
	void recv_msg(Qt::GlobalColor, u_int64_t, const char *, ...);

	void system_msg(const char *, ...);

private slots:
	void manage(void);
	void packet_read(void);
	void resend_pending(void);

private:
	QUdpSocket		socket;
	quint16			port;
	QHostAddress		address;

	QObject			*owner;
	KYRKA			*kyrka;

	QTimer			manager;
	time_t			last_notify;

	/* Timer to periodically flush messages. */
	QTimer			flush;

	/* List of non-ack'd messages. */
	struct litany_msg_list	msgs;
};
/*
 * The liturgy, responsible for discovering what peers are available
 * and updates the list inside of the LitanyWindow.
 */
class Liturgy: public QObject {
	Q_OBJECT

public:
	Liturgy(QJsonObject *);
	~Liturgy(void);
	void socket_send(const void *, size_t);

private slots:
	void packet_read(void);
	void liturgy_send(void);

private:
	quint16		port;
	QUdpSocket	socket;
	QHostAddress	address;

	QTimer		notify;
	KYRKA		*kyrka;
};

/*
 * An entry in the online or offline lists in the LitanyWindow,
 * together with the identifier for the peer.
 */
class LitanyPeer: public QListWidgetItem {
public:
	LitanyPeer(u_int8_t);

	u_int8_t	peer_id;
};

/*
 * The main Litany window class, showing the list of online and offline
 * peers and interaction with them.
 */
class LitanyWindow: public QMainWindow {
	Q_OBJECT

public:
	LitanyWindow(QJsonObject *);
	~LitanyWindow(void);

	void chat_exit(int);
	void chat_open(QListWidgetItem *);
	void peer_set_state(u_int8_t, int);

private:
	QListWidget		*online;
	QListWidget		*offline;

	Liturgy			*discovery;
	LitanyPeer		*peers[KYRKA_PEERS_PER_FLOCK + 1];
};

/* src/main.cc */
extern LitanyWindow	*litany;

/*
 * A litany chat, a new window that maintains a tunnel to the peer
 * on the other side and allows end-to-end encrypted communication.
 */
class LitanyChat: public QMainWindow {
	Q_OBJECT

public:
	LitanyChat(QJsonObject *, const char *);
	~LitanyChat(void);

	void message_show(const char *, u_int64_t, Qt::GlobalColor);

private slots:
	void create_message(void);

private:
	void flush_messages(void);

	/* GUI stuff. */
	QListView			*view;
	QLineEdit			*input;
	QStandardItemModel		*model;

	/* * The libkyrka tunnel object handling our encrypted transport. */
	Tunnel				*tunnel;
};

#endif
