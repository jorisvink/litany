#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QStandardPaths>
#include <QMenuBar>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QMainWindow>
#include <QLineEdit>

#include "litany.h"

#define LABEL_COLUMN 0
#define VALUE_COLUMN 1

void apply_settings(QWidget *, QList<const char *>, QString);
void set_configuration(QJsonObject *);
void setup_settings_menu(QMainWindow *, QJsonObject *);


#endif // CONFIGURATION_H
