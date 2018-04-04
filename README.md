# behemoth

<img src="https://cdn.rawgit.com/hriener/behemoth/master/behemoth.svg" width="64" height="64" align="left" style="margin-right: 12pt" />
behemoth is a C++ library for syntax-guided enumeration and synthesis.

## Example

```c++
#include <behemoth/enumerator.hpp>

behemoth::context ctx;
ctl_expr_printer printer( ctx );

const auto _N = ctx.make_fun( "_N" );
const auto _not = ctx.make_fun( "not", { _N },     expr_attr_enum::_no_double_application );
const auto _and = ctx.make_fun( "and", { _N, _N }, expr_attr_enum::_idempotent | expr_attr_enum::_commutative );

rules.push_back( rule_t{ _N, _not, /* cost = */0u } );
rules.push_back( rule_t{ _N, _and } );

const unsigned num_variabels = 3u;
for ( auto i = 0; i < num_variables; ++i )
{
  const auto v = ctx.make_fun( fmt::format( "x{}", i ) );
  rules.push_back( rule_t{ _N, v } );
}

enumerator en( ctx, printer, rules, max_cost );
en.add_expression( _N );
while ( en.is_running() )
{
  en.deduce();
}
en.print_statistics();
```

