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

#ifndef __H_LITANY_WINDOW_H
#define __H_LITANY_WINDOW_H

#include <QObject>
#include <QMainWindow>
#include <QListWidget>

#include "peer.h"
#include "liturgy.h"

/*
 * The main Litany window class, showing the list of online and offline
 * peers and interaction with them.
 */
class LitanyWindow: public QMainWindow, public LiturgyInterface {
	Q_OBJECT

public:
	LitanyWindow(QJsonObject *);
	~LitanyWindow(void);

	void chat_open(QListWidgetItem *);
	void signaling_state(u_int8_t, int);

	void peer_set_state(u_int8_t, int) override;
	void peer_set_notification(u_int8_t, int) override;

private:
	/*
	 * The UI online and offline lists and the LitanyPeers that
	 * populate said lists.
	 */
	QListWidget		*online;
	QListWidget		*offline;
	LitanyPeer		*peers[KYRKA_PEERS_PER_FLOCK + 1];

	/* The UI join group input field. */
	QLineEdit		*group;

	/* The two liturgies we always have running. */
	Liturgy			*discovery;
	Liturgy			*signaling;
};

#endif
