#pragma once

#include "types.hpp"

#include <stdexcept>

namespace smatch {

// Need forward declaration here
class Engine;

// Employ Visitor pattern using a different kind of dynamic polymorphism without vtable that is,
// rather than 2 levels of indirection (vtable and function pointer) use just one i.e. function
// pointer alone. Also use overloading of constructors i.e. static polymorphism to complement.
class Input
{
    union In {
        Order o;
        Cancel c;
    } input;

    typedef bool (Input::*Type)(Engine&) const;
    Type type;

    // These need to be defined in a .cpp, after full definition of Engine. The return value is
    // to be set by callee of the functions below (Engine) and interpreted by caller of update()
    template <Side side> bool order(Engine&) const;
    bool cancel(Engine&) const;
    bool noop(Engine&) const
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

    bool handle(Engine& e) const
    {
        return (this->*type)(e);
    }

    bool empty() const { return type == &Input::noop; }
};

}
