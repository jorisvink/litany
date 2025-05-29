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

/* Group regex validator. */
static QRegularExpression rgroup("^[0-9a-fA-F]{1,4}$");

/*
 * Constructor for the LitanyWindow class.
 * We setup the UI elements here and prepare the online and offline lists.
 */
LitanyWindow::LitanyWindow(QJsonObject *config)
{
	QString			id;
	QLabel			*label;
	QWidget			*widget;
	QBoxLayout		*layout;
	QPushButton		*button;

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

	label = new QLabel(widget);
	label->setText("Group");
	label->setStyleSheet("color: white");
	layout->addWidget(label);

	group = new QLineEdit(widget);
	group->setStyleSheet("color: white");
	group->setValidator(new QRegularExpressionValidator(rgroup, group));
	layout->addWidget(group);

	button = new QPushButton("JOIN GROUP");
	layout->addWidget(button);
	connect(button, &QPushButton::clicked, this, &LitanyWindow::group_open);

	for (int i = 1; i < 256; i++) {
		id = QString("%1").arg(i, 2, 16, QLatin1Char('0'));
		peers[i] = new LitanyPeer(this, i);
		peers[i]->setText(QString("Peer %1").arg(id));
		offline->addItem(peers[i]);
	}

	connect(online,
	    &QListWidget::itemDoubleClicked, this, &LitanyWindow::chat_open);

	setCentralWidget(widget);

    if (!config->isEmpty()) {
        signaling = new Liturgy(this, config, LITURGY_MODE_SIGNAL, 0);
        discovery = new Liturgy(this, config, LITURGY_MODE_DISCOVERY, 0);
    }
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

/*
 * Open the requested group window.
 */
void
LitanyWindow::group_open(void)
{
	u_int16_t	id;

	id = group->text().toUShort(NULL, 16);
	group->setText("");

	if (groups[id] == NULL)
		groups[id] = new LitanyGroup(this, id);

	groups[id]->chat_open();
}
