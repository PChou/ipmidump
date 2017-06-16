/*
 * parse and print ipmi sdr message
 * ipmi sdr message includes:
 * - get sdr repo info
 * - reserve sdr repo
 * - get sdr
 * - get sensor reading
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "dump.h"
#include "ipmi_cmd.h"
#include "ipmi_sdr_type.h"



/* section 33.9 request data is empty*/
struct ipmi_get_sdr_repo_response {
    u_char              cc;
    u_char              sdr_version;
    unsigned short      sdr_rec_count;
    unsigned short      sdr_rec_free;
    unsigned int        t1;         /* addition timestamp */
    unsigned int        t2;         /* deletion timestamp */
    u_char              sdr_op;     /* operation support */
#define     SDR_OP_SUP_ALLOC_INFO      (1 << 0)
#define     SDR_OP_SUP_RESERVE_REPO    (1 << 1)
#define     SDR_OP_SUP_PARTIAL_ADD     (1 << 2)
#define     SDR_OP_SUP_DELETE          (1 << 3)
#define     SDR_OP_SUP_NON_MODAL_UP    (1 << 5)
#define     SDR_OP_SUP_MODAL_UP        (1 << 6)
#define     SDR_OP_SUP_OVERFLOW        (1 << 7)
}__attribute__ ((packed));

/* section 33.11 reserve repo */
struct ipmi_reserve_sdr_repo_response {
    u_char              cc;
    unsigned short      sdr_res_id;
}__attribute__ ((packed));

/* section 33.12 get sdr */
struct ipmi_get_sdr_request {
    unsigned short      sdr_res_id; /* resevation id */
    unsigned short      sdr_rec_id; /* record id */
    u_char              sdr_rec_offset; /* offset inside record */
    u_char              sdr_byte_read; /* request bytes */
}__attribute__ ((packed));


/* section 33.12 */
struct ipmi_get_sdr_response {
    u_char              cc;
    unsigned short      sdr_next_rec_id;
    /* section 43 */
    struct {
        unsigned short      sdr_rec_id;
        u_char              sdr_version;
        u_char              sdr_rec_type;
#define SDR_RECORD_TYPE_FULL_SENSOR     0x01
#define SDR_RECORD_TYPE_COMPACT_SENSOR      0x02
#define SDR_RECORD_TYPE_EVENTONLY_SENSOR    0x03
#define SDR_RECORD_TYPE_ENTITY_ASSOC        0x08
#define SDR_RECORD_TYPE_DEVICE_ENTITY_ASSOC 0x09
#define SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR  0x10
#define SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR  0x11
#define SDR_RECORD_TYPE_MC_DEVICE_LOCATOR   0x12
#define SDR_RECORD_TYPE_MC_CONFIRMATION     0x13
#define SDR_RECORD_TYPE_BMC_MSG_CHANNEL_INFO    0x14
#define SDR_RECORD_TYPE_OEM         0xc0
        u_char              sdr_rec_len;
    } sdr_rec_header;
}__attribute__ ((packed));

/* chain to store all records */
struct __ipmi_record_complete {
    unsigned short      sdr_rec_id;
    u_char              sdr_rec_type;
    u_char              sdr_rec_len; 
    u_char              offseting; /* current pending offset */
    u_char              reading; /* current reading len */
    u_char              raw[255];
    struct __ipmi_record_complete  *next;
}__attribute__ ((packed));
static struct __ipmi_record_complete  *head;
static struct __ipmi_record_complete   *last;
static struct __ipmi_record_complete* seek_record(unsigned short sdr_rec_id) {
    struct __ipmi_record_complete *p;
    p = head;
    while ( p ){
        if ( p->sdr_rec_id == sdr_rec_id ) break;
        p = p->next;
    }
    return p;
}

static struct __ipmi_record_complete* add_record(){
    struct __ipmi_record_complete *p;
    p = (struct __ipmi_record_complete *)calloc(1, sizeof(struct __ipmi_record_complete));
    if ( head == NULL ){
        head = p;
    }
    else {
        p->next = head;
        head = p;
    }

    return p;
}

static void remove_record(unsigned short sdr_rec_id){
    struct __ipmi_record_complete *p, *pp;
    p = head; pp = NULL;
    while ( p ){
        if ( p->sdr_rec_id == sdr_rec_id ) {
            if ( pp == NULL ){
                head = p->next;
            }
            else {
                pp->next = p->next;
            }
            break;
        }
    }
    if ( p ) free(p);

    return;
}

static void clear_all_record(){
    struct __ipmi_record_complete *p;
    p = head;
    while( p ){
        free(p);
    }

    head = NULL;
}




void print_ipmi_sdr_op_support(u_char op_support){
    if ( op_support & SDR_OP_SUP_ALLOC_INFO ) {
        printf("AllocInfo ");
    }
    if ( op_support & SDR_OP_SUP_RESERVE_REPO ) {
        printf("ReserveRepo ");
    }
    if ( op_support & SDR_OP_SUP_PARTIAL_ADD ) {
        printf("PartialAdd ");
    }
    if ( op_support & SDR_OP_SUP_DELETE ) {
        printf("Delete ");
    }
    if ( op_support & SDR_OP_SUP_NON_MODAL_UP ) {
        printf("NonModaUpdate ");
    }
    if ( op_support & SDR_OP_SUP_MODAL_UP ) {
        printf("ModaUpdate ");
    }
    if ( op_support & SDR_OP_SUP_OVERFLOW ) {
        printf("Overflow ");
    }

}

const char* get_ipmi_sdr_rec_type_str(u_char sdr_rec_type) {
    switch ( sdr_rec_type ){
        case SDR_RECORD_TYPE_FULL_SENSOR:     
            return "Full Sensor";
        case SDR_RECORD_TYPE_COMPACT_SENSOR:      
            return "Compact Sensor";
        case SDR_RECORD_TYPE_EVENTONLY_SENSOR:    
            return "EventOnly Sensor";
        case SDR_RECORD_TYPE_ENTITY_ASSOC:        
            return "Entity Assoc";
        case SDR_RECORD_TYPE_DEVICE_ENTITY_ASSOC: 
            return "Device Entity Assoc";
        case SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR:  
            return "Generic Device Locator";
        case SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR:  
            return "FRU Device Locator";
        case SDR_RECORD_TYPE_MC_DEVICE_LOCATOR: 
            return "MC Device Locator";
        case SDR_RECORD_TYPE_MC_CONFIRMATION:
            return "MC Confirmation";
        case SDR_RECORD_TYPE_BMC_MSG_CHANNEL_INFO:
            return "Msg Channel Info";
        case SDR_RECORD_TYPE_OEM:
            return "Oem";
        default:
            return "unknown";
    }
    
    
}


static void print_ipmi_record_complete(struct __ipmi_record_complete *record){
    if ( record == NULL )
        return;
    u_char    *rbody = &(record->raw[5]);
    printf("  [IPMI] Record Id: %d\n", record->sdr_rec_id);
    printf("  [IPMI] Record Type: %s(0x%02x)\n", get_ipmi_sdr_rec_type_str(record->sdr_rec_type), record->sdr_rec_type);
    printf("  [IPMI] Record Body Length: %d\n", record->sdr_rec_len);


    /* section 43.9 */
    if ( record->sdr_rec_type == SDR_RECORD_TYPE_MC_DEVICE_LOCATOR ){
        struct ipmi_sdr_type_mc_device_locator *l = (struct ipmi_sdr_type_mc_device_locator *)rbody;
        printf("  [IPMI] I2C Slave Address: 0x%02x\n", l->slave_addr);
        printf("  [IPMI] Channel Number: 0x%02x\n", l->chan_num);
        printf("  [IPMI] Power State...: 0x%02x\n", l->psn_gi);
        printf("  [IPMI] Device Cap: 0x%02x\n", l->dev_cap);
        printf("  [IPMI] Entity Id: 0x%02x\n", l->e_id);
        printf("  [IPMI] Entity Instance: 0x%02x\n", l->e_ins);
        printf("  [IPMI] OEM: 0x%02x\n", l->oem);
        int id_length = (int)(l->id_code_type & 0x1f);
        printf("  [IPMI] Id Code: 0x%02x, Id Length: 0x%02x\n", (l->id_code_type >> 6), l->id_code_type & 0x1f);
        if ( (id_length == 0) || (id_length == 0x1f) ){
            return;
        }
        char tmp[16];
        memcpy(tmp, l->id_string, id_length);
        printf("  [IPMI] Id String: %s\n", tmp);
    }
    else {
        fprintf(stderr, "  [IPMI] UnSupport SDR Type.");
    }


}

void print_ipmi_sdr(enum ipmi_direction direction,u_char cmd, u_char *payload, int payload_len, enum dump_level dl) {
    if ( cmd == GET_SDR_REPINFO ){
        if ( direction == IPMI_REQUEST ){
            /* no data need to unpack */
            return;
        }
        else {
            struct ipmi_get_sdr_repo_response *response = (struct ipmi_get_sdr_repo_response *) payload;
            printf("  [IPMI] Completion Code: 0x%02x\n", response->cc);
            printf("  [IPMI] SDR Version: 0x%02x\n", response->sdr_version);
            printf("  [IPMI] Read Count: %d\n", response->sdr_rec_count);
            printf("  [IPMI] Free Bytes: %d\n", response->sdr_rec_free);
            printf("  [IPMI] Last addition time: %d\n", response->t1);
            printf("  [IPMI] Last deletion time: %d\n", response->t2);
            printf("  [IPMI] Operation Support: ");
            print_ipmi_sdr_op_support(response->sdr_op);
            printf("\n");
        }
    }
    else if ( cmd == RESERVE_SDR_REP ){
        if ( direction == IPMI_REQUEST ){
            /* no data need to unpack */
            return;
        }
        else {
            struct ipmi_reserve_sdr_repo_response *response = (struct ipmi_reserve_sdr_repo_response *) payload;
            printf("  [IPMI] Completion Code: 0x%02x\n", response->cc);
            printf("  [IPMI] Reservation Id: %d\n", response->sdr_res_id);
        }
    }
    /* get sdr can request serval times and return partially, we have to track the request and response */
    else if ( cmd == GET_SDR ){
        if ( direction == IPMI_REQUEST ){
            struct ipmi_get_sdr_request *request = (struct ipmi_get_sdr_request *) payload;
            struct __ipmi_record_complete *s = NULL;
            printf("  [IPMI] Reservation Id: %d\n", request->sdr_res_id);
            printf("  [IPMI] Record Id: %d\n", request->sdr_rec_id);
            printf("  [IPMI] Offset: %d\n", request->sdr_rec_offset);
            printf("  [IPMI] Reading bytes: %d\n", request->sdr_byte_read);
            if ( request->sdr_rec_id != 0 ) { /* 0 means try to fetch the first nearest record  */
                last = seek_record(request->sdr_rec_id);
                if ( last == NULL ){
                    last = add_record();
                }
                last->sdr_rec_id = request->sdr_rec_id;
                last->offseting = request->sdr_rec_offset;
                last->reading = request->sdr_byte_read;
            }
        }
        else {
            struct ipmi_get_sdr_response *response = (struct ipmi_get_sdr_response *) payload;
            printf("  [IPMI] Completion Code: 0x%02x\n", response->cc);
            printf("  [IPMI] Next Record Id: %d\n", response->sdr_next_rec_id);
            if ( last == NULL ) { 
            /* this is because the request record id is 0 which means a first attampt read, in this case the payload len must be exactly 9(cc+nextrid+5+checksum) which means fetch the head */
                if ( payload_len == 9){
                    last = add_record();
                    last->sdr_rec_id = response->sdr_rec_header.sdr_rec_id;
                }
            }

            if ( last != NULL ) {
                if ( last->offseting == 0 && last->reading >= 5 ){
                    last->sdr_rec_type = response->sdr_rec_header.sdr_rec_type;
                    last->sdr_rec_len = response->sdr_rec_header.sdr_rec_len;
                }
                memcpy(&(last->raw[last->offseting]), payload + 3 /* skip cc aand next_rec_id */, last->reading );
                if ( last->offseting + last->reading == last->sdr_rec_len ){
                    /* reading complete parse and display */
                    print_ipmi_record_complete(last);
                }
                else {
                    printf("  [IPMI] (delay to display the following bytes until partial reading finish)\n");
                }
                
            }
            else {
                fprintf(stderr, "the response failed to match any request");
            }
        }
    }
    else {
        fprintf(stderr, "unrecognized cmd %d\n", cmd);
    }
    
}
