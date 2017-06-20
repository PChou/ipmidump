/*
 * parse and print ipmi session message
 * ipmi session kick off after the asf ping and pong
 * ipmi session message includes:
 * - get auth capability
 * - get session challenge
 * - activate session
 * - set session privilege level
 * - close session
 */
#include <stdio.h>
#include <sys/types.h>

#include "dump.h"
#include "ipmi_cmd.h"


/* section 22.13 */
struct ipmi_get_auth_cap_request {
    u_char          ch_num;
    u_char          priviege;
    u_char          checksum;
}__attribute__ ((packed));

struct ipmi_get_auth_cap_response {
    u_char          cc;
    u_char          ch_num;
    u_char          auth_cap;
    u_char          auth_method;
    u_char          reserved;
    struct {
        u_char      oem1;
        u_char      oem2;
        u_char      oem3;
    } oem;
    u_char          oem_aux;
}__attribute__ ((packed));

/* section 22.16 */
struct ipmi_get_sess_chal_request {
    u_char          auth_type;
    char            name[16];
}__attribute__ ((packed));

/* section 22.16 */
struct ipmi_get_sess_chal_response {
    u_char          cc;
    unsigned int    sid;        /* temp session id  */
    u_char          chal_str[16];
}__attribute__ ((packed));


/* section 22.17 */
struct ipmi_act_sess_request {
    u_char          auth_type;
    u_char          priviege;
    u_char          chal_str[16];
    unsigned int    ob_seq;     /* outbound sequence number */
};

/* section 22.17 */
struct ipmi_act_sess_response {
    u_char          cc;
    u_char          auth_type;
    unsigned int    sid;        /* reminder session id  */
    unsigned int    ib_seq;     /* inbound sequence number */
    u_char          priviege;
}__attribute__ ((packed));


/* section 22.18 */
struct ipmi_set_sess_priv_request {
    u_char          priviege;
}__attribute__ ((packed));

/* section 22.18 */
struct ipmi_set_sess_priv_response {
    u_char          cc;
    u_char          priviege;
}__attribute__ ((packed));


/* section 22.19 */
struct ipmi_close_sess_request {
    unsigned int    sid;
}__attribute__ ((packed));

struct ipmi_close_sess_response {
    u_char          cc;
}__attribute__ ((packed));

const char* get_ipmi_priviege(u_char priviege){
    switch ( priviege ){
        case 0x01:
            return "Callback Level";
        case 0x02:
            return "User Level";
        case 0x03:
            return "Operator Level";
        case 0x04:
            return "Admin Level";
        case 0x05:
            return "OEM Level";
        default:
            return "unknown";
    }
}

const char* get_ipmi_auth_type_str(u_char auth_type){
    switch ( auth_type ){
        case 0x00:
            return "NONE";
        case 0x01:
            return "MD2";
        case 0x02:
            return "MD5";
        case 0x03:
            return "Reserved";
        case 0x04:
            return "PWD";
        case 0x05:
            return "OEM";
        default:
            return "unknown";
    }
}

void print_ipmi_auth_cap(u_char auth_cap){
#define __NONE   (1 << 0)
#define __MD2    (1 << 1) 
#define __MD5    (1 << 2)
#define __RES    (1 << 3)
#define __PWD    (1 << 4)
#define __OEM    (1 << 5)

    if ( auth_cap & __NONE ) {
        printf("NONE ");
    }
    if ( auth_cap & __MD2 ) {
        printf("MD2 ");
    }
    if ( auth_cap & __MD5 ) {
        printf("MD5 ");
    }
    if ( auth_cap & __PWD ) {
        printf("PWD ");
    }
    if ( auth_cap & __OEM ) {
        printf("OEM ");
    }
}

/* TODO: check the payload length validation */
void print_ipmi_session(enum ipmi_direction direction,u_char cmd, const u_char *payload, int payload_len, enum dump_level dl) {
    if ( cmd ==  GET_CHAN_AUTH){
        if ( direction == IPMI_REQUEST ){
            struct ipmi_get_auth_cap_request *request = (struct ipmi_get_auth_cap_request *) payload;
            printf("  [IPMI] Channel Number: 0x%02x\n", request->ch_num);
            printf("  [IPMI] Privilege: %s(0x%02x)\n", get_ipmi_priviege(request->priviege & 0x0f), request->priviege);
        }
        else {
            struct ipmi_get_auth_cap_response *response = (struct ipmi_get_auth_cap_response *) payload;
            printf("  [IPMI] Completion Code: 0x%02x\n", response->cc);
            printf("  [IPMI] Channel Number: 0x%02x\n", response->ch_num);
            printf("  [IPMI] Authentication Support: (0x%02x)",response->auth_cap);
            print_ipmi_auth_cap(response->auth_cap);
            printf("\n");
            /* TODO extract */
            printf("  [IPMI] Authentication Method: 0x%02x\n", response->auth_method);
            /* TODO print oem */
        }
    }
    else if ( cmd == GET_SESS_CHAL ){
        if ( direction == IPMI_REQUEST ){
            struct ipmi_get_sess_chal_request *request = (struct ipmi_get_sess_chal_request *) payload;
            printf("  [IPMI] Authentication Challege: %s(0x%02x)\n", get_ipmi_auth_type_str(request->auth_type & 0x0f), request->auth_type);
            printf("  [IPMI] Username: %s\n", request->name);
        }
        else {
            int i;
            struct ipmi_get_sess_chal_response *response = (struct ipmi_get_sess_chal_response *) payload;
            printf("  [IPMI] Completion Code: 0x%02x\n", response->cc);
            printf("  [IPMI] Temporary Session ID: %d\n", response->sid);
            printf("  [IPMI] Challege string data: ");
            for (  i = 0 ; i < sizeof(response->chal_str); i++ ) {
                printf(" 0x%02x", response->chal_str[i]);
            }
            printf("\n");
        }
    }
    else if ( cmd ==  ACT_SESSION ) {
        if ( direction == IPMI_REQUEST ){
            int i;
            struct ipmi_act_sess_request *request = (struct ipmi_act_sess_request *) payload;
            printf("  [IPMI] Authentication Type: %s(0x%02x)\n", get_ipmi_auth_type_str(request->auth_type & 0x0f), request->auth_type);
            printf("  [IPMI] Privilage: %s(0x%02x)\n", get_ipmi_priviege(request->priviege & 0x0f), request->priviege);
            printf("  [IPMI] Challege string data: ");
            for (  i = 0 ; i < sizeof(request->chal_str); i++ ) {
                printf(" 0x%02x", request->chal_str[i]);
            }
            printf("\n");
            printf("  [IPMI] Outbound Sequence Number: %d\n", request->ob_seq);
        }
        else {
            struct ipmi_act_sess_response *response = (struct ipmi_act_sess_response *) payload;
            printf("  [IPMI] Completion Code: 0x%02x\n", response->cc);
            printf("  [IPMI] Authentication Type: %s(0x%02x)\n", get_ipmi_auth_type_str(response->auth_type & 0x0f), response->auth_type);
            printf("  [IPMI] Reminder Session ID: %d\n", response->sid);
            printf("  [IPMI] Inbound Sequence Number: %d\n", response->ib_seq);
            printf("  [IPMI] Privilage: %s(0x%02x)\n", get_ipmi_priviege(response->priviege & 0x0f), response->priviege);
        }
    }
    else if ( cmd == SET_SESS_PRIV ){
        if ( direction == IPMI_REQUEST ){
            struct ipmi_set_sess_priv_request *request = (struct ipmi_set_sess_priv_request *) payload;
            printf("  [IPMI] Privilage: %s(0x%02x)\n", get_ipmi_priviege(request->priviege & 0x0f), request->priviege);
        }
        else {
            struct ipmi_set_sess_priv_response *response = (struct ipmi_set_sess_priv_response *) payload;
            printf("  [IPMI] Completion Code: 0x%02x\n", response->cc);
            printf("  [IPMI] Privilage: %s(0x%02x)\n", get_ipmi_priviege(response->priviege & 0x0f), response->priviege);
        }
    }
    else if ( cmd == CLOSE_SESSION ){
        if ( direction == IPMI_REQUEST ){
            struct ipmi_close_sess_request *request = (struct ipmi_close_sess_request*) payload;
            printf("  [IPMI] Session ID: %d\n", request->sid);
        }
        else {
            struct ipmi_close_sess_response *request = (struct ipmi_close_sess_response*) payload;
            printf("  [IPMI] Completion Code: %d\n", request->cc);

        }

    }
    else {
        fprintf(stderr, "unrecognized cmd %d\n", cmd);
    }

}

