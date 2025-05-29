#include <QJsonDocument>
#include <QFile>

#include "configuration.h"

/*
 * Initialize and setup the menubar and action
 */
void
setup_settings_menu(QMainWindow *win, QJsonObject *config) {
	QMenuBar		*menubar;
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
		set_configuration(win, config, settings_action);
	});

	if (config->isEmpty())
		settings_action->trigger();
}

/*
 * Create the configuration dialogue that allows the user to
 * change/set configuration in the json file
 */
void
set_configuration(QMainWindow *win, QJsonObject *config, QAction *menu_item) {
	QString		 path;
	QWidget		*settings_window;
	QGridLayout	*settings_layout;
	QVBoxLayout	*container;
	QHBoxLayout	*row;
	QLabel		*setting_label;
	QLineEdit	*setting_value;
	QPushButton	*apply_btn, *cancel_btn;
	uint		 row_count;
	QMap<const char *, const char *>::const_iterator item, end;
	QMap<const char *, const char *> settings = {
		{"Flock ID", "flock"},
		{"Flock domain", "flock-domain"},
		{"Flock group domain", "flock-domain-group"},
		{"Kek ID", "kek-id"},
		{"Kek path", "kek-path"},
		{"Cathedral ID", "cs-id"},
		{"Cathedral ID key", "cs-path"},
		{"Cathedral IP/Port", "cathedral"}
	};

	PRECOND(config != NULL);

	if (config_file == NULL) {
		path = QStandardPaths::writableLocation(
				   QStandardPaths::AppDataLocation) + "/config.json";
	} else {
		path = config_file;
	}

	settings_layout = new QGridLayout;
	for (item = settings.cbegin(), end = settings.cend();
		 item != end; ++item) {
		setting_label = new QLabel(item.key());
		setting_label->setAlignment(Qt::AlignLeft);

		if (!config->isEmpty())
			setting_value = new QLineEdit(litany_json_string(config,
														 item.value()
														 ));
		else
			setting_value = new QLineEdit();

		setting_value->setAlignment(Qt::AlignLeft);
		setting_value->setObjectName(item.value());

		row_count = settings_layout->rowCount();
		settings_layout->addWidget(setting_label, row_count, LABEL_COLUMN);
		settings_layout->addWidget(setting_value, row_count, VALUE_COLUMN);
	}

	apply_btn = new QPushButton("Save");
	cancel_btn = new QPushButton("Cancel");

	row = new QHBoxLayout();
	row->addWidget(apply_btn);
	row->addWidget(cancel_btn);

	container = new QVBoxLayout();
	container->addLayout(settings_layout);
	container->addLayout(row);

	settings_window = new QWidget;
	settings_window->setMinimumWidth(500);
	settings_window->setLayout(container);
	settings_window->setWindowModality(Qt::WindowModal);

	QObject::connect(cancel_btn, &QPushButton::clicked, [=]() {
		settings_window->close();
	});
	QObject::connect(apply_btn, &QPushButton::clicked, [=]() {
		apply_settings(settings_window,
					settings.values(),
					path,
					(LitanyWindow *)win,
					menu_item);
		settings_window->close();
	});

	settings_window->show();
}

/*
 *
 */
void
apply_settings(QWidget *settings_window,
			QList<const char *> settings_labels,
			QString path,
			LitanyWindow *win,
			QAction *menu_item) {
	QJsonDocument json = QJsonDocument();
	QJsonObject *settings;
	QFile output(path);
	QList<const char *>::const_iterator item, end;

	settings = new QJsonObject();
	for (item = settings_labels.cbegin(), end = settings_labels.cend();
			item != end; ++item) {
		QLineEdit *value = settings_window->findChild<QLineEdit *>(*item);
		settings->insert(*item, value->text());
	}

	json.setObject(*settings);
	if (output.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
		if (output.write(json.toJson())) {
			win->initialize_liturgies(settings);
			menu_item->disconnect(win);
			QObject::connect(menu_item, &QAction::triggered, win, [=](){
				set_configuration(win, settings, menu_item);
			});
		} else {
			fatal("Couldn't write to output file '%s': %s",
				output.fileName().toStdString().c_str(),
				output.errorString().toStdString().c_str());
		}
		output.close();
	} else {
		fatal("Couldn't open output file for writing: %s",
			output.errorString().toStdString().c_str());
	}
}
