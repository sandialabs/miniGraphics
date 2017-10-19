/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2003 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 * the U.S. Government retains certain rights in this software.
 *
 * This source code is released under the New BSD License.
 */

#ifndef __IceTConfig_h
#define __IceTConfig_h

#ifndef WIN32
/* #undef WIN32 */
#endif

/* #undef ICET_BUILD_SHARED_LIBS */

#ifdef WIN32
#include <windows.h>
#endif

#if defined(WIN32) && defined(ICET_BUILD_SHARED_LIBS)
#  ifdef IceTCore_EXPORTS
#    define ICET_EXPORT __declspec( dllexport )
#    define ICET_STRATEGY_EXPORT __declspec( dllexport )
#  else
#    define ICET_EXPORT __declspec( dllimport )
#    define ICET_STRATEGY_EXPORT __declspec( dllimport )
#  endif
#  ifdef IceTGL_EXPORTS
#    define ICET_GL_EXPORT __declspec( dllexport )
#  else
#    define ICET_GL_EXPORT __declspec( dllimport )
#  endif
#  ifdef IceTMPI_EXPORTS
#    define ICET_MPI_EXPORT __declspec( dllexport )
#  else
#    define ICET_MPI_EXPORT __declspec( dllimport )
#  endif
#else /* WIN32 && SHARED_LIBS */
#  define ICET_EXPORT
#  define ICET_GL_EXPORT
#  define ICET_STRATEGY_EXPORT
#  define ICET_MPI_EXPORT
#endif /* WIN32 && SHARED_LIBS */

#define ICET_MAJOR_VERSION      2
#define ICET_MINOR_VERSION      1
#define ICET_PATCH_VERSION      1
#define ICET_VERSION            "2.1.1"

#define ICET_SIZEOF_CHAR           1
#define ICET_SIZEOF_SHORT          2
#define ICET_SIZEOF_INT            4
#define ICET_SIZEOF_LONG           8
#define ICET_SIZEOF_LONG_LONG      8
/* #undef ICET_SIZEOF___INT64 */
#define ICET_SIZEOF_FLOAT          4
#define ICET_SIZEOF_DOUBLE         8
#define ICET_SIZEOF_VOID_P         8

#if ICET_SIZEOF_CHAR == 1
typedef char IceTInt8;
typedef unsigned char IceTUnsignedInt8;
#else
#error "No valid data type for 8 bit integers found."
#endif

#if ICET_SIZEOF_SHORT == 2
typedef short IceTInt16;
typedef unsigned short IceTUnsignedInt16;
#else
#error "No valid data type for 16 bit integers founds."
#endif

#if ICET_SIZEOF_INT == 4
typedef int IceTInt32;
typedef unsigned int IceTUnsignedInt32;
#elif ICET_SIZEOF_LONG == 4
typedef long IceTInt32;
typedef unsigned long IceTUnsignedInt32;
#elif ICET_SIZEOF_SHORT == 4
typedef short IceTInt32;
typedef unsigned short IceTUnsignedInt32;
#else
#error "No valid data type for 32 bit integers found."
#endif

#if ICET_SIZEOF_INT == 8
typedef int IceTInt64;
typedef unsigned int IceTUnsignedInt64;
#elif ICET_SIZEOF_LONG == 8
typedef long IceTInt64;
typedef unsigned long IceTUnsignedInt64;
#elif defined(ICET_SIZEOF_LONG_LONG) && (ICET_SIZEOF_LONG_LONG == 8)
typedef long long IceTInt64;
typedef unsigned long long IceTUnsignedInt64;
#elif defined(ICET_SIZEOF___INT64) && (ICET_SIZEOF___INT64 == 8)
typedef __int64 IceTInt64;
typedef unsigned __int64 IceTUnsignedInt64;
#else
#error "No valid data type for 64 bit integers found."
#endif

#if ICET_SIZEOF_FLOAT == 4
typedef float IceTFloat32;
#else
#error "No valid data type for 32 bit floating point found."
#endif

#if ICET_SIZEOF_DOUBLE == 8
typedef double IceTFloat64;
#else
#error "No valid data type for 64 bit floating point found."
#endif

#if ICET_SIZEOF_VOID_P == 4
typedef IceTInt32 IceTPointerArithmetic;
#elif ICET_SIZEOF_VOID_P == 8
typedef IceTInt64 IceTPointerArithmetic;
#else
#error "Unexpected pointer size."
#endif

#define ICET_MAGIC_K_DEFAULT            8
#define ICET_MAX_IMAGE_SPLIT_DEFAULT    512

/* #undef ICET_USE_MPE */

#endif /*__IceTConfig_h*/
