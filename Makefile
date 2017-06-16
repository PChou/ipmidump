TARGET=ipmidump
CC=cc
CFLAGS=`pcap-config --cflags`
LIBS=`pcap-config --libs`


$(TARGET): main.c rmcp.c ipmi.c ipmi_session.c ipmi_sdr.c
	cc -o $(TARGET) $(CFLAGS) main.c rmcp.c ipmi.c ipmi_session.c ipmi_sdr.c $(LIBS)

.PHONY:

clean:
	rm -f *.o $(TARGET)
