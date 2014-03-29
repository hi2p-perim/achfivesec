#pragma once
#ifndef LIB_FW_COMMON_H
#define LIB_FW_COMMON_H

// C++ compiler is required
#ifndef __cplusplus
	#error "C++ compiler is required"
#endif

// Debug mode?
#ifndef NDEBUG
	#define FW_DEBUG_MODE
#endif

// --------------------------------------------------------------------------------

// Platform
#ifdef _WIN32
	#define FW_PLATFORM_WINDOWS
#elif defined(__linux)
	#define FW_PLATFORM_LINUX
#else
	#error "Unsupportted platform"
#endif

#ifdef FW_PLATFORM_WINDOWS
	#define NOMINMAX
	#define WIN32_LEAN_AND_MEAN
	#pragma warning(disable:4996)	// _SCL_SECURE_NO_WARNINGS
	#pragma warning(disable:4819)	// Character that cannot be represented
	#pragma warning(disable:4290)	// Exception specification ignored
#endif

// Compiler and architecture
#ifdef _MSC_VER
	// Microsoft Visual C++
	#define FW_COMPILER_MSVC
	#ifdef _M_IX86
		#define FW_ARCH_X86
	#elif defined(_M_X64)
		#define FW_ARCH_X64
	#else
		#error "Unsupportted architecture"
	#endif
#elif (defined(__GNUC__) || defined(__MINGW32__))
	// GNU GCC
	#define FW_COMPILER_GCC
	#ifdef __i386__
		#define FW_ARCH_X86
	#elif defined(__x86_64__)
		#define FW_ARCH_X64
	#else
		#error "Unsupportted architecture"
	#endif
#else
	#error "Unsupportted compiler"
#endif

// --------------------------------------------------------------------------------

// Library import or export
#ifdef FW_COMPILER_MSVC
	#ifdef FW_EXPORTS
		#define FW_PUBLIC_API __declspec(dllexport)
	#else
		#define FW_PUBLIC_API __declspec(dllimport)
	#endif
	#define FW_HIDDEN_API
#elif defined(FW_COMPILER_GCC)
	#ifdef FW_EXPORTS
		#define FW_PUBLIC_API __attribute__ ((visibility("default")))
		#define FW_HIDDEN_API __attribute__ ((visibility("hidden")))
	#else
		#define FW_PUBLIC_API
		#define FW_HIDDEN_API
	#endif
#else
	#define FW_PUBLIC_API
	#define FW_HIDDEN_API
#endif

// In the debug mode, the hidden API is exposed
#ifdef RF_DEBUG_MODE
	#undef RF_HIDDEN_API
	#define RF_HIDDEN_API RF_PUBLIC_API
#endif

// Plugin export
#ifdef FW_COMPILER_MSVC
	#define FW_PLUGIN_API __declspec(dllexport)
#elif defined(FW_COMPILER_GCC)
	#define FW_PLUGIN_API __attribute__ ((visibility("default")))
#else
	#define FW_PLUGIN_API 
#endif

// --------------------------------------------------------------------------------

// Force inline
#ifdef FW_COMPILER_MSVC
	#define FW_FORCE_INLINE __forceinline
#elif defined(FW_COMPILER_GCC)
	#define FW_FORCE_INLINE inline __attribute__((always_inline))
#endif

// Alignment
#ifdef FW_COMPILER_MSVC
	#define FW_ALIGN(x) __declspec(align(x))
#elif defined(FW_COMPILER_GCC)
	#define FW_ALIGN(x) __attribute__((aligned(x)))
#endif
#define FW_ALIGN_16 FW_ALIGN(16)
#define FW_ALIGN_32 FW_ALIGN(32)

// --------------------------------------------------------------------------------

// Namespace
#define FW_NAMESPACE_BEGIN namespace fw {
#define FW_NAMESPACE_END }

// --------------------------------------------------------------------------------

#define FW_SAFE_DELETE(val) if ((val) != NULL ) { delete (val); (val) = NULL; }
#define FW_SAFE_DELETE_ARRAY(val) if ((val) != NULL ) { delete[] (val); (val) = NULL; }

#define FW_DISABLE_COPY_AND_MOVE(TypeName) \
	TypeName(const TypeName &); \
	TypeName(TypeName&&); \
	void operator=(const TypeName&); \
	void operator=(TypeName&&)

#endif // LIB_FW_COMMON_H
