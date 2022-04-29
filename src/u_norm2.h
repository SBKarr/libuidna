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

#ifndef MODULES_IDN_UIDNANORMALIZER2_H_
#define MODULES_IDN_UIDNANORMALIZER2_H_

#include "u_edits.h"

namespace uidna {

class Edits;

struct UCPTrie;
class UnicodeSet;
struct USetAdder;
struct CanonIterData;
class ReorderingBuffer;

/**
 * Low-level implementation of the Unicode Normalization Algorithm.
 * For the data structure and details see the documentation at the end of
 * this normalizer2impl.h and in the design doc at
 * https://icu.unicode.org/design/normalization/custom
 */
class Normalizer2Impl {
public:
	Normalizer2Impl(const uint8_t *d);
	~Normalizer2Impl();

	void init(const int32_t *inIndexes, const UCPTrie *inTrie,
			  const uint16_t *inExtraData, const uint8_t *inSmallFCD);

	void addLcccChars(UnicodeSet &set) const;
	void addPropertyStarts(const USetAdder *sa, UErrorCode &errorCode) const;
	void addCanonIterPropertyStarts(const USetAdder *sa, UErrorCode &errorCode) const;

	// low-level properties ------------------------------------------------ ***

	UBool ensureCanonIterData(UErrorCode &errorCode) const;

	// The trie stores values for lead surrogate code *units*.
	// Surrogate code *points* are inert.
	uint16_t getNorm16(UChar32 c) const;
	uint16_t getRawNorm16(UChar32 c) const;

	UNormalizationCheckResult getCompQuickCheck(uint16_t norm16) const {
		if(norm16<minNoNo || MIN_YES_YES_WITH_CC<=norm16) {
			return UNORM_YES;
		} else if(minMaybeYes<=norm16) {
			return UNORM_MAYBE;
		} else {
			return UNORM_NO;
		}
	}
	UBool isAlgorithmicNoNo(uint16_t norm16) const { return limitNoNo<=norm16 && norm16<minMaybeYes; }
	UBool isCompNo(uint16_t norm16) const { return minNoNo<=norm16 && norm16<minMaybeYes; }
	UBool isDecompYes(uint16_t norm16) const { return norm16<minYesNo || minMaybeYes<=norm16; }

	uint8_t getCC(uint16_t norm16) const {
		if(norm16>=MIN_NORMAL_MAYBE_YES) {
			return getCCFromNormalYesOrMaybe(norm16);
		}
		if(norm16<minNoNo || limitNoNo<=norm16) {
			return 0;
		}
		return getCCFromNoNo(norm16);
	}
	static uint8_t getCCFromNormalYesOrMaybe(uint16_t norm16) {
		return (uint8_t)(norm16 >> OFFSET_SHIFT);
	}
	static uint8_t getCCFromYesOrMaybe(uint16_t norm16) {
		return norm16>=MIN_NORMAL_MAYBE_YES ? getCCFromNormalYesOrMaybe(norm16) : 0;
	}
	uint8_t getCCFromYesOrMaybeCP(UChar32 c) const {
		if (c < minCompNoMaybeCP) { return 0; }
		return getCCFromYesOrMaybe(getNorm16(c));
	}

	/**
	 * Returns the FCD data for code point c.
	 * @param c A Unicode code point.
	 * @return The lccc(c) in bits 15..8 and tccc(c) in bits 7..0.
	 */
	uint16_t getFCD16(UChar32 c) const {
		if(c<minDecompNoCP) {
			return 0;
		} else if(c<=0xffff) {
			if(!singleLeadMightHaveNonZeroFCD16(c)) { return 0; }
		}
		return getFCD16FromNormData(c);
	}
	/**
	 * Returns the FCD data for the next code point (post-increment).
	 * Might skip only a lead surrogate rather than the whole surrogate pair if none of
	 * the supplementary code points associated with the lead surrogate have non-zero FCD data.
	 * @param s A valid pointer into a string. Requires s!=limit.
	 * @param limit The end of the string, or NULL.
	 * @return The lccc(c) in bits 15..8 and tccc(c) in bits 7..0.
	 */
	uint16_t nextFCD16(const UChar *&s, const UChar *limit) const;
	/**
	 * Returns the FCD data for the previous code point (pre-decrement).
	 * @param start The start of the string.
	 * @param s A valid pointer into a string. Requires start<s.
	 * @return The lccc(c) in bits 15..8 and tccc(c) in bits 7..0.
	 */
	uint16_t previousFCD16(const UChar *start, const UChar *&s) const;

	/** Returns true if the single-or-lead code unit c might have non-zero FCD data. */
	UBool singleLeadMightHaveNonZeroFCD16(UChar32 lead) const {
		// 0<=lead<=0xffff
		uint8_t bits=smallFCD[lead>>8];
		if(bits==0) { return false; }
		return (UBool)((bits>>((lead>>5)&7))&1);
	}
	/** Returns the FCD value from the regular normalization data. */
	uint16_t getFCD16FromNormData(UChar32 c) const;

	/**
	 * Gets the decomposition for one code point.
	 * @param c code point
	 * @param buffer out-only buffer for algorithmic decompositions
	 * @param length out-only, takes the length of the decomposition, if any
	 * @return pointer to the decomposition, or NULL if none
	 */
	const UChar *getDecomposition(UChar32 c, UChar buffer[4], int32_t &length) const;

	/**
	 * Gets the raw decomposition for one code point.
	 * @param c code point
	 * @param buffer out-only buffer for algorithmic decompositions
	 * @param length out-only, takes the length of the decomposition, if any
	 * @return pointer to the decomposition, or NULL if none
	 */
	const UChar *getRawDecomposition(UChar32 c, UChar buffer[30], int32_t &length) const;

	UChar32 composePair(UChar32 a, UChar32 b) const;

	UBool isCanonSegmentStarter(UChar32 c) const;
	UBool getCanonStartSet(UChar32 c, UnicodeSet &set) const;

	enum {
		// Fixed norm16 values.
		MIN_YES_YES_WITH_CC=0xfe02,
		JAMO_VT=0xfe00,
		MIN_NORMAL_MAYBE_YES=0xfc00,
		JAMO_L=2,  // offset=1 hasCompBoundaryAfter=false
		INERT=1,  // offset=0 hasCompBoundaryAfter=true

		// norm16 bit 0 is comp-boundary-after.
		HAS_COMP_BOUNDARY_AFTER=1,
		OFFSET_SHIFT=1,

		// For algorithmic one-way mappings, norm16 bits 2..1 indicate the
		// tccc (0, 1, >1) for quick FCC boundary-after tests.
		DELTA_TCCC_0=0,
		DELTA_TCCC_1=2,
		DELTA_TCCC_GT_1=4,
		DELTA_TCCC_MASK=6,
		DELTA_SHIFT=3,

		MAX_DELTA=0x40
	};

	enum {
		// Byte offsets from the start of the data, after the generic header.
		IX_NORM_TRIE_OFFSET,
		IX_EXTRA_DATA_OFFSET,
		IX_SMALL_FCD_OFFSET,
		IX_RESERVED3_OFFSET,
		IX_RESERVED4_OFFSET,
		IX_RESERVED5_OFFSET,
		IX_RESERVED6_OFFSET,
		IX_TOTAL_SIZE,

		// Code point thresholds for quick check codes.
		IX_MIN_DECOMP_NO_CP,
		IX_MIN_COMP_NO_MAYBE_CP,

		// Norm16 value thresholds for quick check combinations and types of extra data.

		/** Mappings & compositions in [minYesNo..minYesNoMappingsOnly[. */
		IX_MIN_YES_NO,
		/** Mappings are comp-normalized. */
		IX_MIN_NO_NO,
		IX_LIMIT_NO_NO,
		IX_MIN_MAYBE_YES,

		/** Mappings only in [minYesNoMappingsOnly..minNoNo[. */
		IX_MIN_YES_NO_MAPPINGS_ONLY,
		/** Mappings are not comp-normalized but have a comp boundary before. */
		IX_MIN_NO_NO_COMP_BOUNDARY_BEFORE,
		/** Mappings do not have a comp boundary before. */
		IX_MIN_NO_NO_COMP_NO_MAYBE_CC,
		/** Mappings to the empty string. */
		IX_MIN_NO_NO_EMPTY,

		IX_MIN_LCCC_CP,
		IX_RESERVED19,
		IX_COUNT
	};

	enum {
		MAPPING_HAS_CCC_LCCC_WORD=0x80,
		MAPPING_HAS_RAW_MAPPING=0x40,
		// unused bit 0x20,
		MAPPING_LENGTH_MASK=0x1f
	};

	enum {
		COMP_1_LAST_TUPLE=0x8000,
		COMP_1_TRIPLE=1,
		COMP_1_TRAIL_LIMIT=0x3400,
		COMP_1_TRAIL_MASK=0x7ffe,
		COMP_1_TRAIL_SHIFT=9,  // 10-1 for the "triple" bit
		COMP_2_TRAIL_SHIFT=6,
		COMP_2_TRAIL_MASK=0xffc0
	};

	// higher-level functionality ------------------------------------------ ***

	// NFD without an NFD Normalizer2 instance.
	UnicodeString &decompose(const UnicodeString &src, UnicodeString &dest,
							 UErrorCode &errorCode) const;
	/**
	 * Decomposes [src, limit[ and writes the result to dest.
	 * limit can be NULL if src is NUL-terminated.
	 * destLengthEstimate is the initial dest buffer capacity and can be -1.
	 */
	void decompose(const UChar *src, const UChar *limit,
				   UnicodeString &dest, int32_t destLengthEstimate,
				   UErrorCode &errorCode) const;

	const UChar *decompose(const UChar *src, const UChar *limit,
						   ReorderingBuffer *buffer, UErrorCode &errorCode) const;
	void decomposeAndAppend(const UChar *src, const UChar *limit,
							UBool doDecompose,
							UnicodeString &safeMiddle,
							ReorderingBuffer &buffer,
							UErrorCode &errorCode) const;

	/** sink==nullptr: isNormalized()/spanQuickCheckYes() */
	const uint8_t *decomposeUTF8(uint32_t options,
								 const uint8_t *src, const uint8_t *limit,
								 ByteSink *sink, Edits *edits, UErrorCode &errorCode) const;

	UBool compose(const UChar *src, const UChar *limit, UBool onlyContiguous, UBool doCompose,
			ReorderingBuffer &buffer, UErrorCode &errorCode) const;

	const UChar *composeQuickCheck(const UChar *src, const UChar *limit,
								   UBool onlyContiguous,
								   UNormalizationCheckResult *pQCResult) const;
	void composeAndAppend(const UChar *src, const UChar *limit,
						  UBool doCompose,
						  UBool onlyContiguous,
						  UnicodeString &safeMiddle,
						  ReorderingBuffer &buffer,
						  UErrorCode &errorCode) const;

	/** sink==nullptr: isNormalized() */
	UBool composeUTF8(uint32_t options, UBool onlyContiguous,
					  const uint8_t *src, const uint8_t *limit,
					  ByteSink *sink, Edits *edits, UErrorCode &errorCode) const;

	const UChar *makeFCD(const UChar *src, const UChar *limit,
						 ReorderingBuffer *buffer, UErrorCode &errorCode) const;
	void makeFCDAndAppend(const UChar *src, const UChar *limit,
						  UBool doMakeFCD,
						  UnicodeString &safeMiddle,
						  ReorderingBuffer &buffer,
						  UErrorCode &errorCode) const;

	UBool hasDecompBoundaryBefore(UChar32 c) const;
	UBool norm16HasDecompBoundaryBefore(uint16_t norm16) const;
	UBool hasDecompBoundaryAfter(UChar32 c) const;
	UBool norm16HasDecompBoundaryAfter(uint16_t norm16) const;
	UBool isDecompInert(UChar32 c) const { return isDecompYesAndZeroCC(getNorm16(c)); }

	UBool hasCompBoundaryBefore(UChar32 c) const {
		return c<minCompNoMaybeCP || norm16HasCompBoundaryBefore(getNorm16(c));
	}
	UBool hasCompBoundaryAfter(UChar32 c, UBool onlyContiguous) const {
		return norm16HasCompBoundaryAfter(getNorm16(c), onlyContiguous);
	}
	UBool isCompInert(UChar32 c, UBool onlyContiguous) const {
		uint16_t norm16=getNorm16(c);
		return isCompYesAndZeroCC(norm16) &&
			(norm16 & HAS_COMP_BOUNDARY_AFTER) != 0 &&
			(!onlyContiguous || isInert(norm16) || *getMapping(norm16) <= 0x1ff);
	}

	UBool hasFCDBoundaryBefore(UChar32 c) const { return hasDecompBoundaryBefore(c); }
	UBool hasFCDBoundaryAfter(UChar32 c) const { return hasDecompBoundaryAfter(c); }
	UBool isFCDInert(UChar32 c) const { return getFCD16(c)<=1; }
private:
	friend class InitCanonIterData;
	friend class LcccContext;

	UBool isMaybe(uint16_t norm16) const { return minMaybeYes<=norm16 && norm16<=JAMO_VT; }
	UBool isMaybeOrNonZeroCC(uint16_t norm16) const { return norm16>=minMaybeYes; }
	static UBool isInert(uint16_t norm16) { return norm16==INERT; }
	static UBool isJamoL(uint16_t norm16) { return norm16==JAMO_L; }
	static UBool isJamoVT(uint16_t norm16) { return norm16==JAMO_VT; }
	uint16_t hangulLVT() const { return minYesNoMappingsOnly|HAS_COMP_BOUNDARY_AFTER; }
	UBool isHangulLV(uint16_t norm16) const { return norm16==minYesNo; }
	UBool isHangulLVT(uint16_t norm16) const {
		return norm16==hangulLVT();
	}
	UBool isCompYesAndZeroCC(uint16_t norm16) const { return norm16<minNoNo; }
	// UBool isCompYes(uint16_t norm16) const {
	//     return norm16>=MIN_YES_YES_WITH_CC || norm16<minNoNo;
	// }
	// UBool isCompYesOrMaybe(uint16_t norm16) const {
	//     return norm16<minNoNo || minMaybeYes<=norm16;
	// }
	// UBool hasZeroCCFromDecompYes(uint16_t norm16) const {
	//     return norm16<=MIN_NORMAL_MAYBE_YES || norm16==JAMO_VT;
	// }
	UBool isDecompYesAndZeroCC(uint16_t norm16) const {
		return norm16<minYesNo ||
			   norm16==JAMO_VT ||
			   (minMaybeYes<=norm16 && norm16<=MIN_NORMAL_MAYBE_YES);
	}
	/**
	 * A little faster and simpler than isDecompYesAndZeroCC() but does not include
	 * the MaybeYes which combine-forward and have ccc=0.
	 * (Standard Unicode 10 normalization does not have such characters.)
	 */
	UBool isMostDecompYesAndZeroCC(uint16_t norm16) const {
		return norm16<minYesNo || norm16==MIN_NORMAL_MAYBE_YES || norm16==JAMO_VT;
	}
	UBool isDecompNoAlgorithmic(uint16_t norm16) const { return norm16>=limitNoNo; }

	// For use with isCompYes().
	// Perhaps the compiler can combine the two tests for MIN_YES_YES_WITH_CC.
	// static uint8_t getCCFromYes(uint16_t norm16) {
	//     return norm16>=MIN_YES_YES_WITH_CC ? getCCFromNormalYesOrMaybe(norm16) : 0;
	// }
	uint8_t getCCFromNoNo(uint16_t norm16) const {
		const uint16_t *mapping=getMapping(norm16);
		if(*mapping&MAPPING_HAS_CCC_LCCC_WORD) {
			return (uint8_t)*(mapping-1);
		} else {
			return 0;
		}
	}
	// requires that the [cpStart..cpLimit[ character passes isCompYesAndZeroCC()
	uint8_t getTrailCCFromCompYesAndZeroCC(uint16_t norm16) const {
		if(norm16<=minYesNo) {
			return 0;  // yesYes and Hangul LV have ccc=tccc=0
		} else {
			// For Hangul LVT we harmlessly fetch a firstUnit with tccc=0 here.
			return (uint8_t)(*getMapping(norm16)>>8);  // tccc from yesNo
		}
	}
	uint8_t getPreviousTrailCC(const UChar *start, const UChar *p) const;
	uint8_t getPreviousTrailCC(const uint8_t *start, const uint8_t *p) const;

	// Requires algorithmic-NoNo.
	UChar32 mapAlgorithmic(UChar32 c, uint16_t norm16) const {
		return c+(norm16>>DELTA_SHIFT)-centerNoNoDelta;
	}
	UChar32 getAlgorithmicDelta(uint16_t norm16) const {
		return (norm16>>DELTA_SHIFT)-centerNoNoDelta;
	}

	// Requires minYesNo<norm16<limitNoNo.
	const uint16_t *getMapping(uint16_t norm16) const { return extraData+(norm16>>OFFSET_SHIFT); }
	const uint16_t *getCompositionsListForDecompYes(uint16_t norm16) const {
		if(norm16<JAMO_L || MIN_NORMAL_MAYBE_YES<=norm16) {
			return NULL;
		} else if(norm16<minMaybeYes) {
			return getMapping(norm16);  // for yesYes; if Jamo L: harmless empty list
		} else {
			return maybeYesCompositions+norm16-minMaybeYes;
		}
	}
	const uint16_t *getCompositionsListForComposite(uint16_t norm16) const {
		// A composite has both mapping & compositions list.
		const uint16_t *list=getMapping(norm16);
		return list+  // mapping pointer
			1+  // +1 to skip the first unit with the mapping length
			(*list&MAPPING_LENGTH_MASK);  // + mapping length
	}
	const uint16_t *getCompositionsListForMaybe(uint16_t norm16) const {
		// minMaybeYes<=norm16<MIN_NORMAL_MAYBE_YES
		return maybeYesCompositions+((norm16-minMaybeYes)>>OFFSET_SHIFT);
	}
	/**
	 * @param c code point must have compositions
	 * @return compositions list pointer
	 */
	const uint16_t* getCompositionsList(uint16_t norm16) const {
		return isDecompYes(norm16) ? getCompositionsListForDecompYes(norm16) : getCompositionsListForComposite(norm16);
	}

	const UChar* copyLowPrefixFromNulTerminated(const UChar *src, UChar32 minNeedDataCP,
			ReorderingBuffer *buffer, UErrorCode &errorCode) const;

	enum StopAt { STOP_AT_LIMIT, STOP_AT_DECOMP_BOUNDARY, STOP_AT_COMP_BOUNDARY };

	const UChar* decomposeShort(const UChar *src, const UChar *limit, UBool stopAtCompBoundary,
			UBool onlyContiguous, ReorderingBuffer &buffer, UErrorCode &errorCode) const;
	const uint8_t* decomposeShort(const uint8_t *src, const uint8_t *limit, StopAt stopAt,
			UBool onlyContiguous, ReorderingBuffer &buffer, UErrorCode &errorCode) const;

	UBool decompose(UChar32 c, uint16_t norm16, ReorderingBuffer &buffer, UErrorCode &errorCode) const;

	static int32_t combine(const uint16_t *list, UChar32 trail);
	void addComposites(const uint16_t *list, UnicodeSet &set) const;
	void recompose(ReorderingBuffer &buffer, int32_t recomposeStartIndex,
				   UBool onlyContiguous) const;

	UBool hasCompBoundaryBefore(UChar32 c, uint16_t norm16) const {
		return c<minCompNoMaybeCP || norm16HasCompBoundaryBefore(norm16);
	}
	UBool norm16HasCompBoundaryBefore(uint16_t norm16) const  {
		return norm16 < minNoNoCompNoMaybeCC || isAlgorithmicNoNo(norm16);
	}
	UBool hasCompBoundaryBefore(const UChar *src, const UChar *limit) const;
	UBool hasCompBoundaryBefore(const uint8_t *src, const uint8_t *limit) const;
	UBool hasCompBoundaryAfter(const UChar *start, const UChar *p, UBool onlyContiguous) const;
	UBool hasCompBoundaryAfter(const uint8_t *start, const uint8_t *p, UBool onlyContiguous) const;
	UBool norm16HasCompBoundaryAfter(uint16_t norm16, UBool onlyContiguous) const {
		return (norm16 & HAS_COMP_BOUNDARY_AFTER) != 0 && (!onlyContiguous || isTrailCC01ForCompBoundaryAfter(norm16));
	}
	/** For FCC: Given norm16 HAS_COMP_BOUNDARY_AFTER, does it have tccc<=1? */
	UBool isTrailCC01ForCompBoundaryAfter(uint16_t norm16) const {
		return isInert(norm16) || (isDecompNoAlgorithmic(norm16) ?
			(norm16 & DELTA_TCCC_MASK) <= DELTA_TCCC_1 : *getMapping(norm16) <= 0x1ff);
	}

	const UChar *findPreviousCompBoundary(const UChar *start, const UChar *p, UBool onlyContiguous) const;
	const UChar *findNextCompBoundary(const UChar *p, const UChar *limit, UBool onlyContiguous) const;

	const UChar *findPreviousFCDBoundary(const UChar *start, const UChar *p) const;
	const UChar *findNextFCDBoundary(const UChar *p, const UChar *limit) const;

	void makeCanonIterDataFromNorm16(UChar32 start, UChar32 end, const uint16_t norm16,
									 CanonIterData &newData, UErrorCode &errorCode) const;

	int32_t getCanonValue(UChar32 c) const;
	const UnicodeSet &getCanonStartSet(int32_t n) const;

	// UVersionInfo dataVersion;

	// BMP code point thresholds for quick check loops looking at single UTF-16 code units.
	UChar minDecompNoCP = 0;
	UChar minCompNoMaybeCP = 0;
	UChar minLcccCP = 0;

	// Norm16 value thresholds for quick check combinations and types of extra data.
	uint16_t minYesNo = 0;
	uint16_t minYesNoMappingsOnly = 0;
	uint16_t minNoNo = 0;
	uint16_t minNoNoCompBoundaryBefore = 0;
	uint16_t minNoNoCompNoMaybeCC = 0;
	uint16_t minNoNoEmpty = 0;
	uint16_t limitNoNo = 0;
	uint16_t centerNoNoDelta = 0;
	uint16_t minMaybeYes = 0;

	const UCPTrie *normTrie = nullptr;
	const uint16_t *maybeYesCompositions = nullptr;
	const uint16_t *extraData = nullptr;  // mappings and/or compositions for yesYes, yesNo & noNo characters
	const uint8_t *smallFCD = nullptr;  // [0x100] one bit per 32 BMP code points, set if any FCD!=0

	CanonIterData *fCanonIterData = nullptr;
};

class Normalizer2WithImpl /* : public Normalizer2 */ {
public:
	Normalizer2WithImpl(const uint8_t *d) : impl(d) { }
	~Normalizer2WithImpl() { }

	UBool getDecomposition(UChar32 c, UnicodeString &decomposition) const;
	UBool getRawDecomposition(UChar32 c, UnicodeString &decomposition) const;

	UChar32 composePair(UChar32 a, UChar32 b) const;

	uint8_t getCombiningClass(UChar32 c) const {
		return impl.getCC(impl.getNorm16(c));
	}

    Normalizer2Impl impl;
};

class ComposeNormalizer2 : public Normalizer2WithImpl {
public:
	static const ComposeNormalizer2 *getInstance();

	ComposeNormalizer2(const uint8_t *d, UBool fcc = false);
	~ComposeNormalizer2();

	UnicodeString normalize(const UnicodeString &src, UErrorCode &errorCode) const {
		UnicodeString result;
		normalize(src, result, errorCode);
		return result;
	}

	void normalize(const UChar *src, const UChar *limit, ReorderingBuffer &buffer, UErrorCode &errorCode) const;

	UnicodeString &normalize(const UnicodeString &src, UnicodeString &dest, UErrorCode &errorCode) const;

	void normalizeUTF8(uint32_t options, StringPiece src, ByteSink &sink, Edits *edits, UErrorCode &errorCode) const;

	UnicodeString& normalizeSecondAndAppend(UnicodeString &first, const UnicodeString &second,
			UBool doNormalize, UErrorCode &errorCode) const;

	UnicodeString& normalizeSecondAndAppend(UnicodeString &first, const UnicodeString &second, UErrorCode &errorCode) const;

	void normalizeAndAppend(const UChar *src, const UChar *limit, UBool doNormalize, UnicodeString &safeMiddle,
			ReorderingBuffer &buffer, UErrorCode &errorCode) const;

	UBool isNormalized(const UnicodeString &s, UErrorCode &errorCode) const;
	UBool isNormalizedUTF8(StringPiece sp, UErrorCode &errorCode) const;

	UNormalizationCheckResult quickCheck(const UnicodeString &s, UErrorCode &errorCode) const;

	UnicodeString &append(UnicodeString &first, const UnicodeString &second, UErrorCode &errorCode) const;

	const UChar* spanQuickCheckYes(const UChar *src, const UChar *limit, UErrorCode&) const;
	int32_t spanQuickCheckYes(const UnicodeString &s, UErrorCode &errorCode) const;

	UNormalizationCheckResult getQuickCheck(UChar32 c) const;
	UBool hasBoundaryBefore(UChar32 c) const;
	UBool hasBoundaryAfter(UChar32 c) const;
	UBool isInert(UChar32 c) const;

private:
    const UBool onlyContiguous;
};

}

#endif /* MODULES_IDN_UIDNANORMALIZER2_H_ */
