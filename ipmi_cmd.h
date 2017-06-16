#ifndef _IPMI_DUMP_IPMI_CMD_H
#define _IPMI_DUMP_IPMI_CMD_H

enum ipmi_direction {
    IPMI_REQUEST,
    IPMI_RESPONSE
};

/* session (nf: NETFN_APP) */
#define    GET_CHAN_AUTH  0x38
#define    GET_SESS_CHAL  0x39
#define    ACT_SESSION    0x3A
#define    SET_SESS_PRIV  0x3B
#define    CLOSE_SESSION  0x3C

/* SDR  */
#define     GET_SDR_REPINFO        0x20
#define     RESERVE_SDR_REP        0x22
#define     GET_SDR                0x23


#endif

