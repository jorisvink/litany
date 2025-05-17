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

#include <QBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QApplication>

#include "litany_qt.h"

/*
 * A litany peer, these are added to either the online or offline
 * QListWidget. This exists so we can obtain the peer id in a way
 * that made sense to me.
 */
LitanyPeer::LitanyPeer(u_int8_t id)
{
	peer_id = id;
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

	for (int i = 1; i < 256; i++) {
		peers[i] = new LitanyPeer(i);
		peers[i]->setText(QString("Peer %1").arg(i));
		offline->addItem(peers[i]);
	}

	connect(online,
	    &QListWidget::itemDoubleClicked, this, &LitanyWindow::chat_open);

	setCentralWidget(widget);
	discovery = new Liturgy(config);
}

LitanyWindow::~LitanyWindow(void)
{
	delete discovery;
}

/*
 * Called from liturgy, updates the online/offline peer lists based on
 * what the liturgy told us.
 */
void
LitanyWindow::peer_set_state(u_int8_t id, int is_online)
{
	QString				str;
	QListWidgetItem			*item;
	QList<QListWidgetItem *>	items;

	PRECOND(id > 0);
	PRECOND(is_online == 1 || is_online == 0);

	str = QString("Peer %1").arg(id);

	if (is_online) {
		items = offline->findItems(str, Qt::MatchExactly);
		for (auto *it: items) {
			item = offline->takeItem(offline->row(it));
			online->insertItem(id - 1, item);
		}
	} else {
		items = online->findItems(str, Qt::MatchExactly);
		for (auto *it: items) {
			item = online->takeItem(online->row(it));
			offline->insertItem(id - 1, item);
		}
	}
}

void
LitanyWindow::chat_open(QListWidgetItem *item)
{
	QString			id;
	QStringList		nargs;
	QProcess		*proc;
	LitanyPeer		*peer;
	const QStringList	args(QApplication::instance()->arguments());

	PRECOND(item != NULL);

	peer = (LitanyPeer *)item;

	if (config_file != NULL) {
		nargs.append("-c");
		nargs.append(QString("%1").arg(config_file));
	}

	id = QString("0x%1").arg(peer->peer_id, 2, 16, QLatin1Char('0'));

	nargs.append("chat");
	nargs.append(id);

	proc = new QProcess(this);

	proc->setProgram(args.at(0));
	proc->setArguments(nargs);

	connect(proc, &QProcess::finished, this, &LitanyWindow::chat_exit);

	/* XXX - how do we free proc afterwards? */
	proc->start();
}

void
LitanyWindow::chat_exit(int exit_code)
{
	printf("something exited, %d\n", exit_code);
}
