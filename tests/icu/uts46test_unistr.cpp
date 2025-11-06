// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
******************************************************************************
* Copyright (C) 1999-2016, International Business Machines Corporation and
* others. All Rights Reserved.
******************************************************************************
*
* File unistr.cpp
*
* Modification History:
*
*   Date        Name        Description
*   09/25/98    stephen     Creation.
*   04/20/99    stephen     Overhauled per 4/16 code review.
*   07/09/99    stephen     Renamed {hi,lo},{byte,word} to icu_X for HP/UX
*   11/18/99    aliu        Added handleReplaceBetween() to make inherit from
*                           Replaceable.
*   06/25/01    grhoten     Removed the dependency on iostream
******************************************************************************
*/

#ifdef UIDNA_SOURCES
#include "u_unistr.h"
#include "u_char.h"
#include "u_macro.h"
#endif

#include "uts46test.h"

#define U16_GET(s, start, i, length, c) do { \
    (c)=(s)[i]; \
    if(U16_IS_SURROGATE(c)) { \
        uint16_t __c2; \
        if(U16_IS_SURROGATE_LEAD(c)) { \
            if((i)+1!=(length) && U16_IS_TRAIL(__c2=(s)[(i)+1])) { \
                (c)=U16_GET_SUPPLEMENTARY((c), __c2); \
            } \
        } else { \
            if((i)>(start) && U16_IS_LEAD(__c2=(s)[(i)-1])) { \
                (c)=U16_GET_SUPPLEMENTARY(__c2, (c)); \
            } \
        } \
    } \
} while (0)

#define U16_APPEND(s, i, capacity, c, isError) do { \
    if((uint32_t)(c)<=0xffff) { \
        (s)[(i)++]=(uint16_t)(c); \
    } else if((uint32_t)(c)<=0x10ffff && (i)+1<(capacity)) { \
        (s)[(i)++]=(uint16_t)(((c)>>10)+0xd7c0); \
        (s)[(i)++]=(uint16_t)(((c)&0x3ff)|0xdc00); \
    } else /* c>0x10ffff or not enough space */ { \
        (isError)=true; \
    } \
} while (0)

namespace uidna {

typedef UChar (*UNESCAPE_CHAR_AT)(int32_t offset, void *context);

static const UChar UNESCAPE_MAP[] = {
    /*"   0x22, 0x22 */
    /*'   0x27, 0x27 */
    /*?   0x3F, 0x3F */
    /*\   0x5C, 0x5C */
    /*a*/ 0x61, 0x07,
    /*b*/ 0x62, 0x08,
    /*e*/ 0x65, 0x1b,
    /*f*/ 0x66, 0x0c,
    /*n*/ 0x6E, 0x0a,
    /*r*/ 0x72, 0x0d,
    /*t*/ 0x74, 0x09,
    /*v*/ 0x76, 0x0b
};
enum { UNESCAPE_MAP_LENGTH = UPRV_LENGTHOF(UNESCAPE_MAP) };

/* Convert one octal digit to a numeric value 0..7, or -1 on failure */
static int32_t _digit8(UChar c) {
	if (c >= u'0' && c <= u'7') {
		return c - u'0';
	}
	return -1;
}

/* Convert one hex digit to a numeric value 0..F, or -1 on failure */
static int32_t _digit16(UChar c) {
	if (c >= u'0' && c <= u'9') {
		return c - u'0';
	}
	if (c >= u'A' && c <= u'F') {
		return c - (u'A' - 10);
	}
	if (c >= u'a' && c <= u'f') {
		return c - (u'a' - 10);
	}
	return -1;
}

static UChar32 u_unescapeAt(UNESCAPE_CHAR_AT charAt, int32_t *offset, int32_t length, void *context) {
	int32_t start = *offset;
	UChar32 c;
	UChar32 result = 0;
	int8_t n = 0;
	int8_t minDig = 0;
	int8_t maxDig = 0;
	int8_t bitsPerDigit = 4;
	int32_t dig;
	UBool braces = false;

	/* Check that offset is in range */
	if (*offset < 0 || *offset >= length) {
		goto err;
	}

	/* Fetch first UChar after '\\' */
	c = charAt((*offset)++, context);

	/* Convert hexadecimal and octal escapes */
	switch (c) {
	case u'u':
		minDig = maxDig = 4;
		break;
	case u'U':
		minDig = maxDig = 8;
		break;
	case u'x':
		minDig = 1;
		if (*offset < length && charAt(*offset, context) == u'{') {
			++(*offset);
			braces = true;
			maxDig = 8;
		} else {
			maxDig = 2;
		}
		break;
	default:
		dig = _digit8(c);
		if (dig >= 0) {
			minDig = 1;
			maxDig = 3;
			n = 1; /* Already have first octal digit */
			bitsPerDigit = 3;
			result = dig;
		}
		break;
	}
	if (minDig != 0) {
		while (*offset < length && n < maxDig) {
			c = charAt(*offset, context);
			dig = (bitsPerDigit == 3) ? _digit8(c) : _digit16(c);
			if (dig < 0) {
				break;
			}
			result = (result << bitsPerDigit) | dig;
			++(*offset);
			++n;
		}
		if (n < minDig) {
			goto err;
		}
		if (braces) {
			if (c != u'}') {
				goto err;
			}
			++(*offset);
		}
		if (result < 0 || result >= 0x110000) {
			goto err;
		}
		/* If an escape sequence specifies a lead surrogate, see if
		 * there is a trail surrogate after it, either as an escape or
		 * as a literal.  If so, join them up into a supplementary.
		 */
		if (*offset < length && U16_IS_LEAD(result)) {
			int32_t ahead = *offset + 1;
			c = charAt(*offset, context);
			if (c == u'\\' && ahead < length) {
				// Calling ourselves recursively may cause a stack overflow if
				// we have repeated escaped lead surrogates.
				// Limit the length to 11 ("x{0000DFFF}") after ahead.
				int32_t tailLimit = ahead + 11;
				if (tailLimit > length) {
					tailLimit = length;
				}
				c = u_unescapeAt(charAt, &ahead, tailLimit, context);
			}
			if (U16_IS_TRAIL(c)) {
				*offset = ahead;
				result = U16_GET_SUPPLEMENTARY(result, c);
			}
		}
		return result;
	}

	/* Convert C-style escapes in table */
	for (int32_t i = 0; i < UNESCAPE_MAP_LENGTH; i += 2) {
		if (c == UNESCAPE_MAP[i]) {
			return UNESCAPE_MAP[i + 1];
		} else if (c < UNESCAPE_MAP[i]) {
			break;
		}
	}

	/* Map \cX to control-X: X & 0x1F */
	if (c == u'c' && *offset < length) {
		c = charAt((*offset)++, context);
		if (U16_IS_LEAD(c) && *offset < length) {
			UChar c2 = charAt(*offset, context);
			if (U16_IS_TRAIL(c2)) {
				++(*offset);
				c = U16_GET_SUPPLEMENTARY(c, c2);
			}
		}
		return 0x1F & c;
	}

	/* If no special forms are recognized, then consider
	 * the backslash to generically escape the next character.
	 * Deal with surrogate pairs. */
	if (U16_IS_LEAD(c) && *offset < length) {
		UChar c2 = charAt(*offset, context);
		if (U16_IS_TRAIL(c2)) {
			++(*offset);
			return U16_GET_SUPPLEMENTARY(c, c2);
		}
	}
	return c;

	err:
	/* Invalid escape sequence */
	*offset = start; /* Reset to initial value */
	return (UChar32) 0xFFFFFFFF;
}

UnicodeString& UnicodeString::trim() {
	if (isBogus()) {
		return *this;
	}

	UChar *array = getArrayStart();
	UChar32 c;
	int32_t oldLength = this->length();
	int32_t i = oldLength, length;

	// first cut off trailing white space
	for (;;) {
		length = i;
		if (i <= 0) {
			break;
		}
		U16_PREV(array, 0, i, c);
		if (!(c == 0x20 || u_isWhitespace(c))) {
			break;
		}
	}
	if (length < oldLength) {
		setLength(length);
	}

	// find leading white space
	int32_t start;
	i = 0;
	for (;;) {
		start = i;
		if (i >= length) {
			break;
		}
		U16_NEXT(array, i, length, c);
		if (!(c == 0x20 || u_isWhitespace(c))) {
			break;
		}
	}

	// move string forward over leading white space
	if (start > 0) {
		doReplace(0, start, 0, 0, 0);
	}

	return *this;
}

UnicodeString UnicodeString::unescape() const {
    UnicodeString result(length(), (UChar32)0, (int32_t)0); // construct with capacity
    if (result.isBogus()) {
        return result;
    }
    const UChar *array = getBuffer();
    int32_t len = length();
    int32_t prev = 0;
    for (int32_t i=0;;) {
        if (i == len) {
            result.append(array, prev, len - prev);
            break;
        }
        if (array[i++] == 0x5C /*'\\'*/) {
            result.append(array, prev, (i - 1) - prev);
            UChar32 c = unescapeAt(i); // advances i
            if (c < 0) {
                result.remove(); // return empty string
                break; // invalid escape sequence
            }
            result.append(c);
            prev = i;
        }
    }
    return result;
}

static UChar UnicodeString_charAt(int32_t offset, void *context) {
	return ((UnicodeString*) context)->charAt(offset);
}

UChar32 UnicodeString::unescapeAt(int32_t &offset) const {
	return u_unescapeAt(UnicodeString_charAt, &offset, length(), (void*) this);
}

UnicodeString::UnicodeString(int32_t capacity, UChar32 c, int32_t count) {
	fUnion.fFields.fLengthAndFlags = 0;
	if (count <= 0 || (uint32_t) c > 0x10ffff) {
		// just allocate and do not do anything else
		allocate(capacity);
	} else if (c <= 0xffff) {
		int32_t length = count;
		if (capacity < length) {
			capacity = length;
		}
		if (allocate(capacity)) {
			UChar *array = getArrayStart();
			UChar unit = (UChar) c;
			for (int32_t i = 0; i < length; ++i) {
				array[i] = unit;
			}
			setLength(length);
		}
	} else {  // supplementary code point, write surrogate pairs
		if (count > (INT32_MAX / 2)) {
			// We would get more than 2G UChars.
			allocate(capacity);
			return;
		}
		int32_t length = count * 2;
		if (capacity < length) {
			capacity = length;
		}
		if (allocate(capacity)) {
			UChar *array = getArrayStart();
			UChar lead = U16_LEAD(c);
			UChar trail = U16_TRAIL(c);
			for (int32_t i = 0; i < length; i += 2) {
				array[i] = lead;
				array[i + 1] = trail;
			}
			setLength(length);
		}
	}
}

UnicodeString::UnicodeString(const char *codepageData) {
	fUnion.fFields.fLengthAndFlags = kShortString;
	if (codepageData != 0) {
		setToUTF8(codepageData);
	}
}

UnicodeString::UnicodeString(const char *codepageData, int32_t dataLength) {
	fUnion.fFields.fLengthAndFlags = kShortString;
	// if there's nothing to convert, do nothing
	if (codepageData == 0 || dataLength == 0 || dataLength < -1) {
		return;
	}
	if (dataLength == -1) {
		dataLength = (int32_t) uprv_strlen(codepageData);
	}
	setToUTF8(StringPiece(codepageData, dataLength));
}

UChar32 UnicodeString::char32At(int32_t offset) const {
	int32_t len = length();
	if ((uint32_t) offset < (uint32_t) len) {
		const UChar *array = getArrayStart();
		UChar32 c;
		U16_GET(array, 0, offset, len, c);
		return c;
	} else {
		return kInvalidUChar;
	}
}

UnicodeString &UnicodeString::append(UChar32 srcChar) {
	UChar buffer[U16_MAX_LENGTH];
	int32_t _length = 0;
	UBool isError = false;
	U16_APPEND(buffer, _length, U16_MAX_LENGTH, srcChar, isError);
	// We test isError so that the compiler does not complain that we don't.
	// If isError then _length==0 which turns the doAppend() into a no-op anyway.
	return isError ? *this : doAppend(buffer, 0, _length);
}

static const uint32_t invariantChars[4]={
    0xfffffbff, /* 00..1f but not 0a */
    0xffffffe5, /* 20..3f but not 21 23 24 */
    0x87fffffe, /* 40..5f but not 40 5b..5e */
    0x87fffffe  /* 60..7f but not 60 7b..7e */
};

#define UCHAR_IS_INVARIANT(c) (((c)<=0x7f) && (invariantChars[(c)>>5]&((uint32_t)1<<((c)&0x1f)))!=0)
#define UCHAR_TO_CHAR(c) c

static void u_UCharsToChars(const UChar *us, char *cs, int32_t length) {
	UChar u;

	while (length > 0) {
		u = *us++;
		if (!UCHAR_IS_INVARIANT(u)) {
			// U_ASSERT (FALSE); /* Variant characters were used. These are not portable in ICU. */
			u = 0;
		}
		*cs++ = (char) UCHAR_TO_CHAR(u);
		--length;
	}
}

int32_t UnicodeString::extract(int32_t start, int32_t length, char *target, int32_t targetCapacity, enum EInvariant) const {
	// if the arguments are illegal, then do nothing
	if (targetCapacity < 0 || (targetCapacity > 0 && target == NULL)) {
		return 0;
	}

	// pin the indices to legal values
	pinIndices(start, length);

	if (length <= targetCapacity) {
		u_UCharsToChars(getArrayStart() + start, target, length);
	}
	UErrorCode status = U_ZERO_ERROR;
	return u_terminateChars(target, targetCapacity, length, &status);
}

int32_t UnicodeString::extract(int32_t start, int32_t len, char *target, uint32_t dstSize) const {
	// if the arguments are illegal, then do nothing
	if (/*dstSize < 0 || */(dstSize > 0 && target == 0)) {
		return 0;
	}
	return toUTF8(start, len, target, dstSize <= 0x7fffffff ? (int32_t) dstSize : 0x7fffffff);
}

UnicodeString::UnicodeString(UChar ch) {
	fUnion.fFields.fLengthAndFlags = kLength1 | kShortString;
	fUnion.fStackFields.fBuffer[0] = ch;
}

UnicodeString&
UnicodeString::findAndReplace(int32_t start, int32_t length, const UnicodeString &oldText, int32_t oldStart, int32_t oldLength, const UnicodeString &newText, int32_t newStart, int32_t newLength) {
	if (isBogus() || oldText.isBogus() || newText.isBogus()) {
		return *this;
	}

	pinIndices(start, length);
	oldText.pinIndices(oldStart, oldLength);
	newText.pinIndices(newStart, newLength);

	if (oldLength == 0) {
		return *this;
	}

	while (length > 0 && length >= oldLength) {
		int32_t pos = indexOf(oldText, oldStart, oldLength, start, length);
		if (pos < 0) {
			// no more oldText's here: done
			break;
		} else {
			// we found oldText, replace it by newText and go beyond it
			replace(pos, oldLength, newText, newStart, newLength);
			length -= pos + oldLength - start;
			start = pos + newLength;
		}
	}

	return *this;
}

UnicodeString::UnicodeString(const UnicodeString &that) {
	fUnion.fFields.fLengthAndFlags = kShortString;
	copyFrom(that);
}

UnicodeString::UnicodeString(const char *codepageData, const char *codepage) {
	fUnion.fFields.fLengthAndFlags = kShortString;
	if (codepageData != 0) {
		doCodepageCreate(codepageData, (int32_t) uprv_strlen(codepageData), codepage);
	}
}

UnicodeString::UnicodeString(const char *codepageData, int32_t dataLength, const char *codepage) {
	fUnion.fFields.fLengthAndFlags = kShortString;
	if (codepageData != 0) {
		doCodepageCreate(codepageData, dataLength, codepage);
	}
}

UnicodeString operator+(const UnicodeString &s1, const UnicodeString &s2) {
	return UnicodeString(s1.length() + s2.length() + 1, (UChar32) 0, 0).append(s1).append(s2);
}

UBool UnicodeString::doEquals(const UnicodeString &text, int32_t len) const {
	// Requires: this & text not bogus and have same lengths.
	// Byte-wise comparison works for equality regardless of endianness.
	return ::memcmp(getArrayStart(), text.getArrayStart(), len * sizeof(UChar)) == 0;
}

#define US_INV UnicodeString::kInvariant

IcuTestErrorCode::~IcuTestErrorCode() {
    // Safe because our errlog() does not throw exceptions.
    if(isFailure()) {
        errlog(false, u"destructor: expected success", nullptr);
    }
}

UBool IcuTestErrorCode::errIfFailureAndReset() {
	if (isFailure()) {
		errlog(false, u"expected success", nullptr);
		reset();
		return true;
	} else {
		reset();
		return false;
	}
}

UBool IcuTestErrorCode::errIfFailureAndReset(const char *fmt, ...) {
	if (isFailure()) {
		char buffer[4000];
		va_list ap;
		va_start(ap, fmt);
		vsprintf(buffer, fmt, ap);
		va_end(ap);
		errlog(false, u"expected success", buffer);
		reset();
		return true;
	} else {
		reset();
		return false;
	}
}

UBool IcuTestErrorCode::errDataIfFailureAndReset() {
	if (isFailure()) {
		errlog(true, u"data: expected success", nullptr);
		reset();
		return true;
	} else {
		reset();
		return false;
	}
}

UBool IcuTestErrorCode::errDataIfFailureAndReset(const char *fmt, ...) {
	if (isFailure()) {
		char buffer[4000];
		va_list ap;
		va_start(ap, fmt);
		vsprintf(buffer, fmt, ap);
		va_end(ap);
		errlog(true, u"data: expected success", buffer);
		reset();
		return true;
	} else {
		reset();
		return false;
	}
}

UBool IcuTestErrorCode::expectErrorAndReset(UErrorCode expectedError) {
	if (get() != expectedError) {
		errlog(false, UnicodeString(u"expected: ") + u_errorName(expectedError), nullptr);
	}
	UBool retval = isFailure();
	reset();
	return retval;
}

UBool IcuTestErrorCode::expectErrorAndReset(UErrorCode expectedError, const char *fmt, ...) {
	if (get() != expectedError) {
		char buffer[4000];
		va_list ap;
		va_start(ap, fmt);
		vsprintf(buffer, fmt, ap);
		va_end(ap);
		errlog(false, UnicodeString(u"expected: ") + u_errorName(expectedError), buffer);
	}
	UBool retval = isFailure();
	reset();
	return retval;
}

void IcuTestErrorCode::setScope(const char *message) {
	scopeMessage.remove().append( { message, -1, US_INV });
}

void IcuTestErrorCode::setScope(const UnicodeString &message) {
	scopeMessage = message;
}

void IcuTestErrorCode::handleFailure() const {
	errlog(false, u"(handleFailure)", nullptr);
}

void IcuTestErrorCode::errlog(UBool dataErr, const UnicodeString &mainMessage, const char *extraMessage) const {
	UnicodeString msg(testName, -1, US_INV);
	msg.append(u' ').append(mainMessage);
	msg.append(u" but got error: ").append(UnicodeString(errorName(), -1, US_INV));

	if (!scopeMessage.isEmpty()) {
		msg.append(u" scope: ").append(scopeMessage);
	}

	if (extraMessage != nullptr) {
		msg.append(u" - ").append(UnicodeString(extraMessage, -1, US_INV));
	}

	if (dataErr || get() == U_MISSING_RESOURCE_ERROR || get() == U_FILE_ACCESS_ERROR) {
		testClass.dataerrln(msg);
	} else {
		testClass.errln(msg);
	}
}

#define CHAR_TO_UCHAR(c) c

static void u_charsToUChars(const char *cs, UChar *us, int32_t length) {
	UChar u;
	uint8_t c;

	/*
	 * Allow the entire ASCII repertoire to be mapped _to_ Unicode.
	 * For EBCDIC systems, this works for characters with codes from
	 * codepages 37 and 1047 or compatible.
	 */
	while (length > 0) {
		c = (uint8_t) (*cs++);
		u = (UChar) CHAR_TO_UCHAR(c);
		//U_ASSERT((u != 0 || c == 0)); /* only invariant chars converted? */
		*us++ = u;
		--length;
	}
}

UnicodeString::UnicodeString(const char *src, int32_t length, EInvariant) {
	fUnion.fFields.fLengthAndFlags = kShortString;
	if (src == NULL) {
		// treat as an empty string
	} else {
		if (length < 0) {
			length = (int32_t) uprv_strlen(src);
		}
		if (cloneArrayIfNeeded(length, length, false)) {
			u_charsToUChars(src, getArrayStart(), length);
			setLength(length);
		} else {
			setToBogus();
		}
	}
}

int32_t UnicodeString::doIndexOf(UChar c, int32_t start, int32_t length) const {
	// pin indices
	pinIndices(start, length);

	// find the first occurrence of c
	const UChar *array = getArrayStart();
	const UChar *match = u_memchr(array + start, c, length);
	if (match == NULL) {
		return -1;
	} else {
		return (int32_t) (match - array);
	}
}

StringPiece::StringPiece(const char *str) :
		ptr_(str), length_((str == NULL) ? 0 : static_cast<int32_t>(uprv_strlen(str))) {
}

void StringPiece::set(const char *str) {
	ptr_ = str;
	if (str != NULL)
		length_ = static_cast<int32_t>(uprv_strlen(str));
	else
		length_ = 0;
}

int32_t StringPiece::find(StringPiece needle, int32_t offset) {
	if (length() == 0 && needle.length() == 0) {
		return 0;
	}
	// TODO: Improve to be better than O(N^2)?
	for (int32_t i = offset; i < length(); i++) {
		int32_t j = 0;
		for (; j < needle.length(); i++, j++) {
			if (data()[i] != needle.data()[j]) {
				i -= j;
				goto outer_end;
			}
		}
		return i - j;
		outer_end: void();
	}
	return -1;
}

int32_t StringPiece::compare(StringPiece other) {
	int32_t i = 0;
	for (; i < length(); i++) {
		if (i == other.length()) {
			// this is longer
			return 1;
		}
		char a = data()[i];
		char b = other.data()[i];
		if (a < b) {
			return -1;
		} else if (a > b) {
			return 1;
		}
	}
	if (i < other.length()) {
		// other is longer
		return -1;
	}
	return 0;
}

UBool operator==(const StringPiece& x, const StringPiece& y) {
	int32_t len = x.size();
	if (len != y.size()) {
		return false;
	}
	if (len == 0) {
		return true;
	}
	const char* p = x.data();
	const char* p2 = y.data();
	// Test last byte in case strings share large common prefix
	--len;
	if (p[len] != p2[len]) return false;
	// At this point we can, but don't have to, ignore the last byte.
	return ::memcmp(p, p2, len) == 0;
}

const int32_t StringPiece::npos = 0x7fffffff;

int32_t UnicodeString::indexOf(const UChar *srcChars, int32_t srcStart, int32_t srcLength, int32_t start, int32_t length) const {
	if (isBogus() || srcChars == 0 || srcStart < 0 || srcLength == 0) {
		return -1;
	}

	// UnicodeString does not find empty substrings
	if (srcLength < 0 && srcChars[srcStart] == 0) {
		return -1;
	}

	// get the indices within bounds
	pinIndices(start, length);

	// find the first occurrence of the substring
	const UChar *array = getArrayStart();
	const UChar *match = u_strFindFirst(array + start, length, srcChars + srcStart, srcLength);
	if (match == NULL) {
		return -1;
	} else {
		return (int32_t) (match - array);
	}
}

ErrorCode::~ErrorCode() { }

UErrorCode ErrorCode::reset() {
	UErrorCode code = errorCode;
	errorCode = U_ZERO_ERROR;
	return code;
}

void ErrorCode::assertSuccess() const {
	if (isFailure()) {
		handleFailure();
	}
}

const char* ErrorCode::errorName() const {
	return u_errorName(errorCode);
}

int32_t UnicodeString::toUTF8(int32_t start, int32_t len, char *target, int32_t capacity) const {
	pinIndices(start, len);
	int32_t length8;
	UErrorCode errorCode = U_ZERO_ERROR;
	u_strToUTF8WithSub(target, capacity, &length8, getBuffer() + start, len, 0xFFFD,  // Standard substitution character.
			NULL,    // Don't care about number of substitutions.
			&errorCode);
	return length8;
}

#define UCNV_FAST_IS_UTF8(name) \
    (((name[0]=='U' ? \
      (                name[1]=='T' && name[2]=='F') : \
      (name[0]=='u' && name[1]=='t' && name[2]=='f'))) \
  && (name[3]=='-' ? \
     (name[4]=='8' && name[5]==0) : \
     (name[3]=='8' && name[4]==0)))

static const char *ucnv_getDefaultName() {
    return "UTF-8";
}

void UnicodeString::doCodepageCreate(const char *codepageData, int32_t dataLength, const char *codepage) {
	// if there's nothing to convert, do nothing
	if (codepageData == 0 || dataLength == 0 || dataLength < -1) {
		return;
	}
	if (dataLength == -1) {
		dataLength = (int32_t) uprv_strlen(codepageData);
	}

	UErrorCode status = U_ZERO_ERROR;

	// create the converter
	// if the codepage is the default, use our cache
	// if it is an empty string, then use the "invariant character" conversion
	UConverter *converter;
	if (codepage == 0) {
		const char *defaultName = ucnv_getDefaultName();
		if (UCNV_FAST_IS_UTF8(defaultName)) {
			setToUTF8(StringPiece(codepageData, dataLength));
			return;
		}
		// converter = u_getDefaultConverter(&status);
	} else if (*codepage == 0) {
		// use the "invariant characters" conversion
		/*if (cloneArrayIfNeeded(dataLength, dataLength, FALSE)) {
			u_charsToUChars(codepageData, getArrayStart(), dataLength);
			setLength(dataLength);
		} else {
			setToBogus();
		}*/
		::abort();
		return;
	} else {
		//converter = ucnv_open(codepage, &status);
	}

	::abort();

	// if we failed, set the appropriate flags and return
	/*if (U_FAILURE(status)) {
		setToBogus();
		return;
	}

	// perform the conversion
	doCodepageCreate(codepageData, dataLength, converter, status);
	if (U_FAILURE(status)) {
		setToBogus();
	}

	// close the converter
	if (codepage == 0) {
		u_releaseDefaultConverter(converter);
	} else {
		ucnv_close(converter);
	}*/
}

}
