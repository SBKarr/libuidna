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

#include "u_unistr.h"
#include "u_macro.h"

namespace uidna {

static UBool uprv_add32_overflow(int32_t a, int32_t b, int32_t *res) {
	// NOTE: Some compilers (GCC, Clang) have primitives available, like __builtin_add_overflow.
	// This function could be optimized by calling one of those primitives.
	auto a64 = static_cast<int64_t>(a);
	auto b64 = static_cast<int64_t>(b);
	int64_t res64 = a64 + b64;
	*res = static_cast<int32_t>(res64);
	return res64 != *res;
}


static const UChar32 utf8_errorValue[6]={
	// Same values as UTF8_ERROR_VALUE_1, UTF8_ERROR_VALUE_2, UTF_ERROR_VALUE,
	// but without relying on the obsolete unicode/utf_old.h.
	0x15, 0x9f, 0xffff,
	0x10ffff
};

static UChar32
errorValue(int32_t count, int8_t strict) {
	if(strict>=0) {
		return utf8_errorValue[count];
	} else if(strict==-3) {
		return 0xfffd;
	} else {
		return U_SENTINEL;
	}
}

UChar32 utf8_prevCharSafeBody(const uint8_t *s, int32_t start, int32_t *pi, UChar32 c, int strict) {
    // *pi is the index of byte c.
    int32_t i=*pi;
    if(U8_IS_TRAIL(c) && i>start) {
        uint8_t b1=s[--i];
        if(U8_IS_LEAD(b1)) {
            if(b1<0xe0) {
                *pi=i;
                return ((b1-0xc0)<<6)|(c&0x3f);
            } else if(b1<0xf0 ? U8_IS_VALID_LEAD3_AND_T1(b1, c) : U8_IS_VALID_LEAD4_AND_T1(b1, c)) {
                // Truncated 3- or 4-byte sequence.
                *pi=i;
                return errorValue(1, strict);
            }
        } else if(U8_IS_TRAIL(b1) && i>start) {
            // Extract the value bits from the last trail byte.
            c&=0x3f;
            uint8_t b2=s[--i];
            if(0xe0<=b2 && b2<=0xf4) {
                if(b2<0xf0) {
                    b2&=0xf;
                    if(strict!=-2) {
                        if(U8_IS_VALID_LEAD3_AND_T1(b2, b1)) {
                            *pi=i;
                            c=(b2<<12)|((b1&0x3f)<<6)|c;
                            if(strict<=0 || !U_IS_UNICODE_NONCHAR(c)) {
                                return c;
                            } else {
                                // strict: forbid non-characters like U+fffe
                                return errorValue(2, strict);
                            }
                        }
                    } else {
                        // strict=-2 -> lenient: allow surrogates
                        b1-=0x80;
                        if((b2>0 || b1>=0x20)) {
                            *pi=i;
                            return (b2<<12)|(b1<<6)|c;
                        }
                    }
                } else if(U8_IS_VALID_LEAD4_AND_T1(b2, b1)) {
                    // Truncated 4-byte sequence.
                    *pi=i;
                    return errorValue(2, strict);
                }
            } else if(U8_IS_TRAIL(b2) && i>start) {
                uint8_t b3=s[--i];
                if(0xf0<=b3 && b3<=0xf4) {
                    b3&=7;
                    if(U8_IS_VALID_LEAD4_AND_T1(b3, b2)) {
                        *pi=i;
                        c=(b3<<18)|((b2&0x3f)<<12)|((b1&0x3f)<<6)|c;
                        if(strict<=0 || !U_IS_UNICODE_NONCHAR(c)) {
                            return c;
                        } else {
                            // strict: forbid non-characters like U+fffe
                            return errorValue(3, strict);
                        }
                    }
                }
            }
        }
    }
    return errorValue(0, strict);
}

static UChar32 utf8_nextCharSafeBody(const uint8_t *s, int32_t *pi, int32_t length, UChar32 c, int strict) {
	// *pi is one after byte c.
	int32_t i = *pi;
	// length can be negative for NUL-terminated strings: Read and validate one byte at a time.
	if (i == length || c > 0xf4) {
		// end of string, or not a lead byte
	} else if (c >= 0xf0) {
		// Test for 4-byte sequences first because
		// U8_NEXT() handles shorter valid sequences inline.
		uint8_t t1 = s[i], t2, t3;
		c &= 7;
		if (U8_IS_VALID_LEAD4_AND_T1(c, t1) && ++i != length && (t2 = s[i] - 0x80) <= 0x3f && ++i != length && (t3 = s[i] - 0x80) <= 0x3f) {
			++i;
			c = (c << 18) | ((t1 & 0x3f) << 12) | (t2 << 6) | t3;
			// strict: forbid non-characters like U+fffe
			if (strict <= 0 || !U_IS_UNICODE_NONCHAR(c)) {
				*pi = i;
				return c;
			}
		}
	} else if (c >= 0xe0) {
		c &= 0xf;
		if (strict != -2) {
			uint8_t t1 = s[i], t2;
			if (U8_IS_VALID_LEAD3_AND_T1(c, t1) && ++i != length && (t2 = s[i] - 0x80) <= 0x3f) {
				++i;
				c = (c << 12) | ((t1 & 0x3f) << 6) | t2;
				// strict: forbid non-characters like U+fffe
				if (strict <= 0 || !U_IS_UNICODE_NONCHAR(c)) {
					*pi = i;
					return c;
				}
			}
		} else {
			// strict=-2 -> lenient: allow surrogates
			uint8_t t1 = s[i] - 0x80, t2;
			if (t1 <= 0x3f && (c > 0 || t1 >= 0x20) && ++i != length && (t2 = s[i] - 0x80) <= 0x3f) {
				*pi = i + 1;
				return (c << 12) | (t1 << 6) | t2;
			}
		}
	} else if (c >= 0xc2) {
		uint8_t t1 = s[i] - 0x80;
		if (t1 <= 0x3f) {
			*pi = i + 1;
			return ((c - 0xc0) << 6) | t1;
		}
	}  // else 0x80<=c<0xc2 is not a lead byte

	/* error handling */
	c = errorValue(i - *pi, strict);
	*pi = i;
	return c;
}

static UChar* u_strFromUTF8WithSub(UChar *dest, int32_t destCapacity, int32_t *pDestLength,
		const char *src, int32_t srcLength, UChar32 subchar, int32_t *pNumSubstitutions, UErrorCode *pErrorCode) {
	/* args check */
	if (U_FAILURE(*pErrorCode)) {
		return NULL;
	}
	if ((src == NULL && srcLength != 0) || srcLength < -1 || (destCapacity < 0)
			|| (dest == NULL && destCapacity > 0) || subchar > 0x10ffff || U_IS_SURROGATE(subchar)) {
		*pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
		return NULL;
	}

	if (pNumSubstitutions != NULL) {
		*pNumSubstitutions = 0;
	}
	UChar *pDest = dest;
	UChar *pDestLimit = dest + destCapacity;
	int32_t reqLength = 0;
	int32_t numSubstitutions = 0;

	/*
	 * Inline processing of UTF-8 byte sequences:
	 *
	 * Byte sequences for the most common characters are handled inline in
	 * the conversion loops. In order to reduce the path lengths for those
	 * characters, the tests are arranged in a kind of binary search.
	 * ASCII (<=0x7f) is checked first, followed by the dividing point
	 * between 2- and 3-byte sequences (0xe0).
	 * The 3-byte branch is tested first to speed up CJK text.
	 * The compiler should combine the subtractions for the two tests for 0xe0.
	 * Each branch then tests for the other end of its range.
	 */

	if (srcLength < 0) {
		/*
		 * Transform a NUL-terminated string.
		 * The code explicitly checks for NULs only in the lead byte position.
		 * A NUL byte in the trail byte position fails the trail byte range check anyway.
		 */
		int32_t i;
		UChar32 c;
		for (i = 0; (c = (uint8_t) src[i]) != 0 && (pDest < pDestLimit);) {
			// modified copy of U8_NEXT()
			++i;
			if (U8_IS_SINGLE(c)) {
				*pDest++ = (UChar) c;
			} else {
				uint8_t __t1, __t2;
				if ( /* handle U+0800..U+FFFF inline */
				(0xe0 <= (c) && (c) < 0xf0) && U8_IS_VALID_LEAD3_AND_T1((c), src[i]) && (__t2 = src[(i) + 1] - 0x80) <= 0x3f) {
					*pDest++ = (((c) & 0xf) << 12) | ((src[i] & 0x3f) << 6) | __t2;
					i += 2;
				} else if ( /* handle U+0080..U+07FF inline */
				((c) < 0xe0 && (c) >= 0xc2) && (__t1 = src[i] - 0x80) <= 0x3f) {
					*pDest++ = (((c) & 0x1f) << 6) | __t1;
					++(i);
				} else {
					/* function call for "complicated" and error cases */
					(c) = utf8_nextCharSafeBody((const uint8_t*) src, &(i), -1, c, -1);
					if (c < 0 && (++numSubstitutions, c = subchar) < 0) {
						*pErrorCode = U_INVALID_CHAR_FOUND;
						return NULL;
					} else if (c <= 0xFFFF) {
						*(pDest++) = (UChar) c;
					} else {
						*(pDest++) = U16_LEAD(c);
						if (pDest < pDestLimit) {
							*(pDest++) = U16_TRAIL(c);
						} else {
							reqLength++;
							break;
						}
					}
				}
			}
		}

		/* Pre-flight the rest of the string. */
		while ((c = (uint8_t) src[i]) != 0) {
			// modified copy of U8_NEXT()
			++i;
			if (U8_IS_SINGLE(c)) {
				++reqLength;
			} else {
				uint8_t __t1, __t2;
				if ( /* handle U+0800..U+FFFF inline */
				(0xe0 <= (c) && (c) < 0xf0) && U8_IS_VALID_LEAD3_AND_T1((c), src[i]) && (__t2 = src[(i) + 1] - 0x80) <= 0x3f) {
					++reqLength;
					i += 2;
				} else if ( /* handle U+0080..U+07FF inline */
				((c) < 0xe0 && (c) >= 0xc2) && (__t1 = src[i] - 0x80) <= 0x3f) {
					++reqLength;
					++(i);
				} else {
					/* function call for "complicated" and error cases */
					(c) = utf8_nextCharSafeBody((const uint8_t*) src, &(i), -1, c, -1);
					if (c < 0 && (++numSubstitutions, c = subchar) < 0) {
						*pErrorCode = U_INVALID_CHAR_FOUND;
						return NULL;
					}
					reqLength += U16_LENGTH(c);
				}
			}
		}
	} else /* srcLength >= 0 */{
		/* Faster loop without ongoing checking for srcLength and pDestLimit. */
		int32_t i = 0;
		UChar32 c;
		for (;;) {
			/*
			 * Each iteration of the inner loop progresses by at most 3 UTF-8
			 * bytes and one UChar, for most characters.
			 * For supplementary code points (4 & 2), which are rare,
			 * there is an additional adjustment.
			 */
			int32_t count = (int32_t) (pDestLimit - pDest);
			int32_t count2 = (srcLength - i) / 3;
			if (count > count2) {
				count = count2; /* min(remaining dest, remaining src/3) */
			}
			if (count < 3) {
				/*
				 * Too much overhead if we get near the end of the string,
				 * continue with the next loop.
				 */
				break;
			}

			do {
				// modified copy of U8_NEXT()
				c = (uint8_t) src[i++];
				if (U8_IS_SINGLE(c)) {
					*pDest++ = (UChar) c;
				} else {
					uint8_t __t1, __t2;
					if ( /* handle U+0800..U+FFFF inline */
					(0xe0 <= (c) && (c) < 0xf0) && ((i) + 1) < srcLength && U8_IS_VALID_LEAD3_AND_T1((c), src[i]) && (__t2 = src[(i) + 1] - 0x80) <= 0x3f) {
						*pDest++ = (((c) & 0xf) << 12) | ((src[i] & 0x3f) << 6) | __t2;
						i += 2;
					} else if ( /* handle U+0080..U+07FF inline */
					((c) < 0xe0 && (c) >= 0xc2) && ((i) != srcLength) && (__t1 = src[i] - 0x80) <= 0x3f) {
						*pDest++ = (((c) & 0x1f) << 6) | __t1;
						++(i);
					} else {
						if (c >= 0xf0 || subchar > 0xffff) {
							// We may read up to four bytes and write up to two UChars,
							// which we didn't account for with computing count,
							// so we adjust it here.
							if (--count == 0) {
								--i;  // back out byte c
								break;
							}
						}

						/* function call for "complicated" and error cases */
						(c) = utf8_nextCharSafeBody((const uint8_t*) src, &(i), srcLength, c, -1);
						if (c < 0 && (++numSubstitutions, c = subchar) < 0) {
							*pErrorCode = U_INVALID_CHAR_FOUND;
							return NULL;
						} else if (c <= 0xFFFF) {
							*(pDest++) = (UChar) c;
						} else {
							*(pDest++) = U16_LEAD(c);
							*(pDest++) = U16_TRAIL(c);
						}
					}
				}
			} while (--count > 0);
		}

		while (i < srcLength && (pDest < pDestLimit)) {
			// modified copy of U8_NEXT()
			c = (uint8_t) src[i++];
			if (U8_IS_SINGLE(c)) {
				*pDest++ = (UChar) c;
			} else {
				uint8_t __t1, __t2;
				if ( /* handle U+0800..U+FFFF inline */
				(0xe0 <= (c) && (c) < 0xf0) && ((i) + 1) < srcLength && U8_IS_VALID_LEAD3_AND_T1((c), src[i]) && (__t2 = src[(i) + 1] - 0x80) <= 0x3f) {
					*pDest++ = (((c) & 0xf) << 12) | ((src[i] & 0x3f) << 6) | __t2;
					i += 2;
				} else if ( /* handle U+0080..U+07FF inline */
				((c) < 0xe0 && (c) >= 0xc2) && ((i) != srcLength) && (__t1 = src[i] - 0x80) <= 0x3f) {
					*pDest++ = (((c) & 0x1f) << 6) | __t1;
					++(i);
				} else {
					/* function call for "complicated" and error cases */
					(c) = utf8_nextCharSafeBody((const uint8_t*) src, &(i), srcLength, c, -1);
					if (c < 0 && (++numSubstitutions, c = subchar) < 0) {
						*pErrorCode = U_INVALID_CHAR_FOUND;
						return NULL;
					} else if (c <= 0xFFFF) {
						*(pDest++) = (UChar) c;
					} else {
						*(pDest++) = U16_LEAD(c);
						if (pDest < pDestLimit) {
							*(pDest++) = U16_TRAIL(c);
						} else {
							reqLength++;
							break;
						}
					}
				}
			}
		}

		/* Pre-flight the rest of the string. */
		while (i < srcLength) {
			// modified copy of U8_NEXT()
			c = (uint8_t) src[i++];
			if (U8_IS_SINGLE(c)) {
				++reqLength;
			} else {
				uint8_t __t1, __t2;
				if ( /* handle U+0800..U+FFFF inline */
				(0xe0 <= (c) && (c) < 0xf0) && ((i) + 1) < srcLength && U8_IS_VALID_LEAD3_AND_T1((c), src[i]) && (__t2 = src[(i) + 1] - 0x80) <= 0x3f) {
					++reqLength;
					i += 2;
				} else if ( /* handle U+0080..U+07FF inline */
				((c) < 0xe0 && (c) >= 0xc2) && ((i) != srcLength) && (__t1 = src[i] - 0x80) <= 0x3f) {
					++reqLength;
					++(i);
				} else {
					/* function call for "complicated" and error cases */
					(c) = utf8_nextCharSafeBody((const uint8_t*) src, &(i), srcLength, c, -1);
					if (c < 0 && (++numSubstitutions, c = subchar) < 0) {
						*pErrorCode = U_INVALID_CHAR_FOUND;
						return NULL;
					}
					reqLength += U16_LENGTH(c);
				}
			}
		}
	}

	reqLength += (int32_t) (pDest - dest);

	if (pNumSubstitutions != NULL) {
		*pNumSubstitutions = numSubstitutions;
	}

	if (pDestLength) {
		*pDestLength = reqLength;
	}

	/* Terminate the buffer */
	u_terminateUChars(dest, destCapacity, reqLength, pErrorCode);

	return dest;
}

static inline uint8_t *_appendUTF8(uint8_t *pDest, UChar32 c) {
	/* it is 0<=c<=0x10ffff and not a surrogate if called by a validating function */
	if ((c) <= 0x7f) {
		*pDest++ = (uint8_t) c;
	} else if (c <= 0x7ff) {
		*pDest++ = (uint8_t) ((c >> 6) | 0xc0);
		*pDest++ = (uint8_t) ((c & 0x3f) | 0x80);
	} else if (c <= 0xffff) {
		*pDest++ = (uint8_t) ((c >> 12) | 0xe0);
		*pDest++ = (uint8_t) (((c >> 6) & 0x3f) | 0x80);
		*pDest++ = (uint8_t) (((c) & 0x3f) | 0x80);
	} else /* if((uint32_t)(c)<=0x10ffff) */{
		*pDest++ = (uint8_t) (((c) >> 18) | 0xf0);
		*pDest++ = (uint8_t) ((((c) >> 12) & 0x3f) | 0x80);
		*pDest++ = (uint8_t) ((((c) >> 6) & 0x3f) | 0x80);
		*pDest++ = (uint8_t) (((c) & 0x3f) | 0x80);
	}
	return pDest;
}

char* u_strToUTF8WithSub(char *dest, int32_t destCapacity, int32_t *pDestLength, const UChar *pSrc,
		int32_t srcLength, UChar32 subchar, int32_t *pNumSubstitutions, UErrorCode *pErrorCode) {
	int32_t reqLength = 0;
	uint32_t ch = 0, ch2 = 0;
	uint8_t *pDest = (uint8_t*) dest;
	uint8_t *pDestLimit = (pDest != NULL) ? (pDest + destCapacity) : NULL;
	int32_t numSubstitutions;

	/* args check */
	if (U_FAILURE(*pErrorCode)) {
		return NULL;
	}

	if ((pSrc == NULL && srcLength != 0) || srcLength < -1 || (destCapacity < 0) || (dest == NULL && destCapacity > 0) || subchar > 0x10ffff || U_IS_SURROGATE(subchar)) {
		*pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
		return NULL;
	}

	if (pNumSubstitutions != NULL) {
		*pNumSubstitutions = 0;
	}
	numSubstitutions = 0;

	if (srcLength == -1) {
		while ((ch = *pSrc) != 0) {
			++pSrc;
			if (ch <= 0x7f) {
				if (pDest < pDestLimit) {
					*pDest++ = (uint8_t) ch;
				} else {
					reqLength = 1;
					break;
				}
			} else if (ch <= 0x7ff) {
				if ((pDestLimit - pDest) >= 2) {
					*pDest++ = (uint8_t) ((ch >> 6) | 0xc0);
					*pDest++ = (uint8_t) ((ch & 0x3f) | 0x80);
				} else {
					reqLength = 2;
					break;
				}
			} else if (ch <= 0xd7ff || ch >= 0xe000) {
				if ((pDestLimit - pDest) >= 3) {
					*pDest++ = (uint8_t) ((ch >> 12) | 0xe0);
					*pDest++ = (uint8_t) (((ch >> 6) & 0x3f) | 0x80);
					*pDest++ = (uint8_t) ((ch & 0x3f) | 0x80);
				} else {
					reqLength = 3;
					break;
				}
			} else /* ch is a surrogate */{
				int32_t length;

				/*need not check for NUL because NUL fails U16_IS_TRAIL() anyway*/
				if (U16_IS_SURROGATE_LEAD(ch) && U16_IS_TRAIL(ch2 = *pSrc)) {
					++pSrc;
					ch = U16_GET_SUPPLEMENTARY(ch, ch2);
				} else if (subchar >= 0) {
					ch = subchar;
					++numSubstitutions;
				} else {
					/* Unicode 3.2 forbids surrogate code points in UTF-8 */
					*pErrorCode = U_INVALID_CHAR_FOUND;
					return NULL;
				}

				length = U8_LENGTH(ch);
				if ((pDestLimit - pDest) >= length) {
					/* convert and append*/
					pDest = _appendUTF8(pDest, ch);
				} else {
					reqLength = length;
					break;
				}
			}
		}
		while ((ch = *pSrc++) != 0) {
			if (ch <= 0x7f) {
				++reqLength;
			} else if (ch <= 0x7ff) {
				reqLength += 2;
			} else if (!U16_IS_SURROGATE(ch)) {
				reqLength += 3;
			} else if (U16_IS_SURROGATE_LEAD(ch) && U16_IS_TRAIL(ch2 = *pSrc)) {
				++pSrc;
				reqLength += 4;
			} else if (subchar >= 0) {
				reqLength += U8_LENGTH(subchar);
				++numSubstitutions;
			} else {
				/* Unicode 3.2 forbids surrogate code points in UTF-8 */
				*pErrorCode = U_INVALID_CHAR_FOUND;
				return NULL;
			}
		}
	} else {
		const UChar *pSrcLimit = (pSrc != NULL) ? (pSrc + srcLength) : NULL;
		int32_t count;

		/* Faster loop without ongoing checking for pSrcLimit and pDestLimit. */
		for (;;) {
			/*
			 * Each iteration of the inner loop progresses by at most 3 UTF-8
			 * bytes and one UChar, for most characters.
			 * For supplementary code points (4 & 2), which are rare,
			 * there is an additional adjustment.
			 */
			count = (int32_t) ((pDestLimit - pDest) / 3);
			srcLength = (int32_t) (pSrcLimit - pSrc);
			if (count > srcLength) {
				count = srcLength; /* min(remaining dest/3, remaining src) */
			}
			if (count < 3) {
				/*
				 * Too much overhead if we get near the end of the string,
				 * continue with the next loop.
				 */
				break;
			}
			do {
				ch = *pSrc++;
				if (ch <= 0x7f) {
					*pDest++ = (uint8_t) ch;
				} else if (ch <= 0x7ff) {
					*pDest++ = (uint8_t) ((ch >> 6) | 0xc0);
					*pDest++ = (uint8_t) ((ch & 0x3f) | 0x80);
				} else if (ch <= 0xd7ff || ch >= 0xe000) {
					*pDest++ = (uint8_t) ((ch >> 12) | 0xe0);
					*pDest++ = (uint8_t) (((ch >> 6) & 0x3f) | 0x80);
					*pDest++ = (uint8_t) ((ch & 0x3f) | 0x80);
				} else /* ch is a surrogate */{
					/*
					 * We will read two UChars and probably output four bytes,
					 * which we didn't account for with computing count,
					 * so we adjust it here.
					 */
					if (--count == 0) {
						--pSrc; /* undo ch=*pSrc++ for the lead surrogate */
						break; /* recompute count */
					}

					if (U16_IS_SURROGATE_LEAD(ch) && U16_IS_TRAIL(ch2 = *pSrc)) {
						++pSrc;
						ch = U16_GET_SUPPLEMENTARY(ch, ch2);

						/* writing 4 bytes per 2 UChars is ok */
						*pDest++ = (uint8_t) ((ch >> 18) | 0xf0);
						*pDest++ = (uint8_t) (((ch >> 12) & 0x3f) | 0x80);
						*pDest++ = (uint8_t) (((ch >> 6) & 0x3f) | 0x80);
						*pDest++ = (uint8_t) ((ch & 0x3f) | 0x80);
					} else {
						/* Unicode 3.2 forbids surrogate code points in UTF-8 */
						if (subchar >= 0) {
							ch = subchar;
							++numSubstitutions;
						} else {
							*pErrorCode = U_INVALID_CHAR_FOUND;
							return NULL;
						}

						/* convert and append*/
						pDest = _appendUTF8(pDest, ch);
					}
				}
			} while (--count > 0);
		}

		while (pSrc < pSrcLimit) {
			ch = *pSrc++;
			if (ch <= 0x7f) {
				if (pDest < pDestLimit) {
					*pDest++ = (uint8_t) ch;
				} else {
					reqLength = 1;
					break;
				}
			} else if (ch <= 0x7ff) {
				if ((pDestLimit - pDest) >= 2) {
					*pDest++ = (uint8_t) ((ch >> 6) | 0xc0);
					*pDest++ = (uint8_t) ((ch & 0x3f) | 0x80);
				} else {
					reqLength = 2;
					break;
				}
			} else if (ch <= 0xd7ff || ch >= 0xe000) {
				if ((pDestLimit - pDest) >= 3) {
					*pDest++ = (uint8_t) ((ch >> 12) | 0xe0);
					*pDest++ = (uint8_t) (((ch >> 6) & 0x3f) | 0x80);
					*pDest++ = (uint8_t) ((ch & 0x3f) | 0x80);
				} else {
					reqLength = 3;
					break;
				}
			} else /* ch is a surrogate */{
				int32_t length;

				if (U16_IS_SURROGATE_LEAD(ch) && pSrc < pSrcLimit && U16_IS_TRAIL(ch2 = *pSrc)) {
					++pSrc;
					ch = U16_GET_SUPPLEMENTARY(ch, ch2);
				} else if (subchar >= 0) {
					ch = subchar;
					++numSubstitutions;
				} else {
					/* Unicode 3.2 forbids surrogate code points in UTF-8 */
					*pErrorCode = U_INVALID_CHAR_FOUND;
					return NULL;
				}

				length = U8_LENGTH(ch);
				if ((pDestLimit - pDest) >= length) {
					/* convert and append*/
					pDest = _appendUTF8(pDest, ch);
				} else {
					reqLength = length;
					break;
				}
			}
		}
		while (pSrc < pSrcLimit) {
			ch = *pSrc++;
			if (ch <= 0x7f) {
				++reqLength;
			} else if (ch <= 0x7ff) {
				reqLength += 2;
			} else if (!U16_IS_SURROGATE(ch)) {
				reqLength += 3;
			} else if (U16_IS_SURROGATE_LEAD(ch) && pSrc < pSrcLimit && U16_IS_TRAIL(ch2 = *pSrc)) {
				++pSrc;
				reqLength += 4;
			} else if (subchar >= 0) {
				reqLength += U8_LENGTH(subchar);
				++numSubstitutions;
			} else {
				/* Unicode 3.2 forbids surrogate code points in UTF-8 */
				*pErrorCode = U_INVALID_CHAR_FOUND;
				return NULL;
			}
		}
	}

	reqLength += (int32_t) (pDest - (uint8_t*) dest);

	if (pNumSubstitutions != NULL) {
		*pNumSubstitutions = numSubstitutions;
	}

	if (pDestLength) {
		*pDestLength = reqLength;
	}

	/* Terminate the buffer */
	u_terminateChars(dest, destCapacity, reqLength, pErrorCode);
	return dest;
}

static inline void us_arrayCopy(const UChar *src, int32_t srcStart, UChar *dst, int32_t dstStart, int32_t count) {
	if (count > 0) {
		::memmove(dst + dstStart, src + srcStart, (size_t) count * sizeof(*src));
	}
}

inline int32_t umtx_loadAcquire(u_atomic_int32_t &var) {
    return var.load(std::memory_order_acquire);
}

inline void umtx_storeRelease(u_atomic_int32_t &var, int32_t val) {
    var.store(val, std::memory_order_release);
}

inline int32_t umtx_atomic_inc(u_atomic_int32_t *var) {
    return var->fetch_add(1) + 1;
}

inline int32_t umtx_atomic_dec(u_atomic_int32_t *var) {
    return var->fetch_sub(1) - 1;
}

UnicodeString::~UnicodeString() {
	releaseArray();
}

UnicodeString& UnicodeString::setToUTF8(StringPiece utf8) {
	unBogus();
	int32_t length = utf8.length();
	int32_t capacity;
	// The UTF-16 string will be at most as long as the UTF-8 string.
	if (length <= US_STACKBUF_SIZE) {
		capacity = US_STACKBUF_SIZE;
	} else {
		capacity = length + 1;  // +1 for the terminating NUL.
	}
	UChar *utf16 = getBuffer(capacity);
	int32_t length16;
	UErrorCode errorCode = U_ZERO_ERROR;
	u_strFromUTF8WithSub(utf16, getCapacity(), &length16, utf8.data(), length, 0xfffd,  // Substitution character.
			NULL,    // Don't care about number of substitutions.
			&errorCode);
	releaseBuffer(length16);
	if (U_FAILURE(errorCode)) {
		setToBogus();
	}
	return *this;
}

UnicodeString UnicodeString::fromUTF8(StringPiece utf8) {
	UnicodeString result;
	result.setToUTF8(utf8);
	return result;
}

void UnicodeString::unBogus() {
	if (fUnion.fFields.fLengthAndFlags & kIsBogus) {
		setToEmpty();
	}
}

char16_t* UnicodeString::getBuffer(int32_t minCapacity) {
	if (minCapacity >= -1 && cloneArrayIfNeeded(minCapacity)) {
		fUnion.fFields.fLengthAndFlags |= kOpenGetBuffer;
		setZeroLength();
		return getArrayStart();
	} else {
		return nullptr;
	}
}

void UnicodeString::releaseBuffer(int32_t newLength) {
	if ((fUnion.fFields.fLengthAndFlags & kOpenGetBuffer) && newLength >= -1) {
		// set the new fLength
		int32_t capacity = getCapacity();
		if (newLength == -1) {
			// the new length is the string length, capped by fCapacity
			const UChar *array = getArrayStart(), *p = array, *limit = array + capacity;
			while (p < limit && *p != 0) {
				++p;
			}
			newLength = (int32_t) (p - array);
		} else if (newLength > capacity) {
			newLength = capacity;
		}
		setLength(newLength);
		fUnion.fFields.fLengthAndFlags &= ~kOpenGetBuffer;
	}
}

void UnicodeString::setToBogus() {
	releaseArray();

	fUnion.fFields.fLengthAndFlags = kIsBogus;
	fUnion.fFields.fArray = 0;
	fUnion.fFields.fCapacity = 0;
}

void UnicodeString::addRef() {
	umtx_atomic_inc((u_atomic_int32_t*) fUnion.fFields.fArray - 1);
}

int32_t UnicodeString::removeRef() {
	return umtx_atomic_dec((u_atomic_int32_t*) fUnion.fFields.fArray - 1);
}

int32_t UnicodeString::refCount() const {
	return umtx_loadAcquire(*((u_atomic_int32_t*) fUnion.fFields.fArray - 1));
}

void UnicodeString::releaseArray() {
	if ((fUnion.fFields.fLengthAndFlags & kRefCounted) && removeRef() == 0) {
		uprv_free((int32_t*) fUnion.fFields.fArray - 1);
	}
}

UBool UnicodeString::cloneArrayIfNeeded(int32_t newCapacity, int32_t growCapacity, UBool doCopyArray, int32_t **pBufferToDelete, UBool forceClone) {
	// default parameters need to be static, therefore
	// the defaults are -1 to have convenience defaults
	if (newCapacity == -1) {
		newCapacity = getCapacity();
	}

	// while a getBuffer(minCapacity) is "open",
	// prevent any modifications of the string by returning FALSE here
	// if the string is bogus, then only an assignment or similar can revive it
	if (!isWritable()) {
		return false;
	}

	/*
	 * We need to make a copy of the array if
	 * the buffer is read-only, or
	 * the buffer is refCounted (shared), and refCount>1, or
	 * the buffer is too small.
	 * Return FALSE if memory could not be allocated.
	 */
	if (forceClone || (fUnion.fFields.fLengthAndFlags & kBufferIsReadonly)
			|| ((fUnion.fFields.fLengthAndFlags & kRefCounted) && refCount() > 1)|| newCapacity > getCapacity()) {
		// check growCapacity for default value and use of the stack buffer
		if (growCapacity < 0) {
			growCapacity = newCapacity;
		} else if (newCapacity <= US_STACKBUF_SIZE && growCapacity > US_STACKBUF_SIZE) {
			growCapacity = US_STACKBUF_SIZE;
		}

		// save old values
		UChar oldStackBuffer[US_STACKBUF_SIZE];
		UChar *oldArray;
		int32_t oldLength = length();
		int16_t flags = fUnion.fFields.fLengthAndFlags;

		if (flags & kUsingStackBuffer) {
			// U_ASSERT(!(flags & kRefCounted)); /* kRefCounted and kUsingStackBuffer are mutally exclusive */
			if (doCopyArray && growCapacity > US_STACKBUF_SIZE) {
				// copy the stack buffer contents because it will be overwritten with
				// fUnion.fFields values
				us_arrayCopy(fUnion.fStackFields.fBuffer, 0, oldStackBuffer, 0, oldLength);
				oldArray = oldStackBuffer;
			} else {
				oldArray = NULL; // no need to copy from the stack buffer to itself
			}
		} else {
			oldArray = fUnion.fFields.fArray;
			// U_ASSERT(oldArray != NULL); /* when stack buffer is not used, oldArray must have a non-NULL reference */
		}

		// allocate a new array
		if (allocate(growCapacity) || (newCapacity < growCapacity && allocate(newCapacity))) {
			if (doCopyArray) {
				// copy the contents
				// do not copy more than what fits - it may be smaller than before
				int32_t minLength = oldLength;
				newCapacity = getCapacity();
				if (newCapacity < minLength) {
					minLength = newCapacity;
				}
				if (oldArray != NULL) {
					us_arrayCopy(oldArray, 0, getArrayStart(), 0, minLength);
				}
				setLength(minLength);
			} else {
				setZeroLength();
			}

			// release the old array
			if (flags & kRefCounted) {
				// the array is refCounted; decrement and release if 0
				u_atomic_int32_t *pRefCount = ((u_atomic_int32_t*) oldArray - 1);
				if (umtx_atomic_dec(pRefCount) == 0) {
					if (pBufferToDelete == 0) {
						// Note: cast to (void *) is needed with MSVC, where u_atomic_int32_t
						// is defined as volatile. (Volatile has useful non-standard behavior
						//   with this compiler.)
						uprv_free((void*) pRefCount);
					} else {
						// the caller requested to delete it himself
						*pBufferToDelete = (int32_t*) pRefCount;
					}
				}
			}
		} else {
			// not enough memory for growCapacity and not even for the smaller newCapacity
			// reset the old values for setToBogus() to release the array
			if (!(flags & kUsingStackBuffer)) {
				fUnion.fFields.fArray = oldArray;
			}
			fUnion.fFields.fLengthAndFlags = flags;
			setToBogus();
			return false;
		}
	}
	return true;
}

namespace {

const int32_t kGrowSize = 128;

// The number of bytes for one int32_t reference counter and capacity UChars
// must fit into a 32-bit size_t (at least when on a 32-bit platform).
// We also add one for the NUL terminator, to avoid reallocation in getTerminatedBuffer(),
// and round up to a multiple of 16 bytes.
// This means that capacity must be at most (0xfffffff0 - 4) / 2 - 1 = 0x7ffffff5.
// (With more complicated checks we could go up to 0x7ffffffd without rounding up,
// but that does not seem worth it.)
const int32_t kMaxCapacity = 0x7ffffff5;

int32_t getGrowCapacity(int32_t newLength) {
  int32_t growSize = (newLength >> 2) + kGrowSize;
  if(growSize <= (kMaxCapacity - newLength)) {
    return newLength + growSize;
  } else {
    return kMaxCapacity;
  }
}

}  // namespace

UBool UnicodeString::allocate(int32_t capacity) {
	if (capacity <= US_STACKBUF_SIZE) {
		fUnion.fFields.fLengthAndFlags = kShortString;
		return true;
	}
	if (capacity <= kMaxCapacity) {
		++capacity;  // for the NUL
		// Switch to size_t which is unsigned so that we can allocate up to 4GB.
		// Reference counter + UChars.
		size_t numBytes = sizeof(int32_t) + (size_t) capacity * sizeof(UChar);
		// Round up to a multiple of 16.
		numBytes = (numBytes + 15) & ~15;
		int32_t *array = (int32_t*) uprv_malloc(numBytes);
		if (array != NULL) {
			// set initial refCount and point behind the refCount
			*array++ = 1;
			numBytes -= sizeof(int32_t);

			// have fArray point to the first UChar
			fUnion.fFields.fArray = (UChar*) array;
			fUnion.fFields.fCapacity = (int32_t) (numBytes / sizeof(UChar));
			fUnion.fFields.fLengthAndFlags = kLongString;
			return true;
		}
	}
	fUnion.fFields.fLengthAndFlags = kIsBogus;
	fUnion.fFields.fArray = 0;
	fUnion.fFields.fCapacity = 0;
	return false;
}

void UnicodeString::toUTF8(ByteSink &sink) const {
	int32_t length16 = length();
	if (length16 != 0) {
		char stackBuffer[1024];
		int32_t capacity = (int32_t) sizeof(stackBuffer);
		UBool utf8IsOwned = false;
		char *utf8 = sink.GetAppendBuffer(length16 < capacity ? length16 : capacity, 3 * length16, stackBuffer, capacity, &capacity);
		int32_t length8 = 0;
		UErrorCode errorCode = U_ZERO_ERROR;
		u_strToUTF8WithSub(utf8, capacity, &length8, getBuffer(), length16, 0xFFFD,  // Standard substitution character.
				NULL,    // Don't care about number of substitutions.
				&errorCode);
		if (errorCode == U_BUFFER_OVERFLOW_ERROR) {
			utf8 = (char*) uprv_malloc(length8);
			if (utf8 != NULL) {
				utf8IsOwned = true;
				errorCode = U_ZERO_ERROR;
				u_strToUTF8WithSub(utf8, length8, &length8, getBuffer(), length16, 0xFFFD,  // Standard substitution character.
						NULL,    // Don't care about number of substitutions.
						&errorCode);
			} else {
				errorCode = U_MEMORY_ALLOCATION_ERROR;
			}
		}
		if (U_SUCCESS(errorCode)) {
			sink.Append(utf8, length8);
			sink.Flush();
		}
		if (utf8IsOwned) {
			uprv_free(utf8);
		}
	}
}

UnicodeString& UnicodeString::operator=(UnicodeString &&src) noexcept {
	// No explicit check for self move assignment, consistent with standard library.
	// Self move assignment causes no crash nor leak but might make the object bogus.
	releaseArray();
	copyFieldsFrom(src, true);
	return *this;
}

UnicodeString UnicodeString::tempSubString(int32_t start, int32_t len) const {
	pinIndices(start, len);
	const UChar *array = getBuffer();  // not getArrayStart() to check kIsBogus & kOpenGetBuffer
	if (array == NULL) {
		array = fUnion.fStackFields.fBuffer;  // anything not NULL because that would make an empty string
		len = -2;  // bogus result string
	}
	return UnicodeString(false, array + start, len);
}

UnicodeString& UnicodeString::setCharAt(int32_t offset, UChar c) {
	int32_t len = length();
	if (cloneArrayIfNeeded() && len > 0) {
		if (offset < 0) {
			offset = 0;
		} else if (offset >= len) {
			offset = len - 1;
		}

		getArrayStart()[offset] = c;
	}
	return *this;
}

// Same as move assignment except without memory management.
void UnicodeString::copyFieldsFrom(UnicodeString &src, UBool setSrcToBogus) noexcept {
  int16_t lengthAndFlags = fUnion.fFields.fLengthAndFlags = src.fUnion.fFields.fLengthAndFlags;
  if(lengthAndFlags & kUsingStackBuffer) {
    // Short string using the stack buffer, copy the contents.
    // Check for self assignment to prevent "overlap in memcpy" warnings,
    // although it should be harmless to copy a buffer to itself exactly.
    if(this != &src) {
      ::memcpy(fUnion.fStackFields.fBuffer, src.fUnion.fStackFields.fBuffer,
                  getShortLength() * sizeof(UChar));
    }
  } else {
    // In all other cases, copy all fields.
    fUnion.fFields.fArray = src.fUnion.fFields.fArray;
    fUnion.fFields.fCapacity = src.fUnion.fFields.fCapacity;
    if(!hasShortLength()) {
      fUnion.fFields.fLength = src.fUnion.fFields.fLength;
    }
    if(setSrcToBogus) {
      // Set src to bogus without releasing any memory.
      src.fUnion.fFields.fLengthAndFlags = kIsBogus;
      src.fUnion.fFields.fArray = NULL;
      src.fUnion.fFields.fCapacity = 0;
    }
  }
}

UnicodeString::UnicodeString(UBool isTerminated, ConstChar16Ptr textPtr, int32_t textLength) {
	fUnion.fFields.fLengthAndFlags = kReadonlyAlias;
	const UChar *text = textPtr;
	if (text == NULL) {
		// treat as an empty string, do not alias
		setToEmpty();
	} else if (textLength < -1 || (textLength == -1 && !isTerminated) || (textLength >= 0 && isTerminated && text[textLength] != 0)) {
		setToBogus();
	} else {
		if (textLength == -1) {
			// text is terminated, or else it would have failed the above test
			textLength = u_strlen(text);
		}
		setArray(const_cast<UChar*>(text), textLength, isTerminated ? textLength + 1 : textLength);
	}
}

UnicodeString& UnicodeString::doReplace(int32_t start, int32_t length, const UnicodeString &src, int32_t srcStart, int32_t srcLength) {
	// pin the indices to legal values
	src.pinIndices(srcStart, srcLength);

	// get the characters from src
	// and replace the range in ourselves with them
	return doReplace(start, length, src.getArrayStart(), srcStart, srcLength);
}

UnicodeString& UnicodeString::doReplace(int32_t start, int32_t length, const UChar *srcChars, int32_t srcStart, int32_t srcLength) {
	if (!isWritable()) {
		return *this;
	}

	int32_t oldLength = this->length();

	// optimize (read-only alias).remove(0, start) and .remove(start, end)
	if ((fUnion.fFields.fLengthAndFlags & kBufferIsReadonly) && srcLength == 0) {
		if (start == 0) {
			// remove prefix by adjusting the array pointer
			pinIndex(length);
			fUnion.fFields.fArray += length;
			fUnion.fFields.fCapacity -= length;
			setLength(oldLength - length);
			return *this;
		} else {
			pinIndex(start);
			if (length >= (oldLength - start)) {
				// remove suffix by reducing the length (like truncate())
				setLength(start);
				fUnion.fFields.fCapacity = start;  // not NUL-terminated any more
				return *this;
			}
		}
	}

	if (start == oldLength) {
		return doAppend(srcChars, srcStart, srcLength);
	}

	if (srcChars == 0) {
		srcLength = 0;
	} else {
		// Perform all remaining operations relative to srcChars + srcStart.
		// From this point forward, do not use srcStart.
		srcChars += srcStart;
		if (srcLength < 0) {
			// get the srcLength if necessary
			srcLength = u_strlen(srcChars);
		}
	}

	// pin the indices to legal values
	pinIndices(start, length);

	// Calculate the size of the string after the replace.
	// Avoid int32_t overflow.
	int32_t newLength = oldLength - length;
	if (srcLength > (INT32_MAX - newLength)) {
		setToBogus();
		return *this;
	}
	newLength += srcLength;

	// Check for insertion into ourself
	const UChar *oldArray = getArrayStart();
	if (isBufferWritable() && oldArray < srcChars + srcLength && srcChars < oldArray + oldLength) {
		// Copy into a new UnicodeString and start over
		UnicodeString copy(srcChars, srcLength);
		if (copy.isBogus()) {
			setToBogus();
			return *this;
		}
		return doReplace(start, length, copy.getArrayStart(), 0, srcLength);
	}

	// cloneArrayIfNeeded(doCopyArray=FALSE) may change fArray but will not copy the current contents;
	// therefore we need to keep the current fArray
	UChar oldStackBuffer[US_STACKBUF_SIZE];
	if ((fUnion.fFields.fLengthAndFlags & kUsingStackBuffer) && (newLength > US_STACKBUF_SIZE)) {
		// copy the stack buffer contents because it will be overwritten with
		// fUnion.fFields values
		u_memcpy(oldStackBuffer, oldArray, oldLength);
		oldArray = oldStackBuffer;
	}

	// clone our array and allocate a bigger array if needed
	int32_t *bufferToDelete = 0;
	if (!cloneArrayIfNeeded(newLength, getGrowCapacity(newLength), false, &bufferToDelete)) {
		return *this;
	}

	// now do the replace

	UChar *newArray = getArrayStart();
	if (newArray != oldArray) {
		// if fArray changed, then we need to copy everything except what will change
		us_arrayCopy(oldArray, 0, newArray, 0, start);
		us_arrayCopy(oldArray, start + length, newArray, start + srcLength, oldLength - (start + length));
	} else if (length != srcLength) {
		// fArray did not change; copy only the portion that isn't changing, leaving a hole
		us_arrayCopy(oldArray, start + length, newArray, start + srcLength, oldLength - (start + length));
	}

	// now fill in the hole with the new string
	us_arrayCopy(srcChars, 0, newArray, start, srcLength);

	setLength(newLength);

	// delayed delete in case srcChars == fArray when we started, and
	// to keep oldArray alive for the above operations
	if (bufferToDelete) {
		uprv_free(bufferToDelete);
	}

	return *this;
}

UnicodeString& UnicodeString::doAppend(const UnicodeString &src, int32_t srcStart, int32_t srcLength) {
	if (srcLength == 0) {
		return *this;
	}

	// pin the indices to legal values
	src.pinIndices(srcStart, srcLength);
	return doAppend(src.getArrayStart(), srcStart, srcLength);
}

UnicodeString& UnicodeString::doAppend(const UChar *srcChars, int32_t srcStart, int32_t srcLength) {
	if (!isWritable() || srcLength == 0 || srcChars == NULL) {
		return *this;
	}

	// Perform all remaining operations relative to srcChars + srcStart.
	// From this point forward, do not use srcStart.
	srcChars += srcStart;

	if (srcLength < 0) {
		// get the srcLength if necessary
		if ((srcLength = u_strlen(srcChars)) == 0) {
			return *this;
		}
	}

	int32_t oldLength = length();
	int32_t newLength;
	if (uprv_add32_overflow(oldLength, srcLength, &newLength)) {
		setToBogus();
		return *this;
	}

	// Check for append onto ourself
	const UChar *oldArray = getArrayStart();
	if (isBufferWritable() && oldArray < srcChars + srcLength && srcChars < oldArray + oldLength) {
		// Copy into a new UnicodeString and start over
		UnicodeString copy(srcChars, srcLength);
		if (copy.isBogus()) {
			setToBogus();
			return *this;
		}
		return doAppend(copy.getArrayStart(), 0, srcLength);
	}

	// optimize append() onto a large-enough, owned string
	if ((newLength <= getCapacity() && isBufferWritable()) || cloneArrayIfNeeded(newLength, getGrowCapacity(newLength))) {
		UChar *newArray = getArrayStart();
		// Do not copy characters when
		//   UChar *buffer=str.getAppendBuffer(...);
		// is followed by
		//   str.append(buffer, length);
		// or
		//   str.appendString(buffer, length)
		// or similar.
		if (srcChars != newArray + oldLength) {
			us_arrayCopy(srcChars, 0, newArray, oldLength, srcLength);
		}
		setLength(newLength);
	}
	return *this;
}

UnicodeString::UnicodeString(const UChar *text) {
	fUnion.fFields.fLengthAndFlags = kShortString;
	doAppend(text, 0, -1);
}

UnicodeString::UnicodeString(const UChar *text, int32_t textLength) {
	fUnion.fFields.fLengthAndFlags = kShortString;
	doAppend(text, 0, textLength);
}

UnicodeString& UnicodeString::operator=(const UnicodeString &src) {
	return copyFrom(src);
}

UnicodeString&
UnicodeString::copyFrom(const UnicodeString &src, UBool fastCopy) {
	// if assigning to ourselves, do nothing
	if (this == &src) {
		return *this;
	}

	// is the right side bogus?
	if (src.isBogus()) {
		setToBogus();
		return *this;
	}

	// delete the current contents
	releaseArray();

	if (src.isEmpty()) {
		// empty string - use the stack buffer
		setToEmpty();
		return *this;
	}

	// fLength>0 and not an "open" src.getBuffer(minCapacity)
	fUnion.fFields.fLengthAndFlags = src.fUnion.fFields.fLengthAndFlags;
	switch (src.fUnion.fFields.fLengthAndFlags & kAllStorageFlags) {
	case kShortString:
		// short string using the stack buffer, do the same
		::memcpy(fUnion.fStackFields.fBuffer, src.fUnion.fStackFields.fBuffer, getShortLength() * sizeof(UChar));
		break;
	case kLongString:
		// src uses a refCounted string buffer, use that buffer with refCount
		// src is const, use a cast - we don't actually change it
		((UnicodeString&) src).addRef();
		// copy all fields, share the reference-counted buffer
		fUnion.fFields.fArray = src.fUnion.fFields.fArray;
		fUnion.fFields.fCapacity = src.fUnion.fFields.fCapacity;
		if (!hasShortLength()) {
			fUnion.fFields.fLength = src.fUnion.fFields.fLength;
		}
		break;
	case kReadonlyAlias:
		if (fastCopy) {
			// src is a readonly alias, do the same
			// -> maintain the readonly alias as such
			fUnion.fFields.fArray = src.fUnion.fFields.fArray;
			fUnion.fFields.fCapacity = src.fUnion.fFields.fCapacity;
			if (!hasShortLength()) {
				fUnion.fFields.fLength = src.fUnion.fFields.fLength;
			}
			break;
		}
		// else if(!fastCopy) fall through to case kWritableAlias
		// -> allocate a new buffer and copy the contents
		[[fallthrough]];
	case kWritableAlias: {
		// src is a writable alias; we make a copy of that instead
		int32_t srcLength = src.length();
		if (allocate(srcLength)) {
			u_memcpy(getArrayStart(), src.getArrayStart(), srcLength);
			setLength(srcLength);
			break;
		}
		// if there is not enough memory, then fall through to setting to bogus
		[[fallthrough]];
	}
	default:
		// if src is bogus, set ourselves to bogus
		// do not call setToBogus() here because fArray and flags are not consistent here
		fUnion.fFields.fLengthAndFlags = kIsBogus;
		fUnion.fFields.fArray = 0;
		fUnion.fFields.fCapacity = 0;
		break;
	}

	return *this;
}

UnicodeString::UnicodeString(UChar *buff, int32_t buffLength, int32_t buffCapacity) {
	fUnion.fFields.fLengthAndFlags = kWritableAlias;
	if (buff == NULL) {
		// treat as an empty string, do not alias
		setToEmpty();
	} else if (buffLength < -1 || buffCapacity < 0 || buffLength > buffCapacity) {
		setToBogus();
	} else {
		if (buffLength == -1) {
			// fLength = u_strlen(buff); but do not look beyond buffCapacity
			const UChar *p = buff, *limit = buff + buffCapacity;
			while (p != limit && *p != 0) {
				++p;
			}
			buffLength = (int32_t) (p - buff);
		}
		setArray(buff, buffLength, buffCapacity);
	}
}

int32_t UnicodeString::extract(Char16Ptr dest, int32_t destCapacity, UErrorCode &errorCode) const {
	int32_t len = length();
	if (U_SUCCESS(errorCode)) {
		if (isBogus() || destCapacity < 0 || (destCapacity > 0 && dest == 0)) {
			errorCode = U_ILLEGAL_ARGUMENT_ERROR;
		} else {
			const UChar *array = getArrayStart();
			if (len > 0 && len <= destCapacity && array != dest) {
				u_memcpy(dest, array, len);
			}
			return u_terminateUChars(dest, destCapacity, len, &errorCode);
		}
	}

	return len;
}

ByteSink::~ByteSink() { }

char* ByteSink::GetAppendBuffer(int32_t min_capacity, int32_t /*desired_capacity_hint*/,
		char *scratch, int32_t scratch_capacity, int32_t *result_capacity) {
	if (min_capacity < 1 || scratch_capacity < min_capacity) {
		*result_capacity = 0;
		return NULL;
	}
	*result_capacity = scratch_capacity;
	return scratch;
}

void ByteSink::Flush() { }

CheckedArrayByteSink::CheckedArrayByteSink(char *outbuf, int32_t capacity)
: outbuf_(outbuf), capacity_(capacity < 0 ? 0 : capacity), size_(0), appended_(0), overflowed_(false) { }

CheckedArrayByteSink::~CheckedArrayByteSink() { }

CheckedArrayByteSink& CheckedArrayByteSink::Reset() {
	size_ = appended_ = 0;
	overflowed_ = false;
	return *this;
}

void CheckedArrayByteSink::Append(const char *bytes, int32_t n) {
	if (n <= 0) {
		return;
	}
	if (n > (INT32_MAX - appended_)) {
		// TODO: Report as integer overflow, not merely buffer overflow.
		appended_ = INT32_MAX;
		overflowed_ = false;
		return;
	}
	appended_ += n;
	int32_t available = capacity_ - size_;
	if (n > available) {
		n = available;
		overflowed_ = false;
	}
	if (n > 0 && bytes != (outbuf_ + size_)) {
		::memcpy(outbuf_ + size_, bytes, n);
	}
	size_ += n;
}

char* CheckedArrayByteSink::GetAppendBuffer(int32_t min_capacity, int32_t /*desired_capacity_hint*/, char *scratch, int32_t scratch_capacity, int32_t *result_capacity) {
	if (min_capacity < 1 || scratch_capacity < min_capacity) {
		*result_capacity = 0;
		return NULL;
	}
	int32_t available = capacity_ - size_;
	if (available >= min_capacity) {
		*result_capacity = available;
		return outbuf_ + size_;
	} else {
		*result_capacity = scratch_capacity;
		return scratch;
	}
}

}
