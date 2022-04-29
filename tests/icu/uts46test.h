
#include "unicode/uidna.h"
#include <stddef.h>
#include <stdarg.h>
#include <mutex>

#include "u_unistr.h"
#include "u_uts46.h"

namespace uidna {

class UTS46Test {
public:
	UTS46Test();
	~UTS46Test() { }

	void log(const UnicodeString &message);
	void logln(const UnicodeString &message);
	void logln(void);

	void err(void);
	void err(const UnicodeString &message);
	void errln(const UnicodeString &message);

	void dataerr(const UnicodeString &message);
	void dataerrln(const UnicodeString &message);

	void log(const char *fmt, ...);
	void logln(const char *fmt, ...);
    void errln(const char *fmt, ...);
    void dataerrln(const char *fmt, ...);

	UBool assertTrue(const char *message, UBool condition, UBool quiet = false,
			UBool possibleDataError = false, const char *file = NULL, int line = 0);
	UBool assertFalse(const char *message, UBool condition, UBool quiet = false,
			UBool possibleDataError = false);
	UBool assertEquals(const char *message, const UnicodeString &expected, const UnicodeString &actual,
			UBool possibleDataError = false);
	UBool assertEquals(const char *message, int64_t expected, int64_t actual);

	int32_t IncErrorCount(void);
	int32_t IncDataErrorCount(void);

	void runIndexedTest(int32_t index, UBool exec, const char *&name);
	void TestAPI();
	void TestNotSTD3();
	void TestInvalidPunycodeDigits();
	void TestACELabelEdgeCases();
	void TestTooLong();
	void TestSomeCases();
	void IdnaTest();

	UBool runTestLoop(char *testname, char *par, char *baseName);

	void checkIdnaTestResult(const char *line, const char *type,
			const UnicodeString &expected, const UnicodeString &result, const char *status, const IDNAInfo &info);
	void idnaTestOneLine(char *fields[][2], UErrorCode &errorCode);

private:
	static UnicodeString& prettify(const UnicodeString &source, UnicodeString &target);
	static UnicodeString prettify(const UnicodeString &source, UBool parseBackslash = false);

	void LL_message(UnicodeString message, UBool newline);

	UBool verbose = 1;
	UBool no_err_msg = 0;
	UBool warn_on_missing_data = 0;
	int32_t errorCount = 0;
	int32_t dataErrorCount = 0;
	int32_t LL_indentlevel = 0;
	void *testoutfp = nullptr;

	char basePath[1024];
	char currName[1024]; // current test name

	UErrorCode code = U_ZERO_ERROR;
	UTS46 trans;
	UTS46 nontrans;
};

class ErrorCode {
public:
	ErrorCode() : errorCode(U_ZERO_ERROR) { }
	virtual ~ErrorCode();
	operator UErrorCode &() { return errorCode; }
	operator UErrorCode *() { return &errorCode; }
	UBool isSuccess() const { return U_SUCCESS(errorCode); }
	UBool isFailure() const { return U_FAILURE(errorCode); }
	UErrorCode get() const { return errorCode; }
	void set(UErrorCode value) { errorCode = value; }
	UErrorCode reset();
	void assertSuccess() const;
	const char* errorName() const;

protected:
	UErrorCode errorCode;

	virtual void handleFailure() const { }
};

class IcuTestErrorCode: public ErrorCode {
public:
	IcuTestErrorCode(UTS46Test &callingTestClass, const char *callingTestName) :
			testClass(callingTestClass), testName(callingTestName), scopeMessage() {
	}
	virtual ~IcuTestErrorCode();

	// Returns TRUE if isFailure().
	UBool errIfFailureAndReset();
	UBool errIfFailureAndReset(const char *fmt, ...);
	UBool errDataIfFailureAndReset();
	UBool errDataIfFailureAndReset(const char *fmt, ...);
	UBool expectErrorAndReset(UErrorCode expectedError);
	UBool expectErrorAndReset(UErrorCode expectedError, const char *fmt, ...);

	/** Sets an additional message string to be appended to failure output. */
	void setScope(const char *message);
	void setScope(const UnicodeString &message);

protected:
	virtual void handleFailure() const override;

private:
	UTS46Test &testClass;
	const char *const testName;
	UnicodeString scopeMessage;

	void errlog(UBool dataErr, const UnicodeString &mainMessage, const char *extraMessage) const;
};

}
