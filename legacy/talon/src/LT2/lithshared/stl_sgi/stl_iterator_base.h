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
// talon: SGI STL 3.2's stl_iterator_base.h, the part PreWorld.cpp's code
// uses: the tags, and (no partial specialization, stl_config.h) the
// iterator queries as overloads.

#ifndef __SGI_STL_INTERNAL_ITERATOR_BASE_H
#define __SGI_STL_INTERNAL_ITERATOR_BASE_H

#include <stddef.h>

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

template <class _Tp>
inline random_access_iterator_tag iterator_category(const _Tp*)
  { return random_access_iterator_tag(); }

template <class _Tp>
inline _Tp* value_type(const _Tp*) { return (_Tp*)(0); }

template <class _Tp>
inline ptrdiff_t* distance_type(const _Tp*) { return (ptrdiff_t*)(0); }

#define __ITERATOR_CATEGORY(__i) iterator_category(__i)
#define __DISTANCE_TYPE(__i)     distance_type(__i)
#define __VALUE_TYPE(__i)        value_type(__i)

#endif /* __SGI_STL_INTERNAL_ITERATOR_BASE_H */
