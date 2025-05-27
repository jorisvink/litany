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

#ifndef __H_LITANY_PEER_H
#define __H_LITANY_PEER_H

#include <QObject>
#include <QProcess>
#include <QListWidgetItem>

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
	/* The chat window its process, if running. */
	QProcess	*proc;

	/* The litany window under which we reside. */
	LitanyWindow	*litany;
};

#endif
