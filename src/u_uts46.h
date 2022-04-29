// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
*******************************************************************************
*   Copyright (C) 2010-2012, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  idna.h
*   encoding:   UTF-8
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010mar05
*   created by: Markus W. Scherer
*/

#ifndef MODULES_IDN_UIDNAUTS46_H_
#define MODULES_IDN_UIDNAUTS46_H_

#include "u_unistr.h"
#include "u_norm2.h"

namespace uidna {

class IDNAInfo;
class UnicodeString;

class UTS46 {
public:
    UTS46(uint32_t options, UErrorCode &errorCode);
    ~UTS46();

	UnicodeString& labelToASCII(const UnicodeString &label, UnicodeString &dest, IDNAInfo &info, UErrorCode &errorCode) const;
	UnicodeString& labelToUnicode(const UnicodeString &label, UnicodeString &dest, IDNAInfo &info, UErrorCode &errorCode) const;
	UnicodeString& nameToASCII(const UnicodeString &name, UnicodeString &dest, IDNAInfo &info, UErrorCode &errorCode) const;
	UnicodeString& nameToUnicode(const UnicodeString &name, UnicodeString &dest, IDNAInfo &info, UErrorCode &errorCode) const;

	void labelToASCII_UTF8(StringPiece label, ByteSink &dest, IDNAInfo &info, UErrorCode &errorCode) const;
	void labelToUnicodeUTF8(StringPiece label, ByteSink &dest, IDNAInfo &info, UErrorCode &errorCode) const;
	void nameToASCII_UTF8(StringPiece name, ByteSink &dest, IDNAInfo &info, UErrorCode &errorCode) const;
	void nameToUnicodeUTF8(StringPiece name, ByteSink &dest, IDNAInfo &info, UErrorCode &errorCode) const;

private:
	UnicodeString& process(const UnicodeString &src, UBool isLabel, UBool toASCII, UnicodeString &dest, IDNAInfo &info, UErrorCode &errorCode) const;
	void processUTF8(StringPiece src, UBool isLabel, UBool toASCII, ByteSink &dest, IDNAInfo &info, UErrorCode &errorCode) const;
	UnicodeString& processUnicode(const UnicodeString &src, int32_t labelStart, int32_t mappingStart, UBool isLabel, UBool toASCII, UnicodeString &dest, IDNAInfo &info, UErrorCode &errorCode) const;

	// returns the new dest.length()
	int32_t mapDevChars(UnicodeString &dest, int32_t labelStart, int32_t mappingStart, UErrorCode &errorCode) const;

	// returns the new label length
	int32_t processLabel(UnicodeString &dest, int32_t labelStart, int32_t labelLength, UBool toASCII, IDNAInfo &info, UErrorCode &errorCode) const;
	int32_t markBadACELabel(UnicodeString &dest, int32_t labelStart, int32_t labelLength, UBool toASCII, IDNAInfo &info, UErrorCode &errorCode) const;
	void checkLabelBiDi(const UChar *label, int32_t labelLength, IDNAInfo &info) const;
	UBool isLabelOkContextJ(const UChar *label, int32_t labelLength) const;
	void checkLabelContextO(const UChar *label, int32_t labelLength, IDNAInfo &info) const;

	const ComposeNormalizer2 *uts46Norm2;  // uts46.nrm
	uint32_t options;
};

class IDNAInfo {
public:
	IDNAInfo() : errors(0), labelErrors(0), isTransDiff(false), isBiDi(false), isOkBiDi(true) {}

	bool hasErrors() const { return errors!=0; }
	uint32_t getErrors() const { return errors; }

	bool isTransitionalDifferent() const { return isTransDiff; }

private:
	friend class UTS46;

	IDNAInfo(const IDNAInfo &other);  // no copying
	IDNAInfo &operator=(const IDNAInfo &other);  // no copying

	void reset() {
		errors=labelErrors=0;
		isTransDiff=false;
		isBiDi=false;
		isOkBiDi=true;
	}

	uint32_t errors, labelErrors;
	bool isTransDiff;
	bool isBiDi;
	bool isOkBiDi;
};

}

#endif /* MODULES_IDN_UIDNAUTS46_H_ */
