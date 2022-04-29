
#ifndef MODULES_IDN_UIDNATYPES_H_
#define MODULES_IDN_UIDNATYPES_H_

#include "unicode/uidna.h"
#include <atomic>
#include <string>
#include <string.h>

#define U_IS_SURROGATE(c) (((c)&0xfffff800)==0xd800)
#define U16_IS_SURROGATE(c) U_IS_SURROGATE(c)

namespace uidna {

using UChar32 = int32_t;
using UClassID = void *;

using u_atomic_int32_t = std::atomic<int32_t>;

class StringPiece;
class ByteSink;

typedef enum {
    UNORM2_COMPOSE,
    UNORM2_DECOMPOSE,
    UNORM2_FCD,
    UNORM2_COMPOSE_CONTIGUOUS
} UNormalization2Mode;

typedef enum UNormalizationCheckResult {
	UNORM_NO,
	UNORM_YES,
	UNORM_MAYBE
} UNormalizationCheckResult;

/*
 * IDNA error bit set values.
 * When a domain name or label fails a processing step or does not meet the
 * validity criteria, then one or more of these error bits are set.
 */
enum {
    UIDNA_ERROR_EMPTY_LABEL=1,
    UIDNA_ERROR_LABEL_TOO_LONG=2,
    UIDNA_ERROR_DOMAIN_NAME_TOO_LONG=4,
    UIDNA_ERROR_LEADING_HYPHEN=8,
    UIDNA_ERROR_TRAILING_HYPHEN=0x10,
    UIDNA_ERROR_HYPHEN_3_4=0x20,
    UIDNA_ERROR_LEADING_COMBINING_MARK=0x40,
    UIDNA_ERROR_DISALLOWED=0x80,
    UIDNA_ERROR_PUNYCODE=0x100,
    UIDNA_ERROR_LABEL_HAS_DOT=0x200,
    UIDNA_ERROR_INVALID_ACE_LABEL=0x400,
    UIDNA_ERROR_BIDI=0x800,
    UIDNA_ERROR_CONTEXTJ=0x1000,
    UIDNA_ERROR_CONTEXTO_PUNCTUATION=0x2000,
    UIDNA_ERROR_CONTEXTO_DIGITS=0x4000
};

struct UDataInfo {
    uint16_t size;
    uint16_t reservedWord;
    uint8_t isBigEndian;
    uint8_t charsetFamily;
    uint8_t sizeofUChar;
    uint8_t reservedByte;
    uint8_t dataFormat[4];
    uint8_t formatVersion[4];
    uint8_t dataVersion[4];
};

struct MappedData {
    uint16_t    headerSize;
    uint8_t     magic1;
    uint8_t     magic2;
};

struct DataHeader {
	MappedData  dataHeader;
	UDataInfo   info;
};

UChar* u_strFindFirst(const UChar *s, int32_t length, const UChar *sub, int32_t subLength);

inline UBool U_SUCCESS(UErrorCode code) {
	return (UBool) (code <= U_ZERO_ERROR);
}

inline UBool U_FAILURE(UErrorCode code) {
	return (UBool) (code > U_ZERO_ERROR);
}

inline auto u_strlen(const UChar *s) {
	return std::char_traits<UChar>::length(s);
}

inline auto uprv_strlen(const char *s) {
	return std::char_traits<char>::length(s);
}

inline UChar* u_memmove(UChar *dest, const UChar *src, int32_t count) {
	if (count > 0) {
		::memmove(dest, src, (size_t) count * sizeof(UChar));
	}
	return dest;
}

inline UChar* u_memset(UChar *dest, UChar c, int32_t count) {
	if (count > 0) {
		UChar *ptr = dest;
		UChar *limit = dest + count;

		while (ptr < limit) {
			*(ptr++) = c;
		}
	}
	return dest;
}

inline UChar* u_memcpy(UChar *dest, const UChar *src, int32_t count) {
	if (count > 0) {
		::memcpy(dest, src, (size_t) count * sizeof(UChar));
	}
	return dest;
}

inline int32_t u_memcmp(const UChar *buf1, const UChar *buf2, int32_t count) {
	if (count > 0) {
		const UChar *limit = buf1 + count;
		int32_t result;

		while (buf1 < limit) {
			result = (int32_t) (uint16_t) *buf1 - (int32_t) (uint16_t) *buf2;
			if (result != 0) {
				return result;
			}
			buf1++;
			buf2++;
		}
	}
	return 0;
}

inline UChar* u_strchr(const UChar *s, UChar c) {
	if (U16_IS_SURROGATE(c)) {
		/* make sure to not find half of a surrogate pair */
		return u_strFindFirst(s, -1, &c, 1);
	} else {
		UChar cs;

		/* trivial search for a BMP code point */
		for (;;) {
			if ((cs = *s) == c) {
				return (UChar*) s;
			}
			if (cs == 0) {
				return NULL;
			}
			++s;
		}
	}
	return NULL;
}

inline UChar* u_memchr(const UChar *s, UChar c, int32_t count) {
	if (count <= 0) {
		return NULL; /* no string */
	} else if (U16_IS_SURROGATE(c)) {
		/* make sure to not find half of a surrogate pair */
		return u_strFindFirst(s, count, &c, 1);
	} else {
		/* trivial search for a BMP code point */
		const UChar *limit = s + count;
		do {
			if (*s == c) {
				return (UChar*) s;
			}
		} while (++s != limit);
		return NULL;
	}
}

int32_t u_terminateUChars(UChar *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode);
int32_t u_terminateChars(char *dest, int32_t destCapacity, int32_t length, UErrorCode *pErrorCode);

void * uprv_malloc(size_t s);
void * uprv_realloc(void * buffer, size_t size);
void uprv_free(void *buffer);
void *uprv_calloc(size_t num, size_t size);

UChar32 utf8_prevCharSafeBody(const uint8_t *s, int32_t start, int32_t *pi, UChar32 c, int strict);

char* u_strToUTF8WithSub(char *dest, int32_t destCapacity, int32_t *pDestLength, const UChar *pSrc,
		int32_t srcLength, UChar32 subchar, int32_t *pNumSubstitutions, UErrorCode *pErrorCode);

extern const uint8_t uts46_data[];

}

#endif /* LIBSTAPPLER_MODULES_IDN_SPUIDNATYPES_H_ */
