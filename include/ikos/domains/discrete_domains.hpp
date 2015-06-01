/*******************************************************************************
 * An implementation of discrete domains based on Patricia trees.
 ******************************************************************************/


#ifndef IKOS_DISCRETE_DOMAINS_HPP
#define IKOS_DISCRETE_DOMAINS_HPP

#include <iostream>
#include <ikos/common/types.hpp>
#include <ikos/algorithms/patricia_trees.hpp>
#include <ikos/domains/separate_domains.hpp>

namespace ikos {

  template< typename Element >
  class discrete_domain: public writeable {
    
  private:
    typedef patricia_tree_set< Element > ptset_t;

  public:
    typedef discrete_domain< Element > discrete_domain_t;
    typedef collection< Element > collection_t;
    typedef typename ptset_t::iterator iterator;

  private:
    bool _is_top;
    ptset_t _set;

  private:
    discrete_domain(bool is_top): _is_top(is_top) { }

    discrete_domain(ptset_t set): _is_top(false), _set(set) { }

  public:
    static discrete_domain_t bottom() {
      return discrete_domain_t(false);
    }
    
    static discrete_domain_t top() {
      return discrete_domain_t(true);
    }

  public:
    discrete_domain(): _is_top(true) { }
    
    discrete_domain(const discrete_domain_t& other): writeable(), _is_top(other._is_top), _set(other._set) { }

    discrete_domain(Element s): _is_top(false), _set(s) { }

    discrete_domain(collection_t c): _is_top(false) {
      for (typename collection_t::iterator it = c.begin(); it != c.end(); ++it) {
	this->_set += *it;
      }
    }

    bool is_top() {
      return this->_is_top;
    }

    bool is_bottom() {
      return (!this->_is_top && this->_set.is_empty());
    }

    bool operator<=(discrete_domain_t other) {
      return other._is_top || (!this->_is_top && this->_set <= other._set);
    }

    bool operator==(discrete_domain_t other) {
      return (this->_is_top && other._is_top) || (this->_set == other._set);
    }

    discrete_domain_t operator|(discrete_domain_t other) {
      if (this->_is_top || other._is_top) {
	return discrete_domain_t(true);
      } else {
	return discrete_domain_t(this->_set | other._set);
      }
    }
    
    discrete_domain_t operator&(discrete_domain_t other) {
      if (this->is_bottom() || other.is_bottom()) {
	return discrete_domain_t(false);
      } else if (this->_is_top) {
	return other;
      } else if (other._is_top) {
	return *this;
      } else {
	return discrete_domain_t(this->_set & other._set);
      }
    }

    discrete_domain_t operator||(discrete_domain_t other) {
      return this->operator|(other);
    }
    
    discrete_domain_t operator&&(discrete_domain_t other) {
      return this->operator&(other);
    }

    discrete_domain_t& operator+=(Element s) {
      if (!this->_is_top) {
	this->_set += s;
      }
      return *this;
    }

    discrete_domain_t& operator+=(collection_t c) {
      if (!this->_is_top) {
	for (typename collection_t::iterator it = c.begin(); it != c.end(); ++it) {
	  this->_set += *it;
	}
      }
      return *this;
    }

    discrete_domain_t operator+(Element s) {
      discrete_domain_t r(*this);
      r.operator+=(s);
      return r;
    }

    discrete_domain_t operator+(collection_t c) {
      discrete_domain_t r(*this);
      r.operator+=(c);
      return r;
    }

    discrete_domain_t& operator-=(Element s) {
      if (!this->_is_top) {
	this->_set -= s;
      }
      return *this;
    }

    discrete_domain_t& operator-=(collection_t c) {
      if (!this->_is_top) {
	for (typename collection_t::iterator it = c.begin(); it != c.end(); ++it) {
	  this->_set -= *it;
	}
      }
      return *this;
    }

    discrete_domain_t operator-(Element s) {
      discrete_domain_t r(*this);
      r.operator-=(s);
      return r;
    }

    discrete_domain_t operator-(collection_t c) {
      discrete_domain_t r(*this);
      r.operator-=(c);
      return r;
    }

    std::size_t size() {
      if (this->_is_top) {
	throw error("Size for discrete domain TOP is undefined");
      } else {
	return this->_set.size();
      }
    }
    
    iterator begin() {
      if (this->_is_top) {
	throw error("Iterator for discrete domain TOP is undefined");
      } else {
	return this->_set.begin();
      }
    }
    
    iterator end() {
      if (this->_is_top) {
	throw error("Iterator for discrete domain TOP is undefined");
      } else {
	return this->_set.end();
      }
    }
    
    std::ostream& write(std::ostream& o) {
      if (this->_is_top) {
	o << "{...}";
      } else if (this->_set.is_empty()) {
	o << "_|_";
      } else {
	o << this->_set;
      }
      return o;
    }
    
  }; // class discrete_domain

} // namespace ikos

#endif // IKOS_DISCRETE_DOMAINS_HPP
