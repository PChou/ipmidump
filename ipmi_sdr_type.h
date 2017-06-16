#ifndef _IPMI_DUMP_IPMI_SDR_TYPE_H
#define _IPMI_DUMP_IPMI_SDR_TYPE_H

/* section 43.1 */
struct ipmi_sdr_type_full_sensor {
    u_char      owner;
    u_char      owner_lun;
    u_char      number;

    u_char      e_id;   /* entity id */
    u_char      e_ins;  /* entity instance */
    u_char      init;   /* sensor initialization */
    u_char      cap;    /* sensor capability */
    u_char      type;   /* 42.2 */
    u_char      evn_type; /* 42.1 */
}__attribute__ ((packed));

/* section 43.9 */
struct ipmi_sdr_type_mc_device_locator {
    u_char      slave_addr;
    u_char      chan_num;

    u_char      psn_gi; /* power state notification and global initialization */
    u_char      dev_cap;
    u_char      reserved[3];
    u_char      e_id;   /* entity id */
    u_char      e_ins;  /* entity instance */
    u_char      oem;
    u_char      id_code_type; /* format 43.15  */
    char        id_string[16];
}__attribute__ ((packed));


#endif
