#include <crab/common/debug.hpp>

#include <iostream>

namespace crab {

   std::ostream& outs () { return std::cout; }
   std::ostream& errs () { return std::cerr;}
   
}

#ifndef NCRABLOG
#include <set>
using namespace crab;
bool crab::CrabLogFlag = false;
std::set<std::string> crab::CrabLog;

void crab::CrabEnableLog (std::string x) 
{
  if (x.empty ()) return;
  CrabLogFlag = true;
  CrabLog.insert (x); 
}

namespace crab
{
  // struct LogOpt
  // { void operator=(const std::string &tag) const 
  //   { 
  //     CrabEnableLog (tag); 
  //   } 
  // };  

  // LogOpt loc;

} // end namespace crab

#else
void crab::CrabEnableLog (std::string x) { }
#endif

