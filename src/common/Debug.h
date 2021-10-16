#pragma once

// asserts, errors, logs

#if DEBUG_TOOLS
#include <cassert>

#if _MSC_VER
#define ASSERT_MESSAGE(cond, message) (void)(                                            \
            (!!(cond)) ||                                                                \
            (_wassert(_CRT_WIDE(message), _CRT_WIDE(__FILE__), static_cast<unsigned>(__LINE__)), 0) \
        )

// use codecvt as it's easier than windows
#include <codecvt>
#include <string>
#define ERROR_MESSAGE(message) \
do { std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter; \
std::wstring const m = converter.from_bytes(message); \
_wassert(m.c_str(), _CRT_WIDE(__FILE__), static_cast<unsigned>(__LINE__)); } while(false)
#else
#define ASSERT_MESSAGE(cond, message) assert(cond)
#define ERROR_MESSAGE(message) ASSERT_MESSAGE(false, message)
#endif

#define GET_ASSERT_MACRO(_1,_2,NAME,...) NAME
#define kaAssert(...) GET_ASSERT_MACRO(__VA_ARGS__, ASSERT_MESSAGE, assert)(__VA_ARGS__)
#define kaError(message) ERROR_MESSAGE(message)
void kaLog(std::string const& _message);

#include <thread>
inline const std::thread::id g_mainThreadID = std::this_thread::get_id();

#define kaMainThreadAssert(...) kaAssert(g_mainThreadID == std::this_thread::get_id(), "Assert ran on non-main thread"); kaAssert(__VA_ARGS__)

#else

#define kaAssert(...)
#define kaError(message)
#define kaLog(message)
#define kaMainThreadAssert(...)
#define SOLOUD_NO_ASSERTS

#endif

void InitialiseLogging();

#define SOKOL_GENERAL_ASSERT(c) kaMainThreadAssert(c)
#define SOKOL_FETCH_ASSERT(c) kaAssert(c)
#define IM_ASSERT(c) kaMainThreadAssert(c)
#define STB_ASSERT(c) kaAssert(c)
#define STBDS_ASSERT(c) STB_ASSERT(c)
#define STB_HBWANG_ASSERT(c) STB_ASSERT(c)
#define STBI_ASSERT(c) STB_ASSERT(c)
#define STBIR_ASSERT(c) STB_ASSERT(c)
#define STBIW_ASSERT(c) STB_ASSERT(c)
#define STBRP_ASSERT(c) STB_ASSERT(c)
#define STBTE_ASSERT(c) STB_ASSERT(c)