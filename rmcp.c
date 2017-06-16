/*
 * parse and print rmcp and asf(ping and pong)
 *
 */
#include <stdio.h>
#include <sys/types.h>

#include "dump.h"

/* section 13.6 */
struct rmcp_header {
    u_char      rmcp_v;         /* rmcp version 0x06 asf2.0*/
    u_char      rmcp_reserved;       /* reserved */
    u_char      rmcp_sn;        /* rmcp sequence number 0xff means ipmi*/
    u_char      rmcp_class;     /* payload class */
#define     RMCP_CLASS_ASF      0x06
#define     RMCP_CLASS_IPMI     0x07
}__attribute__ ((packed));

/* section 13.2 */
struct asf_header {
    unsigned int      asf_iana;     /* asf inan enterprise number, should be always 0x000011be */ 
#define     ASF_IANA                    0x000011be
    u_char            asf_mtype;    /* asf message type */
#define     ASF_MESSAGE_TYPE_PING       0x80
#define     ASF_MESSAGE_TYPE_PONG       0x40
    u_char            asf_mtag;     /* asf message tag */
    u_char            asf_reserved; /* asf reserved */
    u_char            asf_dlen;     /* asf data length */
}__attribute__ ((packed));


extern void print_ipmi(const u_char *payload, int payload_len, enum dump_level dl);

static void print_asf(const u_char *payload, int payload_len, enum dump_level dl);

/*
 * parse and print rmcp payload
 *
 * @payload: the raw udp payload(without udp header)
 * @payload_len: the payload valid length
 * @dump_level: to determine what level should be print, rmcp? asf? ipmi?
 *
 */ 

void print_rmcp(const u_char *payload, int payload_len, enum dump_level dl) {

    struct rmcp_header *rmcp_h;

    /* must check whether the payload is a valid rmcp packet */
    if ( payload_len < sizeof(struct rmcp_header) ) {
        fprintf(stderr, "Invalid rmcp: length is too small\n");
        return;
    }

    rmcp_h = (struct rmcp_header *)payload;

    if ( rmcp_h->rmcp_v != 0x06 ) {
        fprintf(stderr, "Invalid rmcp ASF version: only support 2.0(0x06), but got 0x%02x\n", payload[0]);
        return;
    }

    if ( rmcp_h->rmcp_sn != 0xff ) {
        fprintf(stderr, "Invalid rmcp sequence number: only support ipmi(0xff), but got 0x%02x\n", payload[2]);
        return;
    }

    if ( rmcp_h->rmcp_class != RMCP_CLASS_ASF && rmcp_h->rmcp_class != RMCP_CLASS_IPMI ) {
        fprintf(stderr, "Invalid rmcp class: only support ASF(0x%02x) and IPMI(0x%02x), but got 0x%02x\n", RMCP_CLASS_ASF, RMCP_CLASS_IPMI, rmcp_h->rmcp_class);
        return;
    }

    if ( dl <= DL_RMCP ){
        printf("  [RMCP] ASF Version: 2.0\n");
        printf("  [RMCP] SN: IPMI\n");
        printf("  [RMCP] Class: %s(0x%02x)\n", rmcp_h->rmcp_class == RMCP_CLASS_ASF ? "ASF" : "IPMI", rmcp_h->rmcp_class);
    }

    if ( rmcp_h->rmcp_class == RMCP_CLASS_ASF ) {
        print_asf( payload + sizeof(struct rmcp_header) , payload_len - sizeof(struct rmcp_header), dl );
    }
    else {
        print_ipmi( payload + sizeof(struct rmcp_header) , payload_len - sizeof(struct rmcp_header), dl );
    }

}


const char* asf_get_message_type_str(u_char message_type) {
    switch ( message_type ) {
        case ASF_MESSAGE_TYPE_PING:
            return "PING";
        case ASF_MESSAGE_TYPE_PONG:
            return "PONG";
        default:
            return "unknown";
    }
}


/*
 * parse and print asf payload
 *
 * @payload: the raw payload(without rmcp header)
 * @payload_len: the payload valid length
 * @dump_level: to determine what level should be print, rmcp? asf? ipmi?
 *
 */ 
static void print_asf(const u_char *payload, int payload_len, enum dump_level dl){
    struct asf_header *asf_h;

    if ( payload_len < sizeof(struct asf_header) ) {
        fprintf(stderr, "Invalid asf: length is too small\n");
        return;
    }

    asf_h = (struct asf_header *) payload;
    if ( ntohl(asf_h->asf_iana) != ASF_IANA ){
        fprintf(stderr, "Invalid asf IANA which must be 0x000011be\n");
        return;
    }

    if ( asf_h->asf_mtype != ASF_MESSAGE_TYPE_PING 
            && asf_h->asf_mtype != ASF_MESSAGE_TYPE_PONG ) {
       fprintf(stderr, "Invalid asf message type, only support 0x%02x,0x%02x", ASF_MESSAGE_TYPE_PING, ASF_MESSAGE_TYPE_PONG);
    }

    if ( dl <= DL_ASF ) {
        printf("  [ASF] Message Type: %s(0x%02x)\n", asf_get_message_type_str(asf_h->asf_mtype), asf_h->asf_mtype);
        printf("  [ASF] Message Tag: 0x%02x\n", asf_h->asf_mtag);
    }
}

