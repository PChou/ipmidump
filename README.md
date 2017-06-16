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
