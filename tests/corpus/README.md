# Corpus

One input per file, raw bytes, under `tests\corpus\<reader>\<name>.bin`. Files are named after
what they trigger. The files are the only binary content in `tests\`; `.gitattributes` marks
`*.bin` as binary. Inputs are replayed on every run by the `corpus` suite and are also the base
material for the fuzz mutator.

## recvgroup

A sequence of records until end of file. All integers are little endian.

    u16 group, u8 type, u8 src, u8 idx, u8 count, u8 compressed, u16 payloadLen, payload[payloadLen]

The record header is 9 bytes. `payloadLen` must be <= 998; a record that claims more, or one
truncated by end of file, ends the parse. At most 128 records are used and the file is capped at
64 KB. Each record becomes one NetPacket stamped with the header and queued on `inQueue`, then
`NetComm::Recv` is drained for every packet type.

## recvpacket

One raw LAN datagram as it arrives from the socket, the 3 byte LAN prefix implied:

    [type][group lo][group hi][idx][count | 0x80 if compressed] payload...

It is fed through `NetCommP::MakeReceivedPacket` and `NetCommP::RecvPacket`, then drained with `NetComm::Recv`.
Inputs of any length are replayed, including 0 bytes. `empty-datagram`, `short-datagram-3` and `pool-flood` cover the ingress length checks and the receive pool reserve.

## known_bug

Inputs in a `known_bug\` subdirectory trigger a bug that exists today. There are none at the moment; the RecvGroup
infinite-loop inputs (`frag-idx-ge-count`, `frag-classic-gap`) moved up to `recvgroup\` when it was fixed. The plain replay case skips them. Each has its own `known_bug` suite case, which
run.ps1 reports as `[KNOWN]` while it fails or times out and warns about once it passes. When the
bug is fixed, move the file up one directory and delete its `known_bug` case in the same change. A
case whose input is missing exits 3 and reports `[CRASH]`, never `[KNOWN]`.

## NetWire readers

`chat-client`, `chat-server`, `upload`, `rcon`, `load` and `turnrate` hold the bytes after the comm header. `ReplayNetWireBytes` writes the file into a `NetPktGroup` with `WriteBytes`, then reads it back with the matching `NetWire` helper into a canary filled buffer (0xAA) and checks the pool is whole (512 packets).

- `chat-*`: u32 header (`0xffff0000 | length`), then the payload, then a sentinel word. The low 16 bits of the header are the claimed size.
- `upload`: replayed twice per file, once as a chunk (10 bit length in the first word, data from the next word) and once as a name (u8 length then characters).
- `rcon`: a 9 byte header with the command at byte 5, a zero at byte 6, the key at byte 7 and the claimed line length at byte 8, then the line. The whole file also goes through the authorization check.
- `load`: 7 bit player count in the first word. `turnrate`: 10 bit duration then 3 bit turns.

`corpus generate netwire` regenerates the files when `OPENSWBF2_GEN_CORPUS=1`. The `fuzz netwire readers` case mutates these files with its own seeds and pinned checksums.
