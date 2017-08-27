#include "input.hpp"
#include "engine.hpp"

namespace smatch {

template <Side side>
bool Input::order(Engine& e) const
{
    return e.order<side>(input.o);
}

bool Input::cancel(Engine& e) const
{
    return e.cancel(input.c);
}

// Explicit instantiations of the above, for Input::handle() to use
template bool Input::order<Side::Buy>(Engine& ) const;
template bool Input::order<Side::Sell>(Engine& ) const;

}
