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

#define LABEL_COLUMN		0
#define VALUE_COLUMN		1

void	litany_settings_initialize(QMainWindow *, QJsonObject *);
void	litany_settings_show(QMainWindow *, QJsonObject *, QAction *);

#endif
