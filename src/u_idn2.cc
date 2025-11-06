
#ifdef UIDNA_SOURCES 
#include "u_types.h"
#else
#include <string>
#endif

#include <unicode/uidna.h>

#include "idn2.h"

#ifndef UIDNA_SOURCES

static int32_t u_labelToASCII(uint32_t options, const UChar *label, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
	*pErrorCode = U_ZERO_ERROR;

	UIDNA *uidna = uidna_openUTS46(options, pErrorCode);
	if (*pErrorCode != U_ZERO_ERROR) {
		return 0;
	}

	int32_t ret = uidna_labelToASCII(uidna, label, length, dest, capacity, pInfo, pErrorCode);
	uidna_close(uidna);
	return ret;
}

static int32_t u_labelToUnicode(uint32_t options, const UChar *label, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
	*pErrorCode = U_ZERO_ERROR;

	UIDNA *uidna = uidna_openUTS46(options, pErrorCode);
	if (*pErrorCode != U_ZERO_ERROR) {
		return 0;
	}

	int32_t ret = uidna_labelToUnicode(uidna, label, length, dest, capacity, pInfo, pErrorCode);
	uidna_close(uidna);
	return ret;
}

static int32_t u_nameToASCII(uint32_t options, const UChar *name, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
	*pErrorCode = U_ZERO_ERROR;

	UIDNA *uidna = uidna_openUTS46(options, pErrorCode);
	if (*pErrorCode != U_ZERO_ERROR) {
		return 0;
	}

	int32_t ret = uidna_nameToASCII(uidna, name, length, dest, capacity, pInfo, pErrorCode);
	uidna_close(uidna);
	return ret;
}

static int32_t u_nameToUnicode(uint32_t options, const UChar *name, int32_t length,
		UChar *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
	*pErrorCode = U_ZERO_ERROR;

	UIDNA *uidna = uidna_openUTS46(options, pErrorCode);
	if (*pErrorCode != U_ZERO_ERROR) {
		return 0;
	}

	int32_t ret = uidna_nameToUnicode(uidna, name, length, dest, capacity, pInfo, pErrorCode);
	uidna_close(uidna);
	return ret;
}

static int32_t u_labelToASCII_UTF8(uint32_t options, const char *label, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
	*pErrorCode = U_ZERO_ERROR;

	UIDNA *uidna = uidna_openUTS46(options, pErrorCode);
	if (*pErrorCode != U_ZERO_ERROR) {
		return 0;
	}

	int32_t ret = uidna_labelToASCII_UTF8(uidna, label, length, dest, capacity, pInfo, pErrorCode);
	uidna_close(uidna);
	return ret;
}

static int32_t u_labelToUnicodeUTF8(uint32_t options, const char *label, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
	*pErrorCode = U_ZERO_ERROR;

	UIDNA *uidna = uidna_openUTS46(options, pErrorCode);
	if (*pErrorCode != U_ZERO_ERROR) {
		return 0;
	}

	int32_t ret = uidna_labelToUnicodeUTF8(uidna, label, length, dest, capacity, pInfo, pErrorCode);
	uidna_close(uidna);
	return ret;
}

static int32_t u_nameToASCII_UTF8(uint32_t options, const char *name, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
	*pErrorCode = U_ZERO_ERROR;

	UIDNA *uidna = uidna_openUTS46(options, pErrorCode);
	if (*pErrorCode != U_ZERO_ERROR) {
		return 0;
	}

	int32_t ret = uidna_nameToASCII_UTF8(uidna, name, length, dest, capacity, pInfo, pErrorCode);
	uidna_close(uidna);
	return ret;
}

static int32_t u_nameToUnicodeUTF8(uint32_t options, const char *name, int32_t length,
		char *dest, int32_t capacity, UIDNAInfo *pInfo, UErrorCode *pErrorCode) {
	*pErrorCode = U_ZERO_ERROR;

	UIDNA *uidna = uidna_openUTS46(options, pErrorCode);
	if (*pErrorCode != U_ZERO_ERROR) {
		return 0;
	}

	int32_t ret = uidna_nameToUnicodeUTF8(uidna, name, length, dest, capacity, pInfo, pErrorCode);
	uidna_close(uidna);
	return ret;
}

#endif

namespace uidna {

static constexpr size_t DefaultBufferSize = 2048;

U_CAPI const char *u_errorName(UErrorCode code);

static int errorToIdn2(UErrorCode err) {
	if (err >= 0) {
		return IDN2_OK;
	} else {
		return -err;
	}
}

extern "C" int idn2_lookup_u8(const uint8_t *src, uint8_t **lookupname, int flags) {
	if (!src) {
		if (lookupname) {
			*lookupname = nullptr;
		}
		return IDN2_OK;
	}
	uint32_t options = UIDNA_CHECK_BIDI | UIDNA_CHECK_CONTEXTJ | UIDNA_CHECK_CONTEXTO; // IDN2008

	if (flags & IDN2_NO_TR46) {
		options = 0;
	}

	if (flags & IDN2_USE_STD3_ASCII_RULES) {
		options |= UIDNA_USE_STD3_RULES;
	}

	if (flags & IDN2_NONTRANSITIONAL) {
		options |= UIDNA_NONTRANSITIONAL_TO_ASCII;
	}

	char *buf = new char[DefaultBufferSize];

	UIDNAInfo info;
	UErrorCode error = U_ZERO_ERROR;

	u_nameToASCII_UTF8(options, (const char *)src, std::char_traits<char>::length((const char *)src), buf, 512, &info, &error);

	if (error == U_ZERO_ERROR && info.errors == 0) {
		if (lookupname) {
			*lookupname = (uint8_t *)buf;
		} else {
			delete [] buf;
		}
		return IDN2_OK;
	} else {
		if (error != U_ZERO_ERROR) {
			delete [] buf;
			return errorToIdn2(error);
		} else {
			if (info.errors == UIDNA_ERROR_EMPTY_LABEL) {
				if (lookupname) {
					*lookupname = (uint8_t *)buf;
				} else {
					delete [] buf;
				}
				return IDN2_OK;
			}
			return -info.errors;
		}
	}
}

extern "C" int idn2_lookup_ul(const char *src, char **lookupname, int flags) {
	if (!src) {
		if (lookupname) {
			*lookupname = nullptr;
		}
		return IDN2_OK;
	}
	uint32_t options = UIDNA_CHECK_BIDI | UIDNA_CHECK_CONTEXTJ | UIDNA_CHECK_CONTEXTO; // IDN2008

	if (flags & IDN2_NO_TR46) {
		options = 0;
	}

	if (flags & IDN2_USE_STD3_ASCII_RULES) {
		options |= UIDNA_USE_STD3_RULES;
	}

	if (flags & IDN2_NONTRANSITIONAL) {
		options |= UIDNA_NONTRANSITIONAL_TO_ASCII;
	}

	char *buf = new char[DefaultBufferSize];

	UIDNAInfo info;
	UErrorCode error = U_ZERO_ERROR;

	u_nameToASCII_UTF8(options, (const char *)src, std::char_traits<char>::length((const char *)src), buf, 512, &info, &error);

	if (error == U_ZERO_ERROR && info.errors == 0) {
		if (lookupname) {
			*lookupname = buf;
		} else {
			delete [] buf;
		}
		return IDN2_OK;
	} else {
		if (error != U_ZERO_ERROR) {
			delete [] buf;
			return errorToIdn2(error);
		} else {
			if (info.errors == UIDNA_ERROR_EMPTY_LABEL) {
				if (lookupname) {
					*lookupname = buf;
				} else {
					delete [] buf;
				}
				return IDN2_OK;
			}
			return -info.errors;
		}
	}
}

extern "C" int idn2_to_unicode_8z8z(const char *src, char **lookupname, int flags) {
	uint32_t options = UIDNA_CHECK_BIDI | UIDNA_CHECK_CONTEXTJ | UIDNA_CHECK_CONTEXTO; // IDN2008

	if (flags & IDN2_NO_TR46) {
		options = 0;
	}

	if (flags & IDN2_USE_STD3_ASCII_RULES) {
		options |= UIDNA_USE_STD3_RULES;
	}

	if (flags & IDN2_NONTRANSITIONAL) {
		options |= UIDNA_NONTRANSITIONAL_TO_UNICODE;
	}

	char *buf = new char[DefaultBufferSize];

	UIDNAInfo info;
	UErrorCode error = U_ZERO_ERROR;

	auto retLen = u_nameToUnicodeUTF8(0, (const char *)src, std::char_traits<char>::length((const char *)src), buf, 2048, &info, &error);

	if (error == U_ZERO_ERROR && info.errors == 0) {
		if (lookupname) {
			*lookupname = buf;
		} else {
			delete [] buf;
		}
		return IDN2_OK;
	} else {
		if (error != U_ZERO_ERROR) {
			delete [] buf;
			return errorToIdn2(error);
		} else {
			if (info.errors == UIDNA_ERROR_EMPTY_LABEL) {
				if (lookupname) {
					*lookupname = buf;
				} else {
					delete [] buf;
				}
				return IDN2_OK;
			}
			return -info.errors;
		}
	}
}

extern "C" const char* idn2_strerror(int rc) {
	switch (rc) {
	case IDN2_OK:
		return "success";
		break;
	default:
		return u_errorName(UErrorCode(-rc));
		break;
	}
	return nullptr;
}

extern "C" const char* idn2_strerror_name(int rc) {
	switch (rc) {
	case IDN2_OK:
		return "IDN2_OK";
		break;
	default:
		return u_errorName(UErrorCode(-rc));
		break;
	}
	return nullptr;
}

extern "C" void idn2_free(void *ptr) {
	if (ptr) {
		delete [] (char *)ptr;
	}
}

extern "C" const char* idn2_check_version(const char *req_version) {
#if __APPLE__
	if (!req_version || strcmp(req_version, IDN2_VERSION) <= 0) {
#elif __ANDROID__
	if (!req_version || strcmp(req_version, IDN2_VERSION) <= 0) {
#else
	if (!req_version || strverscmp(req_version, IDN2_VERSION) <= 0) {
#endif
		return IDN2_VERSION;
	}

	return NULL;
}

}
