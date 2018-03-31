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

#include <behemoth/expr.hpp>
#include <behemoth/enumerator.hpp>
#include <cli11/CLI11.hpp>
#include <iostream>

class counting_enumerator : public behemoth::enumerator
{
public:
  counting_enumerator( behemoth::context& ctx, const behemoth::expr_printer& printer, const behemoth::rules_t& rules, int max_cost )
    : enumerator( ctx, rules, max_cost )
    , printer( printer )
  {}

  virtual void on_concrete_expression( behemoth::cexpr_t e ) override
  {
    std::cout << printer.as_string( e.first ) << ' ' << e.second << std::endl;
    ++number_of_expressions;
  }

  void print_statistics()
  {
    std::cerr << "#enumerated expressions: " << number_of_expressions << std::endl;
  }

  unsigned long number_of_expressions = 0u;
  const behemoth::expr_printer& printer;
}; // counting_enumerator

class ltl_expr_printer : public behemoth::expr_printer
{
public:
  ltl_expr_printer( behemoth::context& ctx )
    : expr_printer( ctx )
  {}

  virtual std::string as_string( unsigned e ) const override
  {
    const auto& expr = _ctx._exprs[ e ];

    if ( expr._children.size() == 0u )
    {
      return expr._name;
    }
    else if ( expr._children.size() == 1u )
    {
      return fmt::format( "{}({})",
                          expr._name,
                          as_string( expr._children[ 0u ] ) );
    }
    else if ( expr._children.size() == 2u )
    {
      return fmt::format( "(({}){}({}))",
                          as_string( expr._children[ 0u ] ),
                          expr._name,
                          as_string( expr._children[ 1u ] ) );
    }
    else
    {
      throw std::string( "unsupported number of children in LTL formula" );
    }
  }

}; // ltl_expr_printer

int main( int argc, char *argv[] )
{
  behemoth::context ctx;
  ltl_expr_printer printer( ctx );

  CLI::App app{ "Demo application for enumerating simple LTL formulae over a fixed number of variables" };

  int num_variables = 3;
  app.add_option( "-v,--vars", num_variables, "Number of variables" );

  int max_cost = 5;
  app.add_option( "-c,--cost", max_cost, "Maximum bound on the number of rules" );

  std::vector<behemoth::rule_t> rules;

  CLI11_PARSE( app, argc, argv );

  const auto _N = ctx.make_fun( "_N" );
  const auto _not = ctx.make_fun( "!", { _N }, behemoth::expr_attr_enum::_no_double_application );
  const auto _and = ctx.make_fun( "&", { _N, _N }, behemoth::expr_attr_enum::_idempotent | behemoth::expr_attr_enum::_commutative );
  const auto _or = ctx.make_fun( "|", { _N, _N }, behemoth::expr_attr_enum::_idempotent | behemoth::expr_attr_enum::_commutative  );
  const auto _globally = ctx.make_fun( "G", { _N }, behemoth::expr_attr_enum::_no_double_application );
  const auto _eventually = ctx.make_fun( "F", { _N }, behemoth::expr_attr_enum::_no_double_application );
  const auto _next = ctx.make_fun( "X", { _N } );
  const auto _until = ctx.make_fun( "U", { _N, _N }, behemoth::expr_attr_enum::_idempotent );

  rules.push_back( behemoth::rule_t{ _N, _not } );
  rules.push_back( behemoth::rule_t{ _N, _and } );
  rules.push_back( behemoth::rule_t{ _N, _or } );
  rules.push_back( behemoth::rule_t{ _N, _globally } );
  rules.push_back( behemoth::rule_t{ _N, _eventually } );
  rules.push_back( behemoth::rule_t{ _N, _next } );
  rules.push_back( behemoth::rule_t{ _N, _until } );

  for ( auto i = 0; i < num_variables; ++i )
  {
    const auto v = ctx.make_fun( fmt::format( "x{}", i ) );
    rules.push_back( behemoth::rule_t{ _N, v } );
  }

  counting_enumerator en( ctx, printer, rules, max_cost );
  en.add_expression( _N );
  while ( en.is_running() )
  {
    en.deduce();
  }
  en.print_statistics();
  
  return 0;
}
