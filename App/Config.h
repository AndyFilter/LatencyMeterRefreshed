#pragma once

#define NOMINMAX

namespace Config {
	constexpr int JSON_VERSION = 5;

	constexpr size_t MEASUREMENTS_FILE_NAME_LENGTH = 32;

	constexpr char CSV_SEP = ',';
	constexpr size_t TAB_NAME_MAX_SIZE = 64;
} // namespace Config

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#include <cstdint>

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

// inline char* localPath;
