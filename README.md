# Litany

Essentially E2EE and P2P chat using the sanctum protocol, wrapped
in a Qt application because people dislike my terminal approach to
everything.

You can have one-to-one conversations or group conversations. With
group conversations you have tunnels to each participant in the group.

This is a WIP and the UI is wonky, I am not a GUI person.

Double click on a peer to open its chat window (spawned
as a separate process). Your peer will be signaled via
the sanctum protocol that someone is trying to chat with
them. They have to also open your chat window before any
tunnel is able to be established.

Groups can be joined via the JOIN GROUP button and input field. In groups
each member has their own tunnel to each other participant in the group.

Messages are limited to 512 bytes each.

Messages are resent if a peer does not ACK a message after 5 seconds.

## Building

Litany builds on Linux, OpenBSD, MacOS and for Windows.
You need Qt6 and libkyrka with its dependencies installed.

```
$ qmake qt/litany.pro
$ make -j
```

If you want to build on Windows, you need a mingw toolchain on your
Linux machine and cross compile it:

```
$ /path/to/mingw-toolchain/qt6/bin/qmake6 qt/litany.pro CONFIG+=windows
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
