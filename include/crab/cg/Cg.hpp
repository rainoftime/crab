#ifndef CALL_GRAPH_HPP__
#define CALL_GRAPH_HPP__

/* 
   Build a call graph (CG)
*/

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <crab/common/debug.hpp>
#include <crab/common/stats.hpp>
#include <crab/common/types.hpp>
#include <crab/cfg/Cfg.hpp>

#include <functional> // for wrapper_reference

namespace crab {

   namespace cg {

      using namespace std;
      using namespace boost;
      using namespace cfg;

      // Class to build a call graph
      //
      // Important: this class assumes that all function calls have been
      // resolved. This must be ensured by the client.
      template <typename CFG>
      class CallGraph: boost::noncopyable {

        // Wrapper for call graph nodes
        class CgNode {
          
          typedef typename CFG::basic_block_t::callsite_t callsite_t;
          typedef typename CFG::fdecl_t fdecl_t;
          
          
          CFG m_cfg;
          int m_id;
          
         public:

          typedef CFG cfg_t;
          typedef typename CFG::varname_t varname_t;

         public:

          CgNode () { } // needed for BGL
          
          CgNode (CFG cfg, int id): 
              m_cfg (cfg), m_id (id) { }
          
          CFG getCfg () { return m_cfg;  }
          
          int index () const { return m_id; }
          
          varname_t name () const {
            auto d_opt = m_cfg.get_func_decl ();
            if (!d_opt) CRAB_ERROR("No function name found");
            return (*d_opt).get_func_name ();
          }

          string str_name () const {
            auto d_opt = m_cfg.get_func_decl ();
            if (!d_opt) CRAB_ERROR("No function name found");
            return (*d_opt).get_func_name ().str ();
          }

          bool operator==(const CgNode &o) const {
            return index () == o.index ();
          }

          bool operator!=(const CgNode &o) const {
            return !(*this == o);
          }
          
          friend ostream& operator<<(ostream& o, CgNode n) {
            o << n.str_name ();
            return o;
          }
          
          friend size_t hash_value (CgNode n) {
            return n.index ();
          }
        };
        
        // Wrapper for call graph edges
        // BGL complains if we use std::pair<CgNode,CgNode>
        template <typename T>
        struct CgEdge  {
          T m_s;
          T m_d;
          CgEdge (){ }
          CgEdge (T s, T d): m_s (s), m_d (d) { }
          T Src () const { return m_s; }
          T Dest () const { return m_d; }
          bool operator== (const CgEdge<T> &o) const {
            return (m_s == o.Src () && m_d == o.Dest ());
          }
          bool operator!= (const CgEdge<T> &o) const {
            return !(*this == o);
          }
        };

        /// --- begin internal representation of the call graph
        struct  vertex_t { CgNode func; };
        typedef adjacency_list<setS, //disallow parallel edges
                               vecS, bidirectionalS, 
                               property<vertex_color_t, 
                                        default_color_type, 
                                        vertex_t> > cg_t;     
        typedef typename graph_traits<cg_t>::vertex_descriptor vertex_descriptor_t;
        typedef typename graph_traits<cg_t>::edge_descriptor edge_descriptor_t;
        typedef typename graph_traits<cg_t>::vertex_iterator vertex_iterator;
        typedef typename graph_traits<cg_t>::out_edge_iterator out_edge_iterator;
        typedef typename graph_traits<cg_t>::in_edge_iterator in_edge_iterator;
        /// --- end internal representation of the call graph

        typedef boost::unordered_map<std::size_t, vertex_descriptor_t > vertex_map_t;
        typedef boost::unordered_map <CgNode, vertex_descriptor_t > node_vertex_id_map_t;
        typedef typename CFG::varname_t varname_t;
        typedef crab::cfg::StatementVisitor<varname_t> stmt_visitor_t;

        struct mkEdgeVis: public stmt_visitor_t {
          typedef typename stmt_visitor_t::z_bin_op_t z_bin_op_t;
          typedef typename stmt_visitor_t::z_assign_t z_assign_t;
          typedef typename stmt_visitor_t::z_assume_t z_assume_t;
          typedef typename stmt_visitor_t::havoc_t havoc_t;
          typedef typename stmt_visitor_t::unreach_t unreach_t;
          typedef typename stmt_visitor_t::z_select_t z_select_t;
          typedef typename stmt_visitor_t::callsite_t callsite_t;
          typedef typename CFG::fdecl_t fdecl_t;

          cg_t& m_cg;
          vertex_map_t& m_vertex_map;
          std::size_t m_from;

          mkEdgeVis (cg_t &cg, vertex_map_t &vertex_map, fdecl_t &from): 
              m_cg (cg), m_vertex_map (vertex_map), 
              m_from (CfgHasher<CFG>::hash (from)) { 
          }
          
          void visit(callsite_t& cs) { 
            size_t to = CfgHasher<CFG>::hash (cs);
            auto it_from = m_vertex_map.find (m_from);
            auto it_to = m_vertex_map.find (to);
            if (it_from == m_vertex_map.end () || it_to == m_vertex_map.end ())
              return;
            
            auto res = add_edge (it_from->second, it_to->second, m_cg);
            if (res.second)
              CRAB_LOG("cg",
                       crab::outs() << "Added cg edge " <<  it_from->second 
                                 << " --> " <<  it_to->second;); 
                                 
          }
          
          void visit(z_bin_op_t&){ }  
          void visit(z_assign_t&) { }
          void visit(z_assume_t&) { }
          void visit(havoc_t&) { }
          void visit(unreach_t&){ }
          void visit(z_select_t&){ }
        };
                  
        struct MkNode : 
            public std::unary_function < vertex_descriptor_t, CgNode > {

          cg_t* _cg;
          
          MkNode (): _cg(nullptr) { }
          MkNode (cg_t *cg): _cg (cg) { }
          CgNode& operator()(const vertex_descriptor_t& v) const { 
            assert (_cg);
            return (*_cg)[v].func; 
          }
        };
        
        struct MkEdge :  
            public std::unary_function < edge_descriptor_t, CgEdge<CgNode> > {

          cg_t* _cg;
          
          MkEdge(): _cg(nullptr) {}
          MkEdge(cg_t *cg): _cg(cg) { }
          CgEdge<CgNode> operator()(const edge_descriptor_t& e) const { 
            assert (_cg);
            CgNode& s = (*_cg)[boost::source(e, (*_cg))].func;
            CgNode& t = (*_cg)[boost::target(e, (*_cg))].func;
            return CgEdge<CgNode>(s,t); 
          }
        };


       public: 

        typedef CgNode node_t;
        typedef CgEdge <node_t> edge_t;
        typedef boost::transform_iterator<MkNode, vertex_iterator> node_iterator; 
        typedef boost::transform_iterator<MkEdge, in_edge_iterator> pred_iterator; 
        typedef boost::transform_iterator<MkEdge, out_edge_iterator> succ_iterator; 

       private:

        // call graph
        std::shared_ptr<cg_t> m_cg;
        // map hashed values to internal BGL vertex descriptor
        vertex_map_t m_vertex_map;
        // map CgNode to internal BGL vertex descriptor
        node_vertex_id_map_t m_node_vertex_id_map;
        // counter to generate unique ids
        int m_id;

        vertex_descriptor_t get_vertex (const node_t &n) const {
          auto It = m_node_vertex_id_map.find (n);
          if (It != m_node_vertex_id_map.end ())
            return It->second;
          CRAB_ERROR("Call graph could not find node");
        }

        template<typename CFGIt>
        void build_call_graph (CFGIt I, CFGIt E) {
          crab::ScopedCrabStats __st__("CallGraph");

          // --- add vertices in the call graph
          for (auto cfg: boost::make_iterator_range(I,E)) {
            auto decl_opt = cfg.get_func_decl ();
            if (!decl_opt)
              CRAB_ERROR("Could not compute call graph: function info is missing.");
            
            size_t k = CfgHasher<CFG>::hash (*decl_opt);
            vertex_descriptor_t v = add_vertex (*m_cg);
            m_vertex_map.insert (make_pair (k,v));
            node_t f (cfg, m_id++);
            m_node_vertex_id_map.insert(make_pair (f, v));
            (*m_cg)[v].func = f;

            CRAB_LOG("cg", 
                     crab::outs() << "Added call graph node " <<  *decl_opt 
                                  <<  "--- id=" <<  v);
          }
          
          // --- add edges in the call graph
          for (auto cfg: boost::make_iterator_range(I,E)) {
            auto decl_opt = cfg.get_func_decl ();
            assert (decl_opt);
            
            for (auto const &bb : boost::make_iterator_range (cfg.begin (),
                                                              cfg.end ())) { 
              mkEdgeVis vis (*m_cg, m_vertex_map, *decl_opt);
              for (auto it = bb.begin (); it != bb.end (); ++it)
                it->accept(&vis);
            }
          }

        }

        // CallGraph (const CallGraph& o)
        //     : m_cg (o.m_cg), // shallow copy
        //       m_vertex_map (o.m_vertex_map),
        //       m_node_vertex_id_map (o.m_node_vertex_id_map),
        //       m_id (o.m_id) { }

        // CallGraph (CallGraph &&o)
        //     : m_cg (std::move(o.m_cg)),
        //       m_vertex_map (std::move(o.m_vertex_map)),
        //       m_node_vertex_id_map (std::move(o.m_node_vertex_id_map)),
        //       m_id (o.m_id) { }
              

       public:
        
        CallGraph (vector<CFG>& cfgs)
            : m_cg (new cg_t()), m_id (0) {
          build_call_graph (cfgs.begin(), cfgs.end());
        }

        template<typename CFGIt>
        CallGraph (CFGIt I, CFGIt E)
            : m_cg (new cg_t()), m_id (0) {
          build_call_graph (I,E);
        }
        
        std::pair<node_iterator, node_iterator> 
        nodes () const {
          auto p = boost::vertices (*m_cg);
          return std::make_pair (make_transform_iterator (p.first, MkNode (&*m_cg)),
                                 make_transform_iterator (p.second, MkNode (&*m_cg)));
        }

        std::pair<succ_iterator, succ_iterator> 
        succs (const node_t &n) const {
          vertex_descriptor_t v = get_vertex(n); 
          auto p = boost::out_edges (v, *m_cg);
          return std::make_pair (make_transform_iterator (p.first, MkEdge (&*m_cg)),
                                 make_transform_iterator (p.second, MkEdge (&*m_cg)));
        }

        std::pair<pred_iterator, pred_iterator> 
        preds (const node_t &n) const {
          vertex_descriptor_t v = get_vertex(n); 
          auto p = boost::in_edges (v, *m_cg);
          return std::make_pair (make_transform_iterator (p.first, MkEdge (&*m_cg)),
                                 make_transform_iterator (p.second, MkEdge (&*m_cg)));
        }

        std::size_t num_nodes () const {
           return boost::num_vertices (*m_cg);
        }

        std::size_t num_succs (const node_t &n) const {
          vertex_descriptor_t v = get_vertex(n);
          return boost::out_degree (v, *m_cg);
        }

        std::size_t num_preds (const node_t &n) const {
          vertex_descriptor_t v = get_vertex(n); 
          return boost::in_degree (v, *m_cg);
        }

        void write(std::ostream& o) const {
          o << "CG=\n";
          for (auto f: boost::make_iterator_range (nodes ())){
            if (num_succs (f) > 0) {
              for (auto e: boost::make_iterator_range (succs (f)))  {
                o << e.Src() << "--> " << e.Dest() << endl;
              }
            }
          }
        }
        
      }; // end class CallGraph<CFG>
   
      template <typename CFG>
      inline std::ostream& operator<<(std::ostream& o, 
                                      const CallGraph<CFG> &cg) {
        cg.write (o);
        return o;
      }

      // A lightweight object that wraps a reference to a CallGraph into a
      // copyable, assignable object.
      template <class CG>
      class CallGraph_Ref {
       public:
        
        typedef typename CG::node_t node_t;
        typedef typename CG::edge_t edge_t;
        typedef typename CG::node_iterator node_iterator; 
        typedef typename CG::pred_iterator pred_iterator; 
        typedef typename CG::succ_iterator succ_iterator; 
        
       private:
        
        boost::optional<std::reference_wrapper<CG> > _ref;
        
       public:
        
        CallGraph_Ref (CG &cg)
            : _ref(std::reference_wrapper<CG>(cg)) { } 
        
        const CG& get() const { 
          assert (_ref);
          return *_ref;
        }
        
        CG& get() { 
          assert (_ref);
          return *_ref;
        }
        
        std::pair<node_iterator, node_iterator> 
        nodes () const {
          assert (_ref);
          return (*_ref).get().nodes();
        }
        
        std::pair<succ_iterator, succ_iterator> 
        succs (const node_t &n) const {
          assert (_ref);
          return (*_ref).get().succs(n);
        }
        
        std::pair<pred_iterator, pred_iterator> 
        preds (const node_t &n) const {
          assert (_ref);
          return (*_ref).get().preds(n);
        }
        
        std::size_t num_nodes () const {
          assert (_ref);
          return (*_ref).get().num_nodes();
        }
        
        std::size_t num_succs (const node_t &n) const {
          assert (_ref);
          return (*_ref).get().num_succs(n);
        }
        
        std::size_t num_preds (const node_t &n) const {
          assert (_ref);
          return (*_ref).get().num_preds(n);
        }
        
        void write(std::ostream& o) const {
          assert (_ref);
          (*_ref).get().write(o);
        }
      };
  
      template <typename CG>
      inline std::ostream& operator<<(std::ostream& o, 
                                      const CallGraph_Ref<CG> &cg) {
        cg.write (o);
        return o;
      }
 
   } //end namespace cg
} // end namespace crab

#endif 
