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

#include "u_norm2.h"
#include "u_macro.h"
#include "u_trie.h"

namespace uidna {

struct UMutableCPTrie;
class UVector;

struct CanonIterData {
	CanonIterData(UErrorCode &errorCode);
	~CanonIterData() { }
	void addToStartSet(UChar32 origin, UChar32 decompLead, UErrorCode &errorCode);
	UMutableCPTrie *mutableTrie;
	UCPTrie *trie;
	UVector canonStartSets;  // contains UnicodeSet *
};

static UErrorCode s_errorCode = U_ZERO_ERROR;

static UBool dataIsAcceptable(const UDataInfo *pInfo) {
	if (pInfo->size >= 20
			// && pInfo->isBigEndian == U_IS_BIG_ENDIAN
			// && pInfo->charsetFamily == U_CHARSET_FAMILY
			&& pInfo->dataFormat[0] == 0x4e
			&& /* dataFormat="Nrm2" */ pInfo->dataFormat[1] == 0x72
			&& pInfo->dataFormat[2] == 0x6d
			&& pInfo->dataFormat[3] == 0x32
			&& pInfo->formatVersion[0] == 4) {
		// Normalizer2Impl *me=(Normalizer2Impl *)context;
		// uprv_memcpy(me->dataVersion, pInfo->dataVersion, 4);
		return true;
	} else {
		return false;
	}
}

static uint16_t udata_getHeaderSize(const DataHeader *udh) {
    if(udh==NULL) {
        return 0;
    } else if(udh->info.isBigEndian==0) {
        /* same endianness */
        return udh->dataHeader.headerSize;
    } else {
        /* opposite endianness */
        uint16_t x=udh->dataHeader.headerSize;
        return (uint16_t)((x<<8)|(x>>8));
    }
}

static UChar32 codePointFromValidUTF8(const uint8_t *cpStart, const uint8_t *cpLimit) {
    // Similar to U8_NEXT_UNSAFE(s, i, c).
   // U_ASSERT(cpStart < cpLimit);
    uint8_t c = *cpStart;
    switch(cpLimit-cpStart) {
    case 1:
        return c;
    case 2:
        return ((c&0x1f)<<6) | (cpStart[1]&0x3f);
    case 3:
        // no need for (c&0xf) because the upper bits are truncated after <<12 in the cast to (UChar)
        return (UChar)((c<<12) | ((cpStart[1]&0x3f)<<6) | (cpStart[2]&0x3f));
    case 4:
        return ((c&7)<<18) | ((cpStart[1]&0x3f)<<12) | ((cpStart[2]&0x3f)<<6) | (cpStart[3]&0x3f);
    default:
        break;  // Should not occur.
    }
    return 0;
}

uint16_t Normalizer2Impl::getNorm16(UChar32 c) const {
	return U_IS_LEAD(c) ?
		static_cast<uint16_t>(INERT) :
		UCPTRIE_FAST_GET(normTrie, UCPTRIE_16, c);
}
uint16_t Normalizer2Impl::getRawNorm16(UChar32 c) const {
	return UCPTRIE_FAST_GET(normTrie, UCPTRIE_16, c);
}

uint16_t Normalizer2Impl::nextFCD16(const UChar *&s, const UChar *limit) const {
	UChar32 c=*s++;
	if(c<minDecompNoCP || !singleLeadMightHaveNonZeroFCD16(c)) {
		return 0;
	}
	UChar c2;
	if(U16_IS_LEAD(c) && s!=limit && U16_IS_TRAIL(c2=*s)) {
		c=U16_GET_SUPPLEMENTARY(c, c2);
		++s;
	}
	return getFCD16FromNormData(c);
}

uint16_t Normalizer2Impl::previousFCD16(const UChar *start, const UChar *&s) const {
	UChar32 c=*--s;
	if(c<minDecompNoCP) {
		return 0;
	}
	if(!U16_IS_TRAIL(c)) {
		if(!singleLeadMightHaveNonZeroFCD16(c)) {
			return 0;
		}
	} else {
		UChar c2;
		if(start<s && U16_IS_LEAD(c2=*(s-1))) {
			c=U16_GET_SUPPLEMENTARY(c2, c);
			--s;
		}
	}
	return getFCD16FromNormData(c);
}

Normalizer2Impl::Normalizer2Impl(const uint8_t *d) {
	auto header = (const DataHeader *)d;
	if (dataIsAcceptable(&header->info)) {
		const uint8_t *inBytes = (const uint8_t*) d + udata_getHeaderSize(header);
		const int32_t *inIndexes = (const int32_t*) inBytes;

	    int32_t indexesLength=inIndexes[IX_NORM_TRIE_OFFSET]/4;
	    if(indexesLength<=IX_MIN_LCCC_CP) {
	    	s_errorCode=U_INVALID_FORMAT_ERROR;  // Not enough indexes.
	        return;
	    }

		int32_t offset = inIndexes[IX_NORM_TRIE_OFFSET];
		int32_t nextOffset = inIndexes[IX_EXTRA_DATA_OFFSET];
		auto trie = ucptrie_openFromBinary(UCPTRIE_TYPE_FAST, UCPTRIE_VALUE_BITS_16, inBytes + offset,
				nextOffset - offset, NULL, &s_errorCode);
		if (U_FAILURE(s_errorCode)) {
			return;
		}

		offset = nextOffset;
		nextOffset = inIndexes[IX_SMALL_FCD_OFFSET];
		const uint16_t *inExtraData = (const uint16_t*) (inBytes + offset);

		// smallFCD: new in formatVersion 2
		offset = nextOffset;
		const uint8_t *inSmallFCD = inBytes + offset;

	    init(inIndexes, trie, inExtraData, inSmallFCD);
	}
}

Normalizer2Impl::~Normalizer2Impl() {
    delete fCanonIterData;
}

void Normalizer2Impl::init(const int32_t *inIndexes, const UCPTrie *inTrie, const uint16_t *inExtraData, const uint8_t *inSmallFCD) {
	minDecompNoCP = static_cast<UChar>(inIndexes[IX_MIN_DECOMP_NO_CP]);
	minCompNoMaybeCP = static_cast<UChar>(inIndexes[IX_MIN_COMP_NO_MAYBE_CP]);
	minLcccCP = static_cast<UChar>(inIndexes[IX_MIN_LCCC_CP]);

	minYesNo = static_cast<uint16_t>(inIndexes[IX_MIN_YES_NO]);
	minYesNoMappingsOnly = static_cast<uint16_t>(inIndexes[IX_MIN_YES_NO_MAPPINGS_ONLY]);
	minNoNo = static_cast<uint16_t>(inIndexes[IX_MIN_NO_NO]);
	minNoNoCompBoundaryBefore = static_cast<uint16_t>(inIndexes[IX_MIN_NO_NO_COMP_BOUNDARY_BEFORE]);
	minNoNoCompNoMaybeCC = static_cast<uint16_t>(inIndexes[IX_MIN_NO_NO_COMP_NO_MAYBE_CC]);
	minNoNoEmpty = static_cast<uint16_t>(inIndexes[IX_MIN_NO_NO_EMPTY]);
	limitNoNo = static_cast<uint16_t>(inIndexes[IX_LIMIT_NO_NO]);
	minMaybeYes = static_cast<uint16_t>(inIndexes[IX_MIN_MAYBE_YES]);
	// U_ASSERT((minMaybeYes & 7) == 0);  // 8-aligned for noNoDelta bit fields
	centerNoNoDelta = (minMaybeYes >> DELTA_SHIFT) - MAX_DELTA - 1;

	normTrie = inTrie;

	maybeYesCompositions = inExtraData;
	extraData = maybeYesCompositions + ((MIN_NORMAL_MAYBE_YES - minMaybeYes) >> OFFSET_SHIFT);

	smallFCD = inSmallFCD;
}

inline void uprv_checkCanGetBuffer(const UnicodeString &s, UErrorCode &errorCode) {
	if (U_SUCCESS(errorCode) && s.isBogus()) {
		errorCode = U_ILLEGAL_ARGUMENT_ERROR;
	}
}

/*
UBool
	Normalizer2WithImpl::getDecomposition(UChar32 c, UnicodeString &decomposition) const {
        UChar buffer[4];
        int32_t length;
        const UChar *d=impl.getDecomposition(c, buffer, length);
        if(d==NULL) {
            return false;
        }
        if(d==buffer) {
            decomposition.setTo(buffer, length);  // copy the string (Jamos from Hangul syllable c)
        } else {
            decomposition.setTo(false, d, length);  // read-only alias
        }
        return true;
    }
    UBool
	Normalizer2WithImpl::getRawDecomposition(UChar32 c, UnicodeString &decomposition) const {
        UChar buffer[30];
        int32_t length;
        const UChar *d=impl.getRawDecomposition(c, buffer, length);
        if(d==NULL) {
            return false;
        }
        if(d==buffer) {
            decomposition.setTo(buffer, length);  // copy the string (algorithmic decomposition)
        } else {
            decomposition.setTo(false, d, length);  // read-only alias
        }
        return true;
    }

    UChar32 Normalizer2WithImpl::composePair(UChar32 a, UChar32 b) const {
        return impl.composePair(a, b);
    }
*/



const ComposeNormalizer2 *ComposeNormalizer2::getInstance() {
	static ComposeNormalizer2 s_normalizer(uts46_data, false);
	return &s_normalizer;
}

ComposeNormalizer2::ComposeNormalizer2(const uint8_t *d, UBool fcc)
: Normalizer2WithImpl(d), onlyContiguous(fcc) { }

ComposeNormalizer2::~ComposeNormalizer2() { }

void ComposeNormalizer2::normalize(const UChar *src, const UChar *limit, ReorderingBuffer &buffer, UErrorCode &errorCode) const {
	impl.compose(src, limit, onlyContiguous, true, buffer, errorCode);
}

UnicodeString& ComposeNormalizer2::normalize(const UnicodeString &src, UnicodeString &dest, UErrorCode &errorCode) const {
	if (U_FAILURE(errorCode)) {
		dest.setToBogus();
		return dest;
	}
	const UChar *sArray = src.getBuffer();
	if (&dest == &src || sArray == NULL) {
		errorCode = U_ILLEGAL_ARGUMENT_ERROR;
		dest.setToBogus();
		return dest;
	}
	dest.remove();
	ReorderingBuffer buffer(impl, dest);
	if (buffer.init(src.length(), errorCode)) {
		normalize(sArray, sArray + src.length(), buffer, errorCode);
	}
	return dest;
}

/*




 //   using Normalizer2WithImpl::normalize;  // Avoid warning about hiding base class function.

void ComposeNormalizer2::normalizeUTF8(uint32_t options, StringPiece src, ByteSink &sink, Edits *edits, UErrorCode &errorCode) const {
	if (U_FAILURE(errorCode)) {
		return;
	}
	if (edits != nullptr && (options & U_EDITS_NO_RESET) == 0) {
		edits->reset();
	}
	const uint8_t *s = reinterpret_cast<const uint8_t *>(src.data());
	impl.composeUTF8(options, onlyContiguous, s, s + src.length(),
			&sink, edits, errorCode);
	sink.Flush();
}

UBool ComposeNormalizer2::isNormalizedUTF8(StringPiece sp, UErrorCode &errorCode) const {
	if (U_FAILURE(errorCode)) {
		return false;
	}
	const uint8_t *s = reinterpret_cast<const uint8_t*>(sp.data());
	return impl.composeUTF8(0, onlyContiguous, s, s + sp.length(), nullptr, nullptr, errorCode);
}

UNormalizationCheckResult ComposeNormalizer2::quickCheck(const UnicodeString &s, UErrorCode &errorCode) const {
	if (U_FAILURE(errorCode)) {
		return UNORM_MAYBE;
	}
	const UChar *sArray = s.getBuffer();
	if (sArray == NULL) {
		errorCode = U_ILLEGAL_ARGUMENT_ERROR;
		return UNORM_MAYBE;
	}
	UNormalizationCheckResult qcResult = UNORM_YES;
	impl.composeQuickCheck(sArray, sArray + s.length(), onlyContiguous, &qcResult);
	return qcResult;
}

const UChar* ComposeNormalizer2::spanQuickCheckYes(const UChar *src, const UChar *limit, UErrorCode&) const {
	return impl.composeQuickCheck(src, limit, onlyContiguous, NULL);
}

    int32_t ComposeNormalizer2::spanQuickCheckYes(const UnicodeString &s, UErrorCode &errorCode) const {
        if(U_FAILURE(errorCode)) {
            return 0;
        }
        const UChar *sArray=s.getBuffer();
        if(sArray==NULL) {
            errorCode=U_ILLEGAL_ARGUMENT_ERROR;
            return 0;
        }
        return (int32_t)(spanQuickCheckYes(sArray, sArray+s.length(), errorCode)-sArray);
    }

UnicodeString &
ComposeNormalizer2::append(UnicodeString &first,
	   const UnicodeString &second,
	   UErrorCode &errorCode) const {
	return normalizeSecondAndAppend(first, second, false, errorCode);
}

// using Normalizer2WithImpl::spanQuickCheckYes;  // Avoid warning about hiding base class function.

UNormalizationCheckResult ComposeNormalizer2::getQuickCheck(UChar32 c) const {
	return impl.getCompQuickCheck(impl.getNorm16(c));
}

UBool ComposeNormalizer2::hasBoundaryBefore(UChar32 c) const {
	return impl.hasCompBoundaryBefore(c);
}

UBool ComposeNormalizer2::hasBoundaryAfter(UChar32 c) const {
	return impl.hasCompBoundaryAfter(c, onlyContiguous);
}

UBool ComposeNormalizer2::isInert(UChar32 c) const {
	return impl.isCompInert(c, onlyContiguous);
}*/

UBool Normalizer2Impl::compose(const UChar *src, const UChar *limit, UBool onlyContiguous, UBool doCompose,
		ReorderingBuffer &buffer, UErrorCode &errorCode) const {
	const UChar *prevBoundary = src;
	UChar32 minNoMaybeCP = minCompNoMaybeCP;
	if (limit == NULL) {
		src = copyLowPrefixFromNulTerminated(src, minNoMaybeCP, doCompose ? &buffer : NULL, errorCode);
		if (U_FAILURE(errorCode)) {
			return false;
		}
		limit = u_strchr(src, 0);
		if (prevBoundary != src) {
			if (hasCompBoundaryAfter(*(src - 1), onlyContiguous)) {
				prevBoundary = src;
			} else {
				buffer.removeSuffix(1);
				prevBoundary = --src;
			}
		}
	}

	for (;;) {
		// Fast path: Scan over a sequence of characters below the minimum "no or maybe" code point,
		// or with (compYes && ccc==0) properties.
		const UChar *prevSrc;
		UChar32 c = 0;
		uint16_t norm16 = 0;
		for (;;) {
			if (src == limit) {
				if (prevBoundary != limit && doCompose) {
					buffer.appendZeroCC(prevBoundary, limit, errorCode);
				}
				return true;
			}
			if ((c = *src) < minNoMaybeCP || isCompYesAndZeroCC(norm16 = UCPTRIE_FAST_BMP_GET(normTrie, UCPTRIE_16, c))) {
				++src;
			} else {
				prevSrc = src++;
				if (!U16_IS_LEAD(c)) {
					break;
				} else {
					UChar c2;
					if (src != limit && U16_IS_TRAIL(c2 = *src)) {
						++src;
						c = U16_GET_SUPPLEMENTARY(c, c2);
						norm16 = UCPTRIE_FAST_SUPP_GET(normTrie, UCPTRIE_16, c);
						if (!isCompYesAndZeroCC(norm16)) {
							break;
						}
					}
				}
			}
		}
		// isCompYesAndZeroCC(norm16) is false, that is, norm16>=minNoNo.
		// The current character is either a "noNo" (has a mapping)
		// or a "maybeYes" (combines backward)
		// or a "yesYes" with ccc!=0.
		// It is not a Hangul syllable or Jamo L because those have "yes" properties.

		// Medium-fast path: Handle cases that do not require full decomposition and recomposition.
		if (!isMaybeOrNonZeroCC(norm16)) {  // minNoNo <= norm16 < minMaybeYes
			if (!doCompose) {
				return false;
			}
			// Fast path for mapping a character that is immediately surrounded by boundaries.
			// In this case, we need not decompose around the current character.
			if (isDecompNoAlgorithmic(norm16)) {
				// Maps to a single isCompYesAndZeroCC character
				// which also implies hasCompBoundaryBefore.
				if (norm16HasCompBoundaryAfter(norm16, onlyContiguous) || hasCompBoundaryBefore(src, limit)) {
					if (prevBoundary != prevSrc && !buffer.appendZeroCC(prevBoundary, prevSrc, errorCode)) {
						break;
					}
					if (!buffer.append(mapAlgorithmic(c, norm16), 0, errorCode)) {
						break;
					}
					prevBoundary = src;
					continue;
				}
			} else if (norm16 < minNoNoCompBoundaryBefore) {
				// The mapping is comp-normalized which also implies hasCompBoundaryBefore.
				if (norm16HasCompBoundaryAfter(norm16, onlyContiguous) || hasCompBoundaryBefore(src, limit)) {
					if (prevBoundary != prevSrc && !buffer.appendZeroCC(prevBoundary, prevSrc, errorCode)) {
						break;
					}
					const UChar *mapping = reinterpret_cast<const UChar*>(getMapping(norm16));
					int32_t length = *mapping++ & MAPPING_LENGTH_MASK;
					if (!buffer.appendZeroCC(mapping, mapping + length, errorCode)) {
						break;
					}
					prevBoundary = src;
					continue;
				}
			} else if (norm16 >= minNoNoEmpty) {
				// The current character maps to nothing.
				// Simply omit it from the output if there is a boundary before _or_ after it.
				// The character itself implies no boundaries.
				if (hasCompBoundaryBefore(src, limit) || hasCompBoundaryAfter(prevBoundary, prevSrc, onlyContiguous)) {
					if (prevBoundary != prevSrc && !buffer.appendZeroCC(prevBoundary, prevSrc, errorCode)) {
						break;
					}
					prevBoundary = src;
					continue;
				}
			}
			// Other "noNo" type, or need to examine more text around this character:
			// Fall through to the slow path.
		} else if (isJamoVT(norm16) && prevBoundary != prevSrc) {
			UChar prev = *(prevSrc - 1);
			if (c < Hangul::JAMO_T_BASE) {
				// The current character is a Jamo Vowel,
				// compose with previous Jamo L and following Jamo T.
				UChar l = (UChar) (prev - Hangul::JAMO_L_BASE);
				if (l < Hangul::JAMO_L_COUNT) {
					if (!doCompose) {
						return false;
					}
					int32_t t;
					if (src != limit && 0 < (t = ((int32_t) *src - Hangul::JAMO_T_BASE)) && t < Hangul::JAMO_T_COUNT) {
						// The next character is a Jamo T.
						++src;
					} else if (hasCompBoundaryBefore(src, limit)) {
						// No Jamo T follows, not even via decomposition.
						t = 0;
					} else {
						t = -1;
					}
					if (t >= 0) {
						UChar32 syllable = Hangul::HANGUL_BASE + (l * Hangul::JAMO_V_COUNT + (c - Hangul::JAMO_V_BASE)) * Hangul::JAMO_T_COUNT + t;
						--prevSrc;  // Replace the Jamo L as well.
						if (prevBoundary != prevSrc && !buffer.appendZeroCC(prevBoundary, prevSrc, errorCode)) {
							break;
						}
						if (!buffer.appendBMP((UChar) syllable, 0, errorCode)) {
							break;
						}
						prevBoundary = src;
						continue;
					}
					// If we see L+V+x where x!=T then we drop to the slow path,
					// decompose and recompose.
					// This is to deal with NFKC finding normal L and V but a
					// compatibility variant of a T.
					// We need to either fully compose that combination here
					// (which would complicate the code and may not work with strange custom data)
					// or use the slow path.
				}
			} else if (Hangul::isHangulLV(prev)) {
				// The current character is a Jamo Trailing consonant,
				// compose with previous Hangul LV that does not contain a Jamo T.
				if (!doCompose) {
					return false;
				}
				UChar32 syllable = prev + c - Hangul::JAMO_T_BASE;
				--prevSrc;  // Replace the Hangul LV as well.
				if (prevBoundary != prevSrc && !buffer.appendZeroCC(prevBoundary, prevSrc, errorCode)) {
					break;
				}
				if (!buffer.appendBMP((UChar) syllable, 0, errorCode)) {
					break;
				}
				prevBoundary = src;
				continue;
			}
			// No matching context, or may need to decompose surrounding text first:
			// Fall through to the slow path.
		} else if (norm16 > JAMO_VT) {  // norm16 >= MIN_YES_YES_WITH_CC
			// One or more combining marks that do not combine-back:
			// Check for canonical order, copy unchanged if ok and
			// if followed by a character with a boundary-before.
			uint8_t cc = getCCFromNormalYesOrMaybe(norm16);  // cc!=0
			if (onlyContiguous /* FCC */&& getPreviousTrailCC(prevBoundary, prevSrc) > cc) {
				// Fails FCD test, need to decompose and contiguously recompose.
				if (!doCompose) {
					return false;
				}
			} else {
				// If !onlyContiguous (not FCC), then we ignore the tccc of
				// the previous character which passed the quick check "yes && ccc==0" test.
				const UChar *nextSrc;
				uint16_t n16;
				for (;;) {
					if (src == limit) {
						if (doCompose) {
							buffer.appendZeroCC(prevBoundary, limit, errorCode);
						}
						return true;
					}
					uint8_t prevCC = cc;
					nextSrc = src;
					UCPTRIE_FAST_U16_NEXT(normTrie, UCPTRIE_16, nextSrc, limit, c, n16);
					if (n16 >= MIN_YES_YES_WITH_CC) {
						cc = getCCFromNormalYesOrMaybe(n16);
						if (prevCC > cc) {
							if (!doCompose) {
								return false;
							}
							break;
						}
					} else {
						break;
					}
					src = nextSrc;
				}
				// src is after the last in-order combining mark.
				// If there is a boundary here, then we continue with no change.
				if (norm16HasCompBoundaryBefore(n16)) {
					if (isCompYesAndZeroCC(n16)) {
						src = nextSrc;
					}
					continue;
				}
				// Use the slow path. There is no boundary in [prevSrc, src[.
			}
		}

		// Slow path: Find the nearest boundaries around the current character,
		// decompose and recompose.
		if (prevBoundary != prevSrc && !norm16HasCompBoundaryBefore(norm16)) {
			const UChar *p = prevSrc;
			UCPTRIE_FAST_U16_PREV(normTrie, UCPTRIE_16, prevBoundary, p, c, norm16);
			if (!norm16HasCompBoundaryAfter(norm16, onlyContiguous)) {
				prevSrc = p;
			}
		}
		if (doCompose && prevBoundary != prevSrc && !buffer.appendZeroCC(prevBoundary, prevSrc, errorCode)) {
			break;
		}
		int32_t recomposeStartIndex = buffer.length();
		// We know there is not a boundary here.
		decomposeShort(prevSrc, src, false /* !stopAtCompBoundary */, onlyContiguous, buffer, errorCode);
		// Decompose until the next boundary.
		src = decomposeShort(src, limit, true /* stopAtCompBoundary */, onlyContiguous, buffer, errorCode);
		if (U_FAILURE(errorCode)) {
			break;
		}
		if ((src - prevSrc) > INT32_MAX) {  // guard before buffer.equals()
			errorCode = U_INDEX_OUTOFBOUNDS_ERROR;
			return true;
		}
		recompose(buffer, recomposeStartIndex, onlyContiguous);
		if (!doCompose) {
			if (!buffer.equals(prevSrc, src)) {
				return false;
			}
			buffer.remove();
		}
		prevBoundary = src;
	}
	return true;
}

const UChar *Normalizer2Impl::copyLowPrefixFromNulTerminated(const UChar *src, UChar32 minNeedDataCP,
		ReorderingBuffer *buffer, UErrorCode &errorCode) const {
	// Make some effort to support NUL-terminated strings reasonably.
	// Take the part of the fast quick check loop that does not look up
	// data and check the first part of the string.
	// After this prefix, determine the string length to simplify the rest
	// of the code.
	const UChar *prevSrc = src;
	UChar c;
	while ((c = *src++) < minNeedDataCP && c != 0) { }
	// Back out the last character for full processing.
	// Copy this prefix.
	if (--src != prevSrc) {
		if (buffer != NULL) {
			buffer->appendZeroCC(prevSrc, src, errorCode);
		}
	}
	return src;
}

UBool Normalizer2Impl::hasCompBoundaryBefore(const UChar *src, const UChar *limit) const {
	if (src == limit || *src < minCompNoMaybeCP) {
		return true;
	}
	UChar32 c;
	uint16_t norm16;
	UCPTRIE_FAST_U16_NEXT(normTrie, UCPTRIE_16, src, limit, c, norm16);
	return norm16HasCompBoundaryBefore(norm16);
}

UBool Normalizer2Impl::hasCompBoundaryBefore(const uint8_t *src, const uint8_t *limit) const {
	if (src == limit) {
		return true;
	}
	uint16_t norm16;
	UCPTRIE_FAST_U8_NEXT(normTrie, UCPTRIE_16, src, limit, norm16);
	return norm16HasCompBoundaryBefore(norm16);
}

UBool Normalizer2Impl::hasCompBoundaryAfter(const UChar *start, const UChar *p, UBool onlyContiguous) const {
	if (start == p) {
		return true;
	}
	UChar32 c;
	uint16_t norm16;
	UCPTRIE_FAST_U16_PREV(normTrie, UCPTRIE_16, start, p, c, norm16);
	return norm16HasCompBoundaryAfter(norm16, onlyContiguous);
}

UBool Normalizer2Impl::hasCompBoundaryAfter(const uint8_t *start, const uint8_t *p, UBool onlyContiguous) const {
	if (start == p) {
		return true;
	}
	uint16_t norm16;
	UCPTRIE_FAST_U8_PREV(normTrie, UCPTRIE_16, start, p, norm16);
	return norm16HasCompBoundaryAfter(norm16, onlyContiguous);
}

uint8_t Normalizer2Impl::getPreviousTrailCC(const UChar *start, const UChar *p) const {
	if (start == p) {
		return 0;
	}
	int32_t i = (int32_t) (p - start);
	UChar32 c;
	U16_PREV(start, 0, i, c);
	return (uint8_t) getFCD16(c);
}

uint8_t Normalizer2Impl::getPreviousTrailCC(const uint8_t *start, const uint8_t *p) const {
	if (start == p) {
		return 0;
	}
	int32_t i = (int32_t) (p - start);
	UChar32 c;
	U8_PREV(start, 0, i, c);
	return (uint8_t) getFCD16(c);
}

const UChar* Normalizer2Impl::decomposeShort(const UChar *src, const UChar *limit,
		UBool stopAtCompBoundary, UBool onlyContiguous, ReorderingBuffer &buffer, UErrorCode &errorCode) const {
	if (U_FAILURE(errorCode)) {
		return nullptr;
	}
	while (src < limit) {
		if (stopAtCompBoundary && *src < minCompNoMaybeCP) {
			return src;
		}
		const UChar *prevSrc = src;
		UChar32 c;
		uint16_t norm16;
		UCPTRIE_FAST_U16_NEXT(normTrie, UCPTRIE_16, src, limit, c, norm16);
		if (stopAtCompBoundary && norm16HasCompBoundaryBefore(norm16)) {
			return prevSrc;
		}
		if (!decompose(c, norm16, buffer, errorCode)) {
			return nullptr;
		}
		if (stopAtCompBoundary && norm16HasCompBoundaryAfter(norm16, onlyContiguous)) {
			return src;
		}
	}
	return src;
}

const uint8_t*
Normalizer2Impl::decomposeShort(const uint8_t *src, const uint8_t *limit, StopAt stopAt,
		UBool onlyContiguous, ReorderingBuffer &buffer, UErrorCode &errorCode) const {
	if (U_FAILURE(errorCode)) {
		return nullptr;
	}
	while (src < limit) {
		const uint8_t *prevSrc = src;
		uint16_t norm16;
		UCPTRIE_FAST_U8_NEXT(normTrie, UCPTRIE_16, src, limit, norm16);
		// Get the decomposition and the lead and trail cc's.
		UChar32 c = U_SENTINEL;
		if (norm16 >= limitNoNo) {
			if (isMaybeOrNonZeroCC(norm16)) {
				// No comp boundaries around this character.
				uint8_t cc = getCCFromYesOrMaybe(norm16);
				if (cc == 0 && stopAt == STOP_AT_DECOMP_BOUNDARY) {
					return prevSrc;
				}
				c = codePointFromValidUTF8(prevSrc, src);
				if (!buffer.append(c, cc, errorCode)) {
					return nullptr;
				}
				if (stopAt == STOP_AT_DECOMP_BOUNDARY && buffer.getLastCC() <= 1) {
					return src;
				}
				continue;
			}
			// Maps to an isCompYesAndZeroCC.
			if (stopAt != STOP_AT_LIMIT) {
				return prevSrc;
			}
			c = codePointFromValidUTF8(prevSrc, src);
			c = mapAlgorithmic(c, norm16);
			norm16 = getRawNorm16(c);
		} else if (stopAt != STOP_AT_LIMIT && norm16 < minNoNoCompNoMaybeCC) {
			return prevSrc;
		}
		// norm16!=INERT guarantees that [prevSrc, src[ is valid UTF-8.
		// We do not see invalid UTF-8 here because
		// its norm16==INERT is normalization-inert,
		// so it gets copied unchanged in the fast path,
		// and we stop the slow path where invalid UTF-8 begins.
		// c >= 0 is the result of an algorithmic mapping.
		// U_ASSERT(c >= 0 || norm16 != INERT);
		if (norm16 < minYesNo) {
			if (c < 0) {
				c = codePointFromValidUTF8(prevSrc, src);
			}
			// does not decompose
			if (!buffer.append(c, 0, errorCode)) {
				return nullptr;
			}
		} else if (isHangulLV(norm16) || isHangulLVT(norm16)) {
			// Hangul syllable: decompose algorithmically
			if (c < 0) {
				c = codePointFromValidUTF8(prevSrc, src);
			}
			char16_t jamos[3];
			if (!buffer.appendZeroCC(jamos, jamos + Hangul::decompose(c, jamos), errorCode)) {
				return nullptr;
			}
		} else {
			// The character decomposes, get everything from the variable-length extra data.
			const uint16_t *mapping = getMapping(norm16);
			uint16_t firstUnit = *mapping;
			int32_t length = firstUnit & MAPPING_LENGTH_MASK;
			uint8_t trailCC = (uint8_t) (firstUnit >> 8);
			uint8_t leadCC;
			if (firstUnit & MAPPING_HAS_CCC_LCCC_WORD) {
				leadCC = (uint8_t) (*(mapping - 1) >> 8);
			} else {
				leadCC = 0;
			}
			if (leadCC == 0 && stopAt == STOP_AT_DECOMP_BOUNDARY) {
				return prevSrc;
			}
			if (!buffer.append((const char16_t*) mapping + 1, length, true, leadCC, trailCC, errorCode)) {
				return nullptr;
			}
		}
		if ((stopAt == STOP_AT_COMP_BOUNDARY && norm16HasCompBoundaryAfter(norm16, onlyContiguous)) || (stopAt == STOP_AT_DECOMP_BOUNDARY && buffer.getLastCC() <= 1)) {
			return src;
		}
	}
	return src;
}

void Normalizer2Impl::recompose(ReorderingBuffer &buffer, int32_t recomposeStartIndex, UBool onlyContiguous) const {
	UChar *p = buffer.getStart() + recomposeStartIndex;
	UChar *limit = buffer.getLimit();
	if (p == limit) {
		return;
	}

	UChar *starter, *pRemove, *q, *r;
	const uint16_t *compositionsList;
	UChar32 c, compositeAndFwd;
	uint16_t norm16;
	uint8_t cc, prevCC;
	UBool starterIsSupplementary;

	// Some of the following variables are not used until we have a forward-combining starter
	// and are only initialized now to avoid compiler warnings.
	compositionsList = NULL;  // used as indicator for whether we have a forward-combining starter
	starter = NULL;
	starterIsSupplementary = false;
	prevCC = 0;

	for (;;) {
		UCPTRIE_FAST_U16_NEXT(normTrie, UCPTRIE_16, p, limit, c, norm16);
		cc = getCCFromYesOrMaybe(norm16);
		if ( // this character combines backward and
		isMaybe(norm16) &&
		// we have seen a starter that combines forward and
				compositionsList != NULL &&
				// the backward-combining character is not blocked
				(prevCC < cc || prevCC == 0)) {
			if (isJamoVT(norm16)) {
				// c is a Jamo V/T, see if we can compose it with the previous character.
				if (c < Hangul::JAMO_T_BASE) {
					// c is a Jamo Vowel, compose with previous Jamo L and following Jamo T.
					UChar prev = (UChar) (*starter - Hangul::JAMO_L_BASE);
					if (prev < Hangul::JAMO_L_COUNT) {
						pRemove = p - 1;
						UChar syllable = (UChar) (Hangul::HANGUL_BASE + (prev * Hangul::JAMO_V_COUNT + (c - Hangul::JAMO_V_BASE)) * Hangul::JAMO_T_COUNT);
						UChar t;
						if (p != limit && (t = (UChar) (*p - Hangul::JAMO_T_BASE)) < Hangul::JAMO_T_COUNT) {
							++p;
							syllable += t;  // The next character was a Jamo T.
						}
						*starter = syllable;
						// remove the Jamo V/T
						q = pRemove;
						r = p;
						while (r < limit) {
							*q++ = *r++;
						}
						limit = q;
						p = pRemove;
					}
				}
				/*
				 * No "else" for Jamo T:
				 * Since the input is in NFD, there are no Hangul LV syllables that
				 * a Jamo T could combine with.
				 * All Jamo Ts are combined above when handling Jamo Vs.
				 */
				if (p == limit) {
					break;
				}
				compositionsList = NULL;
				continue;
			} else if ((compositeAndFwd = combine(compositionsList, c)) >= 0) {
				// The starter and the combining mark (c) do combine.
				UChar32 composite = compositeAndFwd >> 1;

				// Replace the starter with the composite, remove the combining mark.
				pRemove = p - U16_LENGTH(c);  // pRemove & p: start & limit of the combining mark
				if (starterIsSupplementary) {
					if (U_IS_SUPPLEMENTARY(composite)) {
						// both are supplementary
						starter[0] = U16_LEAD(composite);
						starter[1] = U16_TRAIL(composite);
					} else {
						*starter = (UChar) composite;
						// The composite is shorter than the starter,
						// move the intermediate characters forward one.
						starterIsSupplementary = false;
						q = starter + 1;
						r = q + 1;
						while (r < pRemove) {
							*q++ = *r++;
						}
						--pRemove;
					}
				} else if (U_IS_SUPPLEMENTARY(composite)) {
					// The composite is longer than the starter,
					// move the intermediate characters back one.
					starterIsSupplementary = true;
					++starter;  // temporarily increment for the loop boundary
					q = pRemove;
					r = ++pRemove;
					while (starter < q) {
						*--r = *--q;
					}
					*starter = U16_TRAIL(composite);
					*--starter = U16_LEAD(composite);  // undo the temporary increment
				} else {
					// both are on the BMP
					*starter = (UChar) composite;
				}

				/* remove the combining mark by moving the following text over it */
				if (pRemove < p) {
					q = pRemove;
					r = p;
					while (r < limit) {
						*q++ = *r++;
					}
					limit = q;
					p = pRemove;
				}
				// Keep prevCC because we removed the combining mark.

				if (p == limit) {
					break;
				}
				// Is the composite a starter that combines forward?
				if (compositeAndFwd & 1) {
					compositionsList = getCompositionsListForComposite(getRawNorm16(composite));
				} else {
					compositionsList = NULL;
				}

				// We combined; continue with looking for compositions.
				continue;
			}
		}

		// no combination this time
		prevCC = cc;
		if (p == limit) {
			break;
		}

		// If c did not combine, then check if it is a starter.
		if (cc == 0) {
			// Found a new starter.
			if ((compositionsList = getCompositionsListForDecompYes(norm16)) != NULL) {
				// It may combine with something, prepare for it.
				if (U_IS_BMP(c)) {
					starterIsSupplementary = false;
					starter = p - 1;
				} else {
					starterIsSupplementary = true;
					starter = p - 2;
				}
			}
		} else if (onlyContiguous) {
			// FCC: no discontiguous compositions; any intervening character blocks.
			compositionsList = NULL;
		}
	}
	buffer.setReorderingLimit(limit);
}

UBool Normalizer2Impl::decompose(UChar32 c, uint16_t norm16, ReorderingBuffer &buffer, UErrorCode &errorCode) const {
	// get the decomposition and the lead and trail cc's
	if (norm16 >= limitNoNo) {
		if (isMaybeOrNonZeroCC(norm16)) {
			return buffer.append(c, getCCFromYesOrMaybe(norm16), errorCode);
		}
		// Maps to an isCompYesAndZeroCC.
		c = mapAlgorithmic(c, norm16);
		norm16 = getRawNorm16(c);
	}
	if (norm16 < minYesNo) {
		// c does not decompose
		return buffer.append(c, 0, errorCode);
	} else if (isHangulLV(norm16) || isHangulLVT(norm16)) {
		// Hangul syllable: decompose algorithmically
		UChar jamos[3];
		return buffer.appendZeroCC(jamos, jamos + Hangul::decompose(c, jamos), errorCode);
	}
	// c decomposes, get everything from the variable-length extra data
	const uint16_t *mapping = getMapping(norm16);
	uint16_t firstUnit = *mapping;
	int32_t length = firstUnit & MAPPING_LENGTH_MASK;
	uint8_t leadCC, trailCC;
	trailCC = (uint8_t) (firstUnit >> 8);
	if (firstUnit & MAPPING_HAS_CCC_LCCC_WORD) {
		leadCC = (uint8_t) (*(mapping - 1) >> 8);
	} else {
		leadCC = 0;
	}
	return buffer.append((const UChar*) mapping + 1, length, true, leadCC, trailCC, errorCode);
}

int32_t Normalizer2Impl::combine(const uint16_t *list, UChar32 trail) {
	uint16_t key1, firstUnit;
	if (trail < COMP_1_TRAIL_LIMIT) {
		// trail character is 0..33FF
		// result entry may have 2 or 3 units
		key1 = (uint16_t) (trail << 1);
		while (key1 > (firstUnit = *list)) {
			list += 2 + (firstUnit & COMP_1_TRIPLE);
		}
		if (key1 == (firstUnit & COMP_1_TRAIL_MASK)) {
			if (firstUnit & COMP_1_TRIPLE) {
				return ((int32_t) list[1] << 16) | list[2];
			} else {
				return list[1];
			}
		}
	} else {
		// trail character is 3400..10FFFF
		// result entry has 3 units
		key1 = (uint16_t) (COMP_1_TRAIL_LIMIT + (((trail >> COMP_1_TRAIL_SHIFT)) & ~COMP_1_TRIPLE));
		uint16_t key2 = (uint16_t) (trail << COMP_2_TRAIL_SHIFT);
		uint16_t secondUnit;
		for (;;) {
			if (key1 > (firstUnit = *list)) {
				list += 2 + (firstUnit & COMP_1_TRIPLE);
			} else if (key1 == (firstUnit & COMP_1_TRAIL_MASK)) {
				if (key2 > (secondUnit = list[1])) {
					if (firstUnit & COMP_1_LAST_TUPLE) {
						break;
					} else {
						list += 3;
					}
				} else if (key2 == (secondUnit & COMP_2_TRAIL_MASK)) {
					return ((int32_t) (secondUnit & ~COMP_2_TRAIL_MASK) << 16) | list[2];
				} else {
					break;
				}
			} else {
				break;
			}
		}
	}
	return -1;
}

UnicodeString& ComposeNormalizer2::normalizeSecondAndAppend(UnicodeString &first, const UnicodeString &second, UBool doNormalize, UErrorCode &errorCode) const {
	uprv_checkCanGetBuffer(first, errorCode);
	if (U_FAILURE(errorCode)) {
		return first;
	}
	const UChar *secondArray = second.getBuffer();
	if (&first == &second || secondArray == NULL) {
		errorCode = U_ILLEGAL_ARGUMENT_ERROR;
		return first;
	}
	int32_t firstLength = first.length();
	UnicodeString safeMiddle;
	{
		ReorderingBuffer buffer(impl, first);
		if (buffer.init(firstLength + second.length(), errorCode)) {
			normalizeAndAppend(secondArray, secondArray + second.length(), doNormalize, safeMiddle, buffer, errorCode);
		}
	}  // The ReorderingBuffer destructor finalizes the first string.
	if (U_FAILURE(errorCode)) {
		// Restore the modified suffix of the first string.
		first.replace(firstLength - safeMiddle.length(), 0x7fffffff, safeMiddle);
	}
	return first;
}

UnicodeString&
ComposeNormalizer2::normalizeSecondAndAppend(UnicodeString &first, const UnicodeString &second, UErrorCode &errorCode) const {
	return normalizeSecondAndAppend(first, second, true, errorCode);
}

void ComposeNormalizer2::normalizeAndAppend(const UChar *src, const UChar *limit, UBool doNormalize,
		UnicodeString &safeMiddle, ReorderingBuffer &buffer, UErrorCode &errorCode) const {
	impl.composeAndAppend(src, limit, doNormalize, onlyContiguous, safeMiddle, buffer, errorCode);
}

void Normalizer2Impl::composeAndAppend(const UChar *src, const UChar *limit,
		UBool doCompose, UBool onlyContiguous, UnicodeString &safeMiddle, ReorderingBuffer &buffer, UErrorCode &errorCode) const {
	if (!buffer.isEmpty()) {
		const UChar *firstStarterInSrc = findNextCompBoundary(src, limit, onlyContiguous);
		if (src != firstStarterInSrc) {
			const UChar *lastStarterInDest = findPreviousCompBoundary(buffer.getStart(), buffer.getLimit(), onlyContiguous);
			int32_t destSuffixLength = (int32_t) (buffer.getLimit() - lastStarterInDest);
			UnicodeString middle(lastStarterInDest, destSuffixLength);
			buffer.removeSuffix(destSuffixLength);
			safeMiddle = middle;
			middle.append(src, (int32_t) (firstStarterInSrc - src));
			const UChar *middleStart = middle.getBuffer();
			compose(middleStart, middleStart + middle.length(), onlyContiguous, true, buffer, errorCode);
			if (U_FAILURE(errorCode)) {
				return;
			}
			src = firstStarterInSrc;
		}
	}
	if (doCompose) {
		compose(src, limit, onlyContiguous, true, buffer, errorCode);
	} else {
		if (limit == NULL) {  // appendZeroCC() needs limit!=NULL
			limit = u_strchr(src, 0);
		}
		buffer.appendZeroCC(src, limit, errorCode);
	}
}

const UChar* Normalizer2Impl::findPreviousCompBoundary(const UChar *start, const UChar *p, UBool onlyContiguous) const {
	while (p != start) {
		const UChar *codePointLimit = p;
		UChar32 c;
		uint16_t norm16;
		UCPTRIE_FAST_U16_PREV(normTrie, UCPTRIE_16, start, p, c, norm16);
		if (norm16HasCompBoundaryAfter(norm16, onlyContiguous)) {
			return codePointLimit;
		}
		if (hasCompBoundaryBefore(c, norm16)) {
			return p;
		}
	}
	return p;
}

const UChar* Normalizer2Impl::findNextCompBoundary(const UChar *p, const UChar *limit, UBool onlyContiguous) const {
	while (p != limit) {
		const UChar *codePointStart = p;
		UChar32 c;
		uint16_t norm16;
		UCPTRIE_FAST_U16_NEXT(normTrie, UCPTRIE_16, p, limit, c, norm16);
		if (hasCompBoundaryBefore(c, norm16)) {
			return codePointStart;
		}
		if (norm16HasCompBoundaryAfter(norm16, onlyContiguous)) {
			return p;
		}
	}
	return p;
}

UBool ComposeNormalizer2::isNormalized(const UnicodeString &s, UErrorCode &errorCode) const {
	if (U_FAILURE(errorCode)) {
		return false;
	}
	const UChar *sArray = s.getBuffer();
	if (sArray == NULL) {
		errorCode = U_ILLEGAL_ARGUMENT_ERROR;
		return false;
	}
	UnicodeString temp;
	ReorderingBuffer buffer(impl, temp);
	if (!buffer.init(5, errorCode)) {  // small destCapacity for substring normalization
		return false;
	}
	return impl.compose(sArray, sArray + s.length(), onlyContiguous, false, buffer, errorCode);
}

uint16_t Normalizer2Impl::getFCD16FromNormData(UChar32 c) const {
	uint16_t norm16 = getNorm16(c);
	if (norm16 >= limitNoNo) {
		if (norm16 >= MIN_NORMAL_MAYBE_YES) {
			// combining mark
			norm16 = getCCFromNormalYesOrMaybe(norm16);
			return norm16 | (norm16 << 8);
		} else if (norm16 >= minMaybeYes) {
			return 0;
		} else {  // isDecompNoAlgorithmic(norm16)
			uint16_t deltaTrailCC = norm16 & DELTA_TCCC_MASK;
			if (deltaTrailCC <= DELTA_TCCC_1) {
				return deltaTrailCC >> OFFSET_SHIFT;
			}
			// Maps to an isCompYesAndZeroCC.
			c = mapAlgorithmic(c, norm16);
			norm16 = getRawNorm16(c);
		}
	}
	if (norm16 <= minYesNo || isHangulLVT(norm16)) {
		// no decomposition or Hangul syllable, all zeros
		return 0;
	}
	// c decomposes, get everything from the variable-length extra data
	const uint16_t *mapping = getMapping(norm16);
	uint16_t firstUnit = *mapping;
	norm16 = firstUnit >> 8;  // tccc
	if (firstUnit & MAPPING_HAS_CCC_LCCC_WORD) {
		norm16 |= *(mapping - 1) & 0xff00;  // lccc
	}
	return norm16;
}

}
