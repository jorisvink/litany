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

#ifndef __H_LITANY_GROUP_H
#define __H_LITANY_GROUP_H

#include <QObject>
#include <QProcess>

class LitanyWindow;

/*
 * A group and its accompanying chat window that runs in
 * a separate process.
 */
class LitanyGroup: public QObject {
	Q_OBJECT

public:
	LitanyGroup(LitanyWindow *, u_int16_t);
	~LitanyGroup(void);

	void		chat_open(void);

private slots:
	void		chat_close(int);

private:
	/* The group id we are active in. */
	u_int16_t	group_id;

	/* The group chat window its process, if running. */
	QProcess	*proc;

	/* The litany window under which we reside. */
	LitanyWindow	*litany;
};

#endif
