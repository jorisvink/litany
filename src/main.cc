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

#include "litany.h"
#include "configuration.h"

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
	QJsonObject		*config;
	char			**nargv;
	int			ch, ret, nargc, mode;

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
        qDebug() << "After config load";
		if (nargc == 0) {
            qDebug() << "Creating litany window";
			win = new LitanyWindow(config);
		} else if (nargc == 2) {
			if (!strcmp(nargv[0], "chat")) {
				mode = LITANY_CHAT_MODE_DIRECT;
			} else if (!strcmp(nargv[0], "group")) {
				mode = LITANY_CHAT_MODE_GROUP;
			} else {
				fatal("unknown mode '%s'", nargv[0]);
			}
			win = new Chat(config, nargv[1], mode);
		} else {
			fatal("invalid usage with %d arguments", argc);
		}

        setup_settings_menu(win, config);

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

/*
 * Returns a JSON string field as a C-string.
 * The caller must free the returned result.
 */
char *
litany_json_string(QJsonObject *json, const char *field)
{
	QJsonValue	val;
	char		*value;

	PRECOND(json != NULL);
	PRECOND(field != NULL);

	val = json->value(field);
	if (val.type() != QJsonValue::String)
		fatal("no or invalid '%s' found in configuration", field);

	if ((value = strdup(val.toString().toStdString().c_str())) == NULL)
		fatal("strdup failed");

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
    QJsonDocument		cfg;
	QJsonObject		json;
	QJsonParseError		error;
    QString			dir, path;
    QFile           config_path;

    if (config_file == NULL) {
        dir = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);

        if (mkdir(dir.toUtf8().data(), 0700) == -1 && errno != EEXIST)
            fatal("mkdir: %d", errno);

        config_path.setFileName(dir + "/config.json");
    }

    if (config_path.exists()) {
        if (config_path.open(QFile::ReadOnly | QFile::Text)) {
            cfg = QJsonDocument::fromJson(config_path.readAll(), &error);
            qDebug() << cfg;
            if (error.error == QJsonParseError::NoError) {
                if (cfg.isObject()) {
                    return (new QJsonObject(cfg.object()));
                } else {
                    fatal("Expecting a json object");
                }
            } else {
                fatal("json error at %d: %s", error.offset,
                      error.errorString().toStdString().c_str());
            }
        }
    } else {
        return (new QJsonObject());
    }

    /*
     * This shouldn't be reachable
     */
    return NULL;
}
