/*
 * parse and print ipmi message
 *
 */
#include <stdio.h>
#include <sys/types.h>

#include "align.h"
#include "dump.h"
#include "ipmi_cmd.h"

#define IPMI_AUTH_CODE_LEN      16


/* section 13.6 RMCP/IPMI 1.5 */
struct ipmi_session_header {
    u_char          ish_auth_type TCC_PACKED; /* ipmi auth type */
#define IPMI_AUTH_TYPE_NONE     0x00 //none
#define IPMI_AUTH_TYPE_MD2     0x01 //md2
#define IPMI_AUTH_TYPE_MD5     0x02 //md5
#define IPMI_AUTH_TYPE_PWD     0x04 //straight password
#define IPMI_AUTH_TYPE_OEM     0x05 //oem
    unsigned int    ish_sn TCC_PACKED;        /* ipmi session sequence number */
    unsigned int    ish_id TCC_PACKED;        /* ipmi session id */
    u_char          ish_auth_code[IPMI_AUTH_CODE_LEN] TCC_PACKED; /* ipmi 16 bytes auth code , not present when auth type is none(0x00) */
} GNU_PACKED;



/* section 13.6 RMCP/IPMI 1.5 */
struct ipmi_payload_header {
    u_char          ipd_len TCC_PACKED;        /* ipmi payload length */
    u_char          ipd_to_addr TCC_PACKED;    /* message send to which addr, for request message, this should be 0x20(indicate BMC); for response message, this should be 0x81(indicate the requestor) */
    u_char          ipd_net_fn TCC_PACKED;     /* network function section 5.1 */
#define NETFN_CHAS   0x00  // chassis
#define NETFN_BRIDGE 0x02  // bridge
#define NETFN_SEVT   0x04  // sensor/event
#define NETFN_APP    0x06  // application
#define NETFN_FW     0x08  // firmware
#define NETFN_STOR   0x0a  // storage
#define NETFN_TRANS  0x0c  // transport
#define NETFN_SOL    0x34  // serial-over-lan (in IPMI 2.0, use TRANS)
#define NETFN_PICMG  0x2c  // for ATCA PICMG systems
    u_char          ipd_chksum1 TCC_PACKED;    /* checksum */
    u_char          ipd_from_addr TCC_PACKED;  /* from address */
    u_char          ipd_req_seq TCC_PACKED;    /* request sequence , the requestor and responsor should match same */
    u_char          ipd_cmd TCC_PACKED;        /* the cmd code under network function  */
} GNU_PACKED;


extern void print_ipmi_session(enum ipmi_direction direction,u_char cmd, const u_char *payload, int payload_len, enum dump_level dl);
extern void print_ipmi_sdr(enum ipmi_direction direction,u_char cmd, const u_char *payload, int payload_len, enum dump_level dl);

const char* ipmi_get_auth_type_str(u_char auth_type) {
    switch ( auth_type ) {
        case IPMI_AUTH_TYPE_NONE:
            return "NONE";
        case IPMI_AUTH_TYPE_MD2:
            return "MD2";
        case IPMI_AUTH_TYPE_MD5:
            return "MD5";
        case IPMI_AUTH_TYPE_PWD:
            return "PWD";
        case IPMI_AUTH_TYPE_OEM:
            return "OEM";
        default:
            return "unknown";
    }
    
}

const char* ipmi_get_network_function_str(u_char nf) {
    switch ( nf ) {
        case NETFN_CHAS:
            return "Chassis";
        case NETFN_BRIDGE:
            return "Bridge";
        case NETFN_SEVT:
            return "Sensor/Event";
        case NETFN_APP:
            return "Application";
        case NETFN_FW:
            return "Firmware";
        case NETFN_STOR:
            return "Storage";
        case NETFN_TRANS:
            return "Transport";
        case NETFN_SOL:
            return "SOL";
        case NETFN_PICMG:
            return "PICMG";
        default:
            return "unknown";
    }
}

const char* ipmi_get_cmd_str(u_char nf, u_char cmd){
    if ( nf == NETFN_APP && cmd == GET_CHAN_AUTH ) {
        return "Get Auth Capability";
    }
    else if ( nf == NETFN_APP && cmd == GET_SESS_CHAL ) {
        return "Get Session Challenge";
    } 
    else if ( nf == NETFN_APP && cmd == ACT_SESSION ) {
        return "Activate Session";
    } 
    else if ( nf == NETFN_APP && cmd == SET_SESS_PRIV ) {
        return "Set Session Privilege";
    } 
    else if ( nf == NETFN_APP && cmd == CLOSE_SESSION ) {
        return "Close Session";
    } 
    else if ( nf == NETFN_STOR && cmd == GET_SDR_REPINFO ) {
        return "Get SDR Repo Info";
    } 
    else if ( nf == NETFN_STOR && cmd == RESERVE_SDR_REP ) {
        return "Reserve SDR Repo";
    } 
    else if ( nf == NETFN_STOR && cmd == GET_SDR ) {
        return "Get SDR";
    } 
    else if ( nf == NETFN_SEVT && cmd == GET_SENSOR_READING ) {
        return "Get Sensor Reading";
    } 
    else if ( nf == NETFN_SEVT && cmd == GET_SENSOR_THRESHOLD ) {
        return "Get Sensor Threshold";
    } 
    else {
        return "TODO";
    }
}

/*
 * parse and print ipmi payload
 *
 * @payload: the raw payload(without rmcp header)
 * @payload_len: the payload valid length
 * @dump_level: to determine what level should be print, rmcp? asf? ipmi?
 *
 */ 
void print_ipmi(const u_char *payload, int payload_len, enum dump_level dl){
    struct ipmi_session_header *ish;
    struct ipmi_payload_header *iph;
    const u_char *ipmi_payload_body, *ipb;
    int actual_header_len = sizeof(struct ipmi_session_header);
    int msg_len = 0;
    int i;
    u_char network_fn = 0;
    enum ipmi_direction direction;

    /* auth code is option */
    if ( payload_len < actual_header_len - IPMI_AUTH_CODE_LEN ) {
        goto small_length;
    }

    ish = (struct ipmi_session_header *) payload;
    if ( ish->ish_auth_type == IPMI_AUTH_TYPE_NONE){
        actual_header_len -= IPMI_AUTH_CODE_LEN;
    }

    if ( payload_len < actual_header_len ) {
        goto small_length;
    }

    if ( dl <= DL_IPMI_HEADER ) {
        printf("  [IPMI] Auth Type(%lu): %s(0x%02x)\n",sizeof(ish->ish_auth_type), ipmi_get_auth_type_str(ish->ish_auth_type), ish->ish_auth_type );
        printf("  [IPMI] Sequence(%lu): %u\n", sizeof(ish->ish_sn), ish->ish_sn);
        printf("  [IPMI] Session(%lu): %u\n", sizeof(ish->ish_id), ish->ish_id);
        if ( ish->ish_auth_type != IPMI_AUTH_TYPE_NONE ) {
            printf("  [IPMI] Auth Code(16 bytes):");
            for (  i = 0 ; i < IPMI_AUTH_CODE_LEN; i++ ) {
                printf(" 0x%02x", ish->ish_auth_code[i]);
            }
            printf("\n");
        }
    }


    if ( payload_len < actual_header_len + 1 ){ /* at least contains a byte for msg length */
        goto small_length;
    }

    iph = (struct ipmi_payload_header *)(payload + actual_header_len);
    msg_len = (int)iph->ipd_len;
    if ( payload_len < actual_header_len + msg_len ){
	/* fprintf(stderr, "actual header len is %d, msglen should be %d, but the payload_len is %d\n",actual_header_len ,(int)msg_len, payload_len); */
        goto small_length;
    }

    network_fn = iph->ipd_net_fn >> 2;
    /* NOTE even network function means request and odd means response */
    if ( network_fn % 2 == 0 ) {
        direction = IPMI_REQUEST;
    } else {
        network_fn = network_fn - 1;
        direction = IPMI_RESPONSE;
    }
    if ( dl <= DL_IPMI_HEADER ){
        if ( direction == IPMI_REQUEST ){
            printf("  [IPMI] Request\n");
        }
        else {
            printf("  [IPMI] Response\n");
        }
        printf("  [IPMI] Message length: %d\n", msg_len);
        printf("  [IPMI] Network Function: %s(0x%02x)\n", ipmi_get_network_function_str(network_fn) ,network_fn);
        printf("  [IPMI] toAddr: 0x%02x, fromAddr: 0x%02x, reqSeq: 0x%02x\n", iph->ipd_to_addr, iph->ipd_from_addr, iph->ipd_req_seq);
    }

    printf("  [IPMI] Cmd: %s(0x%02x)\n", ipmi_get_cmd_str(network_fn, iph->ipd_cmd) ,iph->ipd_cmd);
    ipb = payload + actual_header_len + sizeof(struct ipmi_payload_header);

    if ( network_fn == NETFN_APP && (
                iph->ipd_cmd == GET_CHAN_AUTH ||
                iph->ipd_cmd == GET_SESS_CHAL ||
                iph->ipd_cmd == ACT_SESSION ||
                iph->ipd_cmd == SET_SESS_PRIV ||
                iph->ipd_cmd == CLOSE_SESSION
                )  ) {
        print_ipmi_session(direction ,iph->ipd_cmd , ipb ,msg_len-sizeof(struct ipmi_payload_header)+1  , dl);
    }
    else if ( network_fn == NETFN_STOR && (
                iph->ipd_cmd == GET_SDR_REPINFO ||
                iph->ipd_cmd == RESERVE_SDR_REP ||
                iph->ipd_cmd == GET_SDR 
                ) ){
        print_ipmi_sdr(direction, iph->ipd_cmd, ipb, msg_len-sizeof(struct ipmi_payload_header)+1, dl);
    }
    else if ( network_fn == NETFN_SEVT && (
                iph->ipd_cmd == GET_SENSOR_READING ||
                iph->ipd_cmd == GET_SENSOR_THRESHOLD 
                ) ){
        print_ipmi_sdr(direction, iph->ipd_cmd, ipb, msg_len-sizeof(struct ipmi_payload_header)+1, dl);
    }
    else {
    }

    return;

small_length:
    fprintf(stderr, "Invalid ipmi: length is too small\n");
    return;
}
