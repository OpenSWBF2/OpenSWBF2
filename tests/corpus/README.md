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

It is fed through `NetCommP::RecvPacket`, then drained with `NetComm::Recv`.

## known_bug

Inputs in a `known_bug\` subdirectory trigger a bug that exists today (for example the RecvGroup
infinite loop). The plain replay case skips them. Each has its own `known_bug` suite case, which
run.ps1 reports as `[KNOWN]` while it fails or times out and warns about once it passes. When the
bug is fixed, move the file up one directory and delete its `known_bug` case in the same change. A
case whose input is missing exits 3 and reports `[CRASH]`, never `[KNOWN]`.
