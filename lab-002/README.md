### UDP transmitter

Simple UDP transmitter that sends fixed-size UDP datagrams (4096 bytes) to `127.0.0.1:50000` at 100 ms intervals.

The number of datagrams can be configured using `DGRAM_COUNT`.

1. Compile
```
make
```

2. Run
```
./upd_send
```