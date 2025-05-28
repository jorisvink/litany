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

#ifndef __H_LITANY_LITURGY_H
#define __H_LITANY_LITURGY_H

#include <QTimer>
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>

#include <libkyrka/libkyrka.h>

/*
 * The interface objects wanting to use liturgies must adhere too.
 */
class LiturgyInterface {
public:
	virtual void peer_set_state(u_int8_t, int);
	virtual void peer_set_notification(u_int8_t, int);
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
	Liturgy(LiturgyInterface *, QJsonObject *, int, u_int16_t);
	~Liturgy(void);

	void signaling_state(u_int8_t, int);
	void socket_send(const void *, size_t);

	LiturgyInterface	*owner;
	int			runmode;

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

#endif
