/* $Id: ilist.hh,v 1.1 2003-09-12 23:07:20 atterer Exp $ -*- C++ -*-
  __   _
  |_) /|  Copyright (C) 2003  |  richard@
  | \/�|  Richard Atterer     |  atterer.net
  � '` �
  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2. See the file
  COPYING for details.

  Intrusive list, i.e. every list member needs to derive from IListBase

*/

#ifndef ILIST_HH
#define ILIST_HH

#include <config.h>

#include <debug.hh>
#include <log.hh>
//______________________________________________________________________

/** Never use this type or its members in your code apart from deriving
    publicly from it. */
struct IListBase {
  IListBase() : iListBase_prev(0), iListBase_next(0) {
    //msg("IListBase %1", this);
  }
  ~IListBase() {
    //msg("~IListBase %1", this);
    iListBase_prev->iListBase_next = iListBase_next;
    iListBase_next->iListBase_prev = iListBase_prev;
    iListBase_prev = iListBase_next = 0;
  }
  IListBase* iListBase_prev;
  IListBase* iListBase_next;
};

template <class T>
class IList {
public:

  typedef T value_type;
//   class iterator;

  IList() { e.iListBase_prev = e.iListBase_next = &e; }
  bool empty() const { return e.iListBase_next == &e; }
  T& front() { return *static_cast<T*>(e.iListBase_next); }
  void push_back(T& x) {
    msg("IList::push_back %1", &x);
    Assert(x.iListBase_prev == 0 && x.iListBase_next == 0);
    x.iListBase_prev = e.iListBase_prev;
    x.iListBase_next = &e;
    x.iListBase_prev->iListBase_next = &x;
    x.iListBase_next->iListBase_prev = &x;
  }
private:
  IListBase e;
};

// template <class T>
// class IList::iterator {
// public:
// private:
//   IListBase* p;
// };

#endif