// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *******************************************************************************
 *
 *   Copyright (C) 2003-2014, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  uidna.h
 *   encoding:   UTF-8
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2003feb1
 *   created by: Ram Viswanadha
 */

#ifndef MODULES_IDN_UIDNA_H_
#define MODULES_IDN_UIDNA_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef U_EXPORT
    /* Use the predefined value. */
#elif defined(U_STATIC_IMPLEMENTATION)
#   define U_EXPORT
#elif defined(_MSC_VER)
#   define U_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#   define U_EXPORT __attribute__((visibility("default")))
#else
#   define U_EXPORT
#endif

#ifdef __cplusplus
#   define U_CFUNC extern "C"
#else
#   define U_CFUNC extern
#endif

#define U_CAPI U_CFUNC U_EXPORT

typedef char16_t UChar;
typedef int8_t UBool;

typedef enum UErrorCode {
    /* The ordering of U_ERROR_INFO_START Vs U_USING_FALLBACK_WARNING looks weird
     * and is that way because VC++ debugger displays first encountered constant,
     * which is not the what the code is used for
     */

    U_USING_FALLBACK_WARNING  = -128,   /**< A resource bundle lookup returned a fallback result (not an error) */

    U_ERROR_WARNING_START     = -128,   /**< Start of information results (semantically successful) */

    U_USING_DEFAULT_WARNING   = -127,   /**< A resource bundle lookup returned a result from the root locale (not an error) */

    U_SAFECLONE_ALLOCATED_WARNING = -126, /**< A SafeClone operation required allocating memory (informational only) */

    U_STATE_OLD_WARNING       = -125,   /**< ICU has to use compatibility layer to construct the service. Expect performance/memory usage degradation. Consider upgrading */

    U_STRING_NOT_TERMINATED_WARNING = -124,/**< An output string could not be NUL-terminated because output length==destCapacity. */

    U_SORT_KEY_TOO_SHORT_WARNING = -123, /**< Number of levels requested in getBound is higher than the number of levels in the sort key */

    U_AMBIGUOUS_ALIAS_WARNING = -122,   /**< This converter alias can go to different converter implementations */

    U_DIFFERENT_UCA_VERSION = -121,     /**< ucol_open encountered a mismatch between UCA version and collator image version, so the collator was constructed from rules. No impact to further function */

    U_PLUGIN_CHANGED_LEVEL_WARNING = -120, /**< A plugin caused a level change. May not be an error, but later plugins may not load. */

#ifndef U_HIDE_DEPRECATED_API
    /**
     * One more than the highest normal UErrorCode warning value.
     * @deprecated ICU 58 The numeric value may change over time, see ICU ticket #12420.
     */
    U_ERROR_WARNING_LIMIT,
#endif  // U_HIDE_DEPRECATED_API

    U_ZERO_ERROR              =  0,     /**< No error, no warning. */

    U_ILLEGAL_ARGUMENT_ERROR  =  1,     /**< Start of codes indicating failure */
    U_MISSING_RESOURCE_ERROR  =  2,     /**< The requested resource cannot be found */
    U_INVALID_FORMAT_ERROR    =  3,     /**< Data format is not what is expected */
    U_FILE_ACCESS_ERROR       =  4,     /**< The requested file cannot be found */
    U_INTERNAL_PROGRAM_ERROR  =  5,     /**< Indicates a bug in the library code */
    U_MESSAGE_PARSE_ERROR     =  6,     /**< Unable to parse a message (message format) */
    U_MEMORY_ALLOCATION_ERROR =  7,     /**< Memory allocation error */
    U_INDEX_OUTOFBOUNDS_ERROR =  8,     /**< Trying to access the index that is out of bounds */
    U_PARSE_ERROR             =  9,     /**< Equivalent to Java ParseException */
    U_INVALID_CHAR_FOUND      = 10,     /**< Character conversion: Unmappable input sequence. In other APIs: Invalid character. */
    U_TRUNCATED_CHAR_FOUND    = 11,     /**< Character conversion: Incomplete input sequence. */
    U_ILLEGAL_CHAR_FOUND      = 12,     /**< Character conversion: Illegal input sequence/combination of input units. */
    U_INVALID_TABLE_FORMAT    = 13,     /**< Conversion table file found, but corrupted */
    U_INVALID_TABLE_FILE      = 14,     /**< Conversion table file not found */
    U_BUFFER_OVERFLOW_ERROR   = 15,     /**< A result would not fit in the supplied buffer */
    U_UNSUPPORTED_ERROR       = 16,     /**< Requested operation not supported in current context */
    U_RESOURCE_TYPE_MISMATCH  = 17,     /**< an operation is requested over a resource that does not support it */
    U_ILLEGAL_ESCAPE_SEQUENCE = 18,     /**< ISO-2022 illegal escape sequence */
    U_UNSUPPORTED_ESCAPE_SEQUENCE = 19, /**< ISO-2022 unsupported escape sequence */
    U_NO_SPACE_AVAILABLE      = 20,     /**< No space available for in-buffer expansion for Arabic shaping */
    U_CE_NOT_FOUND_ERROR      = 21,     /**< Currently used only while setting variable top, but can be used generally */
    U_PRIMARY_TOO_LONG_ERROR  = 22,     /**< User tried to set variable top to a primary that is longer than two bytes */
    U_STATE_TOO_OLD_ERROR     = 23,     /**< ICU cannot construct a service from this state, as it is no longer supported */
    U_TOO_MANY_ALIASES_ERROR  = 24,     /**< There are too many aliases in the path to the requested resource.
                                             It is very possible that a circular alias definition has occurred */
    U_ENUM_OUT_OF_SYNC_ERROR  = 25,     /**< UEnumeration out of sync with underlying collection */
    U_INVARIANT_CONVERSION_ERROR = 26,  /**< Unable to convert a UChar* string to char* with the invariant converter. */
    U_INVALID_STATE_ERROR     = 27,     /**< Requested operation can not be completed with ICU in its current state */
    U_COLLATOR_VERSION_MISMATCH = 28,   /**< Collator version is not compatible with the base version */
    U_USELESS_COLLATOR_ERROR  = 29,     /**< Collator is options only and no base is specified */
    U_NO_WRITE_PERMISSION     = 30,     /**< Attempt to modify read-only or constant data. */
    /**
     * The input is impractically long for an operation.
     * It is rejected because it may lead to problems such as excessive
     * processing time, stack depth, or heap memory requirements.
     *
     * @stable ICU 68
     */
    U_INPUT_TOO_LONG_ERROR = 31,

#ifndef U_HIDE_DEPRECATED_API
    /**
     * One more than the highest standard error code.
     * @deprecated ICU 58 The numeric value may change over time, see ICU ticket #12420.
     */
    U_STANDARD_ERROR_LIMIT = 32,
#endif  // U_HIDE_DEPRECATED_API

    /*
     * Error codes in the range 0x10000 0x10100 are reserved for Transliterator.
     */
    U_BAD_VARIABLE_DEFINITION=0x10000,/**< Missing '$' or duplicate variable name */
    U_PARSE_ERROR_START = 0x10000,    /**< Start of Transliterator errors */
    U_MALFORMED_RULE,                 /**< Elements of a rule are misplaced */
    U_MALFORMED_SET,                  /**< A UnicodeSet pattern is invalid*/
    U_MALFORMED_SYMBOL_REFERENCE,     /**< UNUSED as of ICU 2.4 */
    U_MALFORMED_UNICODE_ESCAPE,       /**< A Unicode escape pattern is invalid*/
    U_MALFORMED_VARIABLE_DEFINITION,  /**< A variable definition is invalid */
    U_MALFORMED_VARIABLE_REFERENCE,   /**< A variable reference is invalid */
    U_MISMATCHED_SEGMENT_DELIMITERS,  /**< UNUSED as of ICU 2.4 */
    U_MISPLACED_ANCHOR_START,         /**< A start anchor appears at an illegal position */
    U_MISPLACED_CURSOR_OFFSET,        /**< A cursor offset occurs at an illegal position */
    U_MISPLACED_QUANTIFIER,           /**< A quantifier appears after a segment close delimiter */
    U_MISSING_OPERATOR,               /**< A rule contains no operator */
    U_MISSING_SEGMENT_CLOSE,          /**< UNUSED as of ICU 2.4 */
    U_MULTIPLE_ANTE_CONTEXTS,         /**< More than one ante context */
    U_MULTIPLE_CURSORS,               /**< More than one cursor */
    U_MULTIPLE_POST_CONTEXTS,         /**< More than one post context */
    U_TRAILING_BACKSLASH,             /**< A dangling backslash */
    U_UNDEFINED_SEGMENT_REFERENCE,    /**< A segment reference does not correspond to a defined segment */
    U_UNDEFINED_VARIABLE,             /**< A variable reference does not correspond to a defined variable */
    U_UNQUOTED_SPECIAL,               /**< A special character was not quoted or escaped */
    U_UNTERMINATED_QUOTE,             /**< A closing single quote is missing */
    U_RULE_MASK_ERROR,                /**< A rule is hidden by an earlier more general rule */
    U_MISPLACED_COMPOUND_FILTER,      /**< A compound filter is in an invalid location */
    U_MULTIPLE_COMPOUND_FILTERS,      /**< More than one compound filter */
    U_INVALID_RBT_SYNTAX,             /**< A "::id" rule was passed to the RuleBasedTransliterator parser */
    U_INVALID_PROPERTY_PATTERN,       /**< UNUSED as of ICU 2.4 */
    U_MALFORMED_PRAGMA,               /**< A 'use' pragma is invalid */
    U_UNCLOSED_SEGMENT,               /**< A closing ')' is missing */
    U_ILLEGAL_CHAR_IN_SEGMENT,        /**< UNUSED as of ICU 2.4 */
    U_VARIABLE_RANGE_EXHAUSTED,       /**< Too many stand-ins generated for the given variable range */
    U_VARIABLE_RANGE_OVERLAP,         /**< The variable range overlaps characters used in rules */
    U_ILLEGAL_CHARACTER,              /**< A special character is outside its allowed context */
    U_INTERNAL_TRANSLITERATOR_ERROR,  /**< Internal transliterator system error */
    U_INVALID_ID,                     /**< A "::id" rule specifies an unknown transliterator */
    U_INVALID_FUNCTION,               /**< A "&fn()" rule specifies an unknown transliterator */
#ifndef U_HIDE_DEPRECATED_API
    /**
     * One more than the highest normal Transliterator error code.
     * @deprecated ICU 58 The numeric value may change over time, see ICU ticket #12420.
     */
    U_PARSE_ERROR_LIMIT,
#endif  // U_HIDE_DEPRECATED_API

    /*
     * Error codes in the range 0x10100 0x10200 are reserved for the formatting API.
     */
    U_UNEXPECTED_TOKEN=0x10100,       /**< Syntax error in format pattern */
    U_FMT_PARSE_ERROR_START=0x10100,  /**< Start of format library errors */
    U_MULTIPLE_DECIMAL_SEPARATORS,    /**< More than one decimal separator in number pattern */
    U_MULTIPLE_DECIMAL_SEPERATORS = U_MULTIPLE_DECIMAL_SEPARATORS, /**< Typo: kept for backward compatibility. Use U_MULTIPLE_DECIMAL_SEPARATORS */
    U_MULTIPLE_EXPONENTIAL_SYMBOLS,   /**< More than one exponent symbol in number pattern */
    U_MALFORMED_EXPONENTIAL_PATTERN,  /**< Grouping symbol in exponent pattern */
    U_MULTIPLE_PERCENT_SYMBOLS,       /**< More than one percent symbol in number pattern */
    U_MULTIPLE_PERMILL_SYMBOLS,       /**< More than one permill symbol in number pattern */
    U_MULTIPLE_PAD_SPECIFIERS,        /**< More than one pad symbol in number pattern */
    U_PATTERN_SYNTAX_ERROR,           /**< Syntax error in format pattern */
    U_ILLEGAL_PAD_POSITION,           /**< Pad symbol misplaced in number pattern */
    U_UNMATCHED_BRACES,               /**< Braces do not match in message pattern */
    U_UNSUPPORTED_PROPERTY,           /**< UNUSED as of ICU 2.4 */
    U_UNSUPPORTED_ATTRIBUTE,          /**< UNUSED as of ICU 2.4 */
    U_ARGUMENT_TYPE_MISMATCH,         /**< Argument name and argument index mismatch in MessageFormat functions */
    U_DUPLICATE_KEYWORD,              /**< Duplicate keyword in PluralFormat */
    U_UNDEFINED_KEYWORD,              /**< Undefined Plural keyword */
    U_DEFAULT_KEYWORD_MISSING,        /**< Missing DEFAULT rule in plural rules */
    U_DECIMAL_NUMBER_SYNTAX_ERROR,    /**< Decimal number syntax error */
    U_FORMAT_INEXACT_ERROR,           /**< Cannot format a number exactly and rounding mode is ROUND_UNNECESSARY @stable ICU 4.8 */
    U_NUMBER_ARG_OUTOFBOUNDS_ERROR,   /**< The argument to a NumberFormatter helper method was out of bounds; the bounds are usually 0 to 999. @stable ICU 61 */
    U_NUMBER_SKELETON_SYNTAX_ERROR,   /**< The number skeleton passed to C++ NumberFormatter or C UNumberFormatter was invalid or contained a syntax error. @stable ICU 62 */
#ifndef U_HIDE_DEPRECATED_API
    /**
     * One more than the highest normal formatting API error code.
     * @deprecated ICU 58 The numeric value may change over time, see ICU ticket #12420.
     */
    U_FMT_PARSE_ERROR_LIMIT = 0x10114,
#endif  // U_HIDE_DEPRECATED_API

    /*
     * Error codes in the range 0x10200 0x102ff are reserved for BreakIterator.
     */
    U_BRK_INTERNAL_ERROR=0x10200,          /**< An internal error (bug) was detected.             */
    U_BRK_ERROR_START=0x10200,             /**< Start of codes indicating Break Iterator failures */
    U_BRK_HEX_DIGITS_EXPECTED,             /**< Hex digits expected as part of a escaped char in a rule. */
    U_BRK_SEMICOLON_EXPECTED,              /**< Missing ';' at the end of a RBBI rule.            */
    U_BRK_RULE_SYNTAX,                     /**< Syntax error in RBBI rule.                        */
    U_BRK_UNCLOSED_SET,                    /**< UnicodeSet writing an RBBI rule missing a closing ']'. */
    U_BRK_ASSIGN_ERROR,                    /**< Syntax error in RBBI rule assignment statement.   */
    U_BRK_VARIABLE_REDFINITION,            /**< RBBI rule $Variable redefined.                    */
    U_BRK_MISMATCHED_PAREN,                /**< Mis-matched parentheses in an RBBI rule.          */
    U_BRK_NEW_LINE_IN_QUOTED_STRING,       /**< Missing closing quote in an RBBI rule.            */
    U_BRK_UNDEFINED_VARIABLE,              /**< Use of an undefined $Variable in an RBBI rule.    */
    U_BRK_INIT_ERROR,                      /**< Initialization failure.  Probable missing ICU Data. */
    U_BRK_RULE_EMPTY_SET,                  /**< Rule contains an empty Unicode Set.               */
    U_BRK_UNRECOGNIZED_OPTION,             /**< !!option in RBBI rules not recognized.            */
    U_BRK_MALFORMED_RULE_TAG,              /**< The {nnn} tag on a rule is malformed              */
#ifndef U_HIDE_DEPRECATED_API
    /**
     * One more than the highest normal BreakIterator error code.
     * @deprecated ICU 58 The numeric value may change over time, see ICU ticket #12420.
     */
    U_BRK_ERROR_LIMIT,
#endif  // U_HIDE_DEPRECATED_API

    /*
     * Error codes in the range 0x10300-0x103ff are reserved for regular expression related errors.
     */
    U_REGEX_INTERNAL_ERROR=0x10300,       /**< An internal error (bug) was detected.              */
    U_REGEX_ERROR_START=0x10300,          /**< Start of codes indicating Regexp failures          */
    U_REGEX_RULE_SYNTAX,                  /**< Syntax error in regexp pattern.                    */
    U_REGEX_INVALID_STATE,                /**< RegexMatcher in invalid state for requested operation */
    U_REGEX_BAD_ESCAPE_SEQUENCE,          /**< Unrecognized backslash escape sequence in pattern  */
    U_REGEX_PROPERTY_SYNTAX,              /**< Incorrect Unicode property                         */
    U_REGEX_UNIMPLEMENTED,                /**< Use of regexp feature that is not yet implemented. */
    U_REGEX_MISMATCHED_PAREN,             /**< Incorrectly nested parentheses in regexp pattern.  */
    U_REGEX_NUMBER_TOO_BIG,               /**< Decimal number is too large.                       */
    U_REGEX_BAD_INTERVAL,                 /**< Error in {min,max} interval                        */
    U_REGEX_MAX_LT_MIN,                   /**< In {min,max}, max is less than min.                */
    U_REGEX_INVALID_BACK_REF,             /**< Back-reference to a non-existent capture group.    */
    U_REGEX_INVALID_FLAG,                 /**< Invalid value for match mode flags.                */
    U_REGEX_LOOK_BEHIND_LIMIT,            /**< Look-Behind pattern matches must have a bounded maximum length.    */
    U_REGEX_SET_CONTAINS_STRING,          /**< Regexps cannot have UnicodeSets containing strings.*/
#ifndef U_HIDE_DEPRECATED_API
    U_REGEX_OCTAL_TOO_BIG,                /**< Octal character constants must be <= 0377. @deprecated ICU 54. This error cannot occur. */
#endif  /* U_HIDE_DEPRECATED_API */
    U_REGEX_MISSING_CLOSE_BRACKET=U_REGEX_SET_CONTAINS_STRING+2, /**< Missing closing bracket on a bracket expression. */
    U_REGEX_INVALID_RANGE,                /**< In a character range [x-y], x is greater than y.   */
    U_REGEX_STACK_OVERFLOW,               /**< Regular expression backtrack stack overflow.       */
    U_REGEX_TIME_OUT,                     /**< Maximum allowed match time exceeded                */
    U_REGEX_STOPPED_BY_CALLER,            /**< Matching operation aborted by user callback fn.    */
    U_REGEX_PATTERN_TOO_BIG,              /**< Pattern exceeds limits on size or complexity. @stable ICU 55 */
    U_REGEX_INVALID_CAPTURE_GROUP_NAME,   /**< Invalid capture group name. @stable ICU 55 */
#ifndef U_HIDE_DEPRECATED_API
    /**
     * One more than the highest normal regular expression error code.
     * @deprecated ICU 58 The numeric value may change over time, see ICU ticket #12420.
     */
    U_REGEX_ERROR_LIMIT=U_REGEX_STOPPED_BY_CALLER+3,
#endif  // U_HIDE_DEPRECATED_API

    /*
     * Error codes in the range 0x10400-0x104ff are reserved for IDNA related error codes.
     */
    U_IDNA_PROHIBITED_ERROR=0x10400,
    U_IDNA_ERROR_START=0x10400,
    U_IDNA_UNASSIGNED_ERROR,
    U_IDNA_CHECK_BIDI_ERROR,
    U_IDNA_STD3_ASCII_RULES_ERROR,
    U_IDNA_ACE_PREFIX_ERROR,
    U_IDNA_VERIFICATION_ERROR,
    U_IDNA_LABEL_TOO_LONG_ERROR,
    U_IDNA_ZERO_LENGTH_LABEL_ERROR,
    U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR,
#ifndef U_HIDE_DEPRECATED_API
    /**
     * One more than the highest normal IDNA error code.
     * @deprecated ICU 58 The numeric value may change over time, see ICU ticket #12420.
     */
    U_IDNA_ERROR_LIMIT,
#endif  // U_HIDE_DEPRECATED_API
    /*
     * Aliases for StringPrep
     */
    U_STRINGPREP_PROHIBITED_ERROR = U_IDNA_PROHIBITED_ERROR,
    U_STRINGPREP_UNASSIGNED_ERROR = U_IDNA_UNASSIGNED_ERROR,
    U_STRINGPREP_CHECK_BIDI_ERROR = U_IDNA_CHECK_BIDI_ERROR,

    /*
     * Error codes in the range 0x10500-0x105ff are reserved for Plugin related error codes.
     */
    U_PLUGIN_ERROR_START=0x10500,         /**< Start of codes indicating plugin failures */
    U_PLUGIN_TOO_HIGH=0x10500,            /**< The plugin's level is too high to be loaded right now. */
    U_PLUGIN_DIDNT_SET_LEVEL,             /**< The plugin didn't call uplug_setPlugLevel in response to a QUERY */
#ifndef U_HIDE_DEPRECATED_API
    /**
     * One more than the highest normal plug-in error code.
     * @deprecated ICU 58 The numeric value may change over time, see ICU ticket #12420.
     */
    U_PLUGIN_ERROR_LIMIT,
#endif  // U_HIDE_DEPRECATED_API

#ifndef U_HIDE_DEPRECATED_API
    /**
     * One more than the highest normal error code.
     * @deprecated ICU 58 The numeric value may change over time, see ICU ticket #12420.
     */
    U_ERROR_LIMIT=U_PLUGIN_ERROR_LIMIT
#endif  // U_HIDE_DEPRECATED_API
} UErrorCode;

/*
 * IDNA option bit set values.
 */
enum {
	/**
	 * Default options value: None of the other options are set.
	 * For use in static worker and factory methods.
	 * @stable ICU 2.6
	 */
	UIDNA_DEFAULT = 0,
	/**
	 * Option to check whether the input conforms to the STD3 ASCII rules,
	 * for example the restriction of labels to LDH characters
	 * (ASCII Letters, Digits and Hyphen-Minus).
	 * For use in static worker and factory methods.
	 * @stable ICU 2.6
	 */
	UIDNA_USE_STD3_RULES = 2,
	/**
	 * IDNA option to check for whether the input conforms to the BiDi rules.
	 * For use in static worker and factory methods.
	 * <p>This option is ignored by the IDNA2003 implementation.
	 * (IDNA2003 always performs a BiDi check.)
	 * @stable ICU 4.6
	 */
	UIDNA_CHECK_BIDI = 4,
	/**
	 * IDNA option to check for whether the input conforms to the CONTEXTJ rules.
	 * For use in static worker and factory methods.
	 * <p>This option is ignored by the IDNA2003 implementation.
	 * (The CONTEXTJ check is new in IDNA2008.)
	 * @stable ICU 4.6
	 */
	UIDNA_CHECK_CONTEXTJ = 8,
	/**
	 * IDNA option for nontransitional processing in ToASCII().
	 * For use in static worker and factory methods.
	 * <p>By default, ToASCII() uses transitional processing.
	 * <p>This option is ignored by the IDNA2003 implementation.
	 * (This is only relevant for compatibility of newer IDNA implementations with IDNA2003.)
	 * @stable ICU 4.6
	 */
	UIDNA_NONTRANSITIONAL_TO_ASCII = 0x10,
	/**
	 * IDNA option for nontransitional processing in ToUnicode().
	 * For use in static worker and factory methods.
	 * <p>By default, ToUnicode() uses transitional processing.
	 * <p>This option is ignored by the IDNA2003 implementation.
	 * (This is only relevant for compatibility of newer IDNA implementations with IDNA2003.)
	 * @stable ICU 4.6
	 */
	UIDNA_NONTRANSITIONAL_TO_UNICODE = 0x20,
	/**
	 * IDNA option to check for whether the input conforms to the CONTEXTO rules.
	 * For use in static worker and factory methods.
	 * <p>This option is ignored by the IDNA2003 implementation.
	 * (The CONTEXTO check is new in IDNA2008.)
	 * <p>This is for use by registries for IDNA2008 conformance.
	 * UTS #46 does not require the CONTEXTO check.
	 * @stable ICU 49
	 */
	UIDNA_CHECK_CONTEXTO = 0x40
};

struct UIDNA;
typedef struct UIDNA UIDNA;  /**< C typedef for struct UIDNA. @stable ICU 4.6 */

/**
 * Returns a UIDNA instance which implements UTS #46.
 * Returns an unmodifiable instance, owned by the caller.
 * Cache it for multiple operations, and uidna_close() it when done.
 * The instance is thread-safe, that is, it can be used concurrently.
 *
 * For details about the UTS #46 implementation see the IDNA C++ class in idna.h.
 *
 * @param options Bit set to modify the processing and error checking.
 *                See option bit set values in uidna.h.
 * @param pErrorCode Standard ICU error code. Its input value must
 *                  pass the U_SUCCESS() test, or else the function returns
 *                  immediately. Check for U_FAILURE() on output or use with
 *                  function chaining. (See User Guide for details.)
 * @return the UTS #46 UIDNA instance, if successful
 * @stable ICU 4.6
 */
U_CAPI UIDNA * uidna_open(uint32_t options, UErrorCode *pErrorCode);

/**
 * Closes a UIDNA instance.
 * @param idna UIDNA instance to be closed
 * @stable ICU 4.6
 */
U_CAPI void uidna_close(UIDNA *idna);

/**
 * Output container for IDNA processing errors.
 * Initialize with UIDNA_INFO_INITIALIZER:
 * \code
 * UIDNAInfo info = UIDNA_INFO_INITIALIZER;
 * int32_t length = uidna_nameToASCII(..., &info, &errorCode);
 * if(U_SUCCESS(errorCode) && info.errors!=0) { ... }
 * \endcode
 * @stable ICU 4.6
 */
typedef struct UIDNAInfo {
    /**
     * Set to true if transitional and nontransitional processing produce different results.
     * For details see C++ IDNAInfo::isTransitionalDifferent().
     * @stable ICU 4.6
     */
    UBool isTransitionalDifferent;
    /**
     * Bit set indicating IDNA processing errors. 0 if no errors.
     * See UIDNA_ERROR_... constants.
     * @stable ICU 4.6
     */
    uint32_t errors;
} UIDNAInfo;

U_CAPI const char *u_errorName(UErrorCode code);

/**
 * Converts a single domain name label into its ASCII form for DNS lookup.
 * If any processing step fails, then pInfo->errors will be non-zero and
 * the result might not be an ASCII string.
 * The label might be modified according to the types of errors.
 * Labels with severe errors will be left in (or turned into) their Unicode form.
 *
 * The UErrorCode indicates an error only in exceptional cases,
 * such as a U_MEMORY_ALLOCATION_ERROR.
 *
 * @param idna UIDNA instance
 * @param label Input domain name label
 * @param length Label length, or -1 if NUL-terminated
 * @param dest Destination string buffer
 * @param capacity Destination buffer capacity
 * @param pInfo Output container of IDNA processing details.
 * @param pErrorCode Standard ICU error code. Its input value must
 *                  pass the U_SUCCESS() test, or else the function returns
 *                  immediately. Check for U_FAILURE() on output or use with
 *                  function chaining. (See User Guide for details.)
 * @return destination string length
 * @stable ICU 4.6
 */
U_CAPI int32_t uidna_labelToASCII(const UIDNA *idna, const UChar *label, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

/**
 * Converts a single domain name label into its Unicode form for human-readable display.
 * If any processing step fails, then pInfo->errors will be non-zero.
 * The label might be modified according to the types of errors.
 *
 * The UErrorCode indicates an error only in exceptional cases,
 * such as a U_MEMORY_ALLOCATION_ERROR.
 *
 * @param idna UIDNA instance
 * @param label Input domain name label
 * @param length Label length, or -1 if NUL-terminated
 * @param dest Destination string buffer
 * @param capacity Destination buffer capacity
 * @param pInfo Output container of IDNA processing details.
 * @param pErrorCode Standard ICU error code. Its input value must
 *                  pass the U_SUCCESS() test, or else the function returns
 *                  immediately. Check for U_FAILURE() on output or use with
 *                  function chaining. (See User Guide for details.)
 * @return destination string length
 * @stable ICU 4.6
 */
U_CAPI int32_t uidna_labelToUnicode(const UIDNA *idna, const UChar *label, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

/**
 * Converts a whole domain name into its ASCII form for DNS lookup.
 * If any processing step fails, then pInfo->errors will be non-zero and
 * the result might not be an ASCII string.
 * The domain name might be modified according to the types of errors.
 * Labels with severe errors will be left in (or turned into) their Unicode form.
 *
 * The UErrorCode indicates an error only in exceptional cases,
 * such as a U_MEMORY_ALLOCATION_ERROR.
 *
 * @param idna UIDNA instance
 * @param name Input domain name
 * @param length Domain name length, or -1 if NUL-terminated
 * @param dest Destination string buffer
 * @param capacity Destination buffer capacity
 * @param pInfo Output container of IDNA processing details.
 * @param pErrorCode Standard ICU error code. Its input value must
 *                  pass the U_SUCCESS() test, or else the function returns
 *                  immediately. Check for U_FAILURE() on output or use with
 *                  function chaining. (See User Guide for details.)
 * @return destination string length
 * @stable ICU 4.6
 */
U_CAPI int32_t uidna_nameToASCII(const UIDNA *idna, const UChar *name, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

/**
 * Converts a whole domain name into its Unicode form for human-readable display.
 * If any processing step fails, then pInfo->errors will be non-zero.
 * The domain name might be modified according to the types of errors.
 *
 * The UErrorCode indicates an error only in exceptional cases,
 * such as a U_MEMORY_ALLOCATION_ERROR.
 *
 * @param idna UIDNA instance
 * @param name Input domain name
 * @param length Domain name length, or -1 if NUL-terminated
 * @param dest Destination string buffer
 * @param capacity Destination buffer capacity
 * @param pInfo Output container of IDNA processing details.
 * @param pErrorCode Standard ICU error code. Its input value must
 *                  pass the U_SUCCESS() test, or else the function returns
 *                  immediately. Check for U_FAILURE() on output or use with
 *                  function chaining. (See User Guide for details.)
 * @return destination string length
 * @stable ICU 4.6
 */
U_CAPI int32_t uidna_nameToUnicode(const UIDNA *idna, const UChar *name, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

/* UTF-8 versions of the processing methods --------------------------------- */

/**
 * Converts a single domain name label into its ASCII form for DNS lookup.
 * UTF-8 version of uidna_labelToASCII(), same behavior.
 *
 * @param idna UIDNA instance
 * @param label Input domain name label
 * @param length Label length, or -1 if NUL-terminated
 * @param dest Destination string buffer
 * @param capacity Destination buffer capacity
 * @param pInfo Output container of IDNA processing details.
 * @param pErrorCode Standard ICU error code. Its input value must
 *                  pass the U_SUCCESS() test, or else the function returns
 *                  immediately. Check for U_FAILURE() on output or use with
 *                  function chaining. (See User Guide for details.)
 * @return destination string length
 * @stable ICU 4.6
 */
U_CAPI int32_t uidna_labelToASCII_UTF8(const UIDNA *idna, const char *label, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

/**
 * Converts a single domain name label into its Unicode form for human-readable display.
 * UTF-8 version of uidna_labelToUnicode(), same behavior.
 *
 * @param idna UIDNA instance
 * @param label Input domain name label
 * @param length Label length, or -1 if NUL-terminated
 * @param dest Destination string buffer
 * @param capacity Destination buffer capacity
 * @param pInfo Output container of IDNA processing details.
 * @param pErrorCode Standard ICU error code. Its input value must
 *                  pass the U_SUCCESS() test, or else the function returns
 *                  immediately. Check for U_FAILURE() on output or use with
 *                  function chaining. (See User Guide for details.)
 * @return destination string length
 * @stable ICU 4.6
 */
U_CAPI int32_t uidna_labelToUnicodeUTF8(const UIDNA *idna, const char *label, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

/**
 * Converts a whole domain name into its ASCII form for DNS lookup.
 * UTF-8 version of uidna_nameToASCII(), same behavior.
 *
 * @param idna UIDNA instance
 * @param name Input domain name
 * @param length Domain name length, or -1 if NUL-terminated
 * @param dest Destination string buffer
 * @param capacity Destination buffer capacity
 * @param pInfo Output container of IDNA processing details.
 * @param pErrorCode Standard ICU error code. Its input value must
 *                  pass the U_SUCCESS() test, or else the function returns
 *                  immediately. Check for U_FAILURE() on output or use with
 *                  function chaining. (See User Guide for details.)
 * @return destination string length
 * @stable ICU 4.6
 */
U_CAPI int32_t uidna_nameToASCII_UTF8(const UIDNA *idna, const char *name, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

/**
 * Converts a whole domain name into its Unicode form for human-readable display.
 * UTF-8 version of uidna_nameToUnicode(), same behavior.
 *
 * @param idna UIDNA instance
 * @param name Input domain name
 * @param length Domain name length, or -1 if NUL-terminated
 * @param dest Destination string buffer
 * @param capacity Destination buffer capacity
 * @param pInfo Output container of IDNA processing details.
 * @param pErrorCode Standard ICU error code. Its input value must
 *                  pass the U_SUCCESS() test, or else the function returns
 *                  immediately. Check for U_FAILURE() on output or use with
 *                  function chaining. (See User Guide for details.)
 * @return destination string length
 * @stable ICU 4.6
 */
U_CAPI int32_t uidna_nameToUnicodeUTF8(const UIDNA *idna, const char *name, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

/*
 * IDNA error bit set values.
 * When a domain name or label fails a processing step or does not meet the
 * validity criteria, then one or more of these error bits are set.
 */
enum {
    /**
     * A non-final domain name label (or the whole domain name) is empty.
     * @stable ICU 4.6
     */
    UIDNA_ERROR_EMPTY_LABEL=1,
    /**
     * A domain name label is longer than 63 bytes.
     * (See STD13/RFC1034 3.1. Name space specifications and terminology.)
     * This is only checked in ToASCII operations, and only if the output label is all-ASCII.
     * @stable ICU 4.6
     */
    UIDNA_ERROR_LABEL_TOO_LONG=2,
    /**
     * A domain name is longer than 255 bytes in its storage form.
     * (See STD13/RFC1034 3.1. Name space specifications and terminology.)
     * This is only checked in ToASCII operations, and only if the output domain name is all-ASCII.
     * @stable ICU 4.6
     */
    UIDNA_ERROR_DOMAIN_NAME_TOO_LONG=4,
    /**
     * A label starts with a hyphen-minus ('-').
     * @stable ICU 4.6
     */
    UIDNA_ERROR_LEADING_HYPHEN=8,
    /**
     * A label ends with a hyphen-minus ('-').
     * @stable ICU 4.6
     */
    UIDNA_ERROR_TRAILING_HYPHEN=0x10,
    /**
     * A label contains hyphen-minus ('-') in the third and fourth positions.
     * @stable ICU 4.6
     */
    UIDNA_ERROR_HYPHEN_3_4=0x20,
    /**
     * A label starts with a combining mark.
     * @stable ICU 4.6
     */
    UIDNA_ERROR_LEADING_COMBINING_MARK=0x40,
    /**
     * A label or domain name contains disallowed characters.
     * @stable ICU 4.6
     */
    UIDNA_ERROR_DISALLOWED=0x80,
    /**
     * A label starts with "xn--" but does not contain valid Punycode.
     * That is, an xn-- label failed Punycode decoding.
     * @stable ICU 4.6
     */
    UIDNA_ERROR_PUNYCODE=0x100,
    /**
     * A label contains a dot=full stop.
     * This can occur in an input string for a single-label function.
     * @stable ICU 4.6
     */
    UIDNA_ERROR_LABEL_HAS_DOT=0x200,
    /**
     * An ACE label does not contain a valid label string.
     * The label was successfully ACE (Punycode) decoded but the resulting
     * string had severe validation errors. For example,
     * it might contain characters that are not allowed in ACE labels,
     * or it might not be normalized.
     * @stable ICU 4.6
     */
    UIDNA_ERROR_INVALID_ACE_LABEL=0x400,
    /**
     * A label does not meet the IDNA BiDi requirements (for right-to-left characters).
     * @stable ICU 4.6
     */
    UIDNA_ERROR_BIDI=0x800,
    /**
     * A label does not meet the IDNA CONTEXTJ requirements.
     * @stable ICU 4.6
     */
    UIDNA_ERROR_CONTEXTJ=0x1000,
    /**
     * A label does not meet the IDNA CONTEXTO requirements for punctuation characters.
     * Some punctuation characters "Would otherwise have been DISALLOWED"
     * but are allowed in certain contexts. (RFC 5892)
     * @stable ICU 49
     */
    UIDNA_ERROR_CONTEXTO_PUNCTUATION=0x2000,
    /**
     * A label does not meet the IDNA CONTEXTO requirements for digits.
     * Arabic-Indic Digits (U+066x) must not be mixed with Extended Arabic-Indic Digits (U+06Fx).
     * @stable ICU 49
     */
    UIDNA_ERROR_CONTEXTO_DIGITS=0x4000
};

/**
 * u_strToPunycode() converts Unicode to Punycode.
 *
 * The input string must not contain single, unpaired surrogates.
 * The output will be represented as an array of ASCII code points.
 *
 * The output string is NUL-terminated according to normal ICU
 * string output rules.
 *
 * @param src Input Unicode string.
 *            This function handles a limited amount of code points
 *            (the limit is >=64).
 *            U_INDEX_OUTOFBOUNDS_ERROR is set if the limit is exceeded.
 * @param srcLength Number of UChars in src, or -1 if NUL-terminated.
 * @param dest Output Punycode array.
 * @param destCapacity Size of dest.
 * @param caseFlags Vector of boolean values, one per input UChar,
 *                  indicating that the corresponding character is to be
 *                  marked for the decoder optionally
 *                  uppercasing (true) or lowercasing (false)
 *                  the character.
 *                  ASCII characters are output directly in the case as marked.
 *                  Flags corresponding to trail surrogates are ignored.
 *                  If caseFlags==NULL then input characters are not
 *                  case-mapped.
 * @param pErrorCode ICU in/out error code parameter.
 *                   U_INVALID_CHAR_FOUND if src contains
 *                   unmatched single surrogates.
 *                   U_INDEX_OUTOFBOUNDS_ERROR if src contains
 *                   too many code points.
 * @return Number of ASCII characters in puny.
 *
 * @see u_strFromPunycode
 */
U_CAPI int32_t u_strToPunycode(const UChar *src, int32_t srcLength, UChar *dest, int32_t destCapacity,
		const UBool *caseFlags, UErrorCode *pErrorCode);

/**
 * u_strFromPunycode() converts Punycode to Unicode.
 * The Unicode string will be at most as long (in UChars)
 * than the Punycode string (in chars).
 *
 * @param src Input Punycode string.
 * @param srcLength Length of puny, or -1 if NUL-terminated
 * @param dest Output Unicode string buffer.
 * @param destCapacity Size of dest in number of UChars,
 *                     and of caseFlags in numbers of UBools.
 * @param caseFlags Output array for case flags as
 *                  defined by the Punycode string.
 *                  The caller should uppercase (true) or lowercase (FASLE)
 *                  the corresponding character in dest.
 *                  For supplementary characters, only the lead surrogate
 *                  is marked, and false is stored for the trail surrogate.
 *                  This is redundant and not necessary for ASCII characters
 *                  because they are already in the case indicated.
 *                  Can be NULL if the case flags are not needed.
 * @param pErrorCode ICU in/out error code parameter.
 *                   U_INVALID_CHAR_FOUND if a non-ASCII character
 *                   precedes the last delimiter ('-'),
 *                   or if an invalid character (not a-zA-Z0-9) is found
 *                   after the last delimiter.
 *                   U_ILLEGAL_CHAR_FOUND if the delta sequence is ill-formed.
 * @return Number of UChars written to dest.
 *
 * @see u_strToPunycode
 */
U_CAPI int32_t u_strFromPunycode(const UChar *src, int32_t srcLength, UChar *dest, int32_t destCapacity,
		UBool *caseFlags, UErrorCode *pErrorCode);


// variants, that do not requires uidna_open

U_CAPI int32_t u_labelToASCII(uint32_t options, const UChar *label, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

U_CAPI int32_t u_labelToUnicode(uint32_t options, const UChar *label, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

U_CAPI int32_t u_nameToASCII(uint32_t options, const UChar *name, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

U_CAPI int32_t u_nameToUnicode(uint32_t options, const UChar *name, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

U_CAPI int32_t u_labelToASCII_UTF8(uint32_t options, const char *label, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

U_CAPI int32_t u_labelToUnicodeUTF8(uint32_t options, const char *label, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

U_CAPI int32_t u_nameToASCII_UTF8(uint32_t options, const char *name, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

U_CAPI int32_t u_nameToUnicodeUTF8(uint32_t options, const char *name, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode);

#endif /* MODULES_IDN_UIDNA_H_ */
