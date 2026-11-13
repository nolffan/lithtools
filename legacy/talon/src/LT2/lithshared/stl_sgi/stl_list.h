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
// talon: SGI STL 3.2's stl_list.h, the part PreWorld.cpp's code uses
// (list<uint32*> and list<PointStackEl*>: the constructor 0x44a612, the
// destructor 0x44a705, clear 0x44af8d, insert 0x44a6dd, erase 0x449e00).

#ifndef __SGI_STL_INTERNAL_LIST_H
#define __SGI_STL_INTERNAL_LIST_H

__STL_BEGIN_NAMESPACE

template <class _Tp>
struct _List_node {
  typedef void* _Void_pointer;
  _Void_pointer _M_next;
  _Void_pointer _M_prev;
  _Tp _M_data;
};

template<class _Tp, class _Ref, class _Ptr>
struct _List_iterator {
  typedef _List_iterator<_Tp,_Tp&,_Tp*>             iterator;
  typedef _List_iterator<_Tp,const _Tp&,const _Tp*> const_iterator;
  typedef _List_iterator<_Tp,_Ref,_Ptr>             _Self;

  typedef bidirectional_iterator_tag iterator_category;
  typedef _Tp value_type;
  typedef _Ptr pointer;
  typedef _Ref reference;
  typedef _List_node<_Tp> _Node;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  _Node* _M_node;

  _List_iterator(_Node* __x) : _M_node(__x) {}
  _List_iterator() {}
  _List_iterator(const iterator& __x) : _M_node(__x._M_node) {}

  bool operator==(const _Self& __x) const { return _M_node == __x._M_node; }
  bool operator!=(const _Self& __x) const { return _M_node != __x._M_node; }
  reference operator*() const { return (*_M_node)._M_data; }

#ifndef __SGI_STL_NO_ARROW_OPERATOR
  pointer operator->() const { return &(operator*()); }
#endif /* __SGI_STL_NO_ARROW_OPERATOR */

  _Self& operator++() { 
    _M_node = (_Node*)(_M_node->_M_next);
    return *this;
  }
  _Self operator++(int) { 
    _Self __tmp = *this;
    ++*this;
    return __tmp;
  }
  _Self& operator--() { 
    _M_node = (_Node*)(_M_node->_M_prev);
    return *this;
  }
  _Self operator--(int) { 
    _Self __tmp = *this;
    --*this;
    return __tmp;
  }
};

template <class _Tp, class _Ref, class _Ptr>
inline bidirectional_iterator_tag
iterator_category(const _List_iterator<_Tp, _Ref, _Ptr>&)
{
  return bidirectional_iterator_tag();
}

template <class _Tp, class _Ref, class _Ptr>
inline _Tp*
value_type(const _List_iterator<_Tp, _Ref, _Ptr>&)
{
  return 0;
}

template <class _Tp, class _Ref, class _Ptr>
inline ptrdiff_t*
distance_type(const _List_iterator<_Tp, _Ref, _Ptr>&)
{
  return 0;
}


// Base class that encapsulates details of allocators.  Without partial
// specialization (stl_config.h), the allocator is SGI's own.

template <class _Tp, class _Alloc>
class _List_base 
{
public:
  typedef _Alloc allocator_type;
  allocator_type get_allocator() const { return allocator_type(); }

  _List_base(const allocator_type&) {
    _M_node = _M_get_node();
    _M_node->_M_next = _M_node;
    _M_node->_M_prev = _M_node;
  }
  ~_List_base() {
    clear();
    _M_put_node(_M_node);
  }

  void clear();

protected:
  typedef simple_alloc<_List_node<_Tp>, _Alloc> _Alloc_type;
  _List_node<_Tp>* _M_get_node() { return _Alloc_type::allocate(1); }
  void _M_put_node(_List_node<_Tp>* __p) { _Alloc_type::deallocate(__p, 1); } 

protected:
  _List_node<_Tp>* _M_node;
};

template <class _Tp, class _Alloc>
void 
_List_base<_Tp,_Alloc>::clear() 
{
  _List_node<_Tp>* __cur = (_List_node<_Tp>*) _M_node->_M_next;
  while (__cur != _M_node) {
    _List_node<_Tp>* __tmp = __cur;
    __cur = (_List_node<_Tp>*) __cur->_M_next;
    destroy(&__tmp->_M_data);
    _M_put_node(__tmp);
  }
  _M_node->_M_next = _M_node;
  _M_node->_M_prev = _M_node;
}

template <class _Tp, class _Alloc = __STL_DEFAULT_ALLOCATOR(_Tp) >
class list : protected _List_base<_Tp, _Alloc> {
  typedef _List_base<_Tp, _Alloc> _Base;
protected:
  typedef void* _Void_pointer;

public:      
  typedef _Tp value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef _List_node<_Tp> _Node;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef typename _Base::allocator_type allocator_type;
  allocator_type get_allocator() const { return _Base::get_allocator(); }

public:
  typedef _List_iterator<_Tp,_Tp&,_Tp*>             iterator;
  typedef _List_iterator<_Tp,const _Tp&,const _Tp*> const_iterator;

protected:
  _Node* _M_create_node(const _Tp& __x)
  {
    _Node* __p = _M_get_node();
    __STL_TRY {
      construct(&__p->_M_data, __x);
    }
    __STL_UNWIND(_M_put_node(__p));
    return __p;
  }

  _Node* _M_create_node()
  {
    _Node* __p = _M_get_node();
    __STL_TRY {
      construct(&__p->_M_data);
    }
    __STL_UNWIND(_M_put_node(__p));
    return __p;
  }

public:
  explicit list(const allocator_type& __a = allocator_type()) : _Base(__a) {}

  iterator begin()             { return (_Node*)(_M_node->_M_next); }
  const_iterator begin() const { return (_Node*)(_M_node->_M_next); }

  iterator end()             { return _M_node; }
  const_iterator end() const { return _M_node; }

  bool empty() const { return _M_node->_M_next == _M_node; }

  reference front() { return *begin(); }
  const_reference front() const { return *begin(); }
  reference back() { return *(--end()); }
  const_reference back() const { return *(--end()); }

  iterator insert(iterator __position, const _Tp& __x) {
    _Node* __tmp = _M_create_node(__x);
    __tmp->_M_next = __position._M_node;
    __tmp->_M_prev = __position._M_node->_M_prev;
    ((_Node*) (__position._M_node->_M_prev))->_M_next = __tmp;
    __position._M_node->_M_prev = __tmp;
    return __tmp;
  }
  iterator insert(iterator __position) { return insert(__position, _Tp()); }

  void push_front(const _Tp& __x) { insert(begin(), __x); }
  void push_front() {insert(begin());}
  void push_back(const _Tp& __x) { insert(end(), __x); }
  void push_back() {insert(end());}

  iterator erase(iterator __position) {
    _Node* __next_node = (_Node*) (__position._M_node->_M_next);
    _Node* __prev_node = (_Node*) (__position._M_node->_M_prev);
    __prev_node->_M_next = __next_node;
    __next_node->_M_prev = __prev_node;
    destroy(&__position._M_node->_M_data);
    _M_put_node(__position._M_node);
    return iterator(__next_node);
  }

  void clear() { _Base::clear(); }

  void pop_front() { erase(begin()); }
  void pop_back() { 
    iterator __tmp = end();
    erase(--__tmp);
  }

  ~list() { }
};

__STL_END_NAMESPACE 

#endif /* __SGI_STL_INTERNAL_LIST_H */
