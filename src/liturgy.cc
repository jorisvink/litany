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

#include <QJsonValue>

#include <stdio.h>

#include "litany.h"

static void	kyrka_event(KYRKA *, union kyrka_event *, void *);
static void	cathedral_send(const void *, size_t, u_int64_t, void *);

/*
 * The functions the classes that inherit from LiturgyInterface
 * must re-implement. We just fatal in these to draw attention to it.
 */
void
LiturgyInterface::peer_set_state(u_int8_t id, int state)
{
	(void)id;
	(void)state;

	fatal("LiturgyInterface::peer_set_state not overridden");
}

void
LiturgyInterface::peer_set_notification(u_int8_t id, int state)
{
	(void)id;
	(void)state;

	fatal("LiturgyInterface::peer_set_notification not overridden");
}

/*
 * Setup a liturgy with the given parameters.
 *
 * This is entirely self-contained, multiple liturgies could technically
 * be made with different peer configurations.
 *
 * The JSON config should contain the following:
 *	flock flock-domain kek-id cs-id cs-path cathedral:port
 *
 * If group is non 0, it will look at flock-domain-group instead of the
 * flock-domain parameter.
 *
 * The liturgy runs either in discovery mode or signaling mode depending
 * on the mode given to the constructor.
 */
Liturgy::Liturgy(LiturgyInterface *parent, QJsonObject *config,
    int mode, u_int16_t group)
{
	bool					ok;
	struct kyrka_cathedral_cfg		cfg;
	QJsonValue				val;
	char					*path;

	PRECOND(parent != NULL);
	PRECOND(config != NULL);
	PRECOND(mode == LITURGY_MODE_DISCOVERY || mode == LITURGY_MODE_SIGNAL);

	if ((kyrka = kyrka_ctx_alloc(kyrka_event, this)) == NULL)
		fatal("failed to create kyrka event");

	memset(&cfg, 0, sizeof(cfg));

	cfg.udata = this;
	cfg.send = cathedral_send;

	cfg.flock = litany_json_number(config, "flock", ULLONG_MAX);
	if (cfg.flock & 0xff)
		fatal("flock invalid (contains domain bits)");

	cfg.tunnel = litany_json_number(config, "kek-id", UCHAR_MAX);
	cfg.identity = litany_json_number(config, "cs-id", UINT_MAX);

	if (group) {
		cfg.group = group;
		cfg.flock |= litany_json_number(config,
		    "flock-domain-group", UCHAR_MAX);
	} else {
		cfg.flock |= litany_json_number(config,
		    "flock-domain", UCHAR_MAX);
		if (mode == LITURGY_MODE_DISCOVERY)
			cfg.group = USHRT_MAX;
	}

	path = litany_json_string(config, "cs-path");
	cfg.secret = path;

	val = config->value("cathedral");
	if (val.type() != QJsonValue::String)
		fatal("no or invalid catheral found in configuration");

	port = val.toString().split(":")[1].toInt(&ok, 10);
	if (!ok) {
		fatal("invalid port in %s",
		    val.toString().toStdString().c_str());
	}

	address.setAddress(val.toString().split(":")[0]);

	if (kyrka_cathedral_config(kyrka, &cfg) == -1)
		fatal("kyrka_cathedral_config: %d", kyrka_last_error(kyrka));

	free(path);
	runmode = mode;
	owner = parent;

	memset(signaling, 0, sizeof(signaling));

	socket.bind(QHostAddress::AnyIPv4);
	notify.setInterval(2500);

	connect(&notify, &QTimer::timeout, this, &Liturgy::liturgy_send);
	connect(&socket, &QUdpSocket::readyRead, this, &Liturgy::packet_read);

	liturgy_send();
	notify.start();
}

/*
 * Cleanup any and all resources.
 */
Liturgy::~Liturgy(void)
{
	kyrka_ctx_free(kyrka);
}

/*
 * Set the signaling state for a given peer.
 */
void
Liturgy::signaling_state(u_int8_t peer, int onoff)
{
	PRECOND(runmode == LITURGY_MODE_SIGNAL);
	PRECOND(onoff == 0 || onoff == 1);

	signaling[peer] = onoff;
}

/*
 * Callback for our notify QTimer that fires every few seconds. From here
 * we trigger the sending of a cathedral liturgy packet.
 */
void
Liturgy::liturgy_send(void)
{
	size_t			len;
	u_int8_t		*ptr;

	if (runmode == LITURGY_MODE_SIGNAL) {
		ptr = signaling;
		len = sizeof(signaling);
	} else {
		len = 0;
		ptr = NULL;
	}

	if (kyrka_cathedral_liturgy(kyrka, ptr, len) == -1)
		fatal("kyrka_cathedral_liturgy: %d", kyrka_last_error(kyrka));
}

/*
 * Qt calls this when a packet can be read from the underlying udp socket.
 * We read it and feed it into libkyrka which will handle the rest.
 */
void
Liturgy::packet_read(void)
{
	qint64		len;
	char		packet[1500];

	if ((len = socket.readDatagram(packet, sizeof(packet))) == -1) {
		printf("failed to read packet: %d\n", socket.error());
		return;
	}

	if (kyrka_purgatory_input(kyrka, packet, len) == -1)
		fatal("kyrka_purgatory_input: %d", kyrka_last_error(kyrka));
}

/*
 * Send a datagram using the underlying udp socket.
 */
void
Liturgy::socket_send(const void *data, size_t len)
{
	if (socket.writeDatagram((const char *)data, len, address, port) == -1)
		printf("failed to write to cathedral: %d\n", socket.error());
}

/*
 * Called when a new libkyrka event triggers.
 */
static void
kyrka_event(KYRKA *ctx, union kyrka_event *evt, void *udata)
{
	u_int8_t		idx;
	LiturgyInterface	*owner;
	Liturgy			*liturgy;

	PRECOND(ctx != NULL);
	PRECOND(evt != NULL);
	PRECOND(udata != NULL);

	liturgy = (Liturgy *)udata;
	owner = (LiturgyInterface *)liturgy->owner;

	switch (evt->type) {
	case KYRKA_EVENT_LITURGY_RECEIVED:
		if (liturgy->runmode == LITURGY_MODE_DISCOVERY) {
			for (idx = 1; idx < KYRKA_PEERS_PER_FLOCK; idx++) {
				owner->peer_set_state(idx,
				    evt->liturgy.peers[idx]);
			}
		} else {
			for (idx = 1; idx < KYRKA_PEERS_PER_FLOCK; idx++) {
				owner->peer_set_notification(idx,
				    evt->liturgy.peers[idx]);
			}
		}
		break;
	default:
		printf("got a libkyrka event %u\n", evt->type);
		break;
	}
}

/*
 * Called when libkyrka wants to send a cathedral message.
 */
static void
cathedral_send(const void *data, size_t len, u_int64_t magic, void *udata)
{
	Liturgy		*liturgy;

	PRECOND(data != NULL);
	PRECOND(udata != NULL);

	(void)magic;
	liturgy = (Liturgy *)udata;

	liturgy->socket_send(data, len);
}
