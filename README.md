# behemoth

<img src="https://cdn.rawgit.com/hriener/behemoth/master/behemoth.svg" width="64" height="64" align="left" style="margin-right: 12pt" />
behemoth is a C++ library for syntax-guided enumeration and synthesis.

## Example

The following code snippet enumerates expressions over a simple grammar with the two function symbols `not(.)` and `and(.,.)`.

```c++
#include <behemoth/enumerator.hpp>

behemoth::context ctx;
expr_printer printer( ctx );

/* symbols */
const auto _N = ctx.make_fun( "_N" );
const auto _not = ctx.make_fun( "not", { _N },
                  expr_attr_enum::_no_double_application );
const auto _and = ctx.make_fun( "and", { _N, _N },
                  expr_attr_enum::_idempotent | expr_attr_enum::_commutative );

/* grammar rules */
std::vector<rule_t> rules;
rules.push_back( rule_t{ _N, _not, /* cost = */0u } );
rules.push_back( rule_t{ _N, _and } );
for ( auto i = 0; i < /* num_variables = */ 3; ++i )
{
  const auto v = ctx.make_fun( fmt::format( "x{}", i ) );
  rules.push_back( rule_t{ _N, v } );
}

/* enumerate expressions up to cost bound 5 */
enumerator en( ctx, printer, rules, /* cost bound = */ 5 );
en.add_expression( _N );
while ( en.is_running() )
{
  en.deduce();
}
en.print_statistics();
```

