### UDP receiver

Simple UDP receiver that listens on port 50000 and receives fixed-size UDP datagrams (4096 bytes).

Uses two threads:
- цorker thread - receives UDP datagrams from the socket using `poll()` and `recvfrom()`, then stores them in a ring buffer.
- ьain thread - extracts UDP datagrams from the ring buffer and prints their sequence numbers.

The threads communicate using a mutex and a condition variable.

1. Compile
```
make
```

2. Run
```
./upd_recv
```

Press `Ctrl+C` to stop the receiver gracefully.
