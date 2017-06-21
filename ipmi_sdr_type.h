#ifndef _IPMI_DUMP_IPMI_SDR_TYPE_H
#define _IPMI_DUMP_IPMI_SDR_TYPE_H

#include "align.h"

/* section 43.1 43.2 */
struct ipmi_sdr_sensor_common {
    u_char      owner TCC_PACKED;
    u_char      owner_lun TCC_PACKED;
    u_char      number TCC_PACKED;

    u_char      e_id TCC_PACKED;   /* entity id */
    u_char      e_ins TCC_PACKED;  /* entity instance */
    u_char      init TCC_PACKED;   /* sensor initialization */
    u_char      cap TCC_PACKED;    /* sensor capability */
    u_char      type TCC_PACKED;   /* 42.2 */
    u_char      evn_type TCC_PACKED; /* 42.1 */
    unsigned short assert_mask TCC_PACKED;
    unsigned short deassert_mask TCC_PACKED;
    unsigned short read_mask TCC_PACKED;
    u_char      unit TCC_PACKED;
    u_char      unit_base TCC_PACKED;
    u_char      unit_mod TCC_PACKED;
} GNU_PACKED;

/* section 43.1 */
struct ipmi_sdr_type_full_sensor {
    struct ipmi_sdr_sensor_common common TCC_PACKED;
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
    u_char              linearization TCC_PACKED;
    unsigned short      mtol TCC_PACKED;/* M, Tolerence */
    unsigned int        bacc TCC_PACKED;/* accuracy, B, Bexp, Rexp */
    u_char              analog_flag TCC_PACKED;
    u_char              nominal_read TCC_PACKED;
    u_char              normal_max TCC_PACKED;
    u_char              normal_min TCC_PACKED;
    u_char              sensor_max TCC_PACKED;
    u_char              sensor_min TCC_PACKED;
    u_char              u_nr TCC_PACKED; /* upper non-recoverable threshold */
    u_char              u_c TCC_PACKED; /* upper critical threshold */
    u_char              u_nc TCC_PACKED; /* upper non-critical threshold */
    u_char              l_nr TCC_PACKED;
    u_char              l_c TCC_PACKED;
    u_char              l_nc TCC_PACKED;
    u_char              pos_hy TCC_PACKED;
    u_char              neg_hy TCC_PACKED;
    u_char              reserved[2] TCC_PACKED;
    u_char              oem TCC_PACKED;
    u_char              id_code TCC_PACKED;
    char                id_string[17] TCC_PACKED;
} GNU_PACKED;


/* section 43.2 */
struct ipmi_sdr_type_compact_sensor {
    struct ipmi_sdr_sensor_common common TCC_PACKED;
    unsigned short	sharing TCC_PACKED;
    u_char              pos_hy TCC_PACKED;
    u_char              neg_hy TCC_PACKED;
    u_char              reserved[3] TCC_PACKED;
    u_char              oem TCC_PACKED;
    u_char              id_code TCC_PACKED;
    char                id_string[17] TCC_PACKED;
} GNU_PACKED;


/* section 43.9 */
struct ipmi_sdr_type_mc_device_locator {
    u_char      slave_addr TCC_PACKED;
    u_char      chan_num TCC_PACKED;

    u_char      psn_gi TCC_PACKED; /* power state notification and global initialization */
    u_char      dev_cap TCC_PACKED;
    u_char      reserved[3] TCC_PACKED;
    u_char      e_id TCC_PACKED;   /* entity id */
    u_char      e_ins TCC_PACKED;  /* entity instance */
    u_char      oem TCC_PACKED;
    u_char      id_code_type TCC_PACKED; /* format 43.15  */
    char        id_string[17] TCC_PACKED;
} GNU_PACKED;


/* section 43.8 */
struct ipmi_sdr_type_fru_device_locator {
    u_char      dev_addr TCC_PACKED;
    u_char      dev_id TCC_PACKED;
    u_char      acc_info TCC_PACKED;
    u_char      chan_num TCC_PACKED;

    u_char      reserved TCC_PACKED;
    u_char      type TCC_PACKED;
    u_char      type_mod TCC_PACKED;
    u_char      eid TCC_PACKED;
    u_char      eins TCC_PACKED;
    u_char      oem TCC_PACKED;
    u_char      id_string_len TCC_PACKED;/* format 43.15 */
    char        id_string[17] TCC_PACKED;
} GNU_PACKED;



#endif
