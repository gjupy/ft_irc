# ft_irc
### from [@chris-bbrs](https://github.com/chris-bbrs) and [@gjupy](https://github.com/gjupy)<br>

Simple IRC Server based on the RFC (mainly 2812) written in C++.

It does not support server-to-server communication. You can use it with **netcat** (for now).

# Run

Do `make` and then run :

```bash
./ircserv <port> <password>
```
## Handled commands

This following list of commands are handled on our server:

```
- INVITE
- JOIN
- KICK
- TOPIC
- MODE
- NICK
- PRIVMSG
```

## Handled modes :

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

# DOCS

- [RFC2811](https://datatracker.ietf.org/doc/html/rfc2811)
- [RFC2812](https://datatracker.ietf.org/doc/html/rfc2812)
- [RFC2813](https://datatracker.ietf.org/doc/html/rfc2813)
