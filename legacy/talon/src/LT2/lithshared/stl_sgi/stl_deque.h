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
// talon: SGI STL 3.2's stl_deque.h, the part PreWorld.cpp's code uses
// (the priority_queue's deque<PointElement>, 42 to a 504-byte node: the
// iterator's += 0x44c830, - 0x44c7ac, the base's _M_initialize_map
// 0x44b097, _M_create_nodes 0x44bf1c, the destructors 0x44b05a and
// 0x449c8c, clear 0x44b177, _M_push_back_aux 0x44b1da, _M_pop_back_aux
// 0x44b248, _M_reallocate_map 0x44bf81).

#ifndef __SGI_STL_INTERNAL_DEQUE_H
#define __SGI_STL_INTERNAL_DEQUE_H

__STL_BEGIN_NAMESPACE 

// Note: this function is simply a kludge to work around several compilers'
//  bugs in handling constant expressions.
inline size_t
__deque_buf_size(size_t __n, size_t __size)
{
  return __n != 0 ? __n : (__size < 512 ? size_t(512 / __size) : size_t(1));
}

// The iterator has no buffer-size parameter: __STL_NON_TYPE_TMPL_PARAM_BUG
// (stl_config.h).
template <class _Tp, class _Ref, class _Ptr>
struct _Deque_iterator {
  typedef _Deque_iterator<_Tp, _Tp&, _Tp*>             iterator;
  typedef _Deque_iterator<_Tp, const _Tp&, const _Tp*> const_iterator;
  static size_t 
    _S_buffer_size() { return __deque_buf_size(0, sizeof(_Tp)); }

  typedef random_access_iterator_tag iterator_category;
  typedef _Tp value_type;
  typedef _Ptr pointer;
  typedef _Ref reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef _Tp** _Map_pointer;

  typedef _Deque_iterator _Self;

  _Tp* _M_cur;
  _Tp* _M_first;
  _Tp* _M_last;
  _Map_pointer _M_node;

  _Deque_iterator(_Tp* __x, _Map_pointer __y) 
    : _M_cur(__x), _M_first(*__y),
      _M_last(*__y + _S_buffer_size()), _M_node(__y) {}
  _Deque_iterator() : _M_cur(0), _M_first(0), _M_last(0), _M_node(0) {}
  _Deque_iterator(const iterator& __x)
    : _M_cur(__x._M_cur), _M_first(__x._M_first), 
      _M_last(__x._M_last), _M_node(__x._M_node) {}

  reference operator*() const { return *_M_cur; }
#ifndef __SGI_STL_NO_ARROW_OPERATOR
  pointer operator->() const { return _M_cur; }
#endif /* __SGI_STL_NO_ARROW_OPERATOR */

  difference_type operator-(const _Self& __x) const {
    return difference_type(_S_buffer_size()) * (_M_node - __x._M_node - 1) +
      (_M_cur - _M_first) + (__x._M_last - __x._M_cur);
  }

  _Self& operator++() {
    ++_M_cur;
    if (_M_cur == _M_last) {
      _M_set_node(_M_node + 1);
      _M_cur = _M_first;
    }
    return *this; 
  }
  _Self operator++(int)  {
    _Self __tmp = *this;
    ++*this;
    return __tmp;
  }

  _Self& operator--() {
    if (_M_cur == _M_first) {
      _M_set_node(_M_node - 1);
      _M_cur = _M_last;
    }
    --_M_cur;
    return *this;
  }
  _Self operator--(int) {
    _Self __tmp = *this;
    --*this;
    return __tmp;
  }

  _Self& operator+=(difference_type __n)
  {
    difference_type __offset = __n + (_M_cur - _M_first);
    if (__offset >= 0 && __offset < difference_type(_S_buffer_size()))
      _M_cur += __n;
    else {
      difference_type __node_offset =
        __offset > 0 ? __offset / difference_type(_S_buffer_size())
                   : -difference_type((-__offset - 1) / _S_buffer_size()) - 1;
      _M_set_node(_M_node + __node_offset);
      _M_cur = _M_first + 
        (__offset - __node_offset * difference_type(_S_buffer_size()));
    }
    return *this;
  }

  _Self operator+(difference_type __n) const
  {
    _Self __tmp = *this;
    return __tmp += __n;
  }

  _Self& operator-=(difference_type __n) { return *this += -__n; }
 
  _Self operator-(difference_type __n) const {
    _Self __tmp = *this;
    return __tmp -= __n;
  }

  reference operator[](difference_type __n) const { return *(*this + __n); }

  bool operator==(const _Self& __x) const { return _M_cur == __x._M_cur; }
  bool operator!=(const _Self& __x) const { return !(*this == __x); }
  bool operator<(const _Self& __x) const {
    return (_M_node == __x._M_node) ? 
      (_M_cur < __x._M_cur) : (_M_node < __x._M_node);
  }

  void _M_set_node(_Map_pointer __new_node) {
    _M_node = __new_node;
    _M_first = *__new_node;
    _M_last = _M_first + difference_type(_S_buffer_size());
  }
};

template <class _Tp, class _Ref, class _Ptr>
inline random_access_iterator_tag
iterator_category(const _Deque_iterator<_Tp,_Ref,_Ptr>&) {
  return random_access_iterator_tag();
}

template <class _Tp, class _Ref, class _Ptr>
inline _Tp* value_type(const _Deque_iterator<_Tp,_Ref,_Ptr>&) {
  return 0;
}

template <class _Tp, class _Ref, class _Ptr>
inline ptrdiff_t* distance_type(const _Deque_iterator<_Tp,_Ref,_Ptr>&) {
  return 0;
}


// Deque base class.  It has two purposes.  First, its constructor
//  and destructor allocate (but don't initialize) storage.  This makes
//  exception safety easier.  Second, it encapsulates the details of 
//  allocation: without partial specialization (stl_config.h) the
//  allocator is SGI's own.

template <class _Tp, class _Alloc, size_t __bufsiz>
class _Deque_base {
public:
  typedef _Deque_iterator<_Tp,_Tp&,_Tp*>                      iterator;
  typedef _Deque_iterator<_Tp,const _Tp&,const _Tp*>          const_iterator;
  typedef _Alloc allocator_type;
  allocator_type get_allocator() const { return allocator_type(); }

  _Deque_base(const allocator_type&, size_t __num_elements)
    : _M_map(0), _M_map_size(0),  _M_start(), _M_finish() {
    _M_initialize_map(__num_elements);
  }
  _Deque_base(const allocator_type&)
    : _M_map(0), _M_map_size(0),  _M_start(), _M_finish() {}
  ~_Deque_base();    

protected:
  void _M_initialize_map(size_t);
  void _M_create_nodes(_Tp** __nstart, _Tp** __nfinish);
  void _M_destroy_nodes(_Tp** __nstart, _Tp** __nfinish);
  enum { _S_initial_map_size = 8 };

protected:
  _Tp** _M_map;
  size_t _M_map_size;  
  iterator _M_start;
  iterator _M_finish;

  typedef simple_alloc<_Tp, _Alloc>  _Node_alloc_type;
  typedef simple_alloc<_Tp*, _Alloc> _Map_alloc_type;

  _Tp* _M_allocate_node()
    { return _Node_alloc_type::allocate(__deque_buf_size(__bufsiz, 
                                                         sizeof(_Tp))); }
  void _M_deallocate_node(_Tp* __p)
    { _Node_alloc_type::deallocate(__p, __deque_buf_size(__bufsiz, 
                                                         sizeof(_Tp))); }
  _Tp** _M_allocate_map(size_t __n) 
    { return _Map_alloc_type::allocate(__n); }
  void _M_deallocate_map(_Tp** __p, size_t __n) 
    { _Map_alloc_type::deallocate(__p, __n); }
};

// Non-inline member functions from _Deque_base.

template <class _Tp, class _Alloc, size_t __bufsiz>
_Deque_base<_Tp,_Alloc,__bufsiz>::~_Deque_base() {
  if (_M_map) {
    _M_destroy_nodes(_M_start._M_node, _M_finish._M_node + 1);
    _M_deallocate_map(_M_map, _M_map_size);
  }
}

template <class _Tp, class _Alloc, size_t __bufsiz>
void
_Deque_base<_Tp,_Alloc,__bufsiz>::_M_initialize_map(size_t __num_elements)
{
  size_t __num_nodes = 
    __num_elements / __deque_buf_size(__bufsiz, sizeof(_Tp)) + 1;

  _M_map_size = max((size_t) _S_initial_map_size, __num_nodes + 2);
  _M_map = _M_allocate_map(_M_map_size);

  _Tp** __nstart = _M_map + (_M_map_size - __num_nodes) / 2;
  _Tp** __nfinish = __nstart + __num_nodes;
    
  __STL_TRY {
    _M_create_nodes(__nstart, __nfinish);
  }
  __STL_UNWIND((_M_deallocate_map(_M_map, _M_map_size), 
                _M_map = 0, _M_map_size = 0));
  _M_start._M_set_node(__nstart);
  _M_finish._M_set_node(__nfinish - 1);
  _M_start._M_cur = _M_start._M_first;
  _M_finish._M_cur = _M_finish._M_first +
               __num_elements % __deque_buf_size(__bufsiz, sizeof(_Tp));
}

template <class _Tp, class _Alloc, size_t __bufsiz>
void
_Deque_base<_Tp,_Alloc,__bufsiz>::_M_create_nodes(_Tp** __nstart,
                                                  _Tp** __nfinish)
{
  _Tp** __cur;
  __STL_TRY {
    for (__cur = __nstart; __cur < __nfinish; ++__cur)
      *__cur = _M_allocate_node();
  }
  __STL_UNWIND(_M_destroy_nodes(__nstart, __cur));
}

template <class _Tp, class _Alloc, size_t __bufsiz>
void
_Deque_base<_Tp,_Alloc,__bufsiz>::_M_destroy_nodes(_Tp** __nstart,
                                                   _Tp** __nfinish)
{
  for (_Tp** __n = __nstart; __n < __nfinish; ++__n)
    _M_deallocate_node(*__n);
}

template <class _Tp, class _Alloc = __STL_DEFAULT_ALLOCATOR(_Tp), 
          size_t __bufsiz = 0> 
class deque : protected _Deque_base<_Tp, _Alloc, __bufsiz> {
  typedef _Deque_base<_Tp, _Alloc, __bufsiz> _Base;
public:                         // Basic types
  typedef _Tp value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef typename _Base::allocator_type allocator_type;
  allocator_type get_allocator() const { return _Base::get_allocator(); }

public:                         // Iterators
  typedef _Deque_iterator<_Tp,_Tp&,_Tp*>                     iterator;
  typedef _Deque_iterator<_Tp,const _Tp&,const _Tp*>         const_iterator;

protected:                      // Internal typedefs
  typedef pointer* _Map_pointer;
  static size_t _S_buffer_size()
    { return __deque_buf_size(__bufsiz, sizeof(_Tp)); }

public:                         // Basic accessors
  iterator begin() { return _M_start; }
  iterator end() { return _M_finish; }
  const_iterator begin() const { return _M_start; }
  const_iterator end() const { return _M_finish; }

  reference front() { return *_M_start; }
  reference back() {
    iterator __tmp = _M_finish;
    --__tmp;
    return *__tmp;
  }
  const_reference front() const { return *_M_start; }
  const_reference back() const {
    const_iterator __tmp = _M_finish;
    --__tmp;
    return *__tmp;
  }

  size_type size() const { return _M_finish - _M_start; }
  bool empty() const { return _M_finish == _M_start; }

public:                         // Constructor, destructor.
  explicit deque(const allocator_type& __a = allocator_type()) 
    : _Base(__a, 0) {}

  ~deque() { destroy(_M_start, _M_finish); }

public:                         // push_* and pop_*
  
  void push_back(const value_type& __t) {
    if (_M_finish._M_cur != _M_finish._M_last - 1) {
      construct(_M_finish._M_cur, __t);
      ++_M_finish._M_cur;
    }
    else
      _M_push_back_aux(__t);
  }

  void pop_back() {
    if (_M_finish._M_cur != _M_finish._M_first) {
      --_M_finish._M_cur;
      destroy(_M_finish._M_cur);
    }
    else
      _M_pop_back_aux();
  }

public:                         // Erase
  void clear(); 

protected:                        // Internal construction/destruction

  void _M_push_back_aux(const value_type&);
  void _M_pop_back_aux();

protected:                      // Allocation of _M_map and nodes

  // Makes sure the _M_map has space for new nodes.  Does not actually
  //  add the nodes.  Can invalidate _M_map pointers.  (And consequently, 
  //  deque iterators.)

  void _M_reserve_map_at_back (size_type __nodes_to_add = 1) {
    if (__nodes_to_add + 1 > _M_map_size - (_M_finish._M_node - _M_map))
      _M_reallocate_map(__nodes_to_add, false);
  }

  void _M_reallocate_map(size_type __nodes_to_add, bool __add_at_front);
};

// Non-inline member functions

template <class _Tp, class _Alloc, size_t __bufsize> 
void deque<_Tp,_Alloc,__bufsize>::clear()
{
  for (_Map_pointer __node = _M_start._M_node + 1;
       __node < _M_finish._M_node;
       ++__node) {
    destroy(*__node, *__node + _S_buffer_size());
    _M_deallocate_node(*__node);
  }

  if (_M_start._M_node != _M_finish._M_node) {
    destroy(_M_start._M_cur, _M_start._M_last);
    destroy(_M_finish._M_first, _M_finish._M_cur);
    _M_deallocate_node(_M_finish._M_first);
  }
  else
    destroy(_M_start._M_cur, _M_finish._M_cur);

  _M_finish = _M_start;
}

// Called only if _M_finish._M_cur == _M_finish._M_last - 1.
template <class _Tp, class _Alloc, size_t __bufsize>
void
deque<_Tp,_Alloc,__bufsize>::_M_push_back_aux(const value_type& __t)
{
  value_type __t_copy = __t;
  _M_reserve_map_at_back();
  *(_M_finish._M_node + 1) = _M_allocate_node();
  __STL_TRY {
    construct(_M_finish._M_cur, __t_copy);
    _M_finish._M_set_node(_M_finish._M_node + 1);
    _M_finish._M_cur = _M_finish._M_first;
  }
  __STL_UNWIND(_M_deallocate_node(*(_M_finish._M_node + 1)));
}

// Called only if _M_finish._M_cur == _M_finish._M_first.
template <class _Tp, class _Alloc, size_t __bufsize>
void
deque<_Tp,_Alloc,__bufsize>::_M_pop_back_aux()
{
  _M_deallocate_node(_M_finish._M_first);
  _M_finish._M_set_node(_M_finish._M_node - 1);
  _M_finish._M_cur = _M_finish._M_last - 1;
  destroy(_M_finish._M_cur);
}

template <class _Tp, class _Alloc, size_t __bufsize>
void deque<_Tp,_Alloc,__bufsize>::_M_reallocate_map(size_type __nodes_to_add,
                                          bool __add_at_front)
{
  size_type __old_num_nodes = _M_finish._M_node - _M_start._M_node + 1;
  size_type __new_num_nodes = __old_num_nodes + __nodes_to_add;

  _Map_pointer __new_nstart;
  if (_M_map_size > 2 * __new_num_nodes) {
    __new_nstart = _M_map + (_M_map_size - __new_num_nodes) / 2 
                     + (__add_at_front ? __nodes_to_add : 0);
    if (__new_nstart < _M_start._M_node)
      copy(_M_start._M_node, _M_finish._M_node + 1, __new_nstart);
    else
      copy_backward(_M_start._M_node, _M_finish._M_node + 1, 
                    __new_nstart + __old_num_nodes);
  }
  else {
    size_type __new_map_size = 
      _M_map_size + max(_M_map_size, __nodes_to_add) + 2;

    _Map_pointer __new_map = _M_allocate_map(__new_map_size);
    __new_nstart = __new_map + (__new_map_size - __new_num_nodes) / 2
                         + (__add_at_front ? __nodes_to_add : 0);
    copy(_M_start._M_node, _M_finish._M_node + 1, __new_nstart);
    _M_deallocate_map(_M_map, _M_map_size);

    _M_map = __new_map;
    _M_map_size = __new_map_size;
  }

  _M_start._M_set_node(__new_nstart);
  _M_finish._M_set_node(__new_nstart + __old_num_nodes - 1);
}

__STL_END_NAMESPACE 
  
#endif /* __SGI_STL_INTERNAL_DEQUE_H */
