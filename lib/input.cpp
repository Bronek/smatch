#include "input.hpp"
#include "engine.hpp"

namespace smatch {

bool Input::order(Engine& e) const
{
    return e.order(input.o);
}

bool Input::cancel(Engine& e) const
{
    return e.cancel(input.c);
}

}
