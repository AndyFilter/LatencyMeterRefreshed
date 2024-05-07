#pragma once

const int json_version = 5;

const size_t MEASUREMENTS_FILE_NAME_LENGTH = 32;
const char invalidPathCharacters[]{'<', '>', ':', '"', '\\', '/', '|', '?', '*'};
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

//#ifdef __linux__
////const T& _min (const int a, const int b) {
////    return b >= a ? a : b;
////}
//#define _min(a,b) \
//   ({ __typeof__ (a) _a = (a); \
//       __typeof__ (b) _b = (b); \
//     _a < _b ? _a : _b; })
//
//#define _max(a,b) \
//   ({ __typeof__ (a) _a = (a); \
//       __typeof__ (b) _b = (b); \
//     _a > _b ? _a : _b; })
//
//#endif

#ifdef __linux__
	#ifndef UINT
	#define UINT unsigned int
	#endif

	#ifndef BYTE
	#define BYTE unsigned char
	#endif
#endif

// OS specific string / file formats:
#ifdef _WIN32
#define OS_SPEC_PATH_STR std::wstring
#define OS_SPEC_PATH_CHAR wchar_t
#define OS_SPEC_PATH(t) L##t
#else
#define OS_SPEC_PATH_STR std::string
#define OS_SPEC_PATH_CHAR char
#define OS_SPEC_PATH(t) u8##t
#endif

#ifndef NDEBUG
#define _DEBUG
#endif

//inline char* localPath;