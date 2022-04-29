// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
*******************************************************************************
*
*   Copyright (C) 2009-2014, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  normalizer2impl.cpp
*   encoding:   UTF-8
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2009nov22
*   created by: Markus W. Scherer
*/

#include "u_edits.h"
#include "u_macro.h"
#include "u_norm2.h"

namespace uidna {

UBool ReorderingBuffer::init(int32_t destCapacity, UErrorCode &errorCode) {
	int32_t length = str.length();
	start = str.getBuffer(destCapacity);
	if (start == NULL) {
		// getBuffer() already did str.setToBogus()
		errorCode = U_MEMORY_ALLOCATION_ERROR;
		return false;
	}
	limit = start + length;
	remainingCapacity = str.getCapacity() - length;
	reorderStart = start;
	if (start == limit) {
		lastCC = 0;
	} else {
		setIterator();
		lastCC = previousCC();
		// Set reorderStart after the last code point with cc<=1 if there is one.
		if (lastCC > 1) {
			while (previousCC() > 1) { }
		}
		reorderStart = codePointLimit;
	}
	return true;
}

void ReorderingBuffer::skipPrevious() {
	codePointLimit = codePointStart;
	UChar c = *--codePointStart;
	if (U16_IS_TRAIL(c) && start < codePointStart && U16_IS_LEAD(*(codePointStart - 1))) {
		--codePointStart;
	}
}

uint8_t ReorderingBuffer::previousCC() {
	codePointLimit = codePointStart;
	if (reorderStart >= codePointStart) {
		return 0;
	}
	UChar32 c = *--codePointStart;
	UChar c2;
	if (U16_IS_TRAIL(c) && start < codePointStart && U16_IS_LEAD(c2 = *(codePointStart - 1))) {
		--codePointStart;
		c = U16_GET_SUPPLEMENTARY(c2, c);
	}
	return impl.getCCFromYesOrMaybeCP(c);
}

UBool ReorderingBuffer::appendSupplementary(UChar32 c, uint8_t cc, UErrorCode &errorCode) {
	if (remainingCapacity < 2 && !resize(2, errorCode)) {
		return false;
	}
	if (lastCC <= cc || cc == 0) {
		limit[0] = U16_LEAD(c);
		limit[1] = U16_TRAIL(c);
		limit += 2;
		lastCC = cc;
		if (cc <= 1) {
			reorderStart = limit;
		}
	} else {
		insert(c, cc);
	}
	remainingCapacity -= 2;
	return true;
}

UBool ReorderingBuffer::append(const UChar *s, int32_t length, UBool isNFD, uint8_t leadCC, uint8_t trailCC, UErrorCode &errorCode) {
	if (length == 0) {
		return true;
	}
	if (remainingCapacity < length && !resize(length, errorCode)) {
		return false;
	}
	remainingCapacity -= length;
	if (lastCC <= leadCC || leadCC == 0) {
		if (trailCC <= 1) {
			reorderStart = limit + length;
		} else if (leadCC <= 1) {
			reorderStart = limit + 1;  // Ok if not a code point boundary.
		}
		const UChar *sLimit = s + length;
		do {
			*limit++ = *s++;
		} while (s != sLimit);
		lastCC = trailCC;
	} else {
		int32_t i = 0;
		UChar32 c;
		U16_NEXT(s, i, length, c);
		insert(c, leadCC);  // insert first code point
		while (i < length) {
			U16_NEXT(s, i, length, c);
			if (i < length) {
				if (isNFD) {
					leadCC = Normalizer2Impl::getCCFromYesOrMaybe(impl.getRawNorm16(c));
				} else {
					leadCC = impl.getCC(impl.getNorm16(c));
				}
			} else {
				leadCC = trailCC;
			}
			append(c, leadCC, errorCode);
		}
	}
	return true;
}

UBool ReorderingBuffer::appendZeroCC(UChar32 c, UErrorCode &errorCode) {
	int32_t cpLength = U16_LENGTH(c);
	if (remainingCapacity < cpLength && !resize(cpLength, errorCode)) {
		return false;
	}
	remainingCapacity -= cpLength;
	if (cpLength == 1) {
		*limit++ = (UChar) c;
	} else {
		limit[0] = U16_LEAD(c);
		limit[1] = U16_TRAIL(c);
		limit += 2;
	}
	lastCC = 0;
	reorderStart = limit;
	return true;
}

UBool ReorderingBuffer::appendZeroCC(const UChar *s, const UChar *sLimit, UErrorCode &errorCode) {
	if (s == sLimit) {
		return true;
	}
	int32_t length = (int32_t) (sLimit - s);
	if (remainingCapacity < length && !resize(length, errorCode)) {
		return false;
	}
	u_memcpy(limit, s, length);
	limit += length;
	remainingCapacity -= length;
	lastCC = 0;
	reorderStart = limit;
	return true;
}

void ReorderingBuffer::remove() {
	reorderStart = limit = start;
	remainingCapacity = str.getCapacity();
	lastCC = 0;
}

void ReorderingBuffer::removeSuffix(int32_t suffixLength) {
	if (suffixLength < (limit - start)) {
		limit -= suffixLength;
		remainingCapacity += suffixLength;
	} else {
		limit = start;
		remainingCapacity = str.getCapacity();
	}
	lastCC = 0;
	reorderStart = limit;
}

UBool ReorderingBuffer::resize(int32_t appendLength, UErrorCode &errorCode) {
	int32_t reorderStartIndex = (int32_t) (reorderStart - start);
	int32_t length = (int32_t) (limit - start);
	str.releaseBuffer(length);
	int32_t newCapacity = length + appendLength;
	int32_t doubleCapacity = 2 * str.getCapacity();
	if (newCapacity < doubleCapacity) {
		newCapacity = doubleCapacity;
	}
	if (newCapacity < 256) {
		newCapacity = 256;
	}
	start = str.getBuffer(newCapacity);
	if (start == NULL) {
		// getBuffer() already did str.setToBogus()
		errorCode = U_MEMORY_ALLOCATION_ERROR;
		return false;
	}
	reorderStart = start + reorderStartIndex;
	limit = start + length;
	remainingCapacity = str.getCapacity() - length;
	return true;
}

UBool ReorderingBuffer::equals(const UChar *otherStart, const UChar *otherLimit) const {
	int32_t length = (int32_t) (limit - start);
	return length == (int32_t) (otherLimit - otherStart) && 0 == u_memcmp(start, otherStart, length);
}

void ReorderingBuffer::insert(UChar32 c, uint8_t cc) {
	for (setIterator(), skipPrevious(); previousCC() > cc;) { }
	// insert c at codePointLimit, after the character with prevCC<=cc
	UChar *q = limit;
	UChar *r = limit += U16_LENGTH(c);
	do {
		*--r = *--q;
	} while (codePointLimit != q);
	writeCodePoint(q, c);
	if (cc <= 1) {
		reorderStart = r;
	}
}

}
