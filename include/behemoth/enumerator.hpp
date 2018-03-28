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

#include <behemoth/expr.hpp>
#include <queue>
#include <iostream>
#include <cassert>
#include <fmt/format.h>

namespace behemoth
{

struct rule_t
{
  unsigned match;
  unsigned replace;
};

using rules_t = std::vector<rule_t>;

/* expression with associated cost */
using cexpr_t = std::pair<unsigned,unsigned>;

struct expr_greater_than
{
  expr_greater_than( context& ctx )
    : _ctx( ctx )
  {}

  bool operator()(const cexpr_t& a, const cexpr_t& b) const
  {
    /* higher costs means greater */
    if ( a.second > b.second ) return true;
    if ( a.second < b.second ) return false;

    /* more non-terminals means greater */
    if ( _ctx.count_nonterminals( a.first ) > _ctx.count_nonterminals( b.first ) ) return true;
    if ( _ctx.count_nonterminals( a.first ) < _ctx.count_nonterminals( b.first ) ) return false;

    /* more nodes means greater */
    if ( _ctx.count_nodes( a.first ) > _ctx.count_nodes( b.first ) ) return true;
    if ( _ctx.count_nodes( a.first ) < _ctx.count_nodes( b.first ) ) return false;

    return false;
  }

  context& _ctx;
}; // expr_greater_than

struct path_t
{
  path_t ( unsigned initial_depth = std::numeric_limits<unsigned>::max() )
    : depth( initial_depth )
  {}

  bool operator<( const path_t& other ) const
  {
    return ( depth < other.depth );
  }

  std::string as_string() const
  {
    std::string s = "[";
    for ( auto i = 0u; i < indices.size(); ++i )
    {
      s += fmt::format( "%s", indices[indices.size()-1u-i] );
    }
    s += "] ";
    s += invalid() ? "∞" : ( fmt::format( "%d", depth ) );
    return s;
  }

  inline unsigned operator[]( std::size_t i )
  {
    return indices[indices.size()-1u-i];
  }

  inline void push_front( unsigned v )
  {
    indices.push_back( v );
  }

  inline void pop_front()
  {
    indices.pop_back();
  }

  inline void incr_depth()
  {
    ++depth;
  }

  inline bool invalid() const
  {
    return indices.empty() && depth == std::numeric_limits<unsigned>::max();
  }

  inline bool valid() const
  {
    return !invalid();
  }

  /* indices in reverse order */
  std::vector<unsigned> indices;
  unsigned depth;
}; // path_t

std::vector<unsigned> refine_expression_recurse( context& ctx, unsigned e, path_t path, const rules_t& rules )
{
  if ( path.indices.size() == 0u )
  {
    /* apply all rules */
    std::vector<unsigned> results;
    for ( const auto& r : rules )
    {
      if ( e == r.match )
      {
        results.push_back( r.replace );
      }
    }
    return results;
  }

  auto index = path[ 0u ];
  path.pop_front();

  auto candidates = refine_expression_recurse( ctx, ctx._exprs[ e ]._children[ index ], path, rules );

  std::vector<unsigned> results;
  for ( const auto& c : candidates )
  {
    std::vector<unsigned> new_children;

    /* copy the children before index */
    for ( auto i = 0u; i < index; ++i )
    {
      new_children.push_back( ctx._exprs[ e ]._children[ i ] );
    }

    /* add new instantiation */
    new_children.push_back( c );

    /* copy the children after index */
    for ( auto i = index+1; i < ctx._exprs[ e ]._children.size(); ++i )
    {
      new_children.push_back( ctx._exprs[ e ]._children[ i ] );
    }

    results.push_back( ctx.make_fun( ctx._exprs[ e ]._name, new_children, ctx._exprs[ e ]._attr ) );
  }

  return results;
}

path_t get_path_to_concretizable_element( context& ctx, unsigned e )
{
  /* non-terminal */
  if ( ctx._exprs[ e ]._name[0] == '_' )
  {
    return path_t( 0u );
  }

  /* variable or constant */
  if ( ctx._exprs[ e ]._children.size() == 0u )
  {
    return path_t( std::numeric_limits<unsigned>::max() );
  }

  /* others */
  path_t min_path;
  min_path.depth = std::numeric_limits<unsigned>::max();
  for ( auto i = 0u; i < ctx._exprs[e]._children.size(); ++i )
  {
    auto path = get_path_to_concretizable_element( ctx, ctx._exprs[e]._children[ i ] );
    if ( path < min_path )
    {
      auto new_path = path;
      new_path.push_front( i );
      min_path = new_path;
    }
  }

  if ( min_path < std::numeric_limits<unsigned>::max() )
  {
    min_path.incr_depth();
  }

  return min_path;
}

bool is_concrete( context& ctx, unsigned e )
{
  return get_path_to_concretizable_element( ctx, e ).invalid();
}

class enumerator
{
public:
  using expr_queue_t = std::priority_queue<cexpr_t, std::vector<cexpr_t>, expr_greater_than>;

public:
  enumerator( context& ctx, const rules_t& rules, int max_cost )
    : ctx( ctx )
    , rules( rules )
    , max_cost( max_cost )
    , candidate_expressions( ctx ) /* pass ctx to the expr_greater_than */
  {}

  virtual ~enumerator() {}

  void add_expression( unsigned e );
  void deduce( unsigned number_of_steps = 1u );

  virtual bool is_redundant_in_search_order( unsigned e ) const;

  bool check_double_negation( unsigned e ) const;

  virtual void on_expression( cexpr_t e )
  {
    (void)e;
  }

  virtual void on_concrete_expression( cexpr_t e )
  {
    (void)e;
  }

  virtual void on_abstract_expression( cexpr_t e )
  {
    candidate_expressions.push( e );
  }

  void signal_termination()
  {
    quit_enumeration = true;
  }

  bool is_running() const
  {
    return !quit_enumeration;
  }

protected:
  context& ctx;
  bool quit_enumeration = false;

  rules_t rules;
  int max_cost;
  expr_queue_t candidate_expressions;

  unsigned current_costs = 0u;
};

void enumerator::add_expression( unsigned e )
{
  candidate_expressions.push( { e, 0u } );
}

void enumerator::deduce( unsigned number_of_steps )
{
  for ( auto i = 0u; i < number_of_steps; ++i )
  {
    if ( candidate_expressions.empty() )
    {
      quit_enumeration = true;
    }

    if ( !is_running() ) { return; }

    auto next_candidate = candidate_expressions.top();
    candidate_expressions.pop();

    if ( next_candidate.second > current_costs )
    {
      std::cout << "[i] finished considered expressions of cost " << (current_costs+1u) << std::endl;
      current_costs = next_candidate.second;
    }

    if ( next_candidate.second >= max_cost )
    {
      quit_enumeration = true;
      continue;
    }

    auto p = get_path_to_concretizable_element( ctx, next_candidate.first );
    auto new_candidates = refine_expression_recurse( ctx, next_candidate.first, p, rules );
    for ( const auto& c : new_candidates )
    {
      if ( !is_running() ) break;
      if ( is_redundant_in_search_order( c ) ) continue;

      auto cc = cexpr_t{ c, next_candidate.second+1 };
      on_expression( cc );

      if ( is_concrete( ctx, c ) )
      {
        on_concrete_expression(cc);
      }
      else
      {
        on_abstract_expression(cc);
      }
    }
  }
}

bool enumerator::check_double_negation( unsigned e ) const
{
  const auto expr = ctx._exprs[ e ];

  /* no double-negation */
  if ( expr._name[0] != '_' && expr._attr == expr_attr::_not )
  {
    assert( expr._children.size() == 1u );
    const auto child0 = ctx._exprs[ expr._children[0u] ];
    if ( child0._name[0] != '_' && child0._attr == expr_attr::_not )
    {
      return true;
    }
  }

  for ( const auto& c : expr._children )
  {
    if ( check_double_negation( c ) )
    {
      return true;
    }
  }

  return false;
}

bool enumerator::is_redundant_in_search_order( unsigned e ) const
{
  if ( check_double_negation( e ) )
  {
    return true;
  }

  const auto expr = ctx._exprs[ e ];
  if ( expr._name[0] != '_' && expr._children.size() == 2u && expr._attr == expr_attr::_commutative )
  {
    if ( (ctx.count_nonterminals( expr._children[0u] ) == 0) &&
         (ctx.count_nonterminals( expr._children[1u] ) == 0) &&
         expr._children[0u] > expr._children[1u] )
    {
      // expr_printer printer( ctx );
      // std::cout << "REDUNDANT: " << printer.as_string( e ) << std::endl;
      return true;
    }
  }

  /* keep all other expressions */
  return false;
}

} // namespace behemoth

// Local Variables:
// c-basic-offset: 2
// eval: (c-set-offset 'substatement-open 0)
// eval: (c-set-offset 'innamespace 0)
// End:
