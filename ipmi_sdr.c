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
#include <math.h>
#include <sys/types.h>

#include "align.h"
#include "bswap.h"
#include "dump.h"
#include "ipmi_cmd.h"
#include "ipmi_sdr_type.h"


#define tos32(val, bits)    ((val & ((1<<((bits)-1)))) ? (-((val) & (1<<((bits)-1))) | (val)) : (val))

#if WORDS_BIGENDIAN
# define __TO_TOL(mtol)     (uint16_t)(mtol & 0x3f)
# define __TO_M(mtol)       (int16_t)(tos32((((mtol & 0xff00) >> 8) | ((mtol & 0xc0) << 2)), 10))
# define __TO_B(bacc)       (int32_t)(tos32((((bacc & 0xff000000) >> 24) | ((bacc & 0xc00000) >> 14)), 10))
# define __TO_ACC(bacc)     (uint32_t)(((bacc & 0x3f0000) >> 16) | ((bacc & 0xf000) >> 6))
# define __TO_ACC_EXP(bacc) (uint32_t)((bacc & 0xc00) >> 10)
# define __TO_R_EXP(bacc)   (int32_t)(tos32(((bacc & 0xf0) >> 4), 4))
# define __TO_B_EXP(bacc)   (int32_t)(tos32((bacc & 0xf), 4))
#else
# define __TO_TOL(mtol)     (uint16_t)(BSWAP_16(mtol) & 0x3f)
# define __TO_M(mtol)       (int16_t)(tos32((((BSWAP_16(mtol) & 0xff00) >> 8) | ((BSWAP_16(mtol) & 0xc0) << 2)), 10))
# define __TO_B(bacc)       (int32_t)(tos32((((BSWAP_32(bacc) & 0xff000000) >> 24) | \
                                            ((BSWAP_32(bacc) & 0xc00000) >> 14)), 10))
# define __TO_ACC(bacc)     (uint32_t)(((BSWAP_32(bacc) & 0x3f0000) >> 16) | ((BSWAP_32(bacc) & 0xf000) >> 6))
# define __TO_ACC_EXP(bacc) (uint32_t)((BSWAP_32(bacc) & 0xc00) >> 10)
# define __TO_R_EXP(bacc)   (int32_t)(tos32(((BSWAP_32(bacc) & 0xf0) >> 4), 4))
# define __TO_B_EXP(bacc)   (int32_t)(tos32((BSWAP_32(bacc) & 0xf), 4))
#endif


/* unit description codes (IPMI v1.5 section 43.17) */
#define UNIT_MAX    0x90
static const char *unit_desc[] = {
"unspecified",
        "degrees C", "degrees F", "degrees K",
        "Volts", "Amps", "Watts", "Joules",
        "Coulombs", "VA", "Nits",
        "lumen", "lux", "Candela",
        "kPa", "PSI", "Newton",
        "CFM", "RPM", "Hz",
        "microsecond", "millisecond", "second", "minute", "hour",
        "day", "week", "mil", "inches", "feet", "cu in", "cu feet",
        "mm", "cm", "m", "cu cm", "cu m", "liters", "fluid ounce",
        "radians", "steradians", "revolutions", "cycles",
        "gravities", "ounce", "pound", "ft-lb", "oz-in", "gauss",
        "gilberts", "henry", "millihenry", "farad", "microfarad",
        "ohms", "siemens", "mole", "becquerel", "PPM", "reserved",
        "Decibels", "DbA", "DbC", "gray", "sievert",
        "color temp deg K", "bit", "kilobit", "megabit", "gigabit",
        "byte", "kilobyte", "megabyte", "gigabyte", "word", "dword",
        "qword", "line", "hit", "miss", "retry", "reset",
        "overflow", "underrun", "collision", "packets", "messages",
        "characters", "error", "correctable error", "uncorrectable error",};

/* sensor type codes (IPMI v1.5 table 42.2) 
  / Updated to v2.0 Table 42-3, Sensor Type Codes */
#define SENSOR_TYPE_MAX 0x2C
static const char *sensor_type_desc[] = {
"reserved",
        "Temperature", "Voltage", "Current", "Fan",
        "Physical Security", "Platform Security", "Processor",
        "Power Supply", "Power Unit", "Cooling Device", "Other",
        "Memory", "Drive Slot / Bay", "POST Memory Resize",
        "System Firmwares", "Event Logging Disabled", "Watchdog1",
        "System Event", "Critical Interrupt", "Button",
        "Module / Board", "Microcontroller", "Add-in Card",
        "Chassis", "Chip Set", "Other FRU", "Cable / Interconnect",
        "Terminator", "System Boot Initiated", "Boot Error",
        "OS Boot", "OS Critical Stop", "Slot / Connector",
        "System ACPI Power State", "Watchdog2", "Platform Alert",
        "Entity Presence", "Monitor ASIC", "LAN",
        "Management Subsys Health", "Battery", "Session Audit",
        "Version Change", "FRU State" };

/* section 33.9 request data is empty*/
struct ipmi_get_sdr_repo_response {
    u_char              cc TCC_PACKED;
    u_char              sdr_version TCC_PACKED;
    unsigned short      sdr_rec_count TCC_PACKED;
    unsigned short      sdr_rec_free TCC_PACKED;
    unsigned int        t1 TCC_PACKED;         /* addition timestamp */
    unsigned int        t2 TCC_PACKED;         /* deletion timestamp */
    u_char              sdr_op TCC_PACKED;     /* operation support */
#define     SDR_OP_SUP_ALLOC_INFO      (1 << 0)
#define     SDR_OP_SUP_RESERVE_REPO    (1 << 1)
#define     SDR_OP_SUP_PARTIAL_ADD     (1 << 2)
#define     SDR_OP_SUP_DELETE          (1 << 3)
#define     SDR_OP_SUP_NON_MODAL_UP    (1 << 5)
#define     SDR_OP_SUP_MODAL_UP        (1 << 6)
#define     SDR_OP_SUP_OVERFLOW        (1 << 7)
} GNU_PACKED;

/* section 33.11 reserve repo */
struct ipmi_reserve_sdr_repo_response {
    u_char              cc TCC_PACKED;
    unsigned short      sdr_res_id TCC_PACKED;
} GNU_PACKED;

/* section 33.12 get sdr */
struct ipmi_get_sdr_request {
    unsigned short      sdr_res_id TCC_PACKED; /* resevation id */
    unsigned short      sdr_rec_id TCC_PACKED; /* record id */
    u_char              sdr_rec_offset TCC_PACKED; /* offset inside record */
    u_char              sdr_byte_read TCC_PACKED; /* request bytes */
} GNU_PACKED;


/* section 33.12 */
struct ipmi_get_sdr_response {
    u_char              cc TCC_PACKED;
    unsigned short      sdr_next_rec_id TCC_PACKED;
    /* section 43 */
    struct {
        unsigned short      sdr_rec_id TCC_PACKED;
        u_char              sdr_version TCC_PACKED;
        u_char              sdr_rec_type TCC_PACKED;
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
        u_char              sdr_rec_len TCC_PACKED;
    } GNU_PACKED sdr_rec_header;
} GNU_PACKED;


/* section 35.14 */
struct __ipmi_get_sensor_reading_request {
    u_char              s_num TCC_PACKED; /* sensor number */
} GNU_PACKED;

struct __ipmi_get_sensor_reading_response {
    u_char              cc TCC_PACKED;
    u_char              value TCC_PACKED;
    u_char		avail TCC_PACKED;
#define	IS_READING_UNAVAILABLE(val)	((val) & 0x20)
    u_char              extra[2] TCC_PACKED; /* most to 3 bytes */
} GNU_PACKED;

/* section 35.9 */
struct __ipmi_get_sensor_threshold_request {
    u_char              s_num TCC_PACKED;
} GNU_PACKED;

struct __ipmi_get_sensor_threshold_response {
    u_char              cc TCC_PACKED;
    u_char              mask TCC_PACKED;
    u_char              l_nc TCC_PACKED;
    u_char              l_c TCC_PACKED;
    u_char              l_nr TCC_PACKED;
    u_char              u_nc TCC_PACKED;
    u_char              u_c TCC_PACKED;
    u_char              u_nr TCC_PACKED;
} GNU_PACKED;

/* chain to store all records */
struct __ipmi_record_complete {
    unsigned short      sdr_rec_id TCC_PACKED;
    u_char              sdr_sensor_num TCC_PACKED;
    u_char              sdr_rec_type TCC_PACKED;
    u_char              sdr_rec_len TCC_PACKED; 
    u_char              offseting TCC_PACKED; /* current pending offset */
    u_char              reading TCC_PACKED; /* current reading len */
    u_char              raw[255] TCC_PACKED;
    struct __ipmi_record_complete  *next TCC_PACKED;
} GNU_PACKED;
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
static struct __ipmi_record_complete* seek_sensor(u_char num) {
    struct __ipmi_record_complete *p;
    p = head;
    while ( p ){
        if ( p->sdr_sensor_num == num ) break;
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

void print_ipmi_sdr_op_support(u_char op_support){ if ( op_support & SDR_OP_SUP_ALLOC_INFO ) { printf("AllocInfo "); }
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
        printf("NonModalUpdate ");
    }
    if ( op_support & SDR_OP_SUP_MODAL_UP ) {
        printf("ModalUpdate ");
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

/* section 42.1  */
const char* get_ipmi_sdr_sensor_reading_type(u_char rtype) {
   if ( rtype == 0x00 )
	return "unspecified";
   else if ( rtype == 0x01 )
	return "Threshold";
   else if ( rtype >= 0x02 && rtype <= 0x0c )
	return "Generic";
   else if ( rtype >= 0x70 && rtype <= 0x7f )
	return "OEM";
   else if ( rtype == 0x6f )
	return "Sensor-specific";
   else
  	return "out of range";
}

static void print_id_string(u_char len, char *id_string){
    int id_length = (int)(len & 0x1f);
    printf("  [IPMI] Id Code: 0x%02x, Id Length: 0x%02x\n", (len >> 6), len & 0x1f);
    if ( (id_length == 0) || (id_length == 0x1f) ){
        return;
    }
    char tmp[17]; /* most 16 bytes */
    memcpy(tmp, id_string, id_length);
    tmp[id_length]='\0';
    printf("  [IPMI] Id String: %s\n", tmp);
}

static double convert_sensor_reading(struct __ipmi_record_complete *record, u_char val){
    if ( record == NULL ){
	    return 0;
    }

    if ( record->sdr_rec_type != SDR_RECORD_TYPE_FULL_SENSOR ){
	    return 0;
    }


    struct ipmi_sdr_type_full_sensor *fs = (struct ipmi_sdr_type_full_sensor *)&(record->raw[5]);
    int m,b,k1,k2,si;
    double result;
    m = __TO_M(fs->mtol);
    b = __TO_M(fs->bacc);
    k1 = __TO_B_EXP(fs->bacc);
    k2 = __TO_R_EXP(fs->bacc);
    si = ((fs->common.unit & 0xc0) >> 6);
    //fprintf(stderr, "m:%d,b:%d,k1:%d,k2:%d,si:%d\n",m,b,k1,k2,si);

    switch(si) {
        case 0: /* unsigned */
            result = (double) (((m*val)+b*pow(10,k1)) * pow(10,k2));
            break;
        case 1: /* signed 1's complement */
            if ( val & 0x80 ){
                val++;
            }
        case 2: /* signed 2's complement */
            result = (double) (((m*(char)val)+b*pow(10,k1)) * pow(10,k2));
            break;
        default:
            return 0.0;
    }

    /* TODO applying linearization, see ipmitool/lib/ipmi_sdr.c ln:218 */
    return result;

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
        print_id_string(l->id_code_type, l->id_string);
    }
    else if ( record->sdr_rec_type == SDR_RECORD_TYPE_OEM ){
        printf("  [IPMI] Oem Data Not Parsed.\n");
    }
    /* section 43.8 */
    else if ( record->sdr_rec_type == SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR ){
        struct ipmi_sdr_type_fru_device_locator *l = (struct ipmi_sdr_type_fru_device_locator *)rbody;
        printf("  [IPMI] Device Access Address: 0x%02x\n", l->dev_addr);
        printf("  [IPMI] Device Id/Slave Address: 0x%02x\n", l->dev_id);
        printf("  [IPMI] Access Info: 0x%02x\n", l->dev_id);
        if ( (l->dev_id & 0x80) > 0){
            printf("    [IPMI] Logical FRU Device\n");
        }
        else {
            printf("    [IPMI] Non-Logical FRU Device\n");
        }
        printf("    [IPMI] LUN: 0x%02x\n", ((l->dev_id >> 3) & 0x03));
        printf("    [IPMI] Private Bus Id: 0x%02x\n", (l->dev_id & 0x07));
        printf("  [IPMI] Channel Number: 0x%02x\n", l->chan_num);
        /* TODO print type in string */
        printf("  [IPMI] Device Type: 0x%02x\n", l->type);
        printf("  [IPMI] Device Type Modifier: 0x%02x\n", l->type_mod);
        printf("  [IPMI] Entity Id: 0x%02x\n", l->eid);
        printf("  [IPMI] Entity Instance: 0x%02x\n", l->eins);
        printf("  [IPMI] OEM: 0x%02x\n", l->oem);
        print_id_string(l->id_string_len, l->id_string);
    }
    /* section 43.1  */
    else if ( record->sdr_rec_type == SDR_RECORD_TYPE_FULL_SENSOR || record->sdr_rec_type == SDR_RECORD_TYPE_COMPACT_SENSOR ){
        struct ipmi_sdr_sensor_common *s = (struct ipmi_sdr_sensor_common *)rbody;
        printf("  [IPMI] Sensor Number: 0x%02x\n", s->number);
        record->sdr_sensor_num = s->number;	
        printf("  [IPMI] Sensor Entity Id: 0x%02x\n", s->e_id);
        printf("  [IPMI] Sensor Entity Instance: 0x%02x\n", s->e_ins);
        if ( s->type > 0x2c ) {
            printf("  [IPMI] Sensor Type: reserved or oem defined(>=0xc0)(0x%02x)\n",s->type);
        }
        else {
            printf("  [IPMI] Sensor Type: %s(0x%02x)\n",sensor_type_desc[s->type]  ,s->type);
        }
        printf("  [IPMI] Sensor Reading Type: %s(0x%02x)\n", get_ipmi_sdr_sensor_reading_type(s->evn_type) ,s->evn_type);
        printf("  [IPMI] Sensor Unit: 0x%02x\n",s->unit);
        printf("  [IPMI] Sensor Unit Base: %s(0x%02x)\n", unit_desc[s->unit_base] ,s->unit_base);
        printf("  [IPMI] Sensor Unit Modifier: 0x%02x\n", s->unit_mod);
        if ( record->sdr_rec_type == SDR_RECORD_TYPE_FULL_SENSOR ){
            struct ipmi_sdr_type_full_sensor *fs = (struct ipmi_sdr_type_full_sensor *)rbody;
            printf("  [IPMI] Linearization: 0x%02x\n", fs->linearization);
            printf("  [IPMI] M: %d,0x%02x\n",__TO_M(fs->mtol) ,fs->mtol);
            printf("  [IPMI] B: %d\n",__TO_B(fs->bacc));
            printf("  [IPMI] Bexp: %d\n",__TO_B_EXP(fs->bacc));
            printf("  [IPMI] Rexp: %d\n",__TO_R_EXP(fs->bacc));
            printf("  [IPMI] Value convert format: (%dxV+%dxpow(10,%d))xpow(10,%d) (y=(M x V + B x pow(10,Bexp)) x pow(10,Rexp))\n", __TO_M(fs->mtol), __TO_B(fs->bacc), __TO_B_EXP(fs->bacc), __TO_R_EXP(fs->bacc));
            //u_char df = ((s->common.unit & 0xc0) >> 6);
            print_id_string(fs->id_code, fs->id_string);
        }
        else {/* compact sensor */
            struct ipmi_sdr_type_compact_sensor *cs = (struct ipmi_sdr_type_compact_sensor *)rbody;
            print_id_string(cs->id_code, cs->id_string);
        }
    }
    else {
        printf("  [IPMI] UnSupport SDR Type(0x%02x).\n", record->sdr_rec_type);
    }


}

void print_ipmi_sdr(enum ipmi_direction direction,u_char cmd, const u_char *payload, int payload_len, enum dump_level dl) {
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
	    else if ( last != NULL ) {
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
                if ( payload_len == 5+4){
                    last = add_record();
                    last->sdr_rec_id = response->sdr_rec_header.sdr_rec_id;
                    last->offseting = 0;
                    last->reading = 5;
                }
            }

            if ( last != NULL ) {
                if ( last->offseting == 0 && last->reading >= 5 ){
                    last->sdr_rec_type = response->sdr_rec_header.sdr_rec_type;
                    last->sdr_rec_len = response->sdr_rec_header.sdr_rec_len;
                }
                memcpy(&(last->raw[last->offseting]), payload + 3 /* skip cc and next_rec_id */, last->reading );
                if ( last->offseting + last->reading == last->sdr_rec_len+5 ){
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
    else if( cmd == GET_SENSOR_READING ){
	    static u_char pending_sensor_num = 0;
        if ( direction == IPMI_REQUEST ){
            struct __ipmi_get_sensor_reading_request *request = (struct __ipmi_get_sensor_reading_request *) payload;
	    pending_sensor_num = request->s_num;
            printf("  [IPMI] Sensor Number: 0x%02x\n", request->s_num);
        }
        else {
            struct __ipmi_get_sensor_reading_response *response = (struct __ipmi_get_sensor_reading_response *) payload;
            printf("  [IPMI] Completion Code: 0x%02x\n", response->cc);
            struct __ipmi_record_complete  *record = seek_sensor(pending_sensor_num);
	    if ( record != NULL ) {
		if ( IS_READING_UNAVAILABLE(response->avail) ) {
			printf("  [IPMI] Readed Value is unavaliable\n");
		}
		else {
			struct ipmi_sdr_sensor_common *cmn = (struct ipmi_sdr_sensor_common *)&(record->raw[5]);
		  if ( cmn->evn_type == 0x01 ) {
			/* threshold type */ 
			if ( (cmn->unit & 0xc0) != 0xc0 ) {
				/* has analog value */
				double c = convert_sensor_reading(record, response->value);
				printf("  [IPMI] Readed Value: %.2f(0x%02x)\n",c ,response->value);
			}	
			else {
				printf("  [IPMI] Readed Value(No analog): (0x%02x)\n",response->value);
			}
		  }
		  else {
				printf("  [IPMI] Readed Value(discrete or No analog): (0x%02x)\n",response->value);
 			
		  }
		}
	    }
	    else {
		    printf("  [IPMI] Readed Value(unconverted): 0x%02x\n", response->value);
	    }

        }
    }
    else if( cmd == GET_SENSOR_THRESHOLD ){
        if ( direction == IPMI_REQUEST ){
            struct __ipmi_get_sensor_threshold_request *request = (struct __ipmi_get_sensor_threshold_request *) payload;
            printf("  [IPMI] Sensor Number: 0x%02x\n", request->s_num);
        }
        else {
            struct __ipmi_get_sensor_threshold_response *response = (struct __ipmi_get_sensor_threshold_response *) payload;
            printf("  [IPMI] Completion Code: 0x%02x\n", response->cc);
            /* TODO: unpack the mask */
            printf("  [IPMI] Threshold Mask: 0x%02x\n", response->mask);
            /* TODO: show the threshold */
        }
    }
    else {
        fprintf(stderr, "unrecognized cmd 0x%02x\n", cmd);
    }
    
}
