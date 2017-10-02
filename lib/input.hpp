#pragma once

#include "types.hpp"
#include "engine.hpp"

#include <stdexcept>

namespace smatch {

// Employ Visitor pattern using a different kind of dynamic polymorphism without vtable that is,
// rather than 2 levels of indirection (vtable and function pointer) use just one i.e. function
// pointer alone. Also use overloading of constructors i.e. static polymorphism to complement.
class Input
{
    union In {
        Order o;
        Cancel c;
    } input;

    typedef bool (Input::*Type)(Engine&, Engine::matches_t&) const;
    Type type;

    // The return value is to be set by Engine and interpreted by caller of update()
    template <Side side> bool order(Engine& e, Engine::matches_t& m) const
    {
        return e.order<side>(input.o, m);
    }

    bool cancel(Engine& e, Engine::matches_t&) const
    {
        return e.cancel(input.c);
    }

    bool noop(Engine&, Engine::matches_t&) const
    {
        return false;
    }

public:
    Input() : type(&Input::noop)
    { }

    explicit Input(const Order& o)
        : type(o.side == Side::Buy ? &Input::order<Side::Buy> : &Input::order<Side::Sell>)
    {
        input.o = o;
    }

    explicit Input(const Cancel& c) : type(&Input::cancel)
    {
        input.c = c;
    }

    bool handle(Engine& e, Engine::matches_t& m) const
    {
        return (this->*type)(e, m);
    }

    bool empty() const { return type == &Input::noop; }
};

}
