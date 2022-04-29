// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
*******************************************************************************
*
*   Copyright (C) 2009-2014, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  normalizer2impl.h
*   encoding:   UTF-8
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2009nov22
*   created by: Markus W. Scherer
*/

#ifndef MODULES_IDN_UIDNAEDITS_H_
#define MODULES_IDN_UIDNAEDITS_H_

#include "u_unistr.h"

#define U_EDITS_NO_RESET 0x2000
#define U_IS_LEAD(c) (((c)&0xfffffc00)==0xd800)
#define U_IS_TRAIL(c) (((c)&0xfffffc00)==0xdc00)
#define U16_LEAD(supplementary) (UChar)(((supplementary)>>10)+0xd7c0)
#define U16_TRAIL(supplementary) (UChar)(((supplementary)&0x3ff)|0xdc00)

namespace uidna {

class Normalizer2Impl;

class ReorderingBuffer {
public:
	ReorderingBuffer(const Normalizer2Impl &ni, UnicodeString &dest)
	: impl(ni), str(dest), start(NULL), reorderStart(NULL), limit(NULL), remainingCapacity(0), lastCC(0) { }

	ReorderingBuffer(const Normalizer2Impl &ni, UnicodeString &dest, UErrorCode &errorCode);
	~ReorderingBuffer() {
		if (start != NULL) {
			str.releaseBuffer((int32_t) (limit - start));
		}
	}

	UBool init(int32_t destCapacity, UErrorCode &errorCode);

	UBool isEmpty() const { return start == limit; }
	int32_t length() const { return (int32_t) (limit - start); }
	UChar* getStart() { return start; }
	UChar* getLimit() { return limit; }
	uint8_t getLastCC() const { return lastCC; }

	UBool equals(const UChar *start, const UChar *limit) const;
	UBool equals(const uint8_t *otherStart, const uint8_t *otherLimit) const;

	UBool append(UChar32 c, uint8_t cc, UErrorCode &errorCode) {
		return (c <= 0xffff) ? appendBMP((UChar) c, cc, errorCode) : appendSupplementary(c, cc, errorCode);
	}
	UBool append(const UChar *s, int32_t length, UBool isNFD, uint8_t leadCC, uint8_t trailCC, UErrorCode &errorCode);
	UBool appendBMP(UChar c, uint8_t cc, UErrorCode &errorCode) {
		if (remainingCapacity == 0 && !resize(1, errorCode)) {
			return false;
		}
		if (lastCC <= cc || cc == 0) {
			*limit++ = c;
			lastCC = cc;
			if (cc <= 1) {
				reorderStart = limit;
			}
		} else {
			insert(c, cc);
		}
		--remainingCapacity;
		return true;
	}
	UBool appendZeroCC(UChar32 c, UErrorCode &errorCode);
	UBool appendZeroCC(const UChar *s, const UChar *sLimit, UErrorCode &errorCode);
	void remove();
	void removeSuffix(int32_t suffixLength);
	void setReorderingLimit(UChar *newLimit) {
		remainingCapacity += (int32_t) (limit - newLimit);
		reorderStart = limit = newLimit;
		lastCC = 0;
	}
	void copyReorderableSuffixTo(UnicodeString &s) const {
		s.setTo(ConstChar16Ptr(reorderStart), (int32_t) (limit - reorderStart));
	}

private:
	UBool appendSupplementary(UChar32 c, uint8_t cc, UErrorCode &errorCode);
	void insert(UChar32 c, uint8_t cc);
	static void writeCodePoint(UChar *p, UChar32 c) {
		if (c <= 0xffff) {
			*p = (UChar) c;
		} else {
			p[0] = U16_LEAD(c);
			p[1] = U16_TRAIL(c);
		}
	}
	UBool resize(int32_t appendLength, UErrorCode &errorCode);

	const Normalizer2Impl &impl;
	UnicodeString &str;
	UChar *start, *reorderStart, *limit;
	int32_t remainingCapacity;
	uint8_t lastCC;

	// private backward iterator
	void setIterator() { codePointStart = limit; }
	void skipPrevious();  // Requires start<codePointStart.
	uint8_t previousCC();  // Returns 0 if there is no previous character.

	UChar *codePointStart = nullptr, *codePointLimit = nullptr;
};

class Edits final {
public:
	Edits()
	: array(stackArray), capacity(STACK_CAPACITY), length(0), delta(0)
	, numChanges(0), errorCode_(U_ZERO_ERROR) { }

	Edits(const Edits &other)
	: array(stackArray), capacity(STACK_CAPACITY), length(other.length), delta(other.delta)
	, numChanges(other.numChanges), errorCode_(other.errorCode_) {
		copyArray(other);
	}

	Edits(Edits &&src) noexcept
	: array(stackArray), capacity(STACK_CAPACITY), length(src.length), delta(src.delta)
	, numChanges(src.numChanges), errorCode_(src.errorCode_) {
		moveArray(src);
	}

	~Edits();

	Edits& operator=(const Edits &other);
	Edits& operator=(Edits &&src) noexcept;
	void reset() noexcept;
	void addUnchanged(int32_t unchangedLength);
	void addReplace(int32_t oldLength, int32_t newLength);
	UBool copyErrorTo(UErrorCode &outErrorCode) const;
	int32_t lengthDelta() const { return delta; }
	UBool hasChanges() const { return numChanges != 0; }
	int32_t numberOfChanges() const { return numChanges; }

	struct Iterator final {
		Iterator()
		: array(nullptr), index(0), length(0), remaining(0), onlyChanges_(false), coarse(false)
		, dir(0), changed(false), oldLength_(0), newLength_(0), srcIndex(0), replIndex(0), destIndex(0) { }

		Iterator(const Iterator &other) = default;
		Iterator &operator=(const Iterator &other) = default;

		UBool next(UErrorCode &errorCode) { return next(onlyChanges_, errorCode); }
		UBool findSourceIndex(int32_t i, UErrorCode &errorCode) { return findIndex(i, true, errorCode) == 0; }
		UBool findDestinationIndex(int32_t i, UErrorCode &errorCode) { return findIndex(i, false, errorCode) == 0; }

		int32_t destinationIndexFromSourceIndex(int32_t i, UErrorCode &errorCode);
		int32_t sourceIndexFromDestinationIndex(int32_t i, UErrorCode &errorCode);

		UBool hasChange() const { return changed; }
		int32_t oldLength() const { return oldLength_; }
		int32_t newLength() const { return newLength_; }
		int32_t sourceIndex() const { return srcIndex; }
		int32_t replacementIndex() const { return replIndex; }
		int32_t destinationIndex() const { return destIndex; }

		UnicodeString& toString(UnicodeString& appendTo) const;

	private:
		friend class Edits;

		Iterator(const uint16_t *a, int32_t len, UBool oc, UBool crs);

		int32_t readLength(int32_t head);
		void updateNextIndexes();
		void updatePreviousIndexes();
		UBool noNext();
		UBool next(UBool onlyChanges, UErrorCode &errorCode);
		UBool previous(UErrorCode &errorCode);
		int32_t findIndex(int32_t i, UBool findSource, UErrorCode &errorCode);

		const uint16_t *array;
		int32_t index, length;
		int32_t remaining;
		UBool onlyChanges_, coarse;
		int8_t dir;  // iteration direction: back(<0), initial(0), forward(>0)
		UBool changed;
		int32_t oldLength_, newLength_;
		int32_t srcIndex, replIndex, destIndex;
	};

	Iterator getCoarseChangesIterator() const { return Iterator(array, length, true, true); }
	Iterator getCoarseIterator() const { return Iterator(array, length, false, true); }
	Iterator getFineChangesIterator() const { return Iterator(array, length, true, false); }
	Iterator getFineIterator() const { return Iterator(array, length, false, false); }

	Edits &mergeAndAppend(const Edits &ab, const Edits &bc, UErrorCode &errorCode);

private:
	void releaseArray() noexcept;
	Edits& copyArray(const Edits &other);
	Edits& moveArray(Edits &src) noexcept;

	void setLastUnit(int32_t last) { array[length - 1] = (uint16_t) last; }
	int32_t lastUnit() const { return length > 0 ? array[length - 1] : 0xffff; }

	void append(int32_t r);
	UBool growArray();

	static const int32_t STACK_CAPACITY = 100;
	uint16_t *array;
	int32_t capacity;
	int32_t length;
	int32_t delta;
	int32_t numChanges;
	UErrorCode errorCode_;
	uint16_t stackArray[STACK_CAPACITY];
};

struct UElement;
struct UObjectDeleter;
struct UElementsAreEqual;
struct UElementAssigner;
struct UElementComparator;
struct UComparator;

class UVector {
private:
	int32_t count = 0;
	int32_t capacity = 0;

	UElement *elements = nullptr;
	UObjectDeleter *deleter = nullptr;
	UElementsAreEqual *comparer = nullptr;

public:
	UVector(UErrorCode &status);
	UVector(int32_t initialCapacity, UErrorCode &status);
	UVector(UObjectDeleter *d, UElementsAreEqual *c, UErrorCode &status);
	UVector(UObjectDeleter *d, UElementsAreEqual *c, int32_t initialCapacity, UErrorCode &status);

	~UVector() { }

	void assign(const UVector &other, UElementAssigner *assign, UErrorCode &ec);
	bool operator==(const UVector &other) const;

	inline bool operator!=(const UVector &other) const { return !operator==(other); }

	void addElement(void *obj, UErrorCode &status);
	void adoptElement(void *obj, UErrorCode &status);
	void addElement(int32_t elem, UErrorCode &status);
	void setElementAt(void *obj, int32_t index);
	void setElementAt(int32_t elem, int32_t index);
	void insertElementAt(void *obj, int32_t index, UErrorCode &status);
	void insertElementAt(int32_t elem, int32_t index, UErrorCode &status);
	void* elementAt(int32_t index) const;
	int32_t elementAti(int32_t index) const;
	UBool equals(const UVector &other) const;
	inline void* firstElement(void) const { return elementAt(0); }
	inline void* lastElement(void) const { return elementAt(count - 1); }
	inline int32_t lastElementi(void) const { return elementAti(count - 1); }
	int32_t indexOf(void *obj, int32_t startIndex = 0) const;
	int32_t indexOf(int32_t obj, int32_t startIndex = 0) const;
	inline UBool contains(void *obj) const { return indexOf(obj) >= 0; }
	inline UBool contains(int32_t obj) const { return indexOf(obj) >= 0; }
	UBool containsAll(const UVector &other) const;
	UBool removeAll(const UVector &other);
	UBool retainAll(const UVector &other);
	void removeElementAt(int32_t index);
	UBool removeElement(void *obj);
	void removeAllElements();
	inline int32_t size(void) const { return count; }
	inline UBool isEmpty(void) const { return count == 0; }
	UBool ensureCapacity(int32_t minimumCapacity, UErrorCode &status);
	void setSize(int32_t newSize, UErrorCode &status);
	void** toArray(void **result) const;
	UObjectDeleter* setDeleter(UObjectDeleter *d);
	bool hasDeleter() { return deleter != nullptr; }
	UElementsAreEqual* setComparer(UElementsAreEqual *c);
	inline void* operator[](int32_t index) const {
		return elementAt(index);
	}
	void* orphanElementAt(int32_t index);
	UBool containsNone(const UVector &other) const;
	void sortedInsert(void *obj, UElementComparator *compare, UErrorCode &ec);
	void sortedInsert(int32_t obj, UElementComparator *compare, UErrorCode &ec);
	void sorti(UErrorCode &ec);
	void sort(UElementComparator *compare, UErrorCode &ec);
	void sortWithUComparator(UComparator *compare, const void *context, UErrorCode &ec);

	static UClassID getStaticClassID();
	UClassID getDynamicClassID() const;

private:
    int32_t indexOf(UElement key, int32_t startIndex = 0, int8_t hint = 0) const;
    void sortedInsert(UElement e, UElementComparator *compare, UErrorCode& ec);

public:
    UVector(const UVector&) = delete;
    UVector& operator=(const UVector&) = delete;
};

class Hangul {
public:
	/* Korean Hangul and Jamo constants */
	enum {
		JAMO_L_BASE = 0x1100, /* "lead" jamo */
		JAMO_L_END = 0x1112,
		JAMO_V_BASE = 0x1161, /* "vowel" jamo */
		JAMO_V_END = 0x1175,
		JAMO_T_BASE = 0x11a7, /* "trail" jamo */
		JAMO_T_END = 0x11c2,

		HANGUL_BASE = 0xac00,
		HANGUL_END = 0xd7a3,

		JAMO_L_COUNT = 19,
		JAMO_V_COUNT = 21,
		JAMO_T_COUNT = 28,

		JAMO_VT_COUNT = JAMO_V_COUNT * JAMO_T_COUNT,

		HANGUL_COUNT = JAMO_L_COUNT * JAMO_V_COUNT * JAMO_T_COUNT,
		HANGUL_LIMIT = HANGUL_BASE + HANGUL_COUNT
	};

	static inline UBool isHangul(UChar32 c) {
		return HANGUL_BASE <= c && c < HANGUL_LIMIT;
	}
	static inline UBool isHangulLV(UChar32 c) {
		c -= HANGUL_BASE;
		return 0 <= c && c < HANGUL_COUNT && c % JAMO_T_COUNT == 0;
	}
	static inline UBool isJamoL(UChar32 c) {
		return (uint32_t) (c - JAMO_L_BASE) < JAMO_L_COUNT;
	}
	static inline UBool isJamoV(UChar32 c) {
		return (uint32_t) (c - JAMO_V_BASE) < JAMO_V_COUNT;
	}
	static inline UBool isJamoT(UChar32 c) {
		int32_t t = c - JAMO_T_BASE;
		return 0 < t && t < JAMO_T_COUNT;  // not JAMO_T_BASE itself
	}
	static UBool isJamo(UChar32 c) {
		return JAMO_L_BASE <= c && c <= JAMO_T_END && (c <= JAMO_L_END || (JAMO_V_BASE <= c && c <= JAMO_V_END) || JAMO_T_BASE < c);
	}

	/**
	 * Decomposes c, which must be a Hangul syllable, into buffer
	 * and returns the length of the decomposition (2 or 3).
	 */
	static inline int32_t decompose(UChar32 c, UChar buffer[3]) {
		c -= HANGUL_BASE;
		UChar32 c2 = c % JAMO_T_COUNT;
		c /= JAMO_T_COUNT;
		buffer[0] = (UChar) (JAMO_L_BASE + c / JAMO_V_COUNT);
		buffer[1] = (UChar) (JAMO_V_BASE + c % JAMO_V_COUNT);
		if (c2 == 0) {
			return 2;
		} else {
			buffer[2] = (UChar) (JAMO_T_BASE + c2);
			return 3;
		}
	}

	/**
	 * Decomposes c, which must be a Hangul syllable, into buffer.
	 * This is the raw, not recursive, decomposition. Its length is always 2.
	 */
	static inline void getRawDecomposition(UChar32 c, UChar buffer[2]) {
		UChar32 orig = c;
		c -= HANGUL_BASE;
		UChar32 c2 = c % JAMO_T_COUNT;
		if (c2 == 0) {
			c /= JAMO_T_COUNT;
			buffer[0] = (UChar) (JAMO_L_BASE + c / JAMO_V_COUNT);
			buffer[1] = (UChar) (JAMO_V_BASE + c % JAMO_V_COUNT);
		} else {
			buffer[0] = (UChar) (orig - c2);  // LV syllable
			buffer[1] = (UChar) (JAMO_T_BASE + c2);
		}
	}
private:
	Hangul();  // no instantiation
};

}

#endif /* MODULES_IDN_UIDNAEDITS_H_ */
