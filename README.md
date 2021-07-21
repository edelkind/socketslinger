# Socketslinger

Socketslinger is a 2-part network server model.

When a sling input/listener process receives a socket or file descriptor, it passes the descriptor to a Unix domain socket using `sendmsg(2)`.  The receiver ("catcher"), when called, receives the descriptor from the Unix domain socket and passes it to a processor program.


## Concept

There are two types of slingers.

One listens on a port*:
```
sling-listen 12.34.56.78 8888

sling-catch [processor]
```


One operates on stdin/stdout and can, for example, be invoked via `socat`*:
```
socat TCP4-LISTEN:12.34.56.78:8888 EXEC:sling-input

sling-catch [processor]
```

The latter is the version currently implemented.  Some features are currently Linux-specific, but they can be ported to other operating systems that support descriptor passing via Unix domain socket.

> \* _Note, the above are not the final/usable commands.  They are simplified for illustrative purposes._


## Details

There are some things that you can do with this aside from serving a Frankensteinian network service.  As one example, you can combine it with `tmux` to build a reverse shell manager for security testing:

```bash
socat TCP4-LISTEN:12.34.56.78:13337,fork,reuseaddr EXEC:sling-input,nofork &

sling-wait |while read sock; do
  tmux new-window -- sling-catch -c -s "$sock" -- socat FD:5 STDIO
done
```

Let's break down what happens in this configuration.

```bash
socat TCP4-LISTEN:12.34.56.78:13337,fork,reuseaddr EXEC:sling-input,nofork &
```

`socat` runs as a network service.  When a new connection arrives, `socat` forks and passes the network descriptor to `sling-input`, as descriptor 0 (duped as necessary to mimic console stdio).  `socat` does not fork further to execute `sling-input`.  This is important because we need to ensure that socat doesn't close the descriptor when `sling-input` exits.

`sling-input` creates a Unix domain socket and listens for a connection.  Since a new process is spawned for each connection, a different Unix domain socket needs to be created for every connection as well (in the hypothetical/unimplemented `sling-listen` scenario above, the same Unix domain socket can be reused, which would be slightly more efficient).  The `sling-input` process will block until another process connects to the Unix domain socket.  In the meantime, `socat` can continue to serve additional connections and spawn new `sling-input` processes.

```bash
sling-wait |while read sock; do
  tmux new-window -- sling-catch -c -s "$sock" -- socat FD:5 STDIO
done
```

`sling-wait` cycles through any existing sockets in the runtime directory, outputting their names one per line, then it watches for new sockets using inotify.  Notification delay is minimal (measured in nanoseconds on most remotely modern hardware), so in the process above, the biggest inefficiencies occur in cycling through the shell loop and spawning the subprocesses.

In the new `tmux` window (a `tmux` server must already be running), `sling-catch` connects to the supplied Unix domain socket, and `sling-input` passes the descriptor, deletes the Unix domain socket, and exits immediately (without closing the descriptor).

At this point, on the listener end, the only thing running is the one `socat` listener process.

`sling-catch` executes `socat` (since the hypothetical scenario where `sling-catch` implements built-in stdio redirection hasn't been implemented) without forking, and `socat` then utilizes the descriptor and relays content to/from stdio for display in `tmux`.

At this point, on the catcher end, the only thing running is one `socat` process (plus `sling-wait` and our shell loop).

For one connection, the number of persistent processes is 5:
- 1 `socat` TCP listener
- 1 `sling-wait` filesystem watcher
- 1 shell loop
- 1 `tmux` server
- 1 `socat` communication processor

For 51 connections, the number of persistent processes is 55:
- 1 `socat` TCP listener
- 1 `sling-wait` filesystem watcher
- 1 shell loop
- 1 `tmux` server
- 51 `socat` communication processors

More details on how each of these processes work are laid out in [FLOW.txt](FLOW.txt).

Keep in mind, once `sling-input` exits, `socat` no longer manages spawned processes.  This means, if you want to impose limits to avoid DoS, you need to implement them on the catcher side (e.g. in the shell loop).  It also means that you get full, scriptable control of how to implement such protections.
