/*
 * Copyright (c) 2025 Daniel Kuehn <daniel@kuehn.se>
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

#include <QFile>
#include <QMessageBox>
#include <QJsonDocument>

#include "configuration.h"

static int	apply(QWidget *, QString, LitanyWindow *, QAction *);

/*
 * All fields we have in our settings file and the dialog.
 */
struct {
	const char			*title;
	const char			*field;
	QRegularExpressionValidator	validator;
} fields[] = {
	{
		"Flock ID",
		"flock",
		QRegularExpressionValidator(
		    QRegularExpression("^[0-9a-fA-F]{14}00$")
		),
	},

	{
		"Flock domain",
		"flock-domain",
		QRegularExpressionValidator(
		    QRegularExpression("^[0-9a-fA-F]{2}$")
		),
	},

	{
		"Flock domain group",
		"flock-domain-group",
		QRegularExpressionValidator(
		    QRegularExpression("^[0-9a-fA-F]{2}$")
		),
	},

	{
		"Kek ID",
		"kek-id",
		QRegularExpressionValidator(
		    QRegularExpression("^[0-9a-fA-F]{2}$")
		),
	},

	{
		"Kek file path",
		"kek-path",
		QRegularExpressionValidator(
		    QRegularExpression("^.*$")
		),
	},

	{
		"Cathedral ID",
		"cs-id",
		QRegularExpressionValidator(
		    QRegularExpression("^[0-9a-fA-F]{8}$")
		),
	},

	{
		"Cathedral secret path",
		"cs-path",
		QRegularExpressionValidator(
		    QRegularExpression("^.*$")
		),
	},

	{
		"Cathedral ip:port",
		"cathedral",
		QRegularExpressionValidator(
		    QRegularExpression("^.*$")
		),
	},

	{ NULL, NULL, QRegularExpressionValidator(QRegularExpression()) },
};

/*
 * Setup the menu bar on our main window, create the settings dialog
 * and hook it up to the menu bar click.
 */
void
litany_settings_initialize(QMainWindow *win, QJsonObject *config)
{
	QMenuBar	*menubar;
	QMenu		*preferences;
	QAction		*settings_action;

	settings_action = new QAction("Configuration", win);
	settings_action->setStatusTip("See and change the "
	    "configuration for litany");

	menubar = win->menuBar();
	menubar->setStyleSheet("color: white");

	preferences = menubar->addMenu("Settings");
	preferences->addAction(settings_action);

	QObject::connect(settings_action, &QAction::triggered, win, [=](){
		litany_settings_show(win, config, settings_action);
	});

	if (config->isEmpty())
		settings_action->trigger();
}

/*
 * Creates the dialog for editing the settings, populated from the given
 * configuration. This is the action connected to the menu bar click.
 */
void
litany_settings_show(QMainWindow *win, QJsonObject *config, QAction *menu_item)
{
	int				i;
	char				*val;
	QHBoxLayout			*row;
	QString				path;
	QWidget				*dialog;
	uint				row_count;
	QVBoxLayout			*container;
	QLabel				*setting_label;
	QLineEdit			*setting_value;
	QGridLayout			*settings_layout;
	QPushButton			*apply_btn, *cancel_btn;

	PRECOND(win != NULL);
	PRECOND(config != NULL);
	PRECOND(menu_item != NULL);

	if (config_file == NULL) {
		path = QStandardPaths::writableLocation(
		    QStandardPaths::AppDataLocation) + "/config.json";
	} else {
		path = config_file;
	}

	settings_layout = new QGridLayout();

	for (i = 0; fields[i].title != NULL; i++) {
		setting_label = new QLabel(fields[i].title);
		setting_label->setAlignment(Qt::AlignLeft);

		if (!config->isEmpty()) {
			val = litany_json_string(config, fields[i].field);
			setting_value = new QLineEdit(val);
			free(val);
		} else {
			setting_value = new QLineEdit();
		}

		setting_value->setAlignment(Qt::AlignLeft);
		setting_value->setObjectName(fields[i].field);
		setting_value->setValidator(&fields[i].validator);

		row_count = settings_layout->rowCount();
		settings_layout->addWidget(setting_label,
		    row_count, LABEL_COLUMN);
		settings_layout->addWidget(setting_value,
		    row_count, VALUE_COLUMN);
	}

	apply_btn = new QPushButton("Save");
	cancel_btn = new QPushButton("Cancel");

	row = new QHBoxLayout();
	row->addWidget(apply_btn);
	row->addWidget(cancel_btn);

	container = new QVBoxLayout();
	container->addLayout(settings_layout);
	container->addLayout(row);

	dialog = new QWidget();
	dialog->setMinimumWidth(500);
	dialog->setLayout(container);
	dialog->setWindowModality(Qt::WindowModal);

	QObject::connect(cancel_btn, &QPushButton::clicked, [=]() {
		dialog->close();
		delete dialog;
	});

	QObject::connect(apply_btn, &QPushButton::clicked, [=]() {
		if (apply(dialog, path, (LitanyWindow *)win, menu_item) != -1) {
			dialog->close();
			delete dialog;
		}
	});

	dialog->show();
}

/*
 * Apply the given settings by writing them to the correct config file
 * and re-intializing the litany.
 *
 * Note that this re-connects the triggered signal from the menu bar so
 * we pass the newly created QJsonObject to litany_settings_show().
 */
static int
apply(QWidget *dialog, QString path, LitanyWindow *litany, QAction *menu_item)
{
	int			i;
	QString			opath;
	QJsonDocument		*json;
	QLineEdit		*value;
	QFile			*output;
	QJsonObject		*settings;

	PRECOND(dialog != NULL);
	PRECOND(litany != NULL);
	PRECOND(menu_item != NULL);

	opath = path + ".tmp";
	output = new QFile(opath);
	json = new QJsonDocument();
	settings = new QJsonObject();

	for (i = 0; fields[i].title != NULL; i++) {
		value = dialog->findChild<QLineEdit *>(fields[i].field);

		if (value->hasAcceptableInput() == false) {
			QMessageBox::warning(dialog, fields[i].field,
			    "field value does not satisfy requirements",
			    QMessageBox::Ok);
			return (-1);
		}

		settings->insert(fields[i].field, value->text());
	}

	json->setObject(*settings);

	if (output->open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
		if (output->write(json->toJson())) {
			litany->initialize_liturgies(settings);
			menu_item->disconnect(litany);
			QObject::connect(menu_item,
			    &QAction::triggered, litany, [=](){
				litany_settings_show(litany,
				    settings, menu_item);
			});
		} else {
			fatal("Couldn't write to output file '%s': %s",
			    output->fileName().toStdString().c_str(),
			    output->errorString().toStdString().c_str());
		}

		output->close();

		if (rename(opath.toUtf8().data(), path.toUtf8().data()) == -1) {
			fatal("failed to rename settings file: %s",
			    output->errorString().toStdString().c_str());
		}
	} else {
		fatal("Couldn't open output file for writing: %s",
		    output->errorString().toStdString().c_str());
	}

	delete json;
	delete output;

	return (0);
}
