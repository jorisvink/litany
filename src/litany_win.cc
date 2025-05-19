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

#include "litany_qt.h"

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
	last_notification = 0;
}

/*
 * Enable or disable the notification when another peer is trying to reach us.
 */
void
LitanyPeer::show_notification(int onoff)
{
	struct timespec		ts;

	PRECOND(onoff == 0 || onoff == 1);

	if (proc) {
		setForeground(Qt::green);
		setText(QString("Peer %1 (chat open)").arg(peer_id));
	} else {
		if (onoff) {
			(void)clock_gettime(CLOCK_MONOTONIC, &ts);
			if ((ts.tv_sec - last_notification) >= 5) {
				if (litany->alert.isPlaying() == false)
					litany->alert.play();
				last_notification = ts.tv_sec;
			}
			setForeground(Qt::yellow);
			setText(QString("Peer %1 (chat pending)").arg(peer_id));
			if (litany->isActiveWindow() == false)
				app->alert(litany);
		} else {
			setForeground(Qt::gray);
			setText(QString("Peer %1").arg(peer_id));
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
	struct timespec		ts;

	PRECOND(proc != NULL);
	(void)exit_status;

	delete proc;
	proc = NULL;

	(void)clock_gettime(CLOCK_MONOTONIC, &ts);
	last_notification = ts.tv_sec;

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

/*
 * Constructor for the LitanyWindow class.
 * We setup the UI elements here and prepare the online and offline lists.
 */
LitanyWindow::LitanyWindow(QJsonObject *config)
{
	QLabel			*label;
	QWidget			*widget;
	QBoxLayout		*layout;

	PRECOND(config != NULL);

	setFixedWidth(300);
	setMinimumHeight(600);

	setWindowTitle("Litany");
	setStyleSheet("background-color: #101010");

	widget = new QWidget(this);
	layout = new QBoxLayout(QBoxLayout::TopToBottom, widget);

	online = new QListWidget(widget);
	offline = new QListWidget(widget);

	online->setStyleSheet("border: 1px solid #202020;");
	online->setStyleSheet("color: white");
	offline->setStyleSheet("border: 1px solid #202020;");
	offline->setStyleSheet("color: gray");

	label = new QLabel(widget);
	label->setText("Online");
	label->setStyleSheet("color: white");
	layout->addWidget(label);
	layout->addWidget(online);

	label = new QLabel(widget);
	label->setText("Offline");
	label->setStyleSheet("color: white");
	layout->addWidget(label);
	layout->addWidget(offline);

	alert.setSource(QUrl::fromLocalFile(
	    QString("%1/sounds/incoming.wav").arg(LITANY_SHARE_DIR)));

	for (int i = 1; i < 256; i++) {
		peers[i] = new LitanyPeer(this, i);
		peers[i]->setText(QString("Peer %1").arg(i));
		offline->addItem(peers[i]);
	}

	connect(online,
	    &QListWidget::itemDoubleClicked, this, &LitanyWindow::chat_open);

	setCentralWidget(widget);

	signaling = new Liturgy(this, config, LITURGY_MODE_SIGNAL);
	discovery = new Liturgy(this, config, LITURGY_MODE_DISCOVERY);
}

/*
 * Cleanup resources owned by our litany window.
 */
LitanyWindow::~LitanyWindow(void)
{
	delete discovery;
	delete signaling;
}

/*
 * Called from a LitanyPeer, we forward the signaling state to our
 * underlying signaling liturgy.
 */
void
LitanyWindow::signaling_state(u_int8_t peer, int onoff)
{
	signaling->signaling_state(peer, onoff);
}

/*
 * Called from liturgy, updates the online/offline peer lists based on
 * what the liturgy told us.
 */
void
LitanyWindow::peer_set_state(u_int8_t id, int is_online)
{
	PRECOND(id > 0);
	PRECOND(is_online == 1 || is_online == 0);

	if (is_online) {
		if (peers[id]->online == false) {
			offline->takeItem(offline->row(peers[id]));
			online->insertItem(id - 1, peers[id]);
			peers[id]->online = true;
		}
	} else {
		if (peers[id]->online == true) {
			online->takeItem(online->row(peers[id]));
			offline->insertItem(id - 1, peers[id]);
			peers[id]->online = false;
		}
	}
}

/*
 * We received a signaling event from a peer, we record this and
 * place an exclamation mark next to the peer.
 */
void
LitanyWindow::peer_set_notification(u_int8_t peer_id, int onoff)
{
	LitanyPeer	*peer;

	PRECOND(onoff == 0 || onoff == 1);

	peer = peers[peer_id];
	peer->show_notification(onoff);
}

/*
 * Open the peer its chat window and set the signaling state for it
 * so our peer can open its window too.
 */
void
LitanyWindow::chat_open(QListWidgetItem *item)
{
	LitanyPeer	*peer;

	PRECOND(item != NULL);

	peer = (LitanyPeer *)item;
	peer->chat_open();

	signaling->signaling_state(peer->peer_id, 1);
}
