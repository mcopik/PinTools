#ifndef __TAGE_HPP__
#define __TAGE_HPP__

#include <cstdint>

#include <pin.H>

namespace tage {

  struct TageBase {

    virtual ~TageBase(){};
    virtual void take_snapshot(long long instructions) = 0;
    virtual void update(ADDRINT inst_ptr, ADDRINT target, bool taken) = 0;
    virtual void clear() = 0;
    virtual void flush(int) = 0;
    virtual void dump(long long instructions, size_t static_branches, int counter) = 0;
  };

}


#endif
