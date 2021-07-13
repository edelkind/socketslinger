Socketslinger is a 2-part network server model.

When a sling input/listener process receives a socket or file descriptor, it passes the descriptor to a Unix domain socket (`~/.sling-socket` by default) using sendmsg(2).  The receiver ("catcher"), when called, receives the descriptor from the Unix domain socket and passes it to a processor program as stdin, stdout, and stderr.


There are two types of slingers.

One listens on a port:
```
sling-listen 12.34.56.78 8888

sling-catch <processor>
```


One operates on stdin/stdout and can, for example, be invoked via `socat`:
```
socat TCP4-LISTEN:12.34.56.78:8888 EXEC:sling-input

sling-catch [processor]
```


There are some things that you can do with this.  As one example, you can
combine it with tmux to build a reverse shell manager.

socat TCP4-LISTEN:12.34.56.78:13337,fork,reuseaddr EXEC:sling-input &

while sling-wait; do
  tmux new-window sling-catch
done
