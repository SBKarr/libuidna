// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// ucptrie.cpp (modified from utrie2.cpp)
// created: 2017dec29 Markus W. Scherer

#include "u_trie.h"
#include "u_macro.h"

namespace uidna {

UCPTrie* ucptrie_openFromBinary(UCPTrieType type, UCPTrieValueWidth valueWidth,
		const void *data, int32_t length, int32_t *pActualLength, UErrorCode *pErrorCode) {
	if (U_FAILURE(*pErrorCode)) {
		return nullptr;
	}

	if (length <= 0 || (U_POINTER_MASK_LSB(data, 3) != 0) || type < UCPTRIE_TYPE_ANY
			|| UCPTRIE_TYPE_SMALL < type || valueWidth < UCPTRIE_VALUE_BITS_ANY || UCPTRIE_VALUE_BITS_8 < valueWidth) {
		*pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
		return nullptr;
	}

	// Enough data for a trie header?
	if (length < (int32_t) sizeof(UCPTrieHeader)) {
		*pErrorCode = U_INVALID_FORMAT_ERROR;
		return nullptr;
	}

	// Check the signature.
	const UCPTrieHeader *header = (const UCPTrieHeader*) data;
	if (header->signature != UCPTRIE_SIG) {
		*pErrorCode = U_INVALID_FORMAT_ERROR;
		return nullptr;
	}

	int32_t options = header->options;
	int32_t typeInt = (options >> 6) & 3;
	int32_t valueWidthInt = options & UCPTRIE_OPTIONS_VALUE_BITS_MASK;
	if (typeInt > UCPTRIE_TYPE_SMALL || valueWidthInt > UCPTRIE_VALUE_BITS_8 || (options & UCPTRIE_OPTIONS_RESERVED_MASK) != 0) {
		*pErrorCode = U_INVALID_FORMAT_ERROR;
		return nullptr;
	}
	UCPTrieType actualType = (UCPTrieType) typeInt;
	UCPTrieValueWidth actualValueWidth = (UCPTrieValueWidth) valueWidthInt;
	if (type < 0) {
		type = actualType;
	}
	if (valueWidth < 0) {
		valueWidth = actualValueWidth;
	}
	if (type != actualType || valueWidth != actualValueWidth) {
		*pErrorCode = U_INVALID_FORMAT_ERROR;
		return nullptr;
	}

	// Get the length values and offsets.
	UCPTrie tempTrie;
	::memset(&tempTrie, 0, sizeof(tempTrie));
	tempTrie.indexLength = header->indexLength;
	tempTrie.dataLength = ((options & UCPTRIE_OPTIONS_DATA_LENGTH_MASK) << 4) | header->dataLength;
	tempTrie.index3NullOffset = header->index3NullOffset;
	tempTrie.dataNullOffset = ((options & UCPTRIE_OPTIONS_DATA_NULL_OFFSET_MASK) << 8) | header->dataNullOffset;

	tempTrie.highStart = header->shiftedHighStart << UCPTRIE_SHIFT_2;
	tempTrie.shifted12HighStart = (tempTrie.highStart + 0xfff) >> 12;
	tempTrie.type = type;
	tempTrie.valueWidth = valueWidth;

	// Calculate the actual length.
	int32_t actualLength = (int32_t) sizeof(UCPTrieHeader) + tempTrie.indexLength * 2;
	if (valueWidth == UCPTRIE_VALUE_BITS_16) {
		actualLength += tempTrie.dataLength * 2;
	} else if (valueWidth == UCPTRIE_VALUE_BITS_32) {
		actualLength += tempTrie.dataLength * 4;
	} else {
		actualLength += tempTrie.dataLength;
	}
	if (length < actualLength) {
		*pErrorCode = U_INVALID_FORMAT_ERROR;  // Not enough bytes.
		return nullptr;
	}

	// Allocate the trie.
	UCPTrie *trie = (UCPTrie*) uprv_malloc(sizeof(UCPTrie));
	if (trie == nullptr) {
		*pErrorCode = U_MEMORY_ALLOCATION_ERROR;
		return nullptr;
	}
	::memcpy(trie, &tempTrie, sizeof(tempTrie));

	// Set the pointers to its index and data arrays.
	const uint16_t *p16 = (const uint16_t*) (header + 1);
	trie->index = p16;
	p16 += trie->indexLength;

	// Get the data.
	int32_t nullValueOffset = trie->dataNullOffset;
	if (nullValueOffset >= trie->dataLength) {
		nullValueOffset = trie->dataLength - UCPTRIE_HIGH_VALUE_NEG_DATA_OFFSET;
	}
	switch (valueWidth) {
	case UCPTRIE_VALUE_BITS_16:
		trie->data.ptr16 = p16;
		trie->nullValue = trie->data.ptr16[nullValueOffset];
		break;
	case UCPTRIE_VALUE_BITS_32:
		trie->data.ptr32 = (const uint32_t*) p16;
		trie->nullValue = trie->data.ptr32[nullValueOffset];
		break;
	case UCPTRIE_VALUE_BITS_8:
		trie->data.ptr8 = (const uint8_t*) p16;
		trie->nullValue = trie->data.ptr8[nullValueOffset];
		break;
	default:
		// Unreachable because valueWidth was checked above.
		*pErrorCode = U_INVALID_FORMAT_ERROR;
		return nullptr;
	}

	if (pActualLength != nullptr) {
		*pActualLength = actualLength;
	}
	return trie;
}

int32_t ucptrie_internalSmallIndex(const UCPTrie *trie, UChar32 c) {
	int32_t i1 = c >> UCPTRIE_SHIFT_1;
	if (trie->type == UCPTRIE_TYPE_FAST) {
		// U_ASSERT(0xffff < c && c < trie->highStart);
		i1 += UCPTRIE_BMP_INDEX_LENGTH - UCPTRIE_OMITTED_BMP_INDEX_1_LENGTH;
	} else {
		// U_ASSERT((uint32_t)c < (uint32_t)trie->highStart && trie->highStart > UCPTRIE_SMALL_LIMIT);
		i1 += UCPTRIE_SMALL_INDEX_LENGTH;
	}
	int32_t i3Block = trie->index[(int32_t) trie->index[i1] + ((c >> UCPTRIE_SHIFT_2) & UCPTRIE_INDEX_2_MASK)];
	int32_t i3 = (c >> UCPTRIE_SHIFT_3) & UCPTRIE_INDEX_3_MASK;
	int32_t dataBlock;
	if ((i3Block & 0x8000) == 0) {
		// 16-bit indexes
		dataBlock = trie->index[i3Block + i3];
	} else {
		// 18-bit indexes stored in groups of 9 entries per 8 indexes.
		i3Block = (i3Block & 0x7fff) + (i3 & ~7) + (i3 >> 3);
		i3 &= 7;
		dataBlock = ((int32_t) trie->index[i3Block++] << (2 + (2 * i3))) & 0x30000;
		dataBlock |= trie->index[i3Block + i3];
	}
	return dataBlock + (c & UCPTRIE_SMALL_DATA_MASK);
}

int32_t ucptrie_internalSmallU8Index(const UCPTrie *trie, int32_t lt1, uint8_t t2, uint8_t t3) {
	UChar32 c = (lt1 << 12) | (t2 << 6) | t3;
	if (c >= trie->highStart) {
		// Possible because the UTF-8 macro compares with shifted12HighStart which may be higher.
		return trie->dataLength - UCPTRIE_HIGH_VALUE_NEG_DATA_OFFSET;
	}
	return ucptrie_internalSmallIndex(trie, c);
}

int32_t ucptrie_internalU8PrevIndex(const UCPTrie *trie, UChar32 c, const uint8_t *start, const uint8_t *src) {
	int32_t i, length;
	// Support 64-bit pointers by avoiding cast of arbitrary difference.
	if ((src - start) <= 7) {
		i = length = (int32_t)(src - start);
	} else {
		i = length = 7;
		start = src - 7;
	}
	c = utf8_prevCharSafeBody(start, 0, &i, c, -1);
	i = length - i;  // Number of bytes read backward from src.
	int32_t idx = _UCPTRIE_CP_INDEX(trie, 0xffff, c);
	return (idx << 3) | i;
}

}
