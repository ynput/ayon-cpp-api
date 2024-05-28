#ifndef AYONCPPAPICROSSPLATFORMMACROS_H
#define AYONCPPAPICROSSPLATFORMMACROS_H

#ifdef _WIN32
    #include <stdint.h>
    #define u_int8_t  uint8_t
    #define u_int16_t uint16_t
    #define u_int32_t uint32_t
    #define u_int64_t uint64_t
#endif

#endif   // !AYONCPPAPICROSSPLATFORMMACROS_H
