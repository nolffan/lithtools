/*
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Copyright (c) 1996-1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

// talon: the SGI STL (3.2) that NOLF Tools 001's PreWorld.cpp was built
// with (progress/Processor_nolf_map.md): only what PreWorld.cpp uses,
// list and a priority_queue over a deque, each piece written back from the
// shipped code (0x449c8c..0x44ce10), with SGI's names and layout. This is
// stl_config.h's Visual C++ 6 case, and no more.

#ifndef __STL_CONFIG_H
# define __STL_CONFIG_H

// talon: which STL this is (PreWorld.h tests it).
# define __STL_SGI_3_2

// Visual C++ 6: no partial specialization of class templates, so the
// containers take SGI's own allocator (alloc) and the iterator queries
// (value_type(), distance_type(), iterator_category()) are overloads.
# define __STL_NO_DRAND48
# define __STL_STATIC_CONST_INIT_BUG
# define __STL_NON_TYPE_TMPL_PARAM_BUG
# define __SGI_STL_NO_ARROW_OPERATOR
# define __STL_LIMITED_DEFAULT_TEMPLATES
# ifdef _CPPUNWIND
#   define __STL_USE_EXCEPTIONS
# endif
# ifdef _MT
#   define __STL_WIN32THREADS
# endif

# if defined(__STL_WIN32THREADS)
#   define __STL_THREADS
#   define __STL_VOLATILE volatile
# else
#   define __STL_VOLATILE
# endif

# define __STL_DEFAULT_ALLOCATOR(T) alloc

// Global namespace: no __STL_HAS_NAMESPACES for this compiler.
# define __STL_BEGIN_NAMESPACE
# define __STL_END_NAMESPACE

# ifdef __STL_USE_EXCEPTIONS
#   define __STL_TRY try
#   define __STL_CATCH_ALL catch(...)
#   define __STL_THROW(x) throw x
#   define __STL_RETHROW throw
#   define __STL_NOTHROW throw()
#   define __STL_UNWIND(action) catch(...) { action; throw; }
# else
#   define __STL_TRY
#   define __STL_CATCH_ALL if (false)
#   define __STL_THROW(x)
#   define __STL_RETHROW
#   define __STL_NOTHROW
#   define __STL_UNWIND(action)
# endif

#pragma warning(disable : 4786)

#endif /* __STL_CONFIG_H */
