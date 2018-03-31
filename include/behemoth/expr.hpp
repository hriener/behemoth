/* behemoth: A syntax-guided synthesis library
 * Copyright (C) 2018  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace behemoth
{

/******************************************************************************
 * expr_attr                                                                  *
 ******************************************************************************/

enum expr_attr_enum
{
  _no                    = 0,
  _no_double_application = 1,
  _idempotent            = 1 << 1, /* repetition is immaterial */
  _commutative           = 1 << 2  /* order is immaterial */
}; // expr_attr

using expr_attr = unsigned;

/******************************************************************************
 * expr_node                                                                  *
 ******************************************************************************/

struct expr_node
{
  expr_node( std::string name, const std::vector<unsigned>& children, const expr_attr attr = expr_attr_enum::_no )
    : _name( name )
    , _children( children )
    , _attr( attr )
  {}

  bool operator==( const expr_node& e ) const
  {
    if ( _name != e._name ) return false;
    if ( _children.size() != e._children.size() ) return false;
    for ( auto i = 0; i < _children.size(); ++i )
    {
      if ( _children[i] != e._children[i] )
      {
        return false;
      }
    }
    return true;
  }

  std::string _name;
  std::vector<unsigned> _children;
  expr_attr _attr;
}; // expr_node

struct expr_hash : public std::unary_function<expr_node, std::size_t>
{
  std::size_t operator()(const expr_node& e) const
  {
    auto seed = std::hash<std::string>{}( e._name );
    for ( const auto& u : e._children )
    {
      seed ^= (std::hash<unsigned>{}(u) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 ));
    }
    return seed;
  }
}; // expr_hash

/******************************************************************************
 * context                                                                    *
 ******************************************************************************/

class context
{
private:
  using fun_strash_map_t = std::unordered_map<expr_node, unsigned, expr_hash >;

public:
  unsigned make_fun( const std::string& name, const std::vector<unsigned>& children = {}, const expr_attr attr = expr_attr_enum::_no )
  {
    const auto e = expr_node( name, children, attr );

    /* structural hashing */
    const auto it = _fun_strash.find( e );
    if ( it != _fun_strash.end() )
    {
      return it->second;
    }

    const auto index = _exprs.size();
    _exprs.push_back( e );
    _fun_strash[e] = index;
    return index;
  }

  unsigned count_nonterminals( unsigned e ) const
  {
    const auto& expr = _exprs[ e ];
    if ( expr._name[0] == '_' )
    {
      return 1;
    }

    auto counter = 0u;
    for ( const auto& c : expr._children )
    {
      counter += count_nonterminals( c );
    }

    return counter;
  }

  unsigned count_nodes( unsigned e ) const
  {
    const auto& expr = _exprs[ e ];
    if ( expr._children.size() == 0u )
    {
      return 1;
    }

    auto counter = 1u;
    for ( const auto& c : expr._children )
    {
      counter += count_nodes( c );
    }

    return counter;
  }

  fun_strash_map_t _fun_strash;
  std::vector<expr_node> _exprs;
}; // context

class expr_printer
{
public:
  expr_printer( const context& ctx )
    : _ctx( ctx )
  {}

  virtual std::string as_string( unsigned e ) const
  {
    const auto& expr = _ctx._exprs[ e ];

    auto str = expr._name;
    if ( expr._children.size() > 0u )
    {
      str += '(';
      str += as_string( expr._children[ 0u ] );
      for ( auto i = 1u; i < expr._children.size(); ++i )
      {
        str += ',';
        str += as_string( expr._children[ i ] );
      }
      str += ')';
    }

    return str;
  }

  const context& _ctx;
};

} // namespace behemoth

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
