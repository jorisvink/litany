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

LitanyGroup::LitanyGroup(LitanyWindow *parent, u_int16_t id)
{
	PRECOND(parent != NULL);

	proc = NULL;
	group_id = id;
	litany = parent;
}

void
LitanyGroup::chat_open(void)
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

	id = QString("0x%1").arg(group_id, 4, 16, QLatin1Char('0'));
	nargs.append("group");
	nargs.append(id);

	proc = new QProcess(this);

	proc->setProgram(args.at(0));
	proc->setArguments(nargs);

	connect(proc, &QProcess::finished, this, &LitanyGroup::chat_close);
	proc->start();
}

void
LitanyGroup::chat_close(int exit_status)
{
	PRECOND(proc != NULL);
	(void)exit_status;

	delete proc;
	proc = NULL;
}

LitanyGroup::~LitanyGroup(void)
{
	if (proc != NULL)
		proc->close();

	delete proc;
}
