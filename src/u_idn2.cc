
#include "u_types.h"
#include "idn2.h"

namespace uidna {

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
		options |= IDN2_USE_STD3_ASCII_RULES;
	}

	if (flags & IDN2_NONTRANSITIONAL) {
		options |= UIDNA_NONTRANSITIONAL_TO_ASCII;
	}

	char *buf = new char[512];

	UIDNAInfo info;
	UErrorCode error = U_ZERO_ERROR;

	u_nameToASCII_UTF8(options, (const char *)src, uprv_strlen((const char *)src), buf, 512, &info, &error);

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
		options |= IDN2_USE_STD3_ASCII_RULES;
	}

	if (flags & IDN2_NONTRANSITIONAL) {
		options |= UIDNA_NONTRANSITIONAL_TO_ASCII;
	}

	char *buf = new char[512];

	UIDNAInfo info;
	UErrorCode error = U_ZERO_ERROR;

	u_nameToASCII_UTF8(options, (const char *)src, uprv_strlen((const char *)src), buf, 512, &info, &error);

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
	if (!req_version || strverscmp(req_version, IDN2_VERSION) <= 0) {
		return IDN2_VERSION;
	}

	return NULL;
}

}
