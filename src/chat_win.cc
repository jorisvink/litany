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

#include "litany_qt.h"

/*
 * A window that maintains a chat with one peer. We have a sanctum
 * tunnel towards said peer using libkyrka.
 */
LitanyChat::LitanyChat(QJsonObject *config, const char *peer)
{
	QWidget		*widget;
	QBoxLayout	*layout;

	PRECOND(config != NULL);
	PRECOND(peer != NULL);

	setWindowTitle(QString("Litany - Chat with %1").arg(peer));
	setGeometry(100, 100, 500, 400);
	setStyleSheet("background-color: #101010");

	widget = new QWidget(this);
	layout = new QBoxLayout(QBoxLayout::BottomToTop, widget);

	input = new QLineEdit(widget);
	input->setFixedHeight(25);
	input->setMinimumWidth(400);
	input->setPlaceholderText(QString("Message to %1").arg(peer));

	input->setStyleSheet("background-color: #808080;");
	input->setStyleSheet("font-size: 24px;");
	input->setStyleSheet("color: #fff;");

	connect(input,
	    &QLineEdit::returnPressed, this, &LitanyChat::create_message);

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

	message.setSource(QUrl::fromLocalFile(
	    QString("%1/sounds/pending.wav").arg(LITANY_SHARE_DIR)));

	tunnel = new Tunnel(config, peer, this);

	message_show("[exchange]: no keys",
	    LITANY_MESSAGE_SYSTEM_ID, Qt::yellow);
}

/*
 * Signal for returnPressed() on the line input. We format the message
 * display it locally and send it to our peer over the tunnel.
 */
void
LitanyChat::create_message(void)
{
	QString			text, full;

	text = input->text();

	if (text.length() > 0 && text.length() < LITANY_MESSAGE_MAX_SIZE) {
		full = QString(">> %1").arg(text);
		message_show(full.toUtf8().data(),
		    LITANY_MESSAGE_SYSTEM_ID, Qt::white);
		tunnel->send_text(text.toUtf8().data(), text.toUtf8().length());
		input->setText("");
	}
}

/*
 * Add a new message to our model using the given color as long as
 * the message did not yet exist in the model itself.
 */
void
LitanyChat::message_show(const char *msg, u_int64_t id, Qt::GlobalColor color)
{
	QVariant	val;
	qulonglong	qid;
	QModelIndex	index;
	int		i, row;

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

			if (qid == id) {
				printf("already seen %lu\n", id);
				return;
			}
		}

		if (this->isActiveWindow() == false) {
			if (message.isPlaying() == false)
				message.play();
			app->alert(this);
		}
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
 * Destructor, we cleanup any resources.
 */
LitanyChat::~LitanyChat(void)
{
	delete tunnel;
}
