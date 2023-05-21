# ft_irc
### from [@chris-bbrs](https://github.com/chris-bbrs) and [@gjupy](https://github.com/gjupy)<br>

Simple IRC Server based on the RFC (mainly 2812) written in C++ and following the C++ 98 standard.

It does not support server-to-server communication.

Although  [`WeeChat`] was used as a reference client during implementation, the server is functional with most clients that sends packets terminated with `\r\n`.

## Run

Do `make` and then run :

```bash
./ircserv <port> <password>
```

Launch  [Weechat]

	brew install weechat
	weechat
Connect to IRC inside weechat

	/server add <name> <IP-address>/<port> -password=<password>
	/connect <name>
    
## Handled commands

This following list of commands are handled on our server:

```
- PASS
- USER
- NICK
- PRIVMSG
- INVITE
- JOIN
- KICK
- TOPIC
- MODE
- CAP
- PING
- QUIT
```

To run the commands in weechat type ``` /command [args] ``` (in low case letters)

## Handled modes

The following list of modes are handled by the server:

```
CHANNEL MODES :
for users :
    - o : channel operator
    - v : voice
for channels :
    - i : invite only
    - t : topic locked
    - k : key locked
    - l : user limit
```

## DOCS

- [RFC2811](https://datatracker.ietf.org/doc/html/rfc2811)
- [RFC2812](https://datatracker.ietf.org/doc/html/rfc2812)
- [RFC2813](https://datatracker.ietf.org/doc/html/rfc2813)
