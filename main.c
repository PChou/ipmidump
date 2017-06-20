#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pcap.h>

#include "dump.h"


#define ETHER_ADDR_LEN      6

/* ethernet header */
#define SIZE_ETHERNET       14
struct sniff_ethernet {
    u_char  ether_dhost[ETHER_ADDR_LEN];
    u_char  ether_shost[ETHER_ADDR_LEN];
    u_short ether_type;
};


/* loopback  header */
#define SIZE_LOOPBACK   4
struct sniff_loopback {
    unsigned short  ul; /* underline protocol */
};


/* IP header */
struct sniff_ip {
    u_char  ip_vhl;     /* version and header len */
    u_char  ip_tos;     /* type of service */
    u_short ip_len;     /* ip packet len  */
    u_short ip_id;     /* identification */
    u_short ip_off;     /* fragment offset flag */
#define IP_RF   0x8000
#define IP_DF   0x4000
#define IP_MF   0x2000
#define IP_OFFMASK   0x1fff
    u_char ip_ttl;      /* ttl */
    u_char ip_p;        /* protocol  */
    u_short ip_sum;     /* checksum of header */
    struct in_addr  ip_src, ip_dst; /* source and dest address  */
};

/* UDP header  */
struct sniff_udp {
    u_short uh_sport;       /* udp header source port  */
    u_short uh_dport;       /* udp header destination port  */
    u_short uh_len;         /* udp len */
    u_short uh_checksum;    /* udp checksum */
};

#define     IP_HL(ip)       (((ip)->ip_vhl) & 0x0f)
#define     IP_V(ip)       (((ip)->ip_vhl) >> 4)


extern void print_rmcp(const u_char *payload, int payload_len, enum dump_level dl);


static int DL;

/*
 * print data in rows of 16 bytes: offset   hex   ascii
 *
 * 00000   47 45 54 20 2f 20 48 54  54 50 2f 31 2e 31 0d 0a   GET / HTTP/1.1..
 */
void
print_hex_ascii_line(const u_char *payload, int len, int offset)
{

    int i;
    int gap;
    const u_char *ch;

    /* offset */
    printf("%05d   ", offset);
    
    /* hex */
    ch = payload;
    for(i = 0; i < len; i++) {
        printf("%02x ", *ch);
        ch++;
        /* print extra space after 8th byte for visual aid */
        if (i == 7)
            printf(" ");
    }
    /* print space to handle line less than 8 bytes */
    if (len < 8)
        printf(" ");
    
    /* fill hex gap with spaces if not full line */
    if (len < 16) {
        gap = 16 - len;
        for (i = 0; i < gap; i++) {
            printf("   ");
        }
    }
    printf("   ");
    
    /* ascii (if printable) */
    ch = payload;
    for(i = 0; i < len; i++) {
        if (isprint(*ch))
            printf("%c", *ch);
        else
            printf(".");
        ch++;
    }

    printf("\n");

    return;
}

/*
 * print packet payload data (avoid printing binary data)
 */
void print_payload(const u_char *payload, int len) {

    int len_rem = len;
    int line_width = 16;            /* number of bytes per line */
    int line_len;
    int offset = 0;                 /* zero-based offset counter */
    const u_char *ch = payload;

    if (len <= 0)
        return;

    /* data fits on one line */
    if (len <= line_width) {
        print_hex_ascii_line(ch, len, offset);
        return;
    }

    /* data spans multiple lines */
    for ( ;; ) {
        /* compute current line length */
        line_len = line_width % len_rem;
        /* print line */
        print_hex_ascii_line(ch, line_len, offset);
        /* compute total remaining */
        len_rem = len_rem - line_len;
        /* shift pointer to remaining bytes to print */
        ch = ch + line_len;
        /* add offset */
        offset = offset + line_width;
        /* check if we have line width chars or less */
        if (len_rem <= line_width) {
            /* print last line and get out */
            print_hex_ascii_line(ch, len_rem, offset);
            break;
        }
    }

    return;
}

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet){
    const struct sniff_ethernet *ethernet;
    const struct sniff_loopback *loopback;
    const struct sniff_ip       *ip;
    const struct sniff_udp      *udp;
    const u_char                *payload;

    int size_ip, payload_len;

    if ( DL == DLT_NULL ) {
        /* loopback */
        loopback = (struct sniff_loopback *) packet;
        ip = (struct sniff_ip *)(packet + SIZE_LOOPBACK);
    }
    else {
        ethernet = (struct sniff_ethernet *)(packet);
        ip = (struct sniff_ip *)(packet + SIZE_ETHERNET);
    }

    size_ip = IP_HL(ip)*4;
    if ( size_ip < 20 ){
        return;
    }

    switch( ip->ip_p ) {
        case IPPROTO_UDP:
            //printf("    Protocol: UDP\n");
            break;
        default:
            //printf("    Protocol: %d\n", ip->ip_p);
            return;
    }


    udp = (struct sniff_udp *)((u_char *)ip + size_ip);
    payload = (u_char *)udp + sizeof(struct sniff_udp);
    payload_len = ntohs(udp->uh_len) - sizeof(struct sniff_udp);

    printf("[UDP] %s:%d -> %s:%d, PL:%d\n", inet_ntoa(ip->ip_src), ntohs(udp->uh_sport), inet_ntoa(ip->ip_dst), ntohs(udp->uh_dport), payload_len );
    print_payload(payload, payload_len);

    print_rmcp(payload, payload_len, 0);
    
}

void usage(){
    fprintf(stderr, "IPMI dump, Usage:\n");
    fprintf(stderr, "  ipmidump [-i interface] -e filter\n");
    fprintf(stderr, "  -i interface: specify a interface to dump, if empty default interface will be used\n");
    fprintf(stderr, "  -e filter: filter express like tcpdump\n");
}

int main(int argc, char *argv[]) {

    char filter[1024];
    char dev[128], errbuf[PCAP_ERRBUF_SIZE];
    char *lookupdev;
    pcap_t *handle;
    struct bpf_program fp;
    bpf_u_int32 mask;
    bpf_u_int32 net;
    struct pcap_pkthdr header;
    const u_char *packet;

    int ch, invalid=0;
    memset(dev,0, sizeof(dev));
    memset(filter,0, sizeof(filter));

    while( (ch = getopt(argc, argv, "e:i:") ) != -1) {
        switch( ch ){
            case 'i':
                if ( optarg != NULL ){
                    strcpy(dev, optarg);
                }
                break;
            case 'e':
                if ( optarg != NULL ){
                    strcpy(filter, optarg);
                }
                break;
            case '?':
                invalid=1;
        }
    }

    if ( invalid ){
        fprintf(stderr, "invalid opt: %c\n", optopt);
        usage();
        return (2);
    }

    if( strcmp(dev, "") == 0 ){
        /* no dev specify using default */
        lookupdev  = pcap_lookupdev(errbuf);

        if ( lookupdev == NULL ){
            fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
            return (2);
        }

        strcpy(dev, lookupdev);
    }


    printf("Sniffing device: %s\n", dev);

    if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
        fprintf(stderr, "Can't get netmask for device %s\n", dev);
        net = 0;
        mask = 0;
    }

    handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if ( handle == NULL ){
        fprintf(stderr, "Couldn't open device %s:%s\n",dev, errbuf);
        return (2);
    }

    DL = pcap_datalink(handle);

    if (DL != DLT_EN10MB && DL != DLT_NULL) {
        fprintf(stderr, "Only support Ethernet or Loopback datalink, but %d supplied\n", DL);
        return(2);
    }

    if ( pcap_compile(handle, &fp, filter, 0, net) == -1 ){
        fprintf(stderr, "Couldn't parse filter %s: %s\n",filter, pcap_geterr(handle));
        return (2);
    }

    if ( pcap_setfilter(handle, &fp) == -1 ){
        fprintf(stderr, "Couldn't install filter %s: %s\n",filter,  pcap_geterr(handle));
        return (2);
    }

    pcap_loop(handle, -1, got_packet, NULL);

    pcap_freecode(&fp);
    pcap_close(handle);


    return (0);

}
