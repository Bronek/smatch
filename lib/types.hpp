#pragma once

#include <iostream>
#include <limits>
#include <stdexcept>

namespace smatch {

// This is just a shortcut
using uint = unsigned int;

// The types below must be POD, because they might be stored in an union
enum class Side : char
{
    Buy = 'B',
    Sell = 'S'
};

inline std::ostream& operator<< (std::ostream& o, Side s)
{
    return (o << static_cast<char>(s));
}

inline bool parse(Side& s, char c)
{
    switch (c)
    {
        case 'B' : s = Side::Buy; return true;
        case 'S' : s = Side::Sell; return true;
        default: return false;
    }
}

struct Order
{
    Side side;
    uint id;
    uint price;
    // All 3 store size for limit orders, and are only different for icebergs
    uint size;
    uint full;
    uint peak;
    bool add;

    // Reserved for matching engine
    size_t match;
};

struct Cancel
{
    uint id;
};

struct Match
{
    uint buyId;
    uint sellId;
    uint price;
    uint size;
};

struct exception : std::runtime_error
{
    explicit exception(const char* sz) : std::runtime_error(sz)
    { }
};

}
