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

#include <QUrl>
#include <QLabel>
#include <QBoxLayout>
#include <QPushButton>
#include <QApplication>

#include "litany.h"

/*
 * A litany peer, these are added to either the online or offline
 * QListWidget. This exists so we can obtain the peer id in a way
 * that made sense to me.
 */
LitanyPeer::LitanyPeer(LitanyWindow *parent, u_int8_t id)
{
	proc = NULL;
	peer_id = id;
	online = false;
	litany = parent;
}

/*
 * Enable or disable the notification when another peer is trying to reach us.
 */
void
LitanyPeer::show_notification(int onoff)
{
	QString		id;

	PRECOND(onoff == 0 || onoff == 1);

	id = QString("%1").arg(peer_id, 2, 16, QLatin1Char('0'));

	if (proc) {
		setForeground(Qt::green);
		setText(QString("Peer %1 (chat open)").arg(id));
	} else {
		if (onoff) {
			setForeground(Qt::yellow);
			setText(QString("Peer %1 (chat pending)").arg(id));
			app->alert(litany);
		} else {
			setForeground(Qt::gray);
			setText(QString("Peer %1").arg(id));
		}
	}
}

/*
 * Launch the chat window for this peer.
 */
void
LitanyPeer::chat_open(void)
{
	QString			id;
	QStringList		nargs;
	const QStringList	args(QApplication::instance()->arguments());

	if (proc != NULL)
		return;

	if (config_file != NULL) {
		nargs.append("-c");
		nargs.append(QString("%1").arg(config_file));
	}

	id = QString("0x%1").arg(peer_id, 2, 16, QLatin1Char('0'));
	nargs.append("chat");
	nargs.append(id);

	proc = new QProcess(this);

	proc->setProgram(args.at(0));
	proc->setArguments(nargs);

	connect(proc, &QProcess::finished, this, &LitanyPeer::chat_close);
	proc->start();
}

/*
 * The chat window for this peer has closed, turn off signaling to the peer.
 */
void
LitanyPeer::chat_close(int exit_status)
{
	PRECOND(proc != NULL);
	(void)exit_status;

	delete proc;
	proc = NULL;

	litany->signaling_state(peer_id, 0);
}

/*
 * Cleanup any LitanyPeer resources, we kill the chat process here
 * if its still running.
 */
LitanyPeer::~LitanyPeer(void)
{
	if (proc != NULL)
		proc->close();

	delete proc;
}
