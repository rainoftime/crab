/*******************************************************************************
 * Array smashing domain
 * 
 * FIXME: assume all array accesses are aligned wrt to the size of the
 * array element (e.g., if the size of the array element is 4 bytes
 * then all array accesses must be multiple of 4). Note that this
 * assumption does not hold in real programs.
 ******************************************************************************/

#ifndef ARRAY_SMASHING_HPP
#define ARRAY_SMASHING_HPP

#include <crab/common/types.hpp>
#include <crab/common/debug.hpp>
#include <crab/common/stats.hpp>
#include <crab/domains/numerical_domains_api.hpp>
#include <crab/domains/domain_traits.hpp>

using namespace boost;
using namespace ikos;

namespace crab {

   namespace domains {

      //! Abstract domain to reason about summarized variables. All array
      //  elements are `smashed` into a single cell.
      template<typename NumDomain>
      class array_smashing: 
         public ikos::writeable, 
         public numerical_domain<typename NumDomain::number_t,
                                 typename NumDomain::varname_t>,
         public bitwise_operators<typename NumDomain::number_t, 
                                  typename NumDomain::varname_t>, 
         public division_operators<typename NumDomain::number_t,
                                   typename NumDomain::varname_t> {
              
       public:

        typedef typename NumDomain::number_t Number;
        typedef typename NumDomain::varname_t VariableName;

        using typename numerical_domain< Number, VariableName>::linear_expression_t;
        using typename numerical_domain< Number, VariableName>::linear_constraint_t;
        using typename numerical_domain< Number, VariableName>::linear_constraint_system_t;
        using typename numerical_domain< Number, VariableName>::variable_t;
        using typename numerical_domain< Number, VariableName>::number_t;
        using typename numerical_domain< Number, VariableName>::varname_t;

        typedef array_smashing <NumDomain> array_smashing_t;
        typedef NumDomain content_domain_t;

        typedef interval <Number> interval_t;
        
       private:
        
        typedef bound <Number> bound_t; 
        
        //! scalar and summarized array variables        
        NumDomain _inv; 
        
        array_smashing (NumDomain inv): 
            ikos::writeable (), 
            _inv (inv) { }
        
        void strong_update (VariableName lhs, linear_expression_t rhs ) {
          _inv.assign (lhs, rhs);
        }
        
        void weak_update (VariableName lhs, linear_expression_t rhs) {
          NumDomain other (_inv);
          other.assign (lhs, rhs);
          _inv = _inv | other;
        }
        
       public:
        
        array_smashing(): ikos::writeable(), _inv (NumDomain::top()) { }    
        
        static array_smashing_t top() { 
          return array_smashing (NumDomain::top ()); 
        }
        
        static array_smashing_t bottom() {
          return array_smashing (NumDomain::bottom ());
        }
        
        array_smashing (const array_smashing_t& other): 
            ikos::writeable(), 
            _inv (other._inv) { 
          crab::CrabStats::count ("Domain.count.copy");
          crab::ScopedCrabStats __st__("Domain.copy");
        }
        
        array_smashing_t& operator=(const array_smashing_t& other) {
          crab::CrabStats::count ("Domain.count.copy");
          crab::ScopedCrabStats __st__("Domain.copy");
          if (this != &other)
            _inv = other._inv;
          return *this;
        }
        
        bool is_bottom() { 
          return (_inv.is_bottom ());
        }
        
        bool is_top() { 
          return (_inv.is_top());
        }
        
        bool operator<=(array_smashing_t other) {
          return (_inv <= other._inv);
        }

        void operator|=(array_smashing_t other) {
          _inv |= other._inv;
        }
        
        array_smashing_t operator|(array_smashing_t other) {
          return array_smashing_t (_inv | other._inv);
        }
        
        array_smashing_t operator&(array_smashing_t other) {
          return array_smashing_t (_inv & other._inv);
        }
        
        array_smashing_t operator||(array_smashing_t other) {
          return array_smashing_t (_inv || other._inv);
        }

        template<typename Thresholds>
        array_smashing_t widening_thresholds (array_smashing_t other, 
                                              const Thresholds &ts) {
          return array_smashing_t (_inv.widening_thresholds (other._inv, ts));
        }
        
        array_smashing_t operator&& (array_smashing_t other) {
          return array_smashing_t (_inv && other._inv);
        }
        
        void operator-=(VariableName var) {
          _inv -= var;
        }

        // remove all variables [begin,...end)
        template<typename Iterator>
        void forget (Iterator begin, Iterator end) {
          domain_traits<NumDomain>::forget (_inv, begin, end);
        }

        // dual of forget: remove all variables except [begin,...end)
        template<typename Iterator>
        void project (Iterator begin, Iterator end) {
          domain_traits<NumDomain>::project (_inv, begin, end);
        }

        void operator += (linear_constraint_system_t csts) {
          _inv += csts;
        }
        
        void assign (VariableName x, linear_expression_t e) {
          _inv.assign (x, e);
          
          CRAB_LOG("smashing",
                   crab::outs() << "apply "<< x<< " := "<< e<< *this <<"\n";);
        }
        
        void apply (operation_t op, VariableName x, VariableName y, Number z) {
          _inv.apply (op, x, y, z);
          
          CRAB_LOG("smashing",
                   crab::outs() << "apply "<< x<< " := "<< y<< " "<< op<< " "<< z<< *this <<"\n";);
        }
        
        void apply(operation_t op, VariableName x, VariableName y, VariableName z) {
          _inv.apply (op, x, y, z);
          
          CRAB_LOG("smashing",
                   crab::outs() << "apply "<< x<< " := "<< y<< " "<< op<< " "<< z<< *this <<"\n";);
        }
        
        void apply(operation_t op, VariableName x, Number k) {
          _inv.apply (op, x, k);
          
          CRAB_LOG("smashing",
                   crab::outs() << "apply "<< x<< " := "<< x<< " "<< op<< " "<< k<< *this <<"\n";);
        }

        // bitwise_operators_api
        void apply(conv_operation_t op, VariableName x, VariableName y, unsigned width) {
          _inv.apply (op, x, y, width);
        }
        
        void apply(conv_operation_t op, VariableName x, Number k, unsigned width) {
          _inv.apply (op, x, k, width);
        }
        
        void apply(bitwise_operation_t op, VariableName x, VariableName y, VariableName z) {
          _inv.apply (op, x, y, z);

          CRAB_LOG("smashing",
                   crab::outs() << "apply "<< x<< " := "<< y<< " "<< op<< " "<< z<< *this <<"\n";);
        }
        
        void apply(bitwise_operation_t op, VariableName x, VariableName y, Number k) {
          _inv.apply (op, x, y, k);

          CRAB_LOG("smashing",
                   crab::outs() << "apply "<< x<< " := "<< y<< " "<< op<< " "<< k<< *this <<"\n";);
        }
        
        // division_operators_api
        void apply(div_operation_t op, VariableName x, VariableName y, VariableName z) {
          _inv.apply (op, x, y, z);

          CRAB_LOG("smashing",
                   crab::outs() << "apply "<< x<< " := "<< y<< " "<< op<< " "<< z<< *this <<"\n";);
        }
        
        void apply(div_operation_t op, VariableName x, VariableName y, Number k) {
          _inv.apply (op, x, y, k);

          CRAB_LOG("smashing",
                   crab::outs() << "apply "<< x<< " := "<< y<< " "<< op<< " "<< k<< *this <<"\n";);
        }
        
        void array_init (VariableName a, 
                         const vector<ikos::z_number>& values) {
          if (values.empty ()) return;
          
          interval_t init = interval_t::bottom ();
          for (auto const &v: values) {
            // assume automatic conversion from z_number to bound_t
            init = init | interval_t (bound_t (v)); 
          }
          _inv.set (a, init);
          CRAB_LOG("smashing", 
                   crab::outs() << "Array init: " << *this <<"\n";);
        }
        
        // All the array elements are initialized to val
        void assume_array (VariableName a, interval_t val) {
          _inv.set (a, val);
          
          CRAB_LOG("smashing", crab::outs() << "Assume array: " << *this <<"\n";);
        }
        
        void load (VariableName lhs, VariableName a, 
                   VariableName /*i*/, z_number /*n_bytes*/) {
          
          // We need to be careful when assigning a summarized variable a
          // into a non-summarized variable lhs. Simply _inv.assign (lhs,
          // a) is not sound.
          /* ask for a temp var */
          VariableName a_prime = a.getVarFactory().get(); 
          domain_traits<NumDomain>::expand (_inv, a, a_prime);
          _inv.assign (lhs, linear_expression_t (a_prime));
          _inv -= a_prime; 
          
          CRAB_LOG("smashing", crab::outs() << "Load: " << *this <<"\n";);
        }
        
        
        void store (VariableName a, VariableName /*i*/,
                    linear_expression_t val, z_number /*n_bytes*/,
                    bool is_singleton) {
          
          if (is_singleton)
            strong_update (a, val);
          else 
            weak_update (a, val);
          
          CRAB_LOG("smashing", crab::outs() << "Store: " << *this <<"\n";);
        }
        
        linear_constraint_system_t to_linear_constraint_system (){
          return _inv.to_linear_constraint_system ();
        }
        
        NumDomain  get_content_domain () const {      
          return _inv;
        }

        void write(ostream& o) {
          o << _inv;
        }
        
        static std::string getDomainName () {
          std::string name ("ArraySmashing(" + 
                            NumDomain::getDomainName () + 
                            ")");
          return name;
        }  
        
      }; // end array_smashing
   
     template<typename BaseDomain>
     class array_domain_traits<array_smashing<BaseDomain> > {
       
      public:
       
       typedef ikos::z_number z_number;
       typedef typename BaseDomain::number_t number_t;
       typedef typename BaseDomain::varname_t varname_t;
       typedef typename BaseDomain::linear_expression_t linear_expression_t;
       typedef array_smashing<BaseDomain> array_smashing_t;
       
       static void array_init (array_smashing_t& inv, varname_t a, 
                               const vector<z_number> &values) {
         inv.array_init (a, values);
       }
       
       static void assume_array (array_smashing_t& inv, varname_t a, number_t val) {
         inv.assume_array (a, interval<number_t> (bound <number_t> (val)));
       }
       
       static void assume_array (array_smashing_t& inv, varname_t a, 
                                 interval<number_t> val) {
         inv.assume_array (a, val);
       }
       
       static void array_load (array_smashing_t& inv, 
                               varname_t lhs, varname_t a, varname_t i, 
                               z_number n_bytes) {
         inv.load (lhs, a, i, n_bytes);
       }
       
       static void array_store (array_smashing_t& inv, 
                                varname_t a, varname_t i,
                                linear_expression_t val,
                                z_number n_bytes, bool is_singleton) {
         inv.store (a, i, val, n_bytes, is_singleton);
       }
     };
   
     template<typename BaseDomain>
     class domain_traits <array_smashing<BaseDomain> > {
      public:
       
       typedef array_smashing<BaseDomain> array_smashing_t;
       typedef typename BaseDomain::varname_t VariableName;

       static void normalize (array_smashing_t& inv) { 
         CRAB_WARN ("array smashing normalize not implemented");
       }
       
       template <typename Iter>
       static void forget (array_smashing_t& inv, Iter it, Iter end) {
         inv.forget (it, end);
       }
       
       template <typename Iter >
       static void project (array_smashing_t& inv, Iter it, Iter end) {
         inv.project (it, end);
       }

       static void expand (array_smashing_t& inv, VariableName x, VariableName new_x) {
         // -- lose precision if relational or disjunctive domain
         CRAB_WARN ("array smashing expand not implemented");
       }

     };

   } // namespace domains
}// namespace crab
#endif 
