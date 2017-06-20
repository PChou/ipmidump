#ifndef _IPMI_DUMP_IPMI_SDR_TYPE_H
#define _IPMI_DUMP_IPMI_SDR_TYPE_H

/* section 43.1 43.2 */
struct ipmi_sdr_sensor_common {
    u_char      owner;
    u_char      owner_lun;
    u_char      number;

    u_char      e_id;   /* entity id */
    u_char      e_ins;  /* entity instance */
    u_char      init;   /* sensor initialization */
    u_char      cap;    /* sensor capability */
    u_char      type;   /* 42.2 */
    u_char      evn_type; /* 42.1 */
    unsigned short assert_mask;
    unsigned short deassert_mask;
    unsigned short read_mask;
    u_char      unit;
    u_char      unit_base;
    u_char      unit_mod;
} __attribute__ ((packed));

/* section 43.1 */
struct ipmi_sdr_type_full_sensor {
    struct ipmi_sdr_sensor_common common;
#define SDR_SENSOR_L_LINEAR     0x00
#define SDR_SENSOR_L_LN         0x01
#define SDR_SENSOR_L_LOG10      0x02
#define SDR_SENSOR_L_LOG2       0x03
#define SDR_SENSOR_L_E          0x04
#define SDR_SENSOR_L_EXP10      0x05
#define SDR_SENSOR_L_EXP2       0x06
#define SDR_SENSOR_L_1_X        0x07
#define SDR_SENSOR_L_SQR        0x08
#define SDR_SENSOR_L_CUBE       0x09
#define SDR_SENSOR_L_SQRT       0x0a
#define SDR_SENSOR_L_CUBERT     0x0b
#define SDR_SENSOR_L_NONLINEAR  0x70
    u_char              linearization;
    unsigned short      mtol;/* M, Tolerence */
    unsigned int        bacc;/* accuracy, B, Bexp, Rexp */
    u_char              analog_flag;
    u_char              nominal_read;
    u_char              normal_max;
    u_char              normal_min;
    u_char              sensor_max;
    u_char              sensor_min;
    u_char              u_nr; /* upper non-recoverable threshold */
    u_char              u_c; /* upper critical threshold */
    u_char              u_nc; /* upper non-critical threshold */
    u_char              l_nr;
    u_char              l_c;
    u_char              l_nc;
    u_char              pos_hy;
    u_char              neg_hy;
    u_char              reserved[2];
    u_char              oem;
    u_char              id_code;
    char                id_string[17];

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
    char        id_string[17];
}__attribute__ ((packed));


/* section 43.8 */
struct ipmi_sdr_type_fru_device_locator {
    u_char      dev_addr;
    u_char      dev_id;
    u_char      acc_info;
    u_char      chan_num;

    u_char      reserved;
    u_char      type;
    u_char      type_mod;
    u_char      eid;
    u_char      eins;
    u_char      oem;
    u_char      id_string_len;/* format 43.15 */
    char        id_string[17];
}__attribute__ ((packed));



#endif
