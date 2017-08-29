#pragma once

#include "types.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdio>

namespace smatch {

// Need forward declaration here
class Input;

struct bad_input : smatch::exception
{
    using exception::exception;
};

struct Stream
{
    std::istream& in;
    std::ostream& out;

    Stream(std::istream& in, std::ostream& out) : in(in), out(out)
    { }

    bool read(Input&);

    void write(const Match& m)
    {
        out << "M " << m.buyId << ' ' << m.sellId << ' ' << m.price << ' ' << m.size << std::endl;
    }

    void write(const Order& o)
    {
        out << "O " << o.side << ' ' << o.id << ' ' << o.price << ' ' << o.size << std::endl;
    }

    bool report(const exception& e, bool);
};

inline Stream channel(std::istream& in, std::ostream& out)
{
    return Stream(in, out);
}

}
