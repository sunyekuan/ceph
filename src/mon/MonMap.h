// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#ifndef CEPH_MONMAP_H
#define CEPH_MONMAP_H

#include "include/err.h"

#include "msg/Message.h"
#include "include/types.h"
//#include "common/config.h"

namespace ceph {
  class Formatter;
}

class MonMap {
 public:
  epoch_t epoch;       // what epoch/version of the monmap
  uuid_d fsid;
  map<string, entity_addr_t> mon_addr;
  utime_t last_changed;
  utime_t created;

  map<entity_addr_t,string> addr_name;
  vector<string> rank_name;
  vector<entity_addr_t> rank_addr;
  
  void calc_ranks() {
    rank_name.resize(mon_addr.size());
    rank_addr.resize(mon_addr.size());
    addr_name.clear();
    for (map<string,entity_addr_t>::iterator p = mon_addr.begin();
	 p != mon_addr.end();
	 p++) {
      addr_name[p->second] = p->first;
    }
    unsigned i = 0;
    for (map<entity_addr_t,string>::iterator p = addr_name.begin();
	 p != addr_name.end();
	 p++, i++) {
      rank_name[i] = p->second;
      rank_addr[i] = p->first;
    }
  }

  MonMap() 
    : epoch(0) {
    memset(&fsid, 0, sizeof(fsid));
  }

  uuid_d& get_fsid() { return fsid; }

  unsigned size() {
    return mon_addr.size();
  }

  const string& pick_random_mon() {
    unsigned n = rand() % rank_name.size();
    return rank_name[n];
  }
  const string& pick_random_mon_not(const string& butnot) {
    unsigned n = rand() % rank_name.size();
    if (rank_name[n] == butnot && rank_name.size() > 1) {
      if (n)
	n--;
      else
	n++;
    }
    return rank_name[n];
  }

  epoch_t get_epoch() { return epoch; }
  void set_epoch(epoch_t e) { epoch = e; }

  void list_addrs(list<entity_addr_t>& ls) const {
    for (map<string,entity_addr_t>::const_iterator p = mon_addr.begin();
	 p != mon_addr.end();
	 ++p)
      ls.push_back(p->second);
  }

  void add(const string &name, const entity_addr_t &addr) {
    assert(mon_addr.count(name) == 0);
    mon_addr[name] = addr;
    calc_ranks();
  }
  
  void remove(const string &name) {
    assert(mon_addr.count(name));
    mon_addr.erase(name);
    calc_ranks();
  }

  bool contains(const string& name) {
    return mon_addr.count(name);
  }

  bool contains(const entity_addr_t &a) {
    for (map<string,entity_addr_t>::iterator p = mon_addr.begin();
	 p != mon_addr.end();
	 p++)
      if (p->second == a)
	return true;
    return false;
  }

  string get_name(unsigned n) const {
    assert(n < rank_name.size());
    return rank_name[n];
  }

  int get_rank(const string& n) {
    for (unsigned i=0; i<rank_name.size(); i++)
      if (rank_name[i] == n)
	return i;
    return -1;
  }
  int get_rank(entity_addr_t a) {
    for (unsigned i=0; i<rank_addr.size(); i++)
      if (rank_addr[i] == a)
	return i;
    return -1;
  }
  bool get_addr_name(entity_addr_t a, string& name) {
    if (addr_name.count(a) == 0)
      return false;
    name = addr_name[a];
    return true;
  }

  void rename(string oldname, string newname) {
    assert(contains(oldname));
    assert(!contains(newname));
    mon_addr[newname] = mon_addr[oldname];
    mon_addr.erase(oldname);
  }

  const entity_addr_t& get_addr(const string& n) {
    assert(mon_addr.count(n));
    return mon_addr[n];
  }
  const entity_addr_t& get_addr(unsigned m) {
    assert(m < rank_addr.size());
    return rank_addr[m];
  }
  entity_inst_t get_inst(const string& n) {
    assert(mon_addr.count(n));
    int m = get_rank(n);
    entity_inst_t i;
    i.addr = rank_addr[m];
    i.name = entity_name_t::MON(m);
    return i;
  }
  entity_inst_t get_inst(unsigned m) const {
    assert(m < rank_addr.size());
    entity_inst_t i;
    i.addr = rank_addr[m];
    i.name = entity_name_t::MON(m);
    return i;
  }

  void encode(bufferlist& blist) const;
  void encode_v1(bufferlist& blist) const;

  void decode(bufferlist& blist) {
    bufferlist::iterator p = blist.begin();
    decode(p);
  }
  void decode(bufferlist::iterator &p);

  void generate_fsid() {
    fsid.generate_random();
  }

  // read from/write to a file
  int write(const char *fn);
  int read(const char *fn);

  void print(ostream& out) const;
  void print_summary(ostream& out) const;
  void dump(ceph::Formatter *f) const;

  static void generate_test_instances(list<MonMap*>& o);
};
WRITE_CLASS_ENCODER(MonMap)

inline ostream& operator<<(ostream& out, MonMap& m) {
  m.print_summary(out);
  return out;
}

#endif
