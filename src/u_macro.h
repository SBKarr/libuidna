// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
*******************************************************************************
*
*   Copyright (C) 1999-2011, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  utf.h
*   encoding:   UTF-8
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep09
*   created by: Markus W. Scherer
*/

#ifndef MODULES_IDN_UIDNAMACRO_H_
#define MODULES_IDN_UIDNAMACRO_H_

#define UPRV_LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#define U_POINTER_MASK_LSB(ptr, mask) ((uintptr_t)(ptr) & (mask))
#define U_SENTINEL (-1)
#define U_MASK(x) ((uint32_t)1<<(x))
#define U_GET_GC_MASK(c) U_MASK(u_charType(c))
#define U_IS_SUPPLEMENTARY(c) ((uint32_t)((c)-0x10000)<=0xfffff)
#define U_IS_BMP(c) ((uint32_t)(c)<=0xffff)

#define U_IS_UNICODE_NONCHAR(c) \
    ((c)>=0xfdd0 && \
     ((c)<=0xfdef || ((c)&0xfffe)==0xfffe) && (c)<=0x10ffff)

#define U8_IS_SINGLE(c) (((c)&0x80)==0)
#define U8_LEAD3_T1_BITS "\x20\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x10\x30\x30"
#define U8_IS_VALID_LEAD3_AND_T1(lead, t1) (U8_LEAD3_T1_BITS[(lead)&0xf]&(1<<((uint8_t)(t1)>>5)))
#define U8_LEAD4_T1_BITS "\x00\x00\x00\x00\x00\x00\x00\x00\x1E\x0F\x0F\x0F\x00\x00\x00\x00"
#define U8_IS_VALID_LEAD4_AND_T1(lead, t1) (U8_LEAD4_T1_BITS[(uint8_t)(t1)>>4]&(1<<((lead)&7)))
#define U8_IS_TRAIL(c) ((int8_t)(c)<-0x40)
#define U8_IS_LEAD(c) ((uint8_t)((c)-0xc2)<=0x32)

#define U8_LENGTH(c) \
    ((uint32_t)(c)<=0x7f ? 1 : \
        ((uint32_t)(c)<=0x7ff ? 2 : \
            ((uint32_t)(c)<=0xd7ff ? 3 : \
                ((uint32_t)(c)<=0xdfff || (uint32_t)(c)>0x10ffff ? 0 : \
                    ((uint32_t)(c)<=0xffff ? 3 : 4)\
                ) \
            ) \
        ) \
    )

#define U8_PREV(s, start, i, c) do { \
    (c)=(uint8_t)(s)[--(i)]; \
    if(!U8_IS_SINGLE(c)) { \
        (c)=utf8_prevCharSafeBody((const uint8_t *)s, start, &(i), c, -1); \
    } \
} while (0)

#define U16_IS_SINGLE(c) !U_IS_SURROGATE(c)
#define U16_IS_LEAD(c) (((c)&0xfffffc00)==0xd800)
#define U16_IS_TRAIL(c) (((c)&0xfffffc00)==0xdc00)
#define U16_IS_SURROGATE_LEAD(c) (((c)&0x400)==0)
#define U16_IS_SURROGATE_TRAIL(c) (((c)&0x400)!=0)
#define U16_SURROGATE_OFFSET ((0xd800<<10UL)+0xdc00-0x10000)
#define U16_GET_SUPPLEMENTARY(lead, trail) \
	(((UChar32)(lead)<<10UL)+(UChar32)(trail)-U16_SURROGATE_OFFSET)
#define U16_LEAD(supplementary) (UChar)(((supplementary)>>10)+0xd7c0)
#define U16_TRAIL(supplementary) (UChar)(((supplementary)&0x3ff)|0xdc00)
#define U16_LENGTH(c) ((uint32_t)(c)<=0xffff ? 1 : 2)
#define U16_MAX_LENGTH 2

#define U16_FWD_1(s, i, length) do { \
    if(U16_IS_LEAD((s)[(i)++]) && (i)!=(length) && U16_IS_TRAIL((s)[i])) { \
        ++(i); \
    } \
} while (0)

#define U16_FWD_N(s, i, length, n) do { \
    int32_t __N=(n); \
    while(__N>0 && ((i)<(length) || ((length)<0 && (s)[i]!=0))) { \
        U16_FWD_1(s, i, length); \
        --__N; \
    } \
} while (0)

#define U16_NEXT_UNSAFE(s, i, c) do { \
    (c)=(s)[(i)++]; \
    if(U16_IS_LEAD(c)) { \
        (c)=U16_GET_SUPPLEMENTARY((c), (s)[(i)++]); \
    } \
} while (0)

#define U16_PREV_UNSAFE(s, i, c) do { \
    (c)=(s)[--(i)]; \
    if(U16_IS_TRAIL(c)) { \
        (c)=U16_GET_SUPPLEMENTARY((s)[--(i)], (c)); \
    } \
} while (0)

#define U16_NEXT(s, i, length, c) do { \
    (c)=(s)[(i)++]; \
    if(U16_IS_LEAD(c)) { \
        uint16_t __c2; \
        if((i)!=(length) && U16_IS_TRAIL(__c2=(s)[(i)])) { \
            ++(i); \
            (c)=U16_GET_SUPPLEMENTARY((c), __c2); \
        } \
    } \
} while (0)

#define U16_PREV(s, start, i, c) do { \
    (c)=(s)[--(i)]; \
    if(U16_IS_TRAIL(c)) { \
        uint16_t __c2; \
        if((i)>(start) && U16_IS_LEAD(__c2=(s)[(i)-1])) { \
            --(i); \
            (c)=U16_GET_SUPPLEMENTARY(__c2, (c)); \
        } \
    } \
} while (0)

#endif /* MODULES_IDN_UIDNAMACRO_H_ */
