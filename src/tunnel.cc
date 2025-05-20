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

#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "litany_qt.h"

static void	kyrka_event(KYRKA *, union kyrka_event *, void *);
static void	heaven_send(const void *, size_t, u_int64_t, void *);
static void	purgatory_send(const void *, size_t, u_int64_t, void *);
static void	cathedral_send(const void *, size_t, u_int64_t, void *);

/*
 * Setup a tunnel to the given target.
 *
 * This is entirely self-contained, multiple tunnel objects
 * can live in unison together with each other.
 *
 * The JSON config should contain the following:
 *	flock kek-id kek-path cs-id cs-path cathedral:port
 */
Tunnel::Tunnel(QJsonObject *config, const char *peer, QObject *obj)
{
	bool					ok;
	struct kyrka_cathedral_cfg		cfg;
	QJsonValue				val;
	QString					peer_id;
	char					*kek_path, *cs_path;

	PRECOND(config != NULL);
	PRECOND(peer != NULL);
	PRECOND(obj != NULL);

	if ((kyrka = kyrka_ctx_alloc(kyrka_event, this)) == NULL)
		fatal("failed to create kyrka event");

	memset(&cfg, 0, sizeof(cfg));

	owner = obj;
	peer_id = peer;

	cfg.udata = this;
	cfg.send = cathedral_send;

	val = config->take("flock");
	if (val.type() != QJsonValue::String)
		fatal("no or invalid flock found in configuration");

	cfg.flock = val.toString().toULongLong(&ok, 16);
	if (!ok)
		fatal("invalid flock %s", val.toString().toStdString().c_str());

	val = config->take("kek-id");
	if (val.type() != QJsonValue::String)
		fatal("no or invalid kek-id found in configuration");

	cfg.tunnel = (val.toString().toUShort(&ok, 16) << 8);
	if (!ok) {
		fatal("invalid kek-id %s",
		    val.toString().toStdString().c_str());
	}

	cfg.tunnel |= peer_id.toUShort(&ok, 16);
	if (!ok)
		fatal("invalid peer-id %s", peer);

	val = config->take("kek-path");
	if (val.type() != QJsonValue::String)
		fatal("no or invalid kek-path found in configuration");

	if ((kek_path = strdup(val.toString().toUtf8().data())) == NULL)
		fatal("strdup failed");

	cfg.kek = kek_path;

	val = config->take("cs-id");
	if (val.type() != QJsonValue::String)
		fatal("no or invalid cs-id found in configuration");

	cfg.identity = val.toString().toULong(&ok, 16);
	if (!ok)
		fatal("invalid cs-id %s", val.toString().toStdString().c_str());

	val = config->take("cs-path");
	if (val.type() != QJsonValue::String)
		fatal("no or invalid cs-path found in configuration");

	if ((cs_path = strdup(val.toString().toUtf8().data())) == NULL)
		fatal("strdup failed");

	cfg.secret = cs_path;

	val = config->take("cathedral");
	if (val.type() != QJsonValue::String)
		fatal("no or invalid catheral found in configuration");

	cathedral_port = val.toString().split(":")[1].toInt(&ok, 10);
	if (!ok) {
		fatal("invalid port in %s",
		    val.toString().toStdString().c_str());
	}

	cathedral_address.setAddress(val.toString().split(":")[0]);

	peer_port = cathedral_port;
	peer_address = cathedral_address;

	if (kyrka_heaven_ifc(kyrka, heaven_send, this) == -1)
		fatal("kyrka_heaven_ifc: %d", kyrka_last_error(kyrka));

	if (kyrka_purgatory_ifc(kyrka, purgatory_send, this) == -1)
		fatal("kyrka_purgatory_send: %d", kyrka_last_error(kyrka));

	if (kyrka_cathedral_config(kyrka, &cfg) == -1)
		fatal("kyrka_cathedral_config: %d", kyrka_last_error(kyrka));

	free(cs_path);
	free(kek_path);

	socket.bind(QHostAddress::AnyIPv4);

	last_notify = 0;
	last_update = 0;
	last_heartbeat = 0;
	TAILQ_INIT(&msgs);

	flush.setInterval(1000);
	manager.setInterval(500);

	connect(&manager, &QTimer::timeout, this, &Tunnel::manage);
	connect(&flush, &QTimer::timeout, this, &Tunnel::resend_pending);
	connect(&socket, &QUdpSocket::readyRead, this, &Tunnel::packet_read);

	last_notify = 0;

	flush.start();
	manager.start();

	system_msg("[cathedral]: address %s:%u",
	    cathedral_address.toString().toUtf8().data(), cathedral_port);
}

/*
 * Tunnel object went away, make sure we clear the kyrka context.
 */
Tunnel::~Tunnel(void)
{
	struct litany_msg	*msg;

	while ((msg = TAILQ_FIRST(&msgs)) != NULL) {
		TAILQ_REMOVE(&msgs, msg, list);
		free(msg);
	}

	kyrka_ctx_free(kyrka);
}

/*
 * Manage the tunnel by periodically sending a cathedral notification
 * or making forward progress on our keying.
 * 
 * We also send heartbeat packets every second to our peer, this helps
 * facilitate holepunching and to detect if a peer has gone "offline".
 */
void
Tunnel::manage(void)
{
	struct timespec		ts;

	(void)clock_gettime(CLOCK_MONOTONIC, &ts);

	if (kyrka_key_manage(kyrka) == -1 &&
	    kyrka_last_error(kyrka) != KYRKA_ERROR_NO_SECRET)
		fatal("kyrka_key_manage: %d", kyrka_last_error(kyrka));

	if ((ts.tv_sec - last_notify) >= 5) {
		last_notify = ts.tv_sec;

		if (kyrka_cathedral_notify(kyrka) == -1) {
			fatal("kyrka_cathedral_notify: %d",
			    kyrka_last_error(kyrka));
		}

		if (kyrka_cathedral_nat_detection(kyrka) == -1) {
			fatal("kyrka_cathedral_nat_detection: %d",
			    kyrka_last_error(kyrka));
		}
	}

	if ((ts.tv_sec - last_heartbeat) >= 1) {
		last_heartbeat = ts.tv_sec;
		send_heartbeat();
	}

	if (last_update != 0 && (ts.tv_sec - last_update) >= 10) {
		system_msg("[peer]: offline (peer closed window or timeout)");
		last_update = 0;
	}
}

/*
 * Qt calls this when a packet can be read from the underlying udp socket.
 * We read it and feed it into libkyrka which will handle the rest.
 */
void
Tunnel::packet_read(void)
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
 * This is only exposed due to the libkyrka callbacks needing
 * access to the socket in the Tunnel class.
 */
void
Tunnel::socket_send(const void *data, size_t len, int is_cathedral, int is_nat)
{
	QHostAddress		ip;
	quint16			dport;

	PRECOND(data != NULL);
	PRECOND(len > 0);
	PRECOND(is_cathedral == 0 || is_cathedral == 1);
	PRECOND(is_nat == 0 || is_nat == 1);

	if (is_cathedral) {
		ip = cathedral_address;
		dport = cathedral_port;
		if (is_nat)
			dport++;
	} else {
		ip = peer_address;
		dport = peer_port;
	}

	if (socket.writeDatagram((const char *)data, len, ip, dport) == -1)
		printf("failed to write to socket: %d\n", socket.error());
}

/*
 * Send a text packet to our peer.
 */
void
Tunnel::send_text(const void *data, size_t len)
{
	struct litany_msg	*msg;

	PRECOND(data != NULL);
	PRECOND(len > 0 && len < LITANY_MESSAGE_MAX_SIZE);

	msg = litany_msg_register(&msgs, data, len);
	send_msg(&msg->data);
}

/*
 * Send an ack for the given id to our peer.
 */
void
Tunnel::send_ack(u_int64_t id)
{
	struct litany_msg_data		data;

	PRECOND(id != LITANY_MESSAGE_SYSTEM_ID);

	memset(&data, 0, sizeof(data));

	data.id = htobe64(id);
	data.type = LITANY_MESSAGE_TYPE_ACK;

	send_msg(&data);
}

/*
 * Send a simple heartbeat to our peer.
 */
void
Tunnel::send_heartbeat(void)
{
	struct litany_msg_data		data;

	memset(&data, 0, sizeof(data));

	data.id = ULONG_MAX;
	data.type = LITANY_MESSAGE_TYPE_HEARTBEAT;

	send_msg(&data);
}

/*
 * Submit a message to our peer, if we are required to record it for
 * delivery we do so and we will retransmit it after a few seconds
 * unless we received an ack for it.
 */
void
Tunnel::send_msg(struct litany_msg_data *msg)
{
	PRECOND(msg != NULL);

	if (kyrka_heaven_input(kyrka, msg, sizeof(*msg)) == -1 &&
	    kyrka_last_error(kyrka) != KYRKA_ERROR_NO_TX_KEY)
		fatal("kyrka_heaven_input: %d", kyrka_last_error(kyrka));
}

/*
 * We received a message, place it in a buffer and pass it along to
 * the chat window which will add the message if it was not already seen.
 */
void
Tunnel::recv_msg(Qt::GlobalColor color, u_int64_t id, const char *fmt, ...)
{
	int		len;
	va_list		args;
	LitanyChat	*chat;
	char		buf[2048];

	PRECOND(fmt != NULL);
	PRECOND(id != LITANY_MESSAGE_SYSTEM_ID);

	va_start(args, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (len == -1 || (size_t)len >= sizeof(buf))
		fatal("message did not fit");

	chat = (LitanyChat *)owner;
	chat->message_show(buf, id, color);
}

/*
 * We received an ack from our peer, forward it to the chat window
 * so it can remove it from the list of messages that needs to be
 * sent still.
 */
void
Tunnel::recv_ack(u_int64_t id)
{
	PRECOND(id != LITANY_MESSAGE_SYSTEM_ID);

	litany_msg_ack(&msgs, id);
}

/*
 * Send pending messages to our peer again if they are old enough to
 * warrant a new send. Any message in the msgs list is not ACK'd by
 * the peer.
 */
void
Tunnel::resend_pending(void)
{
	struct timespec		ts;
	struct litany_msg	*msg;

	(void)clock_gettime(CLOCK_MONOTONIC, &ts);

	TAILQ_FOREACH(msg, &msgs, list) {
		if ((ts.tv_sec - msg->age) >= 5) {
			msg->age = ts.tv_sec;
			send_msg(&msg->data);
		}
	}
}

/*
 * Potentially update the peer its ip address and port.
 */
void
Tunnel::peer_update(struct kyrka_event_peer *peer)
{
	struct in_addr		in;

	PRECOND(peer != NULL);

	in.s_addr = peer->ip;
	peer->ip = be32toh(peer->ip);
	peer->port = be16toh(peer->port);

	if (peer->ip != peer_address.toIPv4Address() ||
	    peer->port != peer_port) {
		system_msg("[p2p]: peer address %s:%u",
		    inet_ntoa(in), peer->port);

		peer_port = peer->port;
		peer_address = QHostAddress(peer->ip);
	}
}

/*
 * Update the peer its last_update timestamp.
 */
void
Tunnel::peer_alive(void)
{
	struct timespec		ts;

	(void)clock_gettime(CLOCK_MONOTONIC, &ts);
	last_update = ts.tv_sec;
}

/*
 * Called when a new libkyrka event triggers.
 */
static void
kyrka_event(KYRKA *ctx, union kyrka_event *evt, void *udata)
{
	Tunnel		*tunnel;

	PRECOND(ctx != NULL);
	PRECOND(evt != NULL);
	PRECOND(udata != NULL);

	tunnel = (Tunnel *)udata;

	switch (evt->type) {
	case KYRKA_EVENT_KEYS_INFO:
		tunnel->peer_alive();
		tunnel->system_msg("[tunnel]: tx=%08x rx=%08x (peer=%llx)",
		    evt->keys.tx_spi, evt->keys.rx_spi,
		    evt->keys.peer_id);
		if (evt->keys.tx_spi != 0 && evt->keys.rx_spi != 0)
			tunnel->system_msg("[tunnel]: established");
		break;
	case KYRKA_EVENT_EXCHANGE_INFO:
		tunnel->system_msg("[exchange]: %s", evt->exchange.reason);
		break;
	case KYRKA_EVENT_AMBRY_RECEIVED:
		tunnel->system_msg("[cathedral]: got ambry 0x%08x",
		    evt->ambry.generation);
		break;
	case KYRKA_EVENT_PEER_DISCOVERY:
		tunnel->peer_update(&evt->peer);
		break;
	default:
		printf("event 0x%02x\n", evt->type);
		break;
	}
}

/*
 * Log a system message to our chat window.
 */
void
Tunnel::system_msg(const char *fmt, ...)
{
	int		len;
	va_list		args;
	LitanyChat	*chat;
	char		buf[2048];

	PRECOND(fmt != NULL);

	va_start(args, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (len == -1 || (size_t)len >= sizeof(buf))
		fatal("message did not fit");

	chat = (LitanyChat *)owner;
	chat->message_show(buf, LITANY_MESSAGE_SYSTEM_ID, Qt::yellow);
}

/*
 * Called when libkyrka makes a decrypted litany_msg packet available to us.
 */
static void
heaven_send(const void *data, size_t len, u_int64_t seq, void *udata)
{
	struct litany_msg_data	*msg;
	Tunnel			*tunnel;

	PRECOND(data != NULL);
	PRECOND(len > 0);
	PRECOND(seq != 0);
	PRECOND(udata != NULL);

	if (len != sizeof(*msg)) {
		printf("malformed packet (%zu, vs %zu)\n", len, sizeof(*msg));
		return;
	}

	msg = (struct litany_msg_data *)data;
	msg->len = be16toh(msg->len);
	msg->id = be64toh(msg->id);

	if (msg->id == LITANY_MESSAGE_SYSTEM_ID) {
		printf("peer tried sending system message\n");
		return;
	}

	if (msg->len > sizeof(msg->data)) {
		printf("bad length detected (%u)\n", msg->len);
		return;
	}

	tunnel = (Tunnel *)udata;
	tunnel->peer_alive();

	switch (msg->type) {
	case LITANY_MESSAGE_TYPE_TEXT:
		tunnel->recv_msg(Qt::gray, msg->id, "<< %.*s",
		    (int)msg->len, (const char *)msg->data);
		tunnel->send_ack(msg->id);
		break;
	case LITANY_MESSAGE_TYPE_ACK:
		tunnel->recv_ack(msg->id);
		break;
	case LITANY_MESSAGE_TYPE_HEARTBEAT:
		break;
	default:
		printf("unknown packet %u\n", msg->type);
	}
}

/*
 * Called when libkyrka gives us ciphertext to send.
 */
static void
purgatory_send(const void *data, size_t len, u_int64_t seq, void *udata)
{
	Tunnel		*tunnel;

	PRECOND(data != NULL);
	PRECOND(len > 0);
	PRECOND(udata != NULL);

	(void)seq;

	tunnel = (Tunnel *)udata;
	tunnel->socket_send(data, len, 0, 0);
}

/*
 * Called when libkyrka wants to send a cathedral message.
 */
static void
cathedral_send(const void *data, size_t len, u_int64_t magic, void *udata)
{
	int		is_nat;
	Tunnel		*tunnel;

	PRECOND(data != NULL);
	PRECOND(len > 0);
	PRECOND(magic == KYRKA_CATHEDRAL_MAGIC ||
	    magic == KYRKA_CATHEDRAL_NAT_MAGIC);

	PRECOND(udata != NULL);

	if (magic == KYRKA_CATHEDRAL_NAT_MAGIC)
		is_nat = true;
	else
		is_nat = false;

	tunnel = (Tunnel *)udata;
	tunnel->socket_send(data, len, 1, is_nat);
}
