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

#include <sys/types.h>
#include <sys/stat.h>

#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

#include "litany_qt.h"

static QJsonObject		*config_load(void);

/* The global application. */
QApplication	*app = NULL;

/*
 * The path to the given configuration file (-c) if any.
 */
const char	*config_file = NULL;

int
main(int argc, char *argv[])
{
	QMainWindow		*win;
	LitanyChat		*chat;
	QJsonObject		*config;
	char			**nargv;
	LitanyWindow		*litany;
	int			ch, ret, nargc;

	config_file = NULL;
	app = new QApplication(argc, argv);

	while ((ch = getopt(argc, argv, "c:")) != -1) {
		switch (ch) {
		case 'c':
			config_file = optarg;
			break;
		default:
			fatal("unknown option '%c'", ch);
		}
	}

	nargc = argc - optind;
	nargv = argv + optind;

	try {
		config = config_load();

		if (nargc == 0) {
			litany = new LitanyWindow(config);
			win = (QMainWindow *)litany;
		} else if (nargc == 2) {
			chat = new LitanyChat(config, nargv[1]);
			win = (QMainWindow *)chat;
		} else {
			fatal("invalid usage with %d arguments", argc);
		}

		win->show();
		ret = app->exec();

		delete config;
		delete win;
	} catch (const std::exception &e) {
		printf("exception of sorts\n");
		ret = 1;
	}

	delete app;

	return (ret);
}

/*
 * Helper function to convert a JSON string field to a native uint64.
 */
u_int64_t
litany_json_number(QJsonObject *json, const char *field, u_int64_t max)
{
	bool		ok;
	QJsonValue	val;
	u_int64_t	value;

	PRECOND(json != NULL);
	PRECOND(field != NULL);

	val = json->value(field);
	if (val.type() != QJsonValue::String)
		fatal("no or invalid '%s' found in configuration", field);

	value = val.toString().toULongLong(&ok, 16);
	if (!ok) {
		fatal("invalid %s: %s",
		    field, val.toString().toStdString().c_str());
	}

	if (value > max)
		fatal("%s out of range", field);

	return (value);
}

/* Bad juju happened. */
void
fatal(const char *fmt, ...)
{
	va_list		args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");

	exit(1);
}

/*
 * Load and parse the JSON configuration file, if present.
 */
static QJsonObject *
config_load(void)
{
	int			fd;
	struct stat		st;
	ssize_t			ret;
	QJsonDocument		cfg;
	QJsonObject		json;
	QJsonParseError		error;
	QString			dir, path;
	char			buf[2048];

	if (config_file == NULL) {
		dir = QStandardPaths::writableLocation(
		    QStandardPaths::AppDataLocation);

		if (mkdir(dir.toUtf8().data(), 0700) == -1 && errno != EEXIST)
			fatal("mkdir: %d", errno);

		path = dir + "/config.json";
	} else {
		path = config_file;
	}

	if ((fd = open(path.toUtf8().data(), O_RDONLY)) == -1)
		fatal("cannot open '%s'", path.toUtf8().data());

	if (fstat(fd, &st) == -1)
		fatal("fstat(): %d", errno);

	if ((size_t)st.st_size > sizeof(buf))
		fatal("json contents of configuration too large");

	for (;;) {
		if ((ret = read(fd, buf, sizeof(buf))) == -1) {
			if (errno == EINTR)
				continue;
			fatal("read: %d", errno);
		}

		if (ret != st.st_size)
			fatal("partial read on configuration file");

		break;
	}

	(void)close(fd);

	QByteArray data(buf, ret);
	cfg = QJsonDocument::fromJson(data, &error);

	if (error.error != QJsonParseError::NoError) {
		fatal("json error at %d: %s", error.offset,
		    error.errorString().toStdString().c_str());
	}

	if (cfg.isObject() == false)
		fatal("expecting a json object");

	return (new QJsonObject(cfg.object()));
}
