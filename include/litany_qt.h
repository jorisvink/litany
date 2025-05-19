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

#include <QApplication>
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
#include <QSoundEffect>
#include <QStandardItemModel>

#include <libkyrka/libkyrka.h>

#include "litany.h"

/* Path to the place we install things such as sounds etc. */
#define LITANY_SHARE_DIR		"/usr/local/share/litany"

extern QApplication	*app;

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
 * The liturgy, which can be run in either one of two modes:
 *	1) discovery mode for discovering what peers are available and
 *	   updates the list inside of the LitanyWindow.
 *	2) signaling mode to discover who is trying to talk to us.
 */
#define LITURGY_MODE_DISCOVERY		1
#define LITURGY_MODE_SIGNAL		2

class Liturgy: public QObject {
	Q_OBJECT

public:
	Liturgy(QObject *, QJsonObject *, int);
	~Liturgy(void);

	void signaling_state(u_int8_t, int);
	void socket_send(const void *, size_t);

	void		*litany;
	int		runmode;

private slots:
	void packet_read(void);
	void liturgy_send(void);

private:
	u_int8_t	signaling[KYRKA_PEERS_PER_FLOCK];

	quint16		port;
	QUdpSocket	socket;
	QHostAddress	address;

	QTimer		notify;
	KYRKA		*kyrka;
};

class LitanyWindow;

/*
 * An entry in the online or offline lists in the LitanyWindow,
 * together with the identifier for the peer and an attached
 * chat process (if any).
 */
class LitanyPeer: public QObject, public QListWidgetItem {
	Q_OBJECT

public:
	LitanyPeer(LitanyWindow *, u_int8_t);
	~LitanyPeer(void);

	bool		online;
	u_int8_t	peer_id;

	void		chat_open(void);
	void		show_notification(int);

private slots:
	void		chat_close(int);

private:
	/* The last time we played the notification sound. */
	time_t		last_notification;

	/* The chat window its process, if running. */
	QProcess	*proc;

	/* The litany window under which we reside. */
	LitanyWindow	*litany;
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

	void chat_open(QListWidgetItem *);
	void signaling_state(u_int8_t, int);

	void peer_set_state(u_int8_t, int);
	void peer_set_notification(u_int8_t, int);

	/* Sound effects for different notifications. */
	QSoundEffect		alert;
private:
	/*
	 * The UI online and offline lists and the LitanyPeers that
	 * populate said lists.
	 */
	QListWidget		*online;
	QListWidget		*offline;
	LitanyPeer		*peers[KYRKA_PEERS_PER_FLOCK + 1];

	/* The two liturgies we always have running. */
	Liturgy			*discovery;
	Liturgy			*signaling;
};

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
	/* GUI stuff. */
	QListView			*view;
	QLineEdit			*input;
	QStandardItemModel		*model;

	/* Different sound effects. */
	QSoundEffect			message;

	/* * The libkyrka tunnel object handling our encrypted transport. */
	Tunnel				*tunnel;
};

#endif
