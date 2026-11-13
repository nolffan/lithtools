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
// talon: SGI STL 3.2's stl_queue.h, the part PreWorld.cpp's code uses:
// priority_queue (push 0x449cf2, pop with the same unwind). No default
// template arguments for this compiler (__STL_LIMITED_DEFAULT_TEMPLATES,
// stl_config.h), so its user names the deque.

#ifndef __SGI_STL_INTERNAL_QUEUE_H
#define __SGI_STL_INTERNAL_QUEUE_H

__STL_BEGIN_NAMESPACE

#ifndef __STL_LIMITED_DEFAULT_TEMPLATES
template <class _Tp, class _Sequence = deque<_Tp>, 
          class _Compare = less<typename _Sequence::value_type> >
#else
template <class _Tp, class _Sequence, class _Compare>
#endif
class  priority_queue {
public:
  typedef typename _Sequence::value_type      value_type;
  typedef typename _Sequence::size_type       size_type;
  typedef          _Sequence                  container_type;

  typedef typename _Sequence::reference       reference;
  typedef typename _Sequence::const_reference const_reference;
protected:
  _Sequence c;
  _Compare comp;
public:
  priority_queue() : c() {}
  explicit priority_queue(const _Compare& __x) :  c(), comp(__x) {}

  bool empty() const { return c.empty(); }
  size_type size() const { return c.size(); }
  const_reference top() const { return c.front(); }
  void push(const value_type& __x) {
    __STL_TRY {
      c.push_back(__x); 
      push_heap(c.begin(), c.end(), comp);
    }
    __STL_UNWIND(c.clear());
  }
  void pop() {
    __STL_TRY {
      pop_heap(c.begin(), c.end(), comp);
      c.pop_back();
    }
    __STL_UNWIND(c.clear());
  }
};

__STL_END_NAMESPACE

#endif /* __SGI_STL_INTERNAL_QUEUE_H */
