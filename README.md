# IPMI dump

IPMI(Intelligent Platform Management Interface) dump is a tool to sniff the local network traffic to catch ipmi packet(UDP) and dump human readable info. Tipically, the tool is used to learn and debug ipmi protocol.

# Dependency

- libpcap( contributor must install the devel version also )
- glibc > 2.12


# Build

```
make
make clean
```

# Use

```
./ipmidump -i lo0 -e udp
```

```
ipmidump [-i interface] -e filter
  -i interface: specify a interface to dump, if empty default interface will be used
  -e filter: filter express like tcpdump
```

# Sample Output

```
[UDP] 127.0.0.1:64772 -> 127.0.0.1:52795, PL:12
00000   06 00 ff 06 00 00 11 be  80 00 00 00                ............
  [RMCP] ASF Version: 2.0
  [RMCP] SN: IPMI
  [RMCP] Class: ASF(0x06)
  [ASF] Message Type: PING(0x80)
  [ASF] Message Tag: 0x00
[UDP] 127.0.0.1:52795 -> 127.0.0.1:64772, PL:28
00000   06 00 ff 06 00 00 11 be  40 00 00 00 00 00 11 be    ........@.......
00016   00 00 00 00 81 00 00 00  00 00 00 00                ............
  [RMCP] ASF Version: 2.0
  [RMCP] SN: IPMI
  [RMCP] Class: ASF(0x06)
  [ASF] Message Type: PONG(0x40)
  [ASF] Message Tag: 0x00
[UDP] 127.0.0.1:64772 -> 127.0.0.1:52795, PL:23
00000   06 00 ff 07 00 00 00 00  00 00 00 00 00 09 20 18    .............. .
00016   c8 81 04 38 0e 04 31                                ...8..1
  [RMCP] ASF Version: 2.0
  [RMCP] SN: IPMI
  [RMCP] Class: IPMI(0x07)
  [IPMI] Auth Type: NONE(0x00)
  [IPMI] Sequence: 0
  [IPMI] Session: 0
  [IPMI] Request
  [IPMI] Message length: 9
  [IPMI] Network Function: Application(0x06)
  [IPMI] toAddr: 0x20, fromAddr: 0x81, reqSeq: 0x04
  [IPMI] Cmd: Get Auth Capability(0x38)
  [IPMI] Channel Number: 0x0e
  [IPMI] Privilege: Admin Level(0x04)
[UDP] 127.0.0.1:52795 -> 127.0.0.1:64772, PL:29
00000   06 00 ff 07 00 00 00 00  00 00 00 00 00 0f 20 1c    .............. .
00016   c4 81 04 38 00 01 15 00  00 00 00 00 2d             ...8........-
  [RMCP] ASF Version: 2.0
  [RMCP] SN: IPMI
  [RMCP] Class: IPMI(0x07)
  [IPMI] Auth Type: NONE(0x00)
  [IPMI] Sequence: 0
  [IPMI] Session: 0
  [IPMI] Response
  [IPMI] Message length: 15
  [IPMI] Network Function: Application(0x06)
  [IPMI] toAddr: 0x20, fromAddr: 0x81, reqSeq: 0x04
  [IPMI] Cmd: Get Auth Capability(0x38)
  [IPMI] Completion Code: 0x00
  [IPMI] Channel Number: 0x01
  [IPMI] Authentication Support: (0x15)NONE MD5 PWD 
  [IPMI] Authentication Method: 0x00
```
