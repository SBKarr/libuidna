// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
**********************************************************************
*   Copyright (C) 1998-2016, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* File unistr.h
*
* Modification History:
*
*   Date        Name        Description
*   09/25/98    stephen     Creation.
*   11/11/98    stephen     Changed per 11/9 code review.
*   04/20/99    stephen     Overhauled per 4/16 code review.
*   11/18/99    aliu        Made to inherit from Replaceable.  Added method
*                           handleReplaceBetween(); other methods unchanged.
*   06/25/01    grhoten     Remove dependency on iostream.
******************************************************************************
*/

#ifndef MODULES_IDN_UIDNAUNICODESTRING_H_
#define MODULES_IDN_UIDNAUNICODESTRING_H_

#include "u_types.h"

namespace uidna {

class UConverter;
class UnicodeString;
class Locale;
class BreakIterator;
class UStringCaseMapper;

using ConstChar16Ptr = const char16_t *;
using Char16Ptr = char16_t *;

#ifndef UNISTR_OBJECT_SIZE
#define UNISTR_OBJECT_SIZE 64
#endif

class StringPiece {
private:
	const char *ptr_;
	int32_t length_;

public:
	StringPiece() : ptr_(nullptr), length_(0) { }

	StringPiece(const char *str);

	StringPiece(std::nullptr_t p) : ptr_(p), length_(0) { }

	StringPiece(const std::string &str) :
			ptr_(str.data()), length_(static_cast<int32_t>(str.size())) {
	}

	StringPiece(const char *offset, int32_t len) :
			ptr_(offset), length_(len) {
	}

	StringPiece(const StringPiece &x, int32_t pos) {
		if (pos < 0) {
			pos = 0;
		} else if (pos > x.length_) {
			pos = x.length_;
		}
		ptr_ = x.ptr_ + pos;
		length_ = x.length_ - pos;
	}

	StringPiece(const StringPiece &x, int32_t pos, int32_t len) {
		if (pos < 0) {
			pos = 0;
		} else if (pos > x.length_) {
			pos = x.length_;
		}
		if (len < 0) {
			len = 0;
		} else if (len > x.length_ - pos) {
			len = x.length_ - pos;
		}
		ptr_ = x.ptr_ + pos;
		length_ = len;
	}

	const char* data() const { return ptr_; }
	int32_t size() const { return length_; }
	int32_t length() const { return length_; }
	UBool empty() const { return length_ == 0; }

	void clear() { ptr_ = nullptr; length_ = 0; }

	void set(const char *xdata, int32_t len) {
		ptr_ = xdata;
		length_ = len;
	}

	void set(const char *str);

	void remove_prefix(int32_t n) {
		if (n >= 0) {
			if (n > length_) {
				n = length_;
			}
			ptr_ += n;
			length_ -= n;
		}
	}

	void remove_suffix(int32_t n) {
		if (n >= 0) {
			if (n <= length_) {
				length_ -= n;
			} else {
				length_ = 0;
			}
		}
	}

	int32_t find(StringPiece needle, int32_t offset);

	int32_t compare(StringPiece other);

	static const int32_t npos; // = 0x7fffffff;

	StringPiece substr(int32_t pos, int32_t len = npos) const {
		return StringPiece(*this, pos, len);
	}
};

class ByteSink {
public:
	ByteSink() { }
	virtual ~ByteSink();

	virtual void Append(const char *bytes, int32_t n) = 0;

	inline void AppendU8(const char *bytes, int32_t n) {
		Append(bytes, n);
	}

	virtual char* GetAppendBuffer(int32_t min_capacity, int32_t desired_capacity_hint, char *scratch, int32_t scratch_capacity, int32_t *result_capacity);

	virtual void Flush();

private:
	ByteSink(const ByteSink&) = delete;
	ByteSink& operator=(const ByteSink&) = delete;
};

class CheckedArrayByteSink: public ByteSink {
public:
	CheckedArrayByteSink(char *outbuf, int32_t capacity);
	virtual ~CheckedArrayByteSink();
	virtual CheckedArrayByteSink& Reset();
	virtual void Append(const char *bytes, int32_t n) override;
	virtual char* GetAppendBuffer(int32_t min_capacity, int32_t desired_capacity_hint, char *scratch, int32_t scratch_capacity, int32_t *result_capacity) override;

	int32_t NumberOfBytesWritten() const {
		return size_;
	}
	UBool Overflowed() const {
		return overflowed_;
	}
	int32_t NumberOfBytesAppended() const {
		return appended_;
	}
private:
	char *outbuf_;
	const int32_t capacity_;
	int32_t size_;
	int32_t appended_;
	UBool overflowed_;

	CheckedArrayByteSink() = delete;
	CheckedArrayByteSink(const CheckedArrayByteSink&) = delete;
	CheckedArrayByteSink& operator=(const CheckedArrayByteSink&) = delete;
};

template<typename StringClass>
class StringByteSink: public ByteSink {
public:
	StringByteSink(StringClass *dest) : dest_(dest) { }

	StringByteSink(StringClass *dest, int32_t initialAppendCapacity) : dest_(dest) {
		if (initialAppendCapacity > 0 && (uint32_t) initialAppendCapacity > (dest->capacity() - dest->length())) {
			dest->reserve(dest->length() + initialAppendCapacity);
		}
	}

	virtual void Append(const char *data, int32_t n) override {
		dest_->append(data, n);
	}
private:
	StringClass *dest_;

	StringByteSink() = delete;
	StringByteSink(const StringByteSink&) = delete;
	StringByteSink& operator=(const StringByteSink&) = delete;
};

class UnicodeString {
public:
	virtual ~UnicodeString();

	enum EInvariant {
		kInvariant
	};

	inline bool operator==(const UnicodeString &text) const;
	inline bool operator!=(const UnicodeString &text) const;
	inline UBool operator>(const UnicodeString &text) const;
	inline UBool operator<(const UnicodeString &text) const;
	inline UBool operator>=(const UnicodeString &text) const;
	inline UBool operator<=(const UnicodeString &text) const;
	inline int8_t compare(const UnicodeString &text) const;
	inline int8_t compare(int32_t start, int32_t length, const UnicodeString &text) const;
	inline int8_t compare(int32_t start, int32_t length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const;
	inline int8_t compare(ConstChar16Ptr srcChars, int32_t srcLength) const;
	inline int8_t compare(int32_t start, int32_t length, const char16_t *srcChars) const;
	inline int8_t compare(int32_t start, int32_t length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength) const;
	inline int8_t compareBetween(int32_t start, int32_t limit, const UnicodeString &srcText, int32_t srcStart, int32_t srcLimit) const;
	inline int8_t compareCodePointOrder(const UnicodeString &text) const;
	inline int8_t compareCodePointOrder(int32_t start, int32_t length, const UnicodeString &srcText) const;
	inline int8_t compareCodePointOrder(int32_t start, int32_t length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const;
	inline int8_t compareCodePointOrder(ConstChar16Ptr srcChars, int32_t srcLength) const;
	inline int8_t compareCodePointOrder(int32_t start, int32_t length, const char16_t *srcChars) const;
	inline int8_t compareCodePointOrder(int32_t start, int32_t length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength) const;
	inline int8_t compareCodePointOrderBetween(int32_t start, int32_t limit, const UnicodeString &srcText, int32_t srcStart, int32_t srcLimit) const;
	inline int8_t caseCompare(const UnicodeString &text, uint32_t options) const;
	inline int8_t caseCompare(int32_t start, int32_t length, const UnicodeString &srcText, uint32_t options) const;
	inline int8_t caseCompare(int32_t start, int32_t length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength, uint32_t options) const;
	inline int8_t caseCompare(ConstChar16Ptr srcChars, int32_t srcLength, uint32_t options) const;
	inline int8_t caseCompare(int32_t start, int32_t length, const char16_t *srcChars, uint32_t options) const;
	inline int8_t caseCompare(int32_t start, int32_t length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength, uint32_t options) const;
	inline int8_t caseCompareBetween(int32_t start, int32_t limit, const UnicodeString &srcText, int32_t srcStart, int32_t srcLimit, uint32_t options) const;
	inline UBool startsWith(const UnicodeString &text) const;
	inline UBool startsWith(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const;
	inline UBool startsWith(ConstChar16Ptr srcChars, int32_t srcLength) const;
	inline UBool startsWith(const char16_t *srcChars, int32_t srcStart, int32_t srcLength) const;
	inline UBool endsWith(const UnicodeString &text) const;
	inline UBool endsWith(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const;
	inline UBool endsWith(ConstChar16Ptr srcChars, int32_t srcLength) const;
	inline UBool endsWith(const char16_t *srcChars, int32_t srcStart, int32_t srcLength) const;
	inline int32_t indexOf(const UnicodeString &text) const;
	inline int32_t indexOf(const UnicodeString &text, int32_t start) const;
	inline int32_t indexOf(const UnicodeString &text, int32_t start, int32_t length) const;
	inline int32_t indexOf(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength, int32_t start, int32_t length) const;
	inline int32_t indexOf(const char16_t *srcChars, int32_t srcLength, int32_t start) const;
	inline int32_t indexOf(ConstChar16Ptr srcChars, int32_t srcLength, int32_t start, int32_t length) const;
	int32_t indexOf(const char16_t *srcChars, int32_t srcStart, int32_t srcLength, int32_t start, int32_t length) const;
	inline int32_t indexOf(char16_t c) const;
	inline int32_t indexOf(UChar32 c) const;
	inline int32_t indexOf(char16_t c, int32_t start) const;
	inline int32_t indexOf(UChar32 c, int32_t start) const;
	inline int32_t indexOf(char16_t c, int32_t start, int32_t length) const;
	inline int32_t indexOf(UChar32 c, int32_t start, int32_t length) const;
	inline int32_t lastIndexOf(const UnicodeString &text) const;
	inline int32_t lastIndexOf(const UnicodeString &text, int32_t start) const;
	inline int32_t lastIndexOf(const UnicodeString &text, int32_t start, int32_t length) const;
	inline int32_t lastIndexOf(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength, int32_t start, int32_t length) const;
	inline int32_t lastIndexOf(const char16_t *srcChars, int32_t srcLength, int32_t start) const;
	inline int32_t lastIndexOf(ConstChar16Ptr srcChars, int32_t srcLength, int32_t start, int32_t length) const;
	int32_t lastIndexOf(const char16_t *srcChars, int32_t srcStart, int32_t srcLength, int32_t start, int32_t length) const;
	inline int32_t lastIndexOf(char16_t c) const;
	inline int32_t lastIndexOf(UChar32 c) const;
	inline int32_t lastIndexOf(char16_t c, int32_t start) const;
	inline int32_t lastIndexOf(UChar32 c, int32_t start) const;
	inline int32_t lastIndexOf(char16_t c, int32_t start, int32_t length) const;
	inline int32_t lastIndexOf(UChar32 c, int32_t start, int32_t length) const;
	inline char16_t charAt(int32_t offset) const;
	inline char16_t operator[](int32_t offset) const;
	UChar32 char32At(int32_t offset) const;
	int32_t getChar32Start(int32_t offset) const;
	int32_t getChar32Limit(int32_t offset) const;
	int32_t moveIndex32(int32_t index, int32_t delta) const;
	inline void extract(int32_t start, int32_t length, Char16Ptr dst, int32_t dstStart = 0) const;
	int32_t extract(Char16Ptr dest, int32_t destCapacity, UErrorCode &errorCode) const;
	inline void extract(int32_t start, int32_t length, UnicodeString &target) const;
	inline void extractBetween(int32_t start, int32_t limit, char16_t *dst, int32_t dstStart = 0) const;
	void extractBetween(int32_t start, int32_t limit, UnicodeString &target) const;
	int32_t extract(int32_t start, int32_t startLength, char *target, int32_t targetCapacity, enum EInvariant inv) const;
	int32_t extract(int32_t start, int32_t startLength, char *target, uint32_t targetLength) const;
	inline int32_t extract(int32_t start, int32_t startLength, char *target, const char *codepage = 0) const;
	int32_t extract(int32_t start, int32_t startLength, char *target, uint32_t targetLength, const char *codepage) const;
	int32_t extract(char *dest, int32_t destCapacity, UConverter *cnv, UErrorCode &errorCode) const;

	UnicodeString tempSubString(int32_t start = 0, int32_t length = INT32_MAX) const;
	inline UnicodeString tempSubStringBetween(int32_t start, int32_t limit = INT32_MAX) const;
	void toUTF8(ByteSink &sink) const;

	template<typename StringClass>
	StringClass& toUTF8String(StringClass &result) const {
		StringByteSink<StringClass> sbs(&result, length());
		toUTF8(sbs);
		return result;
	}

	int32_t toUTF32(UChar32 *utf32, int32_t capacity, UErrorCode &errorCode) const;
	inline int32_t length(void) const;
	int32_t countChar32(int32_t start = 0, int32_t length = INT32_MAX) const;
	UBool hasMoreChar32Than(int32_t start, int32_t length, int32_t number) const;

	inline UBool isEmpty(void) const;
	inline int32_t getCapacity(void) const;
	inline int32_t hashCode(void) const;
	inline UBool isBogus(void) const;
	UnicodeString& operator=(const UnicodeString &srcText);
	UnicodeString& fastCopyFrom(const UnicodeString &src);
	UnicodeString& operator=(UnicodeString &&src) noexcept;

	void swap(UnicodeString &other) noexcept;

	friend inline void
	swap(UnicodeString &s1, UnicodeString &s2) noexcept {
		s1.swap(s2);
	}

	inline UnicodeString& operator=(char16_t ch);
	inline UnicodeString& operator=(UChar32 ch);
	inline UnicodeString& setTo(const UnicodeString &srcText, int32_t srcStart);
	inline UnicodeString& setTo(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength);
	inline UnicodeString& setTo(const UnicodeString &srcText);
	inline UnicodeString& setTo(const char16_t *srcChars, int32_t srcLength);
	inline UnicodeString& setTo(char16_t srcChar);
	inline UnicodeString& setTo(UChar32 srcChar);

	UnicodeString& setTo(UBool isTerminated, ConstChar16Ptr text, int32_t textLength);
	UnicodeString& setTo(char16_t *buffer, int32_t buffLength, int32_t buffCapacity);
	void setToBogus();
	UnicodeString& setCharAt(int32_t offset, char16_t ch);
	inline UnicodeString& operator+=(char16_t ch);
	inline UnicodeString& operator+=(UChar32 ch);
	inline UnicodeString& operator+=(const UnicodeString &srcText);
	inline UnicodeString& append(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength);
	inline UnicodeString& append(const UnicodeString &srcText);
	inline UnicodeString& append(const char16_t *srcChars, int32_t srcStart, int32_t srcLength);
	inline UnicodeString& append(ConstChar16Ptr srcChars, int32_t srcLength);
	inline UnicodeString& append(char16_t srcChar);
	UnicodeString& append(UChar32 srcChar);
	inline UnicodeString& insert(int32_t start, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength);
	inline UnicodeString& insert(int32_t start, const UnicodeString &srcText);
	inline UnicodeString& insert(int32_t start, const char16_t *srcChars, int32_t srcStart, int32_t srcLength);
	inline UnicodeString& insert(int32_t start, ConstChar16Ptr srcChars, int32_t srcLength);
	inline UnicodeString& insert(int32_t start, char16_t srcChar);
	inline UnicodeString& insert(int32_t start, UChar32 srcChar);
	inline UnicodeString& replace(int32_t start, int32_t length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength);
	inline UnicodeString& replace(int32_t start, int32_t length, const UnicodeString &srcText);
	inline UnicodeString& replace(int32_t start, int32_t length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength);
	inline UnicodeString& replace(int32_t start, int32_t length, ConstChar16Ptr srcChars, int32_t srcLength);
	inline UnicodeString& replace(int32_t start, int32_t length, char16_t srcChar);
	UnicodeString& replace(int32_t start, int32_t length, UChar32 srcChar);
	inline UnicodeString& replaceBetween(int32_t start, int32_t limit, const UnicodeString &srcText);
	inline UnicodeString& replaceBetween(int32_t start, int32_t limit, const UnicodeString &srcText, int32_t srcStart, int32_t srcLimit);
	void handleReplaceBetween(int32_t start, int32_t limit, const UnicodeString &text);
	UBool hasMetaData() const;
	void copy(int32_t start, int32_t limit, int32_t dest);
	inline UnicodeString& findAndReplace(const UnicodeString &oldText, const UnicodeString &newText);
	inline UnicodeString& findAndReplace(int32_t start, int32_t length, const UnicodeString &oldText, const UnicodeString &newText);
	UnicodeString& findAndReplace(int32_t start, int32_t length, const UnicodeString &oldText, int32_t oldStart, int32_t oldLength, const UnicodeString &newText, int32_t newStart, int32_t newLength);
	inline UnicodeString& remove();
	inline UnicodeString& remove(int32_t start, int32_t length = (int32_t) INT32_MAX);
	inline UnicodeString& removeBetween(int32_t start, int32_t limit = (int32_t) INT32_MAX);
	inline UnicodeString& retainBetween(int32_t start, int32_t limit = INT32_MAX);
	UBool padLeading(int32_t targetLength, char16_t padChar = 0x0020);
	UBool padTrailing(int32_t targetLength, char16_t padChar = 0x0020);
	inline UBool truncate(int32_t targetLength);
	UnicodeString& trim(void);
	inline UnicodeString& reverse(void);
	inline UnicodeString& reverse(int32_t start, int32_t length);
	UnicodeString& toUpper(void);
	UnicodeString& toUpper(const Locale &locale);
	UnicodeString& toLower(void);
	UnicodeString& toLower(const Locale &locale);
	UnicodeString& toTitle(BreakIterator *titleIter);
	UnicodeString& toTitle(BreakIterator *titleIter, const Locale &locale);
	UnicodeString& toTitle(BreakIterator *titleIter, const Locale &locale, uint32_t options);
	UnicodeString& foldCase(uint32_t options = 0 /*U_FOLD_CASE_DEFAULT*/);
	char16_t* getBuffer(int32_t minCapacity);
	void releaseBuffer(int32_t newLength = -1);
	inline const char16_t* getBuffer() const;
	const char16_t* getTerminatedBuffer();
	inline UnicodeString();
	UnicodeString(int32_t capacity, UChar32 c, int32_t count);
	explicit UnicodeString(char16_t ch);
	explicit UnicodeString(UChar32 ch);
	UnicodeString(const char16_t *text);
	inline UnicodeString(const std::nullptr_t text);
	UnicodeString(const char16_t *text, int32_t textLength);
	inline UnicodeString(const std::nullptr_t text, int32_t textLength);
	UnicodeString(UBool isTerminated, ConstChar16Ptr text, int32_t textLength);
	UnicodeString(char16_t *buffer, int32_t buffLength, int32_t buffCapacity);
	inline UnicodeString(std::nullptr_t buffer, int32_t buffLength, int32_t buffCapacity);
	UnicodeString(const char *codepageData);
	UnicodeString(const char *codepageData, int32_t dataLength);
	UnicodeString(const char *codepageData, const char *codepage);
	UnicodeString(const char *codepageData, int32_t dataLength, const char *codepage);
	UnicodeString(const char *src, int32_t srcLength, UConverter *cnv, UErrorCode &errorCode);
	UnicodeString(const char *src, int32_t textLength, enum EInvariant inv);
	UnicodeString(const UnicodeString &that);
	UnicodeString(UnicodeString &&src) noexcept;
	UnicodeString(const UnicodeString &src, int32_t srcStart);
	UnicodeString(const UnicodeString &src, int32_t srcStart, int32_t srcLength);

	UnicodeString* clone() const;

	static UnicodeString fromUTF8(StringPiece utf8);
	static UnicodeString fromUTF32(const UChar32 *utf32, int32_t length);

	UnicodeString unescape() const;
	UChar32 unescapeAt(int32_t &offset) const;
	static UClassID getStaticClassID();
	UClassID getDynamicClassID() const;

protected:
	int32_t getLength() const;
	char16_t getCharAt(int32_t offset) const;
	UChar32 getChar32At(int32_t offset) const;

private:
	UnicodeString &setToUTF8(StringPiece utf8);
	int32_t toUTF8(int32_t start, int32_t len, char *target, int32_t capacity) const;
	UBool doEquals(const UnicodeString &text, int32_t len) const;
	inline int8_t doCompare(int32_t start, int32_t length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const;
	int8_t doCompare(int32_t start, int32_t length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength) const;

	inline int8_t doCompareCodePointOrder(int32_t start, int32_t length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const;
	int8_t doCompareCodePointOrder(int32_t start, int32_t length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength) const;
	inline int8_t doCaseCompare(int32_t start, int32_t length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength, uint32_t options) const;
	int8_t doCaseCompare(int32_t start, int32_t length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength, uint32_t options) const;
	int32_t doIndexOf(char16_t c, int32_t start, int32_t length) const;
	int32_t doIndexOf(UChar32 c, int32_t start, int32_t length) const;
	int32_t doLastIndexOf(char16_t c, int32_t start, int32_t length) const;
	int32_t doLastIndexOf(UChar32 c, int32_t start, int32_t length) const;
	void doExtract(int32_t start, int32_t length, char16_t *dst, int32_t dstStart) const;
	inline void doExtract(int32_t start, int32_t length, UnicodeString &target) const;
	inline char16_t doCharAt(int32_t offset) const;
	UnicodeString& doReplace(int32_t start, int32_t length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength);
	UnicodeString& doReplace(int32_t start, int32_t length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength);
	UnicodeString& doAppend(const UnicodeString &src, int32_t srcStart, int32_t srcLength);
	UnicodeString& doAppend(const char16_t *srcChars, int32_t srcStart, int32_t srcLength);
	UnicodeString& doReverse(int32_t start, int32_t length);
	int32_t doHashCode(void) const;
	inline char16_t* getArrayStart(void);
	inline const char16_t* getArrayStart(void) const;
	inline UBool hasShortLength() const;
	inline int32_t getShortLength() const;
	inline UBool isWritable() const;
	inline UBool isBufferWritable() const;
	inline void setZeroLength();
	inline void setShortLength(int32_t len);
	inline void setLength(int32_t len);
	inline void setToEmpty();
	inline void setArray(char16_t *array, int32_t len, int32_t capacity); // sets length but not flags
	UBool allocate(int32_t capacity);
	void releaseArray(void);
	void unBogus();
	UnicodeString& copyFrom(const UnicodeString &src, UBool fastCopy = false);
	void copyFieldsFrom(UnicodeString &src, UBool setSrcToBogus) noexcept;
	inline void pinIndex(int32_t &start) const;
	inline void pinIndices(int32_t &start, int32_t &length) const;
	int32_t doExtract(int32_t start, int32_t length, char *dest, int32_t destCapacity, UConverter *cnv, UErrorCode &errorCode) const;
	void doCodepageCreate(const char *codepageData, int32_t dataLength, const char *codepage);

	void doCodepageCreate(const char *codepageData, int32_t dataLength, UConverter *converter, UErrorCode &status);
	UBool cloneArrayIfNeeded(int32_t newCapacity = -1, int32_t growCapacity = -1, UBool doCopyArray = true, int32_t **pBufferToDelete = 0, UBool forceClone = false);

	UnicodeString& caseMap(int32_t caseLocale, uint32_t options, BreakIterator *iter, UStringCaseMapper *stringCaseMapper);

	void addRef(void);
	int32_t removeRef(void);
	int32_t refCount(void) const;

	enum {
		US_STACKBUF_SIZE = (int32_t) (UNISTR_OBJECT_SIZE - sizeof(void*) - 2) / sizeof(UChar),
		kInvalidUChar = 0xffff, // U+FFFF returned by charAt(invalid index)
		kInvalidHashCode = 0, // invalid hash code
		kEmptyHashCode = 1, // hash code for empty string

		// bit flag values for fLengthAndFlags
		kIsBogus = 1,         // this string is bogus, i.e., not valid or NULL
		kUsingStackBuffer = 2,         // using fUnion.fStackFields instead of fUnion.fFields
		kRefCounted = 4,      // there is a refCount field before the characters in fArray
		kBufferIsReadonly = 8,      // do not write to this buffer
		kOpenGetBuffer = 16,  // getBuffer(minCapacity) was called (is "open"),
							  // and releaseBuffer(newLength) must be called
		kAllStorageFlags = 0x1f,

		kLengthShift = 5,     // remaining 11 bits for non-negative short length, or negative if long
		kLength1 = 1 << kLengthShift,
		kMaxShortLength = 0x3ff,  // max non-negative short length (leaves top bit 0)
		kLengthIsLarge = 0xffe0,  // short length < 0, real length is in fUnion.fFields.fLength

		// combined values for convenience
		kShortString = kUsingStackBuffer,
		kLongString = kRefCounted,
		kReadonlyAlias = kBufferIsReadonly,
		kWritableAlias = 0
	};

	friend class UnicodeStringAppendable;

	union StackBufferOrFields;        // forward declaration necessary before friend declaration
	friend union StackBufferOrFields; // make US_STACKBUF_SIZE visible inside fUnion

	union StackBufferOrFields {
		struct {
			int16_t fLengthAndFlags;          // bit fields: see constants above
			char16_t fBuffer[US_STACKBUF_SIZE];  // buffer for short strings
		} fStackFields;
		struct {
			int16_t fLengthAndFlags;          // bit fields: see constants above
			int32_t fLength;    // number of characters in fArray if >127; else undefined
			int32_t fCapacity;  // capacity of fArray (in char16_ts)
			char16_t *fArray;    // the Unicode data
		} fFields;
	} fUnion;
};

UnicodeString operator+(const UnicodeString &s1, const UnicodeString &s2);

UBool operator==(const StringPiece &x, const StringPiece &y);

inline bool operator!=(const StringPiece &x, const StringPiece &y) {
	return !(x == y);
}

inline void UnicodeString::pinIndex(int32_t &start) const {
	// pin index
	if (start < 0) {
		start = 0;
	} else if (start > length()) {
		start = length();
	}
}

inline void UnicodeString::pinIndices(int32_t &start, int32_t &_length) const {
	// pin indices
	int32_t len = length();
	if (start < 0) {
		start = 0;
	} else if (start > len) {
		start = len;
	}
	if (_length < 0) {
		_length = 0;
	} else if (_length > (len - start)) {
		_length = (len - start);
	}
}

inline char16_t* UnicodeString::getArrayStart() {
	return (fUnion.fFields.fLengthAndFlags & kUsingStackBuffer) ? fUnion.fStackFields.fBuffer : fUnion.fFields.fArray;
}

inline const char16_t* UnicodeString::getArrayStart() const {
	return (fUnion.fFields.fLengthAndFlags & kUsingStackBuffer) ? fUnion.fStackFields.fBuffer : fUnion.fFields.fArray;
}

inline UnicodeString::UnicodeString() {
	fUnion.fStackFields.fLengthAndFlags = kShortString;
}

inline UnicodeString::UnicodeString(const std::nullptr_t /*text*/) {
	fUnion.fStackFields.fLengthAndFlags = kShortString;
}

inline UnicodeString::UnicodeString(const std::nullptr_t /*text*/, int32_t /*length*/) {
	fUnion.fStackFields.fLengthAndFlags = kShortString;
}

inline UnicodeString::UnicodeString(std::nullptr_t /*buffer*/, int32_t /*buffLength*/, int32_t /*buffCapacity*/) {
	fUnion.fStackFields.fLengthAndFlags = kShortString;
}

inline UBool UnicodeString::hasShortLength() const {
	return fUnion.fFields.fLengthAndFlags >= 0;
}

inline int32_t UnicodeString::getShortLength() const {
	return fUnion.fFields.fLengthAndFlags >> kLengthShift;
}

inline int32_t UnicodeString::length() const {
	return hasShortLength() ? getShortLength() : fUnion.fFields.fLength;
}

inline int32_t UnicodeString::getCapacity() const {
	return (fUnion.fFields.fLengthAndFlags & kUsingStackBuffer) ? US_STACKBUF_SIZE : fUnion.fFields.fCapacity;
}

inline int32_t UnicodeString::hashCode() const {
	return doHashCode();
}

inline UBool UnicodeString::isBogus() const {
	return (UBool) (fUnion.fFields.fLengthAndFlags & kIsBogus);
}

inline UBool UnicodeString::isWritable() const {
	return (UBool) !(fUnion.fFields.fLengthAndFlags & (kOpenGetBuffer | kIsBogus));
}

inline UBool UnicodeString::isBufferWritable() const {
	return (UBool) (!(fUnion.fFields.fLengthAndFlags & (kOpenGetBuffer | kIsBogus | kBufferIsReadonly)) && (!(fUnion.fFields.fLengthAndFlags & kRefCounted) || refCount() == 1));
}

inline const char16_t* UnicodeString::getBuffer() const {
	if (fUnion.fFields.fLengthAndFlags & (kIsBogus | kOpenGetBuffer)) {
		return nullptr;
	} else if (fUnion.fFields.fLengthAndFlags & kUsingStackBuffer) {
		return fUnion.fStackFields.fBuffer;
	} else {
		return fUnion.fFields.fArray;
	}
}

inline int8_t UnicodeString::doCompare(int32_t start, int32_t thisLength, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const {
	if (srcText.isBogus()) {
		return (int8_t) !isBogus(); // 0 if both are bogus, 1 otherwise
	} else {
		srcText.pinIndices(srcStart, srcLength);
		return doCompare(start, thisLength, srcText.getArrayStart(), srcStart, srcLength);
	}
}

inline bool UnicodeString::operator==(const UnicodeString &text) const {
	if (isBogus()) {
		return text.isBogus();
	} else {
		int32_t len = length(), textLength = text.length();
		return !text.isBogus() && len == textLength && doEquals(text, len);
	}
}

inline bool UnicodeString::operator!=(const UnicodeString &text) const {
	return (!operator==(text));
}

inline UBool UnicodeString::operator>(const UnicodeString &text) const {
	return doCompare(0, length(), text, 0, text.length()) == 1;
}

inline UBool UnicodeString::operator<(const UnicodeString &text) const {
	return doCompare(0, length(), text, 0, text.length()) == -1;
}

inline UBool UnicodeString::operator>=(const UnicodeString &text) const {
	return doCompare(0, length(), text, 0, text.length()) != -1;
}

inline UBool UnicodeString::operator<=(const UnicodeString &text) const {
	return doCompare(0, length(), text, 0, text.length()) != 1;
}

inline int8_t UnicodeString::compare(const UnicodeString &text) const {
	return doCompare(0, length(), text, 0, text.length());
}

inline int8_t UnicodeString::compare(int32_t start, int32_t _length, const UnicodeString &srcText) const {
	return doCompare(start, _length, srcText, 0, srcText.length());
}

inline int8_t UnicodeString::compare(ConstChar16Ptr srcChars, int32_t srcLength) const {
	return doCompare(0, length(), srcChars, 0, srcLength);
}

inline int8_t UnicodeString::compare(int32_t start, int32_t _length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const {
	return doCompare(start, _length, srcText, srcStart, srcLength);
}

inline int8_t UnicodeString::compare(int32_t start, int32_t _length, const char16_t *srcChars) const {
	return doCompare(start, _length, srcChars, 0, _length);
}

inline int8_t UnicodeString::compare(int32_t start, int32_t _length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength) const {
	return doCompare(start, _length, srcChars, srcStart, srcLength);
}

inline int8_t UnicodeString::compareBetween(int32_t start, int32_t limit, const UnicodeString &srcText, int32_t srcStart, int32_t srcLimit) const {
	return doCompare(start, limit - start, srcText, srcStart, srcLimit - srcStart);
}

inline int8_t UnicodeString::doCompareCodePointOrder(int32_t start, int32_t thisLength, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const {
	if (srcText.isBogus()) {
		return (int8_t) !isBogus(); // 0 if both are bogus, 1 otherwise
	} else {
		srcText.pinIndices(srcStart, srcLength);
		return doCompareCodePointOrder(start, thisLength, srcText.getArrayStart(), srcStart, srcLength);
	}
}

inline int8_t UnicodeString::compareCodePointOrder(const UnicodeString &text) const {
	return doCompareCodePointOrder(0, length(), text, 0, text.length());
}

inline int8_t UnicodeString::compareCodePointOrder(int32_t start, int32_t _length, const UnicodeString &srcText) const {
	return doCompareCodePointOrder(start, _length, srcText, 0, srcText.length());
}

inline int8_t UnicodeString::compareCodePointOrder(ConstChar16Ptr srcChars, int32_t srcLength) const {
	return doCompareCodePointOrder(0, length(), srcChars, 0, srcLength);
}

inline int8_t UnicodeString::compareCodePointOrder(int32_t start, int32_t _length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const {
	return doCompareCodePointOrder(start, _length, srcText, srcStart, srcLength);
}

inline int8_t UnicodeString::compareCodePointOrder(int32_t start, int32_t _length, const char16_t *srcChars) const {
	return doCompareCodePointOrder(start, _length, srcChars, 0, _length);
}

inline int8_t UnicodeString::compareCodePointOrder(int32_t start, int32_t _length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength) const {
	return doCompareCodePointOrder(start, _length, srcChars, srcStart, srcLength);
}

inline int8_t UnicodeString::compareCodePointOrderBetween(int32_t start, int32_t limit, const UnicodeString &srcText, int32_t srcStart, int32_t srcLimit) const {
	return doCompareCodePointOrder(start, limit - start, srcText, srcStart, srcLimit - srcStart);
}

inline int8_t UnicodeString::doCaseCompare(int32_t start, int32_t thisLength, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength, uint32_t options) const {
	if (srcText.isBogus()) {
		return (int8_t) !isBogus(); // 0 if both are bogus, 1 otherwise
	} else {
		srcText.pinIndices(srcStart, srcLength);
		return doCaseCompare(start, thisLength, srcText.getArrayStart(), srcStart, srcLength, options);
	}
}

inline int8_t UnicodeString::caseCompare(const UnicodeString &text, uint32_t options) const {
	return doCaseCompare(0, length(), text, 0, text.length(), options);
}

inline int8_t UnicodeString::caseCompare(int32_t start, int32_t _length, const UnicodeString &srcText, uint32_t options) const {
	return doCaseCompare(start, _length, srcText, 0, srcText.length(), options);
}

inline int8_t UnicodeString::caseCompare(ConstChar16Ptr srcChars, int32_t srcLength, uint32_t options) const {
	return doCaseCompare(0, length(), srcChars, 0, srcLength, options);
}

inline int8_t UnicodeString::caseCompare(int32_t start, int32_t _length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength, uint32_t options) const {
	return doCaseCompare(start, _length, srcText, srcStart, srcLength, options);
}

inline int8_t UnicodeString::caseCompare(int32_t start, int32_t _length, const char16_t *srcChars, uint32_t options) const {
	return doCaseCompare(start, _length, srcChars, 0, _length, options);
}

inline int8_t UnicodeString::caseCompare(int32_t start, int32_t _length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength, uint32_t options) const {
	return doCaseCompare(start, _length, srcChars, srcStart, srcLength, options);
}

inline int8_t UnicodeString::caseCompareBetween(int32_t start, int32_t limit, const UnicodeString &srcText, int32_t srcStart, int32_t srcLimit, uint32_t options) const {
	return doCaseCompare(start, limit - start, srcText, srcStart, srcLimit - srcStart, options);
}

inline int32_t UnicodeString::indexOf(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength, int32_t start, int32_t _length) const {
	if (!srcText.isBogus()) {
		srcText.pinIndices(srcStart, srcLength);
		if (srcLength > 0) {
			return indexOf(srcText.getArrayStart(), srcStart, srcLength, start, _length);
		}
	}
	return -1;
}

inline int32_t UnicodeString::indexOf(const UnicodeString &text) const {
	return indexOf(text, 0, text.length(), 0, length());
}

inline int32_t UnicodeString::indexOf(const UnicodeString &text, int32_t start) const {
	pinIndex(start);
	return indexOf(text, 0, text.length(), start, length() - start);
}

inline int32_t UnicodeString::indexOf(const UnicodeString &text, int32_t start, int32_t _length) const {
	return indexOf(text, 0, text.length(), start, _length);
}

inline int32_t UnicodeString::indexOf(const char16_t *srcChars, int32_t srcLength, int32_t start) const {
	pinIndex(start);
	return indexOf(srcChars, 0, srcLength, start, length() - start);
}

inline int32_t UnicodeString::indexOf(ConstChar16Ptr srcChars, int32_t srcLength, int32_t start, int32_t _length) const {
	return indexOf(srcChars, 0, srcLength, start, _length);
}

inline int32_t UnicodeString::indexOf(char16_t c, int32_t start, int32_t _length) const {
	return doIndexOf(c, start, _length);
}

inline int32_t UnicodeString::indexOf(UChar32 c, int32_t start, int32_t _length) const {
	return doIndexOf(c, start, _length);
}

inline int32_t UnicodeString::indexOf(char16_t c) const {
	return doIndexOf(c, 0, length());
}

inline int32_t UnicodeString::indexOf(UChar32 c) const {
	return indexOf(c, 0, length());
}

inline int32_t UnicodeString::indexOf(char16_t c, int32_t start) const {
	pinIndex(start);
	return doIndexOf(c, start, length() - start);
}

inline int32_t UnicodeString::indexOf(UChar32 c, int32_t start) const {
	pinIndex(start);
	return indexOf(c, start, length() - start);
}

inline int32_t UnicodeString::lastIndexOf(ConstChar16Ptr srcChars, int32_t srcLength, int32_t start, int32_t _length) const {
	return lastIndexOf(srcChars, 0, srcLength, start, _length);
}

inline int32_t UnicodeString::lastIndexOf(const char16_t *srcChars, int32_t srcLength, int32_t start) const {
	pinIndex(start);
	return lastIndexOf(srcChars, 0, srcLength, start, length() - start);
}

inline int32_t UnicodeString::lastIndexOf(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength, int32_t start, int32_t _length) const {
	if (!srcText.isBogus()) {
		srcText.pinIndices(srcStart, srcLength);
		if (srcLength > 0) {
			return lastIndexOf(srcText.getArrayStart(), srcStart, srcLength, start, _length);
		}
	}
	return -1;
}

inline int32_t UnicodeString::lastIndexOf(const UnicodeString &text, int32_t start, int32_t _length) const {
	return lastIndexOf(text, 0, text.length(), start, _length);
}

inline int32_t UnicodeString::lastIndexOf(const UnicodeString &text, int32_t start) const {
	pinIndex(start);
	return lastIndexOf(text, 0, text.length(), start, length() - start);
}

inline int32_t UnicodeString::lastIndexOf(const UnicodeString &text) const {
	return lastIndexOf(text, 0, text.length(), 0, length());
}

inline int32_t UnicodeString::lastIndexOf(char16_t c, int32_t start, int32_t _length) const {
	return doLastIndexOf(c, start, _length);
}

inline int32_t UnicodeString::lastIndexOf(UChar32 c, int32_t start, int32_t _length) const {
	return doLastIndexOf(c, start, _length);
}

inline int32_t UnicodeString::lastIndexOf(char16_t c) const {
	return doLastIndexOf(c, 0, length());
}

inline int32_t UnicodeString::lastIndexOf(UChar32 c) const {
	return lastIndexOf(c, 0, length());
}

inline int32_t UnicodeString::lastIndexOf(char16_t c, int32_t start) const {
	pinIndex(start);
	return doLastIndexOf(c, start, length() - start);
}

inline int32_t UnicodeString::lastIndexOf(UChar32 c, int32_t start) const {
	pinIndex(start);
	return lastIndexOf(c, start, length() - start);
}

inline UBool UnicodeString::startsWith(const UnicodeString &text) const {
	return compare(0, text.length(), text, 0, text.length()) == 0;
}

inline UBool UnicodeString::startsWith(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const {
	return doCompare(0, srcLength, srcText, srcStart, srcLength) == 0;
}

inline UBool UnicodeString::startsWith(ConstChar16Ptr srcChars, int32_t srcLength) const {
	if (srcLength < 0) {
		srcLength = u_strlen(srcChars);
	}
	return doCompare(0, srcLength, srcChars, 0, srcLength) == 0;
}

inline UBool UnicodeString::startsWith(const char16_t *srcChars, int32_t srcStart, int32_t srcLength) const {
	if (srcLength < 0) {
		srcLength = u_strlen(srcChars);
	}
	return doCompare(0, srcLength, srcChars, srcStart, srcLength) == 0;
}

inline UBool UnicodeString::endsWith(const UnicodeString &text) const {
	return doCompare(length() - text.length(), text.length(), text, 0, text.length()) == 0;
}

inline UBool UnicodeString::endsWith(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) const {
	srcText.pinIndices(srcStart, srcLength);
	return doCompare(length() - srcLength, srcLength, srcText, srcStart, srcLength) == 0;
}

inline UBool UnicodeString::endsWith(ConstChar16Ptr srcChars, int32_t srcLength) const {
	if (srcLength < 0) {
		srcLength = u_strlen(srcChars);
	}
	return doCompare(length() - srcLength, srcLength, srcChars, 0, srcLength) == 0;
}

inline UBool UnicodeString::endsWith(const char16_t *srcChars, int32_t srcStart, int32_t srcLength) const {
	if (srcLength < 0) {
		srcLength = u_strlen(srcChars + srcStart);
	}
	return doCompare(length() - srcLength, srcLength, srcChars, srcStart, srcLength) == 0;
}

inline UnicodeString& UnicodeString::replace(int32_t start, int32_t _length, const UnicodeString &srcText) {
	return doReplace(start, _length, srcText, 0, srcText.length());
}

inline UnicodeString& UnicodeString::replace(int32_t start, int32_t _length, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) {
	return doReplace(start, _length, srcText, srcStart, srcLength);
}

inline UnicodeString& UnicodeString::replace(int32_t start, int32_t _length, ConstChar16Ptr srcChars, int32_t srcLength) {
	return doReplace(start, _length, srcChars, 0, srcLength);
}

inline UnicodeString& UnicodeString::replace(int32_t start, int32_t _length, const char16_t *srcChars, int32_t srcStart, int32_t srcLength) {
	return doReplace(start, _length, srcChars, srcStart, srcLength);
}

inline UnicodeString& UnicodeString::replace(int32_t start, int32_t _length, char16_t srcChar) {
	return doReplace(start, _length, &srcChar, 0, 1);
}

inline UnicodeString& UnicodeString::replaceBetween(int32_t start, int32_t limit, const UnicodeString &srcText) {
	return doReplace(start, limit - start, srcText, 0, srcText.length());
}

inline UnicodeString& UnicodeString::replaceBetween(int32_t start, int32_t limit, const UnicodeString &srcText, int32_t srcStart, int32_t srcLimit) {
	return doReplace(start, limit - start, srcText, srcStart, srcLimit - srcStart);
}

inline UnicodeString& UnicodeString::findAndReplace(const UnicodeString &oldText, const UnicodeString &newText) {
	return findAndReplace(0, length(), oldText, 0, oldText.length(), newText, 0, newText.length());
}

inline UnicodeString&
UnicodeString::findAndReplace(int32_t start, int32_t _length, const UnicodeString &oldText, const UnicodeString &newText) {
	return findAndReplace(start, _length, oldText, 0, oldText.length(), newText, 0, newText.length());
}

inline void UnicodeString::doExtract(int32_t start, int32_t _length, UnicodeString &target) const {
	target.replace(0, target.length(), *this, start, _length);
}

inline void UnicodeString::extract(int32_t start, int32_t _length, Char16Ptr target, int32_t targetStart) const {
	doExtract(start, _length, target, targetStart);
}

inline void UnicodeString::extract(int32_t start, int32_t _length, UnicodeString &target) const {
	doExtract(start, _length, target);
}

#if !UCONFIG_NO_CONVERSION

inline int32_t UnicodeString::extract(int32_t start, int32_t _length, char *dst, const char *codepage) const

{
	// This dstSize value will be checked explicitly
	return extract(start, _length, dst, dst != 0 ? 0xffffffff : 0, codepage);
}

#endif

inline void UnicodeString::extractBetween(int32_t start, int32_t limit, char16_t *dst, int32_t dstStart) const {
	pinIndex(start);
	pinIndex(limit);
	doExtract(start, limit - start, dst, dstStart);
}

inline UnicodeString UnicodeString::tempSubStringBetween(int32_t start, int32_t limit) const {
	return tempSubString(start, limit - start);
}

inline char16_t UnicodeString::doCharAt(int32_t offset) const {
	if ((uint32_t) offset < (uint32_t) length()) {
		return getArrayStart()[offset];
	} else {
		return kInvalidUChar;
	}
}

inline char16_t UnicodeString::charAt(int32_t offset) const {
	return doCharAt(offset);
}

inline char16_t UnicodeString::operator[](int32_t offset) const {
	return doCharAt(offset);
}

inline UBool UnicodeString::isEmpty() const {
	// Arithmetic or logical right shift does not matter: only testing for 0.
	return (fUnion.fFields.fLengthAndFlags >> kLengthShift) == 0;
}

inline void UnicodeString::setZeroLength() {
	fUnion.fFields.fLengthAndFlags &= kAllStorageFlags;
}

inline void UnicodeString::setShortLength(int32_t len) {
	// requires 0 <= len <= kMaxShortLength
	fUnion.fFields.fLengthAndFlags = (int16_t) ((fUnion.fFields.fLengthAndFlags & kAllStorageFlags) | (len << kLengthShift));
}

inline void UnicodeString::setLength(int32_t len) {
	if (len <= kMaxShortLength) {
		setShortLength(len);
	} else {
		fUnion.fFields.fLengthAndFlags |= kLengthIsLarge;
		fUnion.fFields.fLength = len;
	}
}

inline void UnicodeString::setToEmpty() {
	fUnion.fFields.fLengthAndFlags = kShortString;
}

inline void UnicodeString::setArray(char16_t *array, int32_t len, int32_t capacity) {
	setLength(len);
	fUnion.fFields.fArray = array;
	fUnion.fFields.fCapacity = capacity;
}

inline UnicodeString&
UnicodeString::operator=(char16_t ch) {
	return doReplace(0, length(), &ch, 0, 1);
}

inline UnicodeString&
UnicodeString::operator=(UChar32 ch) {
	return replace(0, length(), ch);
}

inline UnicodeString&
UnicodeString::setTo(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) {
	unBogus();
	return doReplace(0, length(), srcText, srcStart, srcLength);
}

inline UnicodeString&
UnicodeString::setTo(const UnicodeString &srcText, int32_t srcStart) {
	unBogus();
	srcText.pinIndex(srcStart);
	return doReplace(0, length(), srcText, srcStart, srcText.length() - srcStart);
}

inline UnicodeString&
UnicodeString::setTo(const UnicodeString &srcText) {
	return copyFrom(srcText);
}

inline UnicodeString&
UnicodeString::setTo(const char16_t *srcChars, int32_t srcLength) {
	unBogus();
	return doReplace(0, length(), srcChars, 0, srcLength);
}

inline UnicodeString&
UnicodeString::setTo(char16_t srcChar) {
	unBogus();
	return doReplace(0, length(), &srcChar, 0, 1);
}

inline UnicodeString&
UnicodeString::setTo(UChar32 srcChar) {
	unBogus();
	return replace(0, length(), srcChar);
}

inline UnicodeString&
UnicodeString::append(const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) {
	return doAppend(srcText, srcStart, srcLength);
}

inline UnicodeString&
UnicodeString::append(const UnicodeString &srcText) {
	return doAppend(srcText, 0, srcText.length());
}

inline UnicodeString&
UnicodeString::append(const char16_t *srcChars, int32_t srcStart, int32_t srcLength) {
	return doAppend(srcChars, srcStart, srcLength);
}

inline UnicodeString&
UnicodeString::append(ConstChar16Ptr srcChars, int32_t srcLength) {
	return doAppend(srcChars, 0, srcLength);
}

inline UnicodeString&
UnicodeString::append(char16_t srcChar) {
	return doAppend(&srcChar, 0, 1);
}

inline UnicodeString&
UnicodeString::operator+=(char16_t ch) {
	return doAppend(&ch, 0, 1);
}

inline UnicodeString&
UnicodeString::operator+=(UChar32 ch) {
	return append(ch);
}

inline UnicodeString&
UnicodeString::operator+=(const UnicodeString &srcText) {
	return doAppend(srcText, 0, srcText.length());
}

inline UnicodeString&
UnicodeString::insert(int32_t start, const UnicodeString &srcText, int32_t srcStart, int32_t srcLength) {
	return doReplace(start, 0, srcText, srcStart, srcLength);
}

inline UnicodeString&
UnicodeString::insert(int32_t start, const UnicodeString &srcText) {
	return doReplace(start, 0, srcText, 0, srcText.length());
}

inline UnicodeString&
UnicodeString::insert(int32_t start, const char16_t *srcChars, int32_t srcStart, int32_t srcLength) {
	return doReplace(start, 0, srcChars, srcStart, srcLength);
}

inline UnicodeString&
UnicodeString::insert(int32_t start, ConstChar16Ptr srcChars, int32_t srcLength) {
	return doReplace(start, 0, srcChars, 0, srcLength);
}

inline UnicodeString&
UnicodeString::insert(int32_t start, char16_t srcChar) {
	return doReplace(start, 0, &srcChar, 0, 1);
}

inline UnicodeString&
UnicodeString::insert(int32_t start, UChar32 srcChar) {
	return replace(start, 0, srcChar);
}

inline UnicodeString&
UnicodeString::remove() {
	// remove() of a bogus string makes the string empty and non-bogus
	if (isBogus()) {
		setToEmpty();
	} else {
		setZeroLength();
	}
	return *this;
}

inline UnicodeString&
UnicodeString::remove(int32_t start, int32_t _length) {
	if (start <= 0 && _length == INT32_MAX) {
		// remove(guaranteed everything) of a bogus string makes the string empty and non-bogus
		return remove();
	}
	return doReplace(start, _length, NULL, 0, 0);
}

inline UnicodeString&
UnicodeString::removeBetween(int32_t start, int32_t limit) {
	return doReplace(start, limit - start, NULL, 0, 0);
}

inline UnicodeString&
UnicodeString::retainBetween(int32_t start, int32_t limit) {
	truncate(limit);
	return doReplace(0, start, NULL, 0, 0);
}

inline UBool UnicodeString::truncate(int32_t targetLength) {
	if (isBogus() && targetLength == 0) {
		// truncate(0) of a bogus string makes the string empty and non-bogus
		unBogus();
		return false;
	} else if ((uint32_t) targetLength < (uint32_t) length()) {
		setLength(targetLength);
		return true;
	} else {
		return false;
	}
}

inline UnicodeString&
UnicodeString::reverse() {
	return doReverse(0, length());
}

inline UnicodeString&
UnicodeString::reverse(int32_t start, int32_t _length) {
	return doReverse(start, _length);
}

}

#endif /* MODULES_IDN_UIDNAUNICODESTRING_H_ */
