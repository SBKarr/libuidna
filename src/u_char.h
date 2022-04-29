// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
**********************************************************************
*   Copyright (C) 1997-2016, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* File UCHAR.H
*
* Modification History:
*
*   Date        Name        Description
*   04/02/97    aliu        Creation.
*   03/29/99    helena      Updated for C APIs.
*   4/15/99     Madhu       Updated for C Implementation and Javadoc
*   5/20/99     Madhu       Added the function u_getVersion()
*   8/19/1999   srl         Upgraded scripts to Unicode 3.0
*   8/27/1999   schererm    UCharDirection constants: U_...
*   11/11/1999  weiv        added u_isalnum(), cleaned comments
*   01/11/2000  helena      Renamed u_getVersion to u_getUnicodeVersion().
******************************************************************************
*/

#ifndef MODULES_IDN_UINDACHAR_H_
#define MODULES_IDN_UINDACHAR_H_

#include "u_types.h"

namespace uidna {

/**
 * Data for enumerated Unicode general category types.
 * See http://www.unicode.org/Public/UNIDATA/UnicodeData.html .
 * @stable ICU 2.0
 */
typedef enum UCharCategory {
	/** Non-category for unassigned and non-character code points. @stable ICU 2.0 */
	U_UNASSIGNED = 0,
	/** Cn "Other, Not Assigned (no characters in [UnicodeData.txt] have this property)" (same as U_UNASSIGNED!) @stable ICU 2.0 */
	U_GENERAL_OTHER_TYPES = 0,
	/** Lu @stable ICU 2.0 */
	U_UPPERCASE_LETTER = 1,
	/** Ll @stable ICU 2.0 */
	U_LOWERCASE_LETTER = 2,
	/** Lt @stable ICU 2.0 */
	U_TITLECASE_LETTER = 3,
	/** Lm @stable ICU 2.0 */
	U_MODIFIER_LETTER = 4,
	/** Lo @stable ICU 2.0 */
	U_OTHER_LETTER = 5,
	/** Mn @stable ICU 2.0 */
	U_NON_SPACING_MARK = 6,
	/** Me @stable ICU 2.0 */
	U_ENCLOSING_MARK = 7,
	/** Mc @stable ICU 2.0 */
	U_COMBINING_SPACING_MARK = 8,
	/** Nd @stable ICU 2.0 */
	U_DECIMAL_DIGIT_NUMBER = 9,
	/** Nl @stable ICU 2.0 */
	U_LETTER_NUMBER = 10,
	/** No @stable ICU 2.0 */
	U_OTHER_NUMBER = 11,
	/** Zs @stable ICU 2.0 */
	U_SPACE_SEPARATOR = 12,
	/** Zl @stable ICU 2.0 */
	U_LINE_SEPARATOR = 13,
	/** Zp @stable ICU 2.0 */
	U_PARAGRAPH_SEPARATOR = 14,
	/** Cc @stable ICU 2.0 */
	U_CONTROL_CHAR = 15,
	/** Cf @stable ICU 2.0 */
	U_FORMAT_CHAR = 16,
	/** Co @stable ICU 2.0 */
	U_PRIVATE_USE_CHAR = 17,
	/** Cs @stable ICU 2.0 */
	U_SURROGATE = 18,
	/** Pd @stable ICU 2.0 */
	U_DASH_PUNCTUATION = 19,
	/** Ps @stable ICU 2.0 */
	U_START_PUNCTUATION = 20,
	/** Pe @stable ICU 2.0 */
	U_END_PUNCTUATION = 21,
	/** Pc @stable ICU 2.0 */
	U_CONNECTOR_PUNCTUATION = 22,
	/** Po @stable ICU 2.0 */
	U_OTHER_PUNCTUATION = 23,
	/** Sm @stable ICU 2.0 */
	U_MATH_SYMBOL = 24,
	/** Sc @stable ICU 2.0 */
	U_CURRENCY_SYMBOL = 25,
	/** Sk @stable ICU 2.0 */
	U_MODIFIER_SYMBOL = 26,
	/** So @stable ICU 2.0 */
	U_OTHER_SYMBOL = 27,
	/** Pi @stable ICU 2.0 */
	U_INITIAL_PUNCTUATION = 28,
	/** Pf @stable ICU 2.0 */
	U_FINAL_PUNCTUATION = 29,
	/**
	 * One higher than the last enum UCharCategory constant.
	 * This numeric value is stable (will not change), see
	 * http://www.unicode.org/policies/stability_policy.html#Property_Value
	 *
	 * @stable ICU 2.0
	 */
	U_CHAR_CATEGORY_COUNT
} UCharCategory;

/**
 * U_GC_XX_MASK constants are bit flags corresponding to Unicode
 * general category values.
 * For each category, the nth bit is set if the numeric value of the
 * corresponding UCharCategory constant is n.
 *
 * There are also some U_GC_Y_MASK constants for groups of general categories
 * like L for all letter categories.
 *
 * @see u_charType
 * @see U_GET_GC_MASK
 * @see UCharCategory
 * @stable ICU 2.1
 */
#define U_GC_CN_MASK    U_MASK(U_GENERAL_OTHER_TYPES)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_LU_MASK    U_MASK(U_UPPERCASE_LETTER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_LL_MASK    U_MASK(U_LOWERCASE_LETTER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_LT_MASK    U_MASK(U_TITLECASE_LETTER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_LM_MASK    U_MASK(U_MODIFIER_LETTER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_LO_MASK    U_MASK(U_OTHER_LETTER)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_MN_MASK    U_MASK(U_NON_SPACING_MARK)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_ME_MASK    U_MASK(U_ENCLOSING_MARK)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_MC_MASK    U_MASK(U_COMBINING_SPACING_MARK)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_ND_MASK    U_MASK(U_DECIMAL_DIGIT_NUMBER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_NL_MASK    U_MASK(U_LETTER_NUMBER)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_NO_MASK    U_MASK(U_OTHER_NUMBER)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_ZS_MASK    U_MASK(U_SPACE_SEPARATOR)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_ZL_MASK    U_MASK(U_LINE_SEPARATOR)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_ZP_MASK    U_MASK(U_PARAGRAPH_SEPARATOR)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_CC_MASK    U_MASK(U_CONTROL_CHAR)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_CF_MASK    U_MASK(U_FORMAT_CHAR)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_CO_MASK    U_MASK(U_PRIVATE_USE_CHAR)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_CS_MASK    U_MASK(U_SURROGATE)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PD_MASK    U_MASK(U_DASH_PUNCTUATION)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PS_MASK    U_MASK(U_START_PUNCTUATION)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PE_MASK    U_MASK(U_END_PUNCTUATION)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PC_MASK    U_MASK(U_CONNECTOR_PUNCTUATION)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PO_MASK    U_MASK(U_OTHER_PUNCTUATION)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_SM_MASK    U_MASK(U_MATH_SYMBOL)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_SC_MASK    U_MASK(U_CURRENCY_SYMBOL)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_SK_MASK    U_MASK(U_MODIFIER_SYMBOL)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_SO_MASK    U_MASK(U_OTHER_SYMBOL)

/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PI_MASK    U_MASK(U_INITIAL_PUNCTUATION)
/** Mask constant for a UCharCategory. @stable ICU 2.1 */
#define U_GC_PF_MASK    U_MASK(U_FINAL_PUNCTUATION)

/** Mask constant for multiple UCharCategory bits (L Letters). @stable ICU 2.1 */
#define U_GC_L_MASK \
			(U_GC_LU_MASK|U_GC_LL_MASK|U_GC_LT_MASK|U_GC_LM_MASK|U_GC_LO_MASK)

/** Mask constant for multiple UCharCategory bits (LC Cased Letters). @stable ICU 2.1 */
#define U_GC_LC_MASK \
			(U_GC_LU_MASK|U_GC_LL_MASK|U_GC_LT_MASK)

/** Mask constant for multiple UCharCategory bits (M Marks). @stable ICU 2.1 */
#define U_GC_M_MASK (U_GC_MN_MASK|U_GC_ME_MASK|U_GC_MC_MASK)

/** Mask constant for multiple UCharCategory bits (N Numbers). @stable ICU 2.1 */
#define U_GC_N_MASK (U_GC_ND_MASK|U_GC_NL_MASK|U_GC_NO_MASK)

/** Mask constant for multiple UCharCategory bits (Z Separators). @stable ICU 2.1 */
#define U_GC_Z_MASK (U_GC_ZS_MASK|U_GC_ZL_MASK|U_GC_ZP_MASK)

/** Mask constant for multiple UCharCategory bits (C Others). @stable ICU 2.1 */
#define U_GC_C_MASK \
			(U_GC_CN_MASK|U_GC_CC_MASK|U_GC_CF_MASK|U_GC_CO_MASK|U_GC_CS_MASK)

/** Mask constant for multiple UCharCategory bits (P Punctuation). @stable ICU 2.1 */
#define U_GC_P_MASK \
			(U_GC_PD_MASK|U_GC_PS_MASK|U_GC_PE_MASK|U_GC_PC_MASK|U_GC_PO_MASK| \
			 U_GC_PI_MASK|U_GC_PF_MASK)

/** Mask constant for multiple UCharCategory bits (S Symbols). @stable ICU 2.1 */
#define U_GC_S_MASK (U_GC_SM_MASK|U_GC_SC_MASK|U_GC_SK_MASK|U_GC_SO_MASK)

typedef enum UCharDirection {
    /** L @stable ICU 2.0 */
    U_LEFT_TO_RIGHT               = 0,
    /** R @stable ICU 2.0 */
    U_RIGHT_TO_LEFT               = 1,
    /** EN @stable ICU 2.0 */
    U_EUROPEAN_NUMBER             = 2,
    /** ES @stable ICU 2.0 */
    U_EUROPEAN_NUMBER_SEPARATOR   = 3,
    /** ET @stable ICU 2.0 */
    U_EUROPEAN_NUMBER_TERMINATOR  = 4,
    /** AN @stable ICU 2.0 */
    U_ARABIC_NUMBER               = 5,
    /** CS @stable ICU 2.0 */
    U_COMMON_NUMBER_SEPARATOR     = 6,
    /** B @stable ICU 2.0 */
    U_BLOCK_SEPARATOR             = 7,
    /** S @stable ICU 2.0 */
    U_SEGMENT_SEPARATOR           = 8,
    /** WS @stable ICU 2.0 */
    U_WHITE_SPACE_NEUTRAL         = 9,
    /** ON @stable ICU 2.0 */
    U_OTHER_NEUTRAL               = 10,
    /** LRE @stable ICU 2.0 */
    U_LEFT_TO_RIGHT_EMBEDDING     = 11,
    /** LRO @stable ICU 2.0 */
    U_LEFT_TO_RIGHT_OVERRIDE      = 12,
    /** AL @stable ICU 2.0 */
    U_RIGHT_TO_LEFT_ARABIC        = 13,
    /** RLE @stable ICU 2.0 */
    U_RIGHT_TO_LEFT_EMBEDDING     = 14,
    /** RLO @stable ICU 2.0 */
    U_RIGHT_TO_LEFT_OVERRIDE      = 15,
    /** PDF @stable ICU 2.0 */
    U_POP_DIRECTIONAL_FORMAT      = 16,
    /** NSM @stable ICU 2.0 */
    U_DIR_NON_SPACING_MARK        = 17,
    /** BN @stable ICU 2.0 */
    U_BOUNDARY_NEUTRAL            = 18,
    /** FSI @stable ICU 52 */
    U_FIRST_STRONG_ISOLATE        = 19,
    /** LRI @stable ICU 52 */
    U_LEFT_TO_RIGHT_ISOLATE       = 20,
    /** RLI @stable ICU 52 */
    U_RIGHT_TO_LEFT_ISOLATE       = 21,
    /** PDI @stable ICU 52 */
    U_POP_DIRECTIONAL_ISOLATE     = 22,
} UCharDirection;

typedef enum UJoiningType {
    U_JT_NON_JOINING,       /*[U]*/
    U_JT_JOIN_CAUSING,      /*[C]*/
    U_JT_DUAL_JOINING,      /*[D]*/
    U_JT_LEFT_JOINING,      /*[L]*/
    U_JT_RIGHT_JOINING,     /*[R]*/
    U_JT_TRANSPARENT,       /*[T]*/
} UJoiningType;


typedef enum UScriptCode {
    /*
     * Note: UScriptCode constants and their ISO script code comments
     * are parsed by preparseucd.py.
     * It matches lines like
     *     USCRIPT_<Unicode Script value name> = <integer>,  / * <ISO script code> * /
     */

      /** @stable ICU 2.2 */
      USCRIPT_INVALID_CODE = -1,
      /** @stable ICU 2.2 */
      USCRIPT_COMMON       =  0,  /* Zyyy */
      /** @stable ICU 2.2 */
      USCRIPT_INHERITED    =  1,  /* Zinh */ /* "Code for inherited script", for non-spacing combining marks; also Qaai */
      /** @stable ICU 2.2 */
      USCRIPT_ARABIC       =  2,  /* Arab */
      /** @stable ICU 2.2 */
      USCRIPT_ARMENIAN     =  3,  /* Armn */
      /** @stable ICU 2.2 */
      USCRIPT_BENGALI      =  4,  /* Beng */
      /** @stable ICU 2.2 */
      USCRIPT_BOPOMOFO     =  5,  /* Bopo */
      /** @stable ICU 2.2 */
      USCRIPT_CHEROKEE     =  6,  /* Cher */
      /** @stable ICU 2.2 */
      USCRIPT_COPTIC       =  7,  /* Copt */
      /** @stable ICU 2.2 */
      USCRIPT_CYRILLIC     =  8,  /* Cyrl */
      /** @stable ICU 2.2 */
      USCRIPT_DESERET      =  9,  /* Dsrt */
      /** @stable ICU 2.2 */
      USCRIPT_DEVANAGARI   = 10,  /* Deva */
      /** @stable ICU 2.2 */
      USCRIPT_ETHIOPIC     = 11,  /* Ethi */
      /** @stable ICU 2.2 */
      USCRIPT_GEORGIAN     = 12,  /* Geor */
      /** @stable ICU 2.2 */
      USCRIPT_GOTHIC       = 13,  /* Goth */
      /** @stable ICU 2.2 */
      USCRIPT_GREEK        = 14,  /* Grek */
      /** @stable ICU 2.2 */
      USCRIPT_GUJARATI     = 15,  /* Gujr */
      /** @stable ICU 2.2 */
      USCRIPT_GURMUKHI     = 16,  /* Guru */
      /** @stable ICU 2.2 */
      USCRIPT_HAN          = 17,  /* Hani */
      /** @stable ICU 2.2 */
      USCRIPT_HANGUL       = 18,  /* Hang */
      /** @stable ICU 2.2 */
      USCRIPT_HEBREW       = 19,  /* Hebr */
      /** @stable ICU 2.2 */
      USCRIPT_HIRAGANA     = 20,  /* Hira */
      /** @stable ICU 2.2 */
      USCRIPT_KANNADA      = 21,  /* Knda */
      /** @stable ICU 2.2 */
      USCRIPT_KATAKANA     = 22,  /* Kana */
      /** @stable ICU 2.2 */
      USCRIPT_KHMER        = 23,  /* Khmr */
      /** @stable ICU 2.2 */
      USCRIPT_LAO          = 24,  /* Laoo */
      /** @stable ICU 2.2 */
      USCRIPT_LATIN        = 25,  /* Latn */
      /** @stable ICU 2.2 */
      USCRIPT_MALAYALAM    = 26,  /* Mlym */
      /** @stable ICU 2.2 */
      USCRIPT_MONGOLIAN    = 27,  /* Mong */
      /** @stable ICU 2.2 */
      USCRIPT_MYANMAR      = 28,  /* Mymr */
      /** @stable ICU 2.2 */
      USCRIPT_OGHAM        = 29,  /* Ogam */
      /** @stable ICU 2.2 */
      USCRIPT_OLD_ITALIC   = 30,  /* Ital */
      /** @stable ICU 2.2 */
      USCRIPT_ORIYA        = 31,  /* Orya */
      /** @stable ICU 2.2 */
      USCRIPT_RUNIC        = 32,  /* Runr */
      /** @stable ICU 2.2 */
      USCRIPT_SINHALA      = 33,  /* Sinh */
      /** @stable ICU 2.2 */
      USCRIPT_SYRIAC       = 34,  /* Syrc */
      /** @stable ICU 2.2 */
      USCRIPT_TAMIL        = 35,  /* Taml */
      /** @stable ICU 2.2 */
      USCRIPT_TELUGU       = 36,  /* Telu */
      /** @stable ICU 2.2 */
      USCRIPT_THAANA       = 37,  /* Thaa */
      /** @stable ICU 2.2 */
      USCRIPT_THAI         = 38,  /* Thai */
      /** @stable ICU 2.2 */
      USCRIPT_TIBETAN      = 39,  /* Tibt */
      /** Canadian_Aboriginal script. @stable ICU 2.6 */
      USCRIPT_CANADIAN_ABORIGINAL = 40,  /* Cans */
      /** Canadian_Aboriginal script (alias). @stable ICU 2.2 */
      USCRIPT_UCAS         = USCRIPT_CANADIAN_ABORIGINAL,
      /** @stable ICU 2.2 */
      USCRIPT_YI           = 41,  /* Yiii */
      /* New scripts in Unicode 3.2 */
      /** @stable ICU 2.2 */
      USCRIPT_TAGALOG      = 42,  /* Tglg */
      /** @stable ICU 2.2 */
      USCRIPT_HANUNOO      = 43,  /* Hano */
      /** @stable ICU 2.2 */
      USCRIPT_BUHID        = 44,  /* Buhd */
      /** @stable ICU 2.2 */
      USCRIPT_TAGBANWA     = 45,  /* Tagb */

      /* New scripts in Unicode 4 */
      /** @stable ICU 2.6 */
      USCRIPT_BRAILLE      = 46,  /* Brai */
      /** @stable ICU 2.6 */
      USCRIPT_CYPRIOT      = 47,  /* Cprt */
      /** @stable ICU 2.6 */
      USCRIPT_LIMBU        = 48,  /* Limb */
      /** @stable ICU 2.6 */
      USCRIPT_LINEAR_B     = 49,  /* Linb */
      /** @stable ICU 2.6 */
      USCRIPT_OSMANYA      = 50,  /* Osma */
      /** @stable ICU 2.6 */
      USCRIPT_SHAVIAN      = 51,  /* Shaw */
      /** @stable ICU 2.6 */
      USCRIPT_TAI_LE       = 52,  /* Tale */
      /** @stable ICU 2.6 */
      USCRIPT_UGARITIC     = 53,  /* Ugar */

      /** New script code in Unicode 4.0.1 @stable ICU 3.0 */
      USCRIPT_KATAKANA_OR_HIRAGANA = 54,/*Hrkt */

      /* New scripts in Unicode 4.1 */
      /** @stable ICU 3.4 */
      USCRIPT_BUGINESE      = 55, /* Bugi */
      /** @stable ICU 3.4 */
      USCRIPT_GLAGOLITIC    = 56, /* Glag */
      /** @stable ICU 3.4 */
      USCRIPT_KHAROSHTHI    = 57, /* Khar */
      /** @stable ICU 3.4 */
      USCRIPT_SYLOTI_NAGRI  = 58, /* Sylo */
      /** @stable ICU 3.4 */
      USCRIPT_NEW_TAI_LUE   = 59, /* Talu */
      /** @stable ICU 3.4 */
      USCRIPT_TIFINAGH      = 60, /* Tfng */
      /** @stable ICU 3.4 */
      USCRIPT_OLD_PERSIAN   = 61, /* Xpeo */

      /* New script codes from Unicode and ISO 15924 */
      /** @stable ICU 3.6 */
      USCRIPT_BALINESE                      = 62, /* Bali */
      /** @stable ICU 3.6 */
      USCRIPT_BATAK                         = 63, /* Batk */
      /** @stable ICU 3.6 */
      USCRIPT_BLISSYMBOLS                   = 64, /* Blis */
      /** @stable ICU 3.6 */
      USCRIPT_BRAHMI                        = 65, /* Brah */
      /** @stable ICU 3.6 */
      USCRIPT_CHAM                          = 66, /* Cham */
      /** @stable ICU 3.6 */
      USCRIPT_CIRTH                         = 67, /* Cirt */
      /** @stable ICU 3.6 */
      USCRIPT_OLD_CHURCH_SLAVONIC_CYRILLIC  = 68, /* Cyrs */
      /** @stable ICU 3.6 */
      USCRIPT_DEMOTIC_EGYPTIAN              = 69, /* Egyd */
      /** @stable ICU 3.6 */
      USCRIPT_HIERATIC_EGYPTIAN             = 70, /* Egyh */
      /** @stable ICU 3.6 */
      USCRIPT_EGYPTIAN_HIEROGLYPHS          = 71, /* Egyp */
      /** @stable ICU 3.6 */
      USCRIPT_KHUTSURI                      = 72, /* Geok */
      /** @stable ICU 3.6 */
      USCRIPT_SIMPLIFIED_HAN                = 73, /* Hans */
      /** @stable ICU 3.6 */
      USCRIPT_TRADITIONAL_HAN               = 74, /* Hant */
      /** @stable ICU 3.6 */
      USCRIPT_PAHAWH_HMONG                  = 75, /* Hmng */
      /** @stable ICU 3.6 */
      USCRIPT_OLD_HUNGARIAN                 = 76, /* Hung */
      /** @stable ICU 3.6 */
      USCRIPT_HARAPPAN_INDUS                = 77, /* Inds */
      /** @stable ICU 3.6 */
      USCRIPT_JAVANESE                      = 78, /* Java */
      /** @stable ICU 3.6 */
      USCRIPT_KAYAH_LI                      = 79, /* Kali */
      /** @stable ICU 3.6 */
      USCRIPT_LATIN_FRAKTUR                 = 80, /* Latf */
      /** @stable ICU 3.6 */
      USCRIPT_LATIN_GAELIC                  = 81, /* Latg */
      /** @stable ICU 3.6 */
      USCRIPT_LEPCHA                        = 82, /* Lepc */
      /** @stable ICU 3.6 */
      USCRIPT_LINEAR_A                      = 83, /* Lina */
      /** @stable ICU 4.6 */
      USCRIPT_MANDAIC                       = 84, /* Mand */
      /** @stable ICU 3.6 */
      USCRIPT_MANDAEAN                      = USCRIPT_MANDAIC,
      /** @stable ICU 3.6 */
      USCRIPT_MAYAN_HIEROGLYPHS             = 85, /* Maya */
      /** @stable ICU 4.6 */
      USCRIPT_MEROITIC_HIEROGLYPHS          = 86, /* Mero */
      /** @stable ICU 3.6 */
      USCRIPT_MEROITIC                      = USCRIPT_MEROITIC_HIEROGLYPHS,
      /** @stable ICU 3.6 */
      USCRIPT_NKO                           = 87, /* Nkoo */
      /** @stable ICU 3.6 */
      USCRIPT_ORKHON                        = 88, /* Orkh */
      /** @stable ICU 3.6 */
      USCRIPT_OLD_PERMIC                    = 89, /* Perm */
      /** @stable ICU 3.6 */
      USCRIPT_PHAGS_PA                      = 90, /* Phag */
      /** @stable ICU 3.6 */
      USCRIPT_PHOENICIAN                    = 91, /* Phnx */
      /** @stable ICU 52 */
      USCRIPT_MIAO                          = 92, /* Plrd */
      /** @stable ICU 3.6 */
      USCRIPT_PHONETIC_POLLARD              = USCRIPT_MIAO,
      /** @stable ICU 3.6 */
      USCRIPT_RONGORONGO                    = 93, /* Roro */
      /** @stable ICU 3.6 */
      USCRIPT_SARATI                        = 94, /* Sara */
      /** @stable ICU 3.6 */
      USCRIPT_ESTRANGELO_SYRIAC             = 95, /* Syre */
      /** @stable ICU 3.6 */
      USCRIPT_WESTERN_SYRIAC                = 96, /* Syrj */
      /** @stable ICU 3.6 */
      USCRIPT_EASTERN_SYRIAC                = 97, /* Syrn */
      /** @stable ICU 3.6 */
      USCRIPT_TENGWAR                       = 98, /* Teng */
      /** @stable ICU 3.6 */
      USCRIPT_VAI                           = 99, /* Vaii */
      /** @stable ICU 3.6 */
      USCRIPT_VISIBLE_SPEECH                = 100,/* Visp */
      /** @stable ICU 3.6 */
      USCRIPT_CUNEIFORM                     = 101,/* Xsux */
      /** @stable ICU 3.6 */
      USCRIPT_UNWRITTEN_LANGUAGES           = 102,/* Zxxx */
      /** @stable ICU 3.6 */
      USCRIPT_UNKNOWN                       = 103,/* Zzzz */ /* Unknown="Code for uncoded script", for unassigned code points */

      /** @stable ICU 3.8 */
      USCRIPT_CARIAN                        = 104,/* Cari */
      /** @stable ICU 3.8 */
      USCRIPT_JAPANESE                      = 105,/* Jpan */
      /** @stable ICU 3.8 */
      USCRIPT_LANNA                         = 106,/* Lana */
      /** @stable ICU 3.8 */
      USCRIPT_LYCIAN                        = 107,/* Lyci */
      /** @stable ICU 3.8 */
      USCRIPT_LYDIAN                        = 108,/* Lydi */
      /** @stable ICU 3.8 */
      USCRIPT_OL_CHIKI                      = 109,/* Olck */
      /** @stable ICU 3.8 */
      USCRIPT_REJANG                        = 110,/* Rjng */
      /** @stable ICU 3.8 */
      USCRIPT_SAURASHTRA                    = 111,/* Saur */
      /** Sutton SignWriting @stable ICU 3.8 */
      USCRIPT_SIGN_WRITING                  = 112,/* Sgnw */
      /** @stable ICU 3.8 */
      USCRIPT_SUNDANESE                     = 113,/* Sund */
      /** @stable ICU 3.8 */
      USCRIPT_MOON                          = 114,/* Moon */
      /** @stable ICU 3.8 */
      USCRIPT_MEITEI_MAYEK                  = 115,/* Mtei */

      /** @stable ICU 4.0 */
      USCRIPT_IMPERIAL_ARAMAIC              = 116,/* Armi */
      /** @stable ICU 4.0 */
      USCRIPT_AVESTAN                       = 117,/* Avst */
      /** @stable ICU 4.0 */
      USCRIPT_CHAKMA                        = 118,/* Cakm */
      /** @stable ICU 4.0 */
      USCRIPT_KOREAN                        = 119,/* Kore */
      /** @stable ICU 4.0 */
      USCRIPT_KAITHI                        = 120,/* Kthi */
      /** @stable ICU 4.0 */
      USCRIPT_MANICHAEAN                    = 121,/* Mani */
      /** @stable ICU 4.0 */
      USCRIPT_INSCRIPTIONAL_PAHLAVI         = 122,/* Phli */
      /** @stable ICU 4.0 */
      USCRIPT_PSALTER_PAHLAVI               = 123,/* Phlp */
      /** @stable ICU 4.0 */
      USCRIPT_BOOK_PAHLAVI                  = 124,/* Phlv */
      /** @stable ICU 4.0 */
      USCRIPT_INSCRIPTIONAL_PARTHIAN        = 125,/* Prti */
      /** @stable ICU 4.0 */
      USCRIPT_SAMARITAN                     = 126,/* Samr */
      /** @stable ICU 4.0 */
      USCRIPT_TAI_VIET                      = 127,/* Tavt */
      /** @stable ICU 4.0 */
      USCRIPT_MATHEMATICAL_NOTATION         = 128,/* Zmth */
      /** @stable ICU 4.0 */
      USCRIPT_SYMBOLS                       = 129,/* Zsym */

      /** @stable ICU 4.4 */
      USCRIPT_BAMUM                         = 130,/* Bamu */
      /** @stable ICU 4.4 */
      USCRIPT_LISU                          = 131,/* Lisu */
      /** @stable ICU 4.4 */
      USCRIPT_NAKHI_GEBA                    = 132,/* Nkgb */
      /** @stable ICU 4.4 */
      USCRIPT_OLD_SOUTH_ARABIAN             = 133,/* Sarb */

      /** @stable ICU 4.6 */
      USCRIPT_BASSA_VAH                     = 134,/* Bass */
      /** @stable ICU 54 */
      USCRIPT_DUPLOYAN                      = 135,/* Dupl */
#ifndef U_HIDE_DEPRECATED_API
      /** @deprecated ICU 54 Typo, use USCRIPT_DUPLOYAN */
      USCRIPT_DUPLOYAN_SHORTAND             = USCRIPT_DUPLOYAN,
#endif  /* U_HIDE_DEPRECATED_API */
      /** @stable ICU 4.6 */
      USCRIPT_ELBASAN                       = 136,/* Elba */
      /** @stable ICU 4.6 */
      USCRIPT_GRANTHA                       = 137,/* Gran */
      /** @stable ICU 4.6 */
      USCRIPT_KPELLE                        = 138,/* Kpel */
      /** @stable ICU 4.6 */
      USCRIPT_LOMA                          = 139,/* Loma */
      /** Mende Kikakui @stable ICU 4.6 */
      USCRIPT_MENDE                         = 140,/* Mend */
      /** @stable ICU 4.6 */
      USCRIPT_MEROITIC_CURSIVE              = 141,/* Merc */
      /** @stable ICU 4.6 */
      USCRIPT_OLD_NORTH_ARABIAN             = 142,/* Narb */
      /** @stable ICU 4.6 */
      USCRIPT_NABATAEAN                     = 143,/* Nbat */
      /** @stable ICU 4.6 */
      USCRIPT_PALMYRENE                     = 144,/* Palm */
      /** @stable ICU 54 */
      USCRIPT_KHUDAWADI                     = 145,/* Sind */
      /** @stable ICU 4.6 */
      USCRIPT_SINDHI                        = USCRIPT_KHUDAWADI,
      /** @stable ICU 4.6 */
      USCRIPT_WARANG_CITI                   = 146,/* Wara */

      /** @stable ICU 4.8 */
      USCRIPT_AFAKA                         = 147,/* Afak */
      /** @stable ICU 4.8 */
      USCRIPT_JURCHEN                       = 148,/* Jurc */
      /** @stable ICU 4.8 */
      USCRIPT_MRO                           = 149,/* Mroo */
      /** @stable ICU 4.8 */
      USCRIPT_NUSHU                         = 150,/* Nshu */
      /** @stable ICU 4.8 */
      USCRIPT_SHARADA                       = 151,/* Shrd */
      /** @stable ICU 4.8 */
      USCRIPT_SORA_SOMPENG                  = 152,/* Sora */
      /** @stable ICU 4.8 */
      USCRIPT_TAKRI                         = 153,/* Takr */
      /** @stable ICU 4.8 */
      USCRIPT_TANGUT                        = 154,/* Tang */
      /** @stable ICU 4.8 */
      USCRIPT_WOLEAI                        = 155,/* Wole */

      /** @stable ICU 49 */
      USCRIPT_ANATOLIAN_HIEROGLYPHS         = 156,/* Hluw */
      /** @stable ICU 49 */
      USCRIPT_KHOJKI                        = 157,/* Khoj */
      /** @stable ICU 49 */
      USCRIPT_TIRHUTA                       = 158,/* Tirh */

      /** @stable ICU 52 */
      USCRIPT_CAUCASIAN_ALBANIAN            = 159,/* Aghb */
      /** @stable ICU 52 */
      USCRIPT_MAHAJANI                      = 160,/* Mahj */

      /** @stable ICU 54 */
      USCRIPT_AHOM                          = 161,/* Ahom */
      /** @stable ICU 54 */
      USCRIPT_HATRAN                        = 162,/* Hatr */
      /** @stable ICU 54 */
      USCRIPT_MODI                          = 163,/* Modi */
      /** @stable ICU 54 */
      USCRIPT_MULTANI                       = 164,/* Mult */
      /** @stable ICU 54 */
      USCRIPT_PAU_CIN_HAU                   = 165,/* Pauc */
      /** @stable ICU 54 */
      USCRIPT_SIDDHAM                       = 166,/* Sidd */

      /** @stable ICU 58 */
      USCRIPT_ADLAM                         = 167,/* Adlm */
      /** @stable ICU 58 */
      USCRIPT_BHAIKSUKI                     = 168,/* Bhks */
      /** @stable ICU 58 */
      USCRIPT_MARCHEN                       = 169,/* Marc */
      /** @stable ICU 58 */
      USCRIPT_NEWA                          = 170,/* Newa */
      /** @stable ICU 58 */
      USCRIPT_OSAGE                         = 171,/* Osge */

      /** @stable ICU 58 */
      USCRIPT_HAN_WITH_BOPOMOFO             = 172,/* Hanb */
      /** @stable ICU 58 */
      USCRIPT_JAMO                          = 173,/* Jamo */
      /** @stable ICU 58 */
      USCRIPT_SYMBOLS_EMOJI                 = 174,/* Zsye */

      /** @stable ICU 60 */
      USCRIPT_MASARAM_GONDI                 = 175,/* Gonm */
      /** @stable ICU 60 */
      USCRIPT_SOYOMBO                       = 176,/* Soyo */
      /** @stable ICU 60 */
      USCRIPT_ZANABAZAR_SQUARE              = 177,/* Zanb */

      /** @stable ICU 62 */
      USCRIPT_DOGRA                         = 178,/* Dogr */
      /** @stable ICU 62 */
      USCRIPT_GUNJALA_GONDI                 = 179,/* Gong */
      /** @stable ICU 62 */
      USCRIPT_MAKASAR                       = 180,/* Maka */
      /** @stable ICU 62 */
      USCRIPT_MEDEFAIDRIN                   = 181,/* Medf */
      /** @stable ICU 62 */
      USCRIPT_HANIFI_ROHINGYA               = 182,/* Rohg */
      /** @stable ICU 62 */
      USCRIPT_SOGDIAN                       = 183,/* Sogd */
      /** @stable ICU 62 */
      USCRIPT_OLD_SOGDIAN                   = 184,/* Sogo */

      /** @stable ICU 64 */
      USCRIPT_ELYMAIC                       = 185,/* Elym */
      /** @stable ICU 64 */
      USCRIPT_NYIAKENG_PUACHUE_HMONG        = 186,/* Hmnp */
      /** @stable ICU 64 */
      USCRIPT_NANDINAGARI                   = 187,/* Nand */
      /** @stable ICU 64 */
      USCRIPT_WANCHO                        = 188,/* Wcho */

      /** @stable ICU 66 */
      USCRIPT_CHORASMIAN                    = 189,/* Chrs */
      /** @stable ICU 66 */
      USCRIPT_DIVES_AKURU                   = 190,/* Diak */
      /** @stable ICU 66 */
      USCRIPT_KHITAN_SMALL_SCRIPT           = 191,/* Kits */
      /** @stable ICU 66 */
      USCRIPT_YEZIDI                        = 192,/* Yezi */

      /** @stable ICU 70 */
      USCRIPT_CYPRO_MINOAN                  = 193,/* Cpmn */
      /** @stable ICU 70 */
      USCRIPT_OLD_UYGHUR                    = 194,/* Ougr */
      /** @stable ICU 70 */
      USCRIPT_TANGSA                        = 195,/* Tnsa */
      /** @stable ICU 70 */
      USCRIPT_TOTO                          = 196,/* Toto */
      /** @stable ICU 70 */
      USCRIPT_VITHKUQI                      = 197,/* Vith */
} UScriptCode;

int8_t u_charType(UChar32 c);
UBool u_isWhitespace(UChar32 c);
UCharDirection u_charDirection(UChar32 c);
UJoiningType ubidi_getJoiningType(UChar32 c);
UScriptCode uscript_getScript(UChar32 c, UErrorCode *pErrorCode);

}

#endif /* MODULES_IDN_UINDACHAR_H_ */
