/* Generated by cmake */

#ifndef CPL_CONFIG_H
#define CPL_CONFIG_H

#ifdef _MSC_VER
# ifndef CPL_DISABLE_STDCALL
#  define CPL_STDCALL __stdcall
# endif
#endif

/* --prefix directory for GDAL install */
#define GDAL_PREFIX "D:/MyHub/build/gdal-3.5.3/install_dir"

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `unsigned long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG 4

/* The size of `void*', as computed by sizeof. */
#define SIZEOF_VOIDP 8

/* Define to 1, if you have LARGEFILE64_SOURCE */
/* #undef VSI_NEED_LARGEFILE64_SOURCE */

/* Define to 1 if you want to use the -fvisibility GCC flag */
/* #undef USE_GCC_VISIBILITY_FLAG */

/* Define to 1 if GCC atomic builtins are available */
/* #undef HAVE_GCC_ATOMIC_BUILTINS */

/* Define to 1 if GCC bswap builtins are available */
/* #undef HAVE_GCC_BSWAP */

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to name of 64bit stat structure */
#define VSI_STAT64_T __stat64

/* Define to 1 if you have the `std::isnan' function. */
#define HAVE_STD_IS_NAN 1


#ifdef GDAL_COMPILATION

/* Define if you want to use pthreads based multiprocessing support */
/* #undef CPL_MULTIPROC_PTHREAD */

/* Define to 1 if you have the `PTHREAD_MUTEX_RECURSIVE' constant. */
/* #undef HAVE_PTHREAD_MUTEX_RECURSIVE */

/* Define to 1 if you have the `PTHREAD_MUTEX_ADAPTIVE_NP' constant. */
/* #undef HAVE_PTHREAD_MUTEX_ADAPTIVE_NP */

/* Define to 1 if you have the `pthread_spinlock_t' type. */
/* #undef HAVE_PTHREAD_SPINLOCK */

/* Define to 1 if you have the `pthread_atfork' function. */
/* #undef HAVE_PTHREAD_ATFORK */

/* Define to 1 if you have the 5 args `mremap' function. */
/* #undef HAVE_5ARGS_MREMAP */

/* Define to 1 if you have the `getrlimit' function. */
/* #undef HAVE_GETRLIMIT */

/* Define to 1 if you have the `RLIMIT_AS' constant. */
/* #undef HAVE_RLIMIT_AS */

/* Define to 1 if you have the <direct.h> header file. */
#define HAVE_DIRECT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define to 1 if the system has the type `__uint128_t'. */
/* #undef HAVE_UINT128_T */

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
/* #undef HAVE_SYS_STAT_H */

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define to 1 if you have the <xlocale.h> header file. */
/* #undef HAVE_XLOCALE_H */

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `readlink' function. */
/* #undef HAVE_READLINK */

/* Define to 1 if you have the `posix_spawnp' function. */
/* #undef HAVE_POSIX_SPAWNP */

/* Define to 1 if you have the `posix_memalign' function. */
/* #undef HAVE_POSIX_MEMALIGN */

/* Define to 1 if you have the `vfork' function. */
/* #undef HAVE_VFORK */

/* Define to 1 if you have the `mmap' function. */
/* #undef HAVE_MMAP */

/* Define to 1 if you have the `sigaction' function. */
/* #undef HAVE_SIGACTION */

/* Define to 1 if you have the `statvfs' function. */
/* #undef HAVE_STATVFS */

/* Define to 1 if you have the `statvfs64' function. */
/* #undef HAVE_STATVFS64 */

/* Define to 1 if you have the `lstat' function. */
/* #undef HAVE_LSTAT */

/* For .cpp files, define as const if the declaration of iconv() needs const. */
#define ICONV_CPP_CONST 

/* Define for Mac OSX Framework build */
/* #undef MACOSX_FRAMEWORK */

/* Define to 1 if you have fseek64, ftell64 */
/* #undef UNIX_STDIO_64 */

/* Define to name of 64bit fopen function */
/* #undef VSI_FOPEN64 */

/* Define to name of 64bit ftruncate function */
/* #undef VSI_FTRUNCATE64 */

/* Define to name of 64bit fseek func */
/* #undef VSI_FSEEK64 */

/* Define to name of 64bit ftell func */
/* #undef VSI_FTELL64 */

/* Define to name of 64bit stat function */
#define VSI_STAT64 _stat64

/* Use this file to override settings in instances where you're doing FAT compiles
   on Apple.  It is currently off by default because it doesn't seem to work with
   newish ( XCode >= 3/28/11) XCodes */
/* #include "cpl_config_extras.h" */


/* Define to 1 if you have the _SC_PHYS_PAGES' constant. */
/* #undef HAVE_SC_PHYS_PAGES */

/* Define to 1 if you have the `uselocale' function. */
/* #undef HAVE_USELOCALE */

/* Define to 1 if libc don't deprecate sprintf */
#ifndef DONT_DEPRECATE_SPRINTF
/* #undef DONT_DEPRECATE_SPRINTF */
#endif

/* Define to 1 if the compiler supports -Wzero-as-null-pointer-constant */
/* #undef HAVE_GCC_WARNING_ZERO_AS_NULL_POINTER_CONSTANT */

/* Define if building a static windows lib */
/* #undef CPL_DISABLE_DLL */

/* Define to 1 if you have the <atlbase.h> header file. */
#define HAVE_ATLBASE_H 1

#endif /* GDAL_COMPILATION */

#endif
