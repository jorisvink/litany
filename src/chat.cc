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
#include <QEvent>

#include "litany.h"
#include "chat.h"

/*
 * A chat window for either talking to a single peer or multiple peers
 * in a group setting.
 */
Chat::Chat(QJsonObject *config, const char *which, int mode)
{
	u_int8_t	id;
	QWidget		*widget;
	QBoxLayout	*layout;

	PRECOND(config != NULL);
	PRECOND(which != NULL);
	PRECOND(mode == LITANY_CHAT_MODE_DIRECT ||
	    mode == LITANY_CHAT_MODE_GROUP);

	chat_mode = mode;
	memset(tunnels, 0, sizeof(tunnels));

	id = litany_json_number(config, "kek-id", UCHAR_MAX) & 0xff;
	litany_msg_number_reset(id);

	tunnel_config = config;
	kek_id = QString("%1").arg(id, 2, 16, QLatin1Char('0'));

	if (chat_mode == LITANY_CHAT_MODE_DIRECT)
		setWindowTitle(QString("Litany - Chat with %1").arg(which));
	else
		setWindowTitle(QString("Litany - Group %1").arg(which));

	setGeometry(100, 100, 500, 400);
	setStyleSheet("background-color: #101010");

	widget = new QWidget(this);
	layout = new QBoxLayout(QBoxLayout::BottomToTop, widget);

	input = new QLineEdit(widget);
	input->setFixedHeight(25);
	input->setMinimumWidth(400);
	input->setPlaceholderText("Write a message ...");

	input->setStyleSheet("background-color: #808080;");
	input->setStyleSheet("font-size: 24px;");
	input->setStyleSheet("color: #fff;");

	connect(input,
	    &QLineEdit::returnPressed, this, &Chat::create_message);

	model = new QStandardItemModel(this);
	model->insertColumn(0);

	view = new QListView(widget);
	view->setWordWrap(true);
	view->setModel(model);
	view->setMinimumHeight(400);
	view->setMinimumWidth(500);
	view->setSelectionMode(QAbstractItemView::NoSelection);
	view->setEditTriggers(QAbstractItemView::NoEditTriggers);

	view->setStyleSheet("border: 1px solid #353535");

	layout->addWidget(input);
	layout->addWidget(view);
	layout->addStretch();

	setCentralWidget(widget);

	id = QString(which).toUShort(NULL, 16) & 0xff;

	if (chat_mode == LITANY_CHAT_MODE_DIRECT) {
		tunnels[id] = new Tunnel(this, config, id, false);
	} else {
		discovery = new Liturgy(this,
		    config, LITURGY_MODE_DISCOVERY, id);
	}
}

/*
 * Signal for returnPressed() on the line input. We format the message
 * display it locally and send it to all connected peer(s).
 */
void
Chat::create_message(void)
{
	int			i;
	QString			text, full;

	text = input->text();

	if (text.length() > 0 && text.length() < LITANY_MESSAGE_MAX_SIZE) {
		full = QString("<%1> %2").arg(kek_id).arg(text);
		message_show(full.toUtf8().data(),
		    LITANY_MESSAGE_SYSTEM_ID, Qt::white);

		/* XXX */
		for (i = 0; i < KYRKA_PEERS_PER_FLOCK; i++) {
			if (tunnels[i] != NULL) {
				tunnels[i]->send_text(text.toUtf8().data(),
				    text.toUtf8().length());
			}
		}

		input->setText("");
	}
}

/*
 * Add a new message to our model using the given color as long as
 * the message did not yet exist in the model itself.
 */
void
Chat::message_show(const char *msg, u_int64_t id, Qt::GlobalColor color)
{
	QVariant		val;
	qulonglong		qid;
	QModelIndex		index;
	int			i, row;

	PRECOND(msg != NULL);

	row = model->rowCount();

	/*
	 * XXX - can be a lot smarter, but this works for now.
	 * When (if) this actually becomes an issue we can fix it.
	 */
	if (id != LITANY_MESSAGE_SYSTEM_ID) {
		for (i = 0; i < row; i++) {
			index = model->index(i, 0);
			val = model->data(index, Qt::UserRole);
			qid = val.toULongLong();

			if (qid == id)
				return;
		}

		if (this->isActiveWindow() == false)
			app->alert(this);
	}

	qid = id;
	model->insertRow(row);
	index = model->index(row, 0);

	model->setData(index, msg);
	model->setData(index, qid, Qt::UserRole);
	model->setData(index, Qt::AlignLeft, Qt::TextAlignmentRole);
	model->setData(index, QBrush(color), Qt::ForegroundRole);

	view->scrollToBottom();
}

/*
 * A peer might be discovered, check if we need to update its state
 * and potentially stop/start its tunnel.
 */
void
Chat::peer_set_state(u_int8_t id, int state)
{
	if (tunnels[id] == NULL && state == 1) {
		tunnels[id] = new Tunnel(this, tunnel_config, id, true);
	}

	if (tunnels[id] != NULL && state == 0) {
		delete tunnels[id];
		tunnels[id] = NULL;
	}
}

/*
 * Destructor, we cleanup any resources.
 */
Chat::~Chat(void)
{
	int		i;

	delete discovery;

	for (i = 0; i < KYRKA_PEERS_PER_FLOCK; i++) {
		if (tunnels[i] != NULL)
			delete tunnels[i];
	}
}
