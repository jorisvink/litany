# Litany

This is an end-to-end encrypted and peer-to-peer chat program
using the sanctum protocol as its transport layer.

For details on how the underlying tunnels works see
<a href="https://github.com/jorisvink/sanctum/blob/master/docs/crypto.md">docs/crypto.md</a> in the sanctum repository.

## Features

Litany supports having one-to-one or group conversations. The litany
establishes a sanctum tunnel for each peer in a conversation, meaning
group conversations have multiple active tunnels.

## Usage

Double click on an online peer in the list to open its chat window
(spawned as a separate process). Your peer will be signaled via the
sanctum protocol that someone is trying to chat with them. They have
to also open your chat window before any tunnel is able to be established.

Groups can be joined via the JOIN GROUP button and input field. In groups
each member has their own tunnel to each other participant in the group.

You can join group conversations or create direct chat windows without
being seen as "online" in Litany by starting it directly from the terminal:

Establish a chat window to peer 0xf:

```
$ litany chat 0f
```

Join the 0xcafebabe group chat:

```
$ litany group cafebabe
```

## Limitations & traffic analysis.

Messages are limited 512 bytes.

This is because litany will send full-sized message frames for
each message that is sent regardless of the length of the plaintext
message.

## Building

Litany builds on Linux, OpenBSD, MacOS and for Windows.

You need Qt6 and
<a href="https://github.com/jorisvink/libkyrka">libkyrka</a>to be able
to build Litany.

```
$ qmake qt/litany.pro
$ make -j
```

If you want to build on Windows, you need a mingw toolchain on your
Linux machine and cross compile it:

```
$ env KYRKA=/path/to/libkyrka/<target> \
    /path/to/mingw-toolchain/qt6/bin/qmake6 qt/litany.pro CONFIG+=windows
$ make -j
```

## Configuration

When you start litany for the first time without any configuration
present it will prompt you to fill in the settings via a dialog
window.

You can also specify this configuration from the command-line when
starting Litany using the **-c** flag:

```
$ litany -c $HOME/.litany.json
```

This JSON based configuration contains the relevant information
for your terminal.

```
{
    "flock": "493abf95a07e0c00",
    "kek-id": "0d",
    "flock-domain": "0b",
    "flock-domain-group": "0c",
    "kek-path": "493abf95a07e0c00-0x0d/kek-0x0d",
    "cs-id": "e365d227",
    "cs-path": "493abf95a07e0c00-0x0d/id-e365d227",
    "cathedral": "ip:port"
}
```

## Screenshots

<img src="images/litany01.png">
<img src="images/litany03.png">
<img src="images/litany02.png">
