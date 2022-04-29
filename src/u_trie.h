// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// ucptrie.h (modified from utrie2.h)
// created: 2017dec29 Markus W. Scherer

#ifndef MODULES_IDN_UIDNATRIE_H_
#define MODULES_IDN_UIDNATRIE_H_

#include "u_types.h"

namespace uidna {

#define UNEWTRIE2_MAX_DATA_LENGTH (0x110000+0x40+0x40+0x400)

enum {
	UTRIE2_SHIFT_1=6+5,
	UTRIE2_SHIFT_2=5,
	UTRIE2_SHIFT_1_2=UTRIE2_SHIFT_1-UTRIE2_SHIFT_2,
	UTRIE2_OMITTED_BMP_INDEX_1_LENGTH=0x10000>>UTRIE2_SHIFT_1,
	UTRIE2_CP_PER_INDEX_1_ENTRY=1<<UTRIE2_SHIFT_1,
	UTRIE2_INDEX_2_BLOCK_LENGTH=1<<UTRIE2_SHIFT_1_2,
	UTRIE2_INDEX_2_MASK=UTRIE2_INDEX_2_BLOCK_LENGTH-1,
	UTRIE2_DATA_BLOCK_LENGTH=1<<UTRIE2_SHIFT_2,
	UTRIE2_DATA_MASK=UTRIE2_DATA_BLOCK_LENGTH-1,
	UTRIE2_INDEX_SHIFT=2,
	UTRIE2_DATA_GRANULARITY=1<<UTRIE2_INDEX_SHIFT,
	UTRIE2_INDEX_2_OFFSET=0,
	UTRIE2_LSCP_INDEX_2_OFFSET=0x10000>>UTRIE2_SHIFT_2,
	UTRIE2_LSCP_INDEX_2_LENGTH=0x400>>UTRIE2_SHIFT_2,
	UTRIE2_INDEX_2_BMP_LENGTH=UTRIE2_LSCP_INDEX_2_OFFSET+UTRIE2_LSCP_INDEX_2_LENGTH,
	UTRIE2_UTF8_2B_INDEX_2_OFFSET=UTRIE2_INDEX_2_BMP_LENGTH,
	UTRIE2_UTF8_2B_INDEX_2_LENGTH=0x800>>6,  /* U+0800 is the first code point after 2-byte UTF-8 */
	UTRIE2_INDEX_1_OFFSET=UTRIE2_UTF8_2B_INDEX_2_OFFSET+UTRIE2_UTF8_2B_INDEX_2_LENGTH,
	UTRIE2_MAX_INDEX_1_LENGTH=0x100000>>UTRIE2_SHIFT_1,
	UTRIE2_BAD_UTF8_DATA_OFFSET=0x80,
	UTRIE2_DATA_START_OFFSET=0xc0
};

enum {
	UNEWTRIE2_INDEX_GAP_OFFSET=UTRIE2_INDEX_2_BMP_LENGTH,
	UNEWTRIE2_INDEX_GAP_LENGTH=
		((UTRIE2_UTF8_2B_INDEX_2_LENGTH+UTRIE2_MAX_INDEX_1_LENGTH)+UTRIE2_INDEX_2_MASK)&
		~UTRIE2_INDEX_2_MASK,

	UNEWTRIE2_MAX_INDEX_2_LENGTH=
		(0x110000>>UTRIE2_SHIFT_2)+
		UTRIE2_LSCP_INDEX_2_LENGTH+
		UNEWTRIE2_INDEX_GAP_LENGTH+
		UTRIE2_INDEX_2_BLOCK_LENGTH,

	UNEWTRIE2_INDEX_1_LENGTH=0x110000>>UTRIE2_SHIFT_1
};

struct UNewTrie2 {
	int32_t index1[UNEWTRIE2_INDEX_1_LENGTH];
	int32_t index2[UNEWTRIE2_MAX_INDEX_2_LENGTH];
	uint32_t *data;
	uint32_t initialValue, errorValue;
	int32_t index2Length, dataCapacity, dataLength;
	int32_t firstFreeBlock;
	int32_t index2NullOffset, dataNullOffset;
	UChar32 highStart;
	UBool isCompacted;

	/**
	 * Multi-purpose per-data-block table.
	 *
	 * Before compacting:
	 *
	 * Per-data-block reference counters/free-block list.
	 *  0: unused
	 * >0: reference counter (number of index-2 entries pointing here)
	 * <0: next free data block in free-block list
	 *
	 * While compacting:
	 *
	 * Map of adjusted indexes, used in compactData() and compactIndex2().
	 * Maps from original indexes to new ones.
	 */
	int32_t map[UNEWTRIE2_MAX_DATA_LENGTH>>UTRIE2_SHIFT_2];
};

struct UTrie2 {
	/* protected: used by macros and functions for reading values */
	const uint16_t *index;
	const uint16_t *data16;     /* for fast UTF-8 ASCII access, if 16b data */
	const uint32_t *data32;     /* NULL if 16b data is used via index */

	int32_t indexLength, dataLength;
	uint16_t index2NullOffset;  /* 0xffff if there is no dedicated index-2 null block */
	uint16_t dataNullOffset;
	uint32_t initialValue;
	/** Value returned for out-of-range code points and illegal UTF-8. */
	uint32_t errorValue;

	/* Start of the last range which ends at U+10ffff, and its value. */
	UChar32 highStart;
	int32_t highValueIndex;

	/* private: used by builder and unserialization functions */
	void *memory;           /* serialized bytes; NULL if not frozen yet */
	int32_t length;         /* number of serialized bytes at memory; 0 if not frozen yet */
	UBool isMemoryOwned;    /* true if the trie owns the memory */
	UBool padding1;
	int16_t padding2;
	UNewTrie2 *newTrie;     /* builder object; NULL when frozen */
};

#define _UTRIE2_INDEX_FROM_SUPP(trieIndex, c) \
    (((int32_t)((trieIndex)[ \
        (trieIndex)[(UTRIE2_INDEX_1_OFFSET-UTRIE2_OMITTED_BMP_INDEX_1_LENGTH)+ \
                      ((c)>>UTRIE2_SHIFT_1)]+ \
        (((c)>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK)]) \
    <<UTRIE2_INDEX_SHIFT)+ \
    ((c)&UTRIE2_DATA_MASK))

/** Internal low-level trie getter. Returns a data index. */
#define _UTRIE2_INDEX_RAW(offset, trieIndex, c) \
    (((int32_t)((trieIndex)[(offset)+((c)>>UTRIE2_SHIFT_2)]) \
    <<UTRIE2_INDEX_SHIFT)+ \
    ((c)&UTRIE2_DATA_MASK))

#define _UTRIE2_INDEX_FROM_CP(trie, asciiOffset, c) \
    ((uint32_t)(c)<0xd800 ? \
        _UTRIE2_INDEX_RAW(0, (trie)->index, c) : \
        (uint32_t)(c)<=0xffff ? \
            _UTRIE2_INDEX_RAW( \
                (c)<=0xdbff ? UTRIE2_LSCP_INDEX_2_OFFSET-(0xd800>>UTRIE2_SHIFT_2) : 0, \
                (trie)->index, c) : \
            (uint32_t)(c)>0x10ffff ? \
                (asciiOffset)+UTRIE2_BAD_UTF8_DATA_OFFSET : \
                (c)>=(trie)->highStart ? \
                    (trie)->highValueIndex : \
                    _UTRIE2_INDEX_FROM_SUPP((trie)->index, c))

#define _UTRIE2_GET(trie, data, asciiOffset, c) \
    (trie)->data[_UTRIE2_INDEX_FROM_CP(trie, asciiOffset, c)]

#define UTRIE2_GET16(trie, c) _UTRIE2_GET((trie), index, (trie)->indexLength, (c))


enum UCPTrieType {
	UCPTRIE_TYPE_ANY = -1,
	UCPTRIE_TYPE_FAST,
	UCPTRIE_TYPE_SMALL
};

enum UCPTrieValueWidth {
	UCPTRIE_VALUE_BITS_ANY = -1,
	UCPTRIE_VALUE_BITS_16,
	UCPTRIE_VALUE_BITS_32,
	UCPTRIE_VALUE_BITS_8
};

enum {
    UCPTRIE_OPTIONS_DATA_LENGTH_MASK = 0xf000,
    UCPTRIE_OPTIONS_DATA_NULL_OFFSET_MASK = 0xf00,
    UCPTRIE_OPTIONS_RESERVED_MASK = 0x38,
    UCPTRIE_OPTIONS_VALUE_BITS_MASK = 7,
    /**
     * Value for index3NullOffset which indicates that there is no index-3 null block.
     * Bit 15 is unused for this value because this bit is used if the index-3 contains
     * 18-bit indexes.
     */
    UCPTRIE_NO_INDEX3_NULL_OFFSET = 0x7fff,
    UCPTRIE_NO_DATA_NULL_OFFSET = 0xfffff
};
enum {
    /** @internal */
    UCPTRIE_FAST_SHIFT = 6,

    /** Number of entries in a data block for code points below the fast limit. 64=0x40 @internal */
    UCPTRIE_FAST_DATA_BLOCK_LENGTH = 1 << UCPTRIE_FAST_SHIFT,

    /** Mask for getting the lower bits for the in-fast-data-block offset. @internal */
    UCPTRIE_FAST_DATA_MASK = UCPTRIE_FAST_DATA_BLOCK_LENGTH - 1,

    /** @internal */
    UCPTRIE_SMALL_MAX = 0xfff,

    /**
     * Offset from dataLength (to be subtracted) for fetching the
     * value returned for out-of-range code points and ill-formed UTF-8/16.
     * @internal
     */
    UCPTRIE_ERROR_VALUE_NEG_DATA_OFFSET = 1,
    /**
     * Offset from dataLength (to be subtracted) for fetching the
     * value returned for code points highStart..U+10FFFF.
     * @internal
     */
    UCPTRIE_HIGH_VALUE_NEG_DATA_OFFSET = 2
};

enum {
    /** The length of the BMP index table. 1024=0x400 */
    UCPTRIE_BMP_INDEX_LENGTH = 0x10000 >> UCPTRIE_FAST_SHIFT,

    UCPTRIE_SMALL_LIMIT = 0x1000,
    UCPTRIE_SMALL_INDEX_LENGTH = UCPTRIE_SMALL_LIMIT >> UCPTRIE_FAST_SHIFT,

    /** Shift size for getting the index-3 table offset. */
    UCPTRIE_SHIFT_3 = 4,

    /** Shift size for getting the index-2 table offset. */
    UCPTRIE_SHIFT_2 = 5 + UCPTRIE_SHIFT_3,

    /** Shift size for getting the index-1 table offset. */
    UCPTRIE_SHIFT_1 = 5 + UCPTRIE_SHIFT_2,

    /**
     * Difference between two shift sizes,
     * for getting an index-2 offset from an index-3 offset. 5=9-4
     */
    UCPTRIE_SHIFT_2_3 = UCPTRIE_SHIFT_2 - UCPTRIE_SHIFT_3,

    /**
     * Difference between two shift sizes,
     * for getting an index-1 offset from an index-2 offset. 5=14-9
     */
    UCPTRIE_SHIFT_1_2 = UCPTRIE_SHIFT_1 - UCPTRIE_SHIFT_2,

    /**
     * Number of index-1 entries for the BMP. (4)
     * This part of the index-1 table is omitted from the serialized form.
     */
    UCPTRIE_OMITTED_BMP_INDEX_1_LENGTH = 0x10000 >> UCPTRIE_SHIFT_1,

    /** Number of entries in an index-2 block. 32=0x20 */
    UCPTRIE_INDEX_2_BLOCK_LENGTH = 1 << UCPTRIE_SHIFT_1_2,

    /** Mask for getting the lower bits for the in-index-2-block offset. */
    UCPTRIE_INDEX_2_MASK = UCPTRIE_INDEX_2_BLOCK_LENGTH - 1,

    /** Number of code points per index-2 table entry. 512=0x200 */
    UCPTRIE_CP_PER_INDEX_2_ENTRY = 1 << UCPTRIE_SHIFT_2,

    /** Number of entries in an index-3 block. 32=0x20 */
    UCPTRIE_INDEX_3_BLOCK_LENGTH = 1 << UCPTRIE_SHIFT_2_3,

    /** Mask for getting the lower bits for the in-index-3-block offset. */
    UCPTRIE_INDEX_3_MASK = UCPTRIE_INDEX_3_BLOCK_LENGTH - 1,

    /** Number of entries in a small data block. 16=0x10 */
    UCPTRIE_SMALL_DATA_BLOCK_LENGTH = 1 << UCPTRIE_SHIFT_3,

    /** Mask for getting the lower bits for the in-small-data-block offset. */
    UCPTRIE_SMALL_DATA_MASK = UCPTRIE_SMALL_DATA_BLOCK_LENGTH - 1
};

union UCPTrieData {
    /** @internal */
    const void *ptr0;
    /** @internal */
    const uint16_t *ptr16;
    /** @internal */
    const uint32_t *ptr32;
    /** @internal */
    const uint8_t *ptr8;
};

struct UCPTrieHeader {
    /** "Tri3" in big-endian US-ASCII (0x54726933) */
    uint32_t signature;

    /**
     * Options bit field:
     * Bits 15..12: Data length bits 19..16.
     * Bits 11..8: Data null block offset bits 19..16.
     * Bits 7..6: UCPTrieType
     * Bits 5..3: Reserved (0).
     * Bits 2..0: UCPTrieValueWidth
     */
    uint16_t options;

    /** Total length of the index tables. */
    uint16_t indexLength;

    /** Data length bits 15..0. */
    uint16_t dataLength;

    /** Index-3 null block offset, 0x7fff or 0xffff if none. */
    uint16_t index3NullOffset;

    /** Data null block offset bits 15..0, 0xfffff if none. */
    uint16_t dataNullOffset;

    /**
     * First code point of the single-value range ending with U+10ffff,
     * rounded up and then shifted right by UCPTRIE_SHIFT_2.
     */
    uint16_t shiftedHighStart;
};

struct UCPTrie {
    /** @internal */
    const uint16_t *index;
    /** @internal */
    UCPTrieData data;

    /** @internal */
    int32_t indexLength;
    /** @internal */
    int32_t dataLength;
    /** Start of the last range which ends at U+10FFFF. @internal */
    UChar32 highStart;
    /** highStart>>12 @internal */
    uint16_t shifted12HighStart;

    /** @internal */
    int8_t type;  // UCPTrieType
    /** @internal */
    int8_t valueWidth;  // UCPTrieValueWidth

    /** padding/reserved @internal */
    uint32_t reserved32;
    /** padding/reserved @internal */
    uint16_t reserved16;

    /**
     * Internal index-3 null block offset.
     * Set to an impossibly high value (e.g., 0xffff) if there is no dedicated index-3 null block.
     * @internal
     */
    uint16_t index3NullOffset;
    /**
     * Internal data null block offset, not shifted.
     * Set to an impossibly high value (e.g., 0xfffff) if there is no dedicated data null block.
     * @internal
     */
    int32_t dataNullOffset;
    /** @internal */
    uint32_t nullValue;
};


#define UCPTRIE_SIG     0x54726933
#define UCPTRIE_OE_SIG  0x33697254

#define _UCPTRIE_FAST_INDEX(trie, c) \
    ((int32_t)(trie)->index[(c) >> UCPTRIE_FAST_SHIFT] + ((c) & UCPTRIE_FAST_DATA_MASK))

/** Internal trie getter for a code point at or above the fast limit. Returns the data index. @internal */
#define _UCPTRIE_SMALL_INDEX(trie, c) \
    ((c) >= (trie)->highStart ? \
        (trie)->dataLength - UCPTRIE_HIGH_VALUE_NEG_DATA_OFFSET : \
        ucptrie_internalSmallIndex(trie, c))

#define _UCPTRIE_CP_INDEX(trie, fastMax, c) \
    ((uint32_t)(c) <= (uint32_t)(fastMax) ? \
        _UCPTRIE_FAST_INDEX(trie, c) : \
        (uint32_t)(c) <= 0x10ffff ? \
            _UCPTRIE_SMALL_INDEX(trie, c) : \
            (trie)->dataLength - UCPTRIE_ERROR_VALUE_NEG_DATA_OFFSET)

#define UCPTRIE_FAST_BMP_GET(trie, dataAccess, c) dataAccess(trie, _UCPTRIE_FAST_INDEX(trie, c))
#define UCPTRIE_FAST_SUPP_GET(trie, dataAccess, c) dataAccess(trie, _UCPTRIE_SMALL_INDEX(trie, c))

#define UCPTRIE_16(trie, i) ((trie)->data.ptr16[i])
#define UCPTRIE_FAST_GET(trie, dataAccess, c) dataAccess(trie, _UCPTRIE_CP_INDEX(trie, 0xffff, c))

#define UCPTRIE_FAST_U16_NEXT(trie, dataAccess, src, limit, c, result) do { \
    (c) = *(src)++; \
    int32_t __index; \
    if (!U16_IS_SURROGATE(c)) { \
        __index = _UCPTRIE_FAST_INDEX(trie, c); \
    } else { \
        uint16_t __c2; \
        if (U16_IS_SURROGATE_LEAD(c) && (src) != (limit) && U16_IS_TRAIL(__c2 = *(src))) { \
            ++(src); \
            (c) = U16_GET_SUPPLEMENTARY((c), __c2); \
            __index = _UCPTRIE_SMALL_INDEX(trie, c); \
        } else { \
            __index = (trie)->dataLength - UCPTRIE_ERROR_VALUE_NEG_DATA_OFFSET; \
        } \
    } \
    (result) = dataAccess(trie, __index); \
} while (0)

#define UCPTRIE_FAST_U16_PREV(trie, dataAccess, start, src, c, result) do { \
    (c) = *--(src); \
    int32_t __index; \
    if (!U16_IS_SURROGATE(c)) { \
        __index = _UCPTRIE_FAST_INDEX(trie, c); \
    } else { \
        uint16_t __c2; \
        if (U16_IS_SURROGATE_TRAIL(c) && (src) != (start) && U16_IS_LEAD(__c2 = *((src) - 1))) { \
            --(src); \
            (c) = U16_GET_SUPPLEMENTARY(__c2, (c)); \
            __index = _UCPTRIE_SMALL_INDEX(trie, c); \
        } else { \
            __index = (trie)->dataLength - UCPTRIE_ERROR_VALUE_NEG_DATA_OFFSET; \
        } \
    } \
    (result) = dataAccess(trie, __index); \
} while (0)

#define UCPTRIE_FAST_U8_NEXT(trie, dataAccess, src, limit, result) do { \
    int32_t __lead = (uint8_t)*(src)++; \
    if (!U8_IS_SINGLE(__lead)) { \
        uint8_t __t1, __t2, __t3; \
        if ((src) != (limit) && \
            (__lead >= 0xe0 ? \
                __lead < 0xf0 ?  /* U+0800..U+FFFF except surrogates */ \
                    U8_LEAD3_T1_BITS[__lead &= 0xf] & (1 << ((__t1 = *(src)) >> 5)) && \
                    ++(src) != (limit) && (__t2 = *(src) - 0x80) <= 0x3f && \
                    (__lead = ((int32_t)(trie)->index[(__lead << 6) + (__t1 & 0x3f)]) + __t2, 1) \
                :  /* U+10000..U+10FFFF */ \
                    (__lead -= 0xf0) <= 4 && \
                    U8_LEAD4_T1_BITS[(__t1 = *(src)) >> 4] & (1 << __lead) && \
                    (__lead = (__lead << 6) | (__t1 & 0x3f), ++(src) != (limit)) && \
                    (__t2 = *(src) - 0x80) <= 0x3f && \
                    ++(src) != (limit) && (__t3 = *(src) - 0x80) <= 0x3f && \
                    (__lead = __lead >= (trie)->shifted12HighStart ? \
                        (trie)->dataLength - UCPTRIE_HIGH_VALUE_NEG_DATA_OFFSET : \
                        ucptrie_internalSmallU8Index((trie), __lead, __t2, __t3), 1) \
            :  /* U+0080..U+07FF */ \
                __lead >= 0xc2 && (__t1 = *(src) - 0x80) <= 0x3f && \
                (__lead = (int32_t)(trie)->index[__lead & 0x1f] + __t1, 1))) { \
            ++(src); \
        } else { \
            __lead = (trie)->dataLength - UCPTRIE_ERROR_VALUE_NEG_DATA_OFFSET;  /* ill-formed*/ \
        } \
    } \
    (result) = dataAccess(trie, __lead); \
} while (0)

#define UCPTRIE_FAST_U8_PREV(trie, dataAccess, start, src, result) do { \
    int32_t __index = (uint8_t)*--(src); \
    if (!U8_IS_SINGLE(__index)) { \
        __index = ucptrie_internalU8PrevIndex((trie), __index, (const uint8_t *)(start), \
                                              (const uint8_t *)(src)); \
        (src) -= __index & 7; \
        __index >>= 3; \
    } \
    (result) = dataAccess(trie, __index); \
} while (0)

UCPTrie* ucptrie_openFromBinary(UCPTrieType type, UCPTrieValueWidth valueWidth,
		const void *data, int32_t length, int32_t *pActualLength, UErrorCode *pErrorCode);
int32_t ucptrie_internalSmallIndex(const UCPTrie *trie, UChar32 c);
int32_t ucptrie_internalSmallU8Index(const UCPTrie *trie, int32_t lt1, uint8_t t2, uint8_t t3);
int32_t ucptrie_internalU8PrevIndex(const UCPTrie *trie, UChar32 c, const uint8_t *start, const uint8_t *src);

}

#endif /* MODULES_IDN_UIDNATRIE_H_ */
