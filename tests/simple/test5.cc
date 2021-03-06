#include "../common.hpp"

using namespace std;
using namespace crab::analyzer;
using namespace crab::cfg_impl;
using namespace crab::domain_impl;

cfg_t* prog (VariableFactory &vfac)  {

  // Definining program variables
  z_var i (vfac ["i"]);
  z_var x (vfac ["x"]);
  z_var y (vfac ["y"]);
  z_var z (vfac ["z"]);
  z_var w (vfac ["w"]);
  z_var nd1 (vfac ["nd1"]);
  z_var nd2 (vfac ["nd2"]);
  // entry and exit block
  cfg_t* cfg = new cfg_t("entry","ret");
  // adding blocks
  basic_block_t& entry = cfg->insert ("entry");
  basic_block_t& bb1   = cfg->insert ("bb1");
  basic_block_t& bb1_t = cfg->insert ("bb1_t");
  basic_block_t& bb1_f = cfg->insert ("bb1_f");
  basic_block_t& bb2   = cfg->insert ("bb2");
  basic_block_t& exit  = cfg->insert ("exit");
  basic_block_t& ret   = cfg->insert ("ret");
  // adding control flow 
  entry >> bb1;
  bb1 >> bb1_t; bb1 >> bb1_f;
  bb1_t >> bb2; bb2 >> bb1; bb1_f >> exit; exit >> ret;
  // adding statements
  entry.assign (i, 0);
  entry.assign (x, 1);
  entry.assign (y, 0);
  entry.assign (z, 3);
  entry.assign (w, 3);
  bb1_t.assume (i <= 99);
  bb1_f.assume (i >= 100);
  bb2.havoc(nd1.name());
  bb2.havoc(nd2.name());
  bb2.add(x,x,y);
  bb2.add(y,y,1);
  bb2.bitwise_xor(z,z,nd1);
  bb2.bitwise_xor(w,w,nd1);
  bb2.add(i, i, 1);
  exit.assume (x <= y);

  return cfg;
}

int main (int argc, char** argv )
{
  SET_LOGGER(argc,argv)

  VariableFactory vfac;
  cfg_t* cfg = prog (vfac);
  crab::outs() << *cfg << endl;
  crab::outs() << endl;
  Liveness<cfg_ref_t> live (*cfg);
  live.exec ();

  {
    NumFwdAnalyzer <cfg_ref_t, dbm_domain_t,VariableFactory>::type a (*cfg,vfac,&live);
    a.Run (dbm_domain_t::top ());
    crab::outs() << "Invariants using " << dbm_domain_t::getDomainName () << "\n";
    for (auto &b : *cfg)
    {
      auto inv = a [b.label ()];
      crab::outs() << get_label_str (b.label ()) << "=" << inv << "\n";
    }
  }

  {
    NumFwdAnalyzer <cfg_ref_t, sdbm_domain_t,VariableFactory>::type a (*cfg,vfac,&live);
    a.Run (sdbm_domain_t::top ());
    crab::outs() << "Invariants using " << sdbm_domain_t::getDomainName () << "\n";
    for (auto &b : *cfg)
    {
      auto inv = a [b.label ()];
      crab::outs() << get_label_str (b.label ()) << "=" << inv << "\n";
    }
  }


  {
    NumFwdAnalyzer <cfg_ref_t, term_dis_int_t,VariableFactory>::type a (*cfg,vfac,&live);
    a.Run (term_dis_int_t::top ());
    crab::outs() << "Invariants using " << term_dis_int_t::getDomainName () << "\n";
    for (auto &b : *cfg)
    {
      auto inv = a [b.label ()];
      crab::outs() << get_label_str (b.label ()) << "=" << inv << "\n";
    }
  }

  {
    NumFwdAnalyzer <cfg_ref_t, num_domain_t,VariableFactory>::type a (*cfg,vfac,&live, 1, 2, 20);
    a.Run (num_domain_t::top ());
    crab::outs() << "Invariants using " << num_domain_t::getDomainName () << "\n";
    for (auto &b : *cfg) 
    {
      auto inv = a [b.label ()];
      crab::outs() << get_label_str (b.label ()) << "=" << inv << "\n";
    }
  }

  delete cfg;

  return 0;
}
