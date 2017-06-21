#ifndef __IPMI_ALIGN_H
#define __IPMI_ALIGN_H


#define GNU_PACKED __attribute__((packed))

/*
 * Tiny C Compile support GNU extension __attribute__,
 * but when using tcc, __GNUC__ predefine is missing,
 * which causes __attribute__(xyz) to be defined as empty(see sys/cdefs.h).
 * so one solution is to delete the definition is sys/cdefs.h
 *
 * On the other hand, tcc only support using `packed` on variable or struct field(not entire struct),
 * in order to complie with tcc, we have to using below TCC_PACKED on each field of struct
 *
 * https://stackoverflow.com/questions/28637879
 */
#ifdef __TINYC__
#define TCC_PACKED GNU_PACKED
#else
#define TCC_PACKED
#endif



#endif
