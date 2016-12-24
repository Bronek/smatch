#pragma once

#include <iostream>

namespace smatch {

// This typedef is just a shortcut
typedef unsigned int uint;

// The types below must be POD, because they might be stored in an union
enum class Side { Buy, Sell };

inline std::ostream& operator<< (std::ostream& o, Side s)
{
    if (s == Side::Buy)
        return (o << 'B');
    else // if (s == Side::Sell)
        return (o << 'S');
}

struct Order
{
    Side side;
    uint id;
    uint price;
    uint size;

    // Reserved for matching engine
    size_t match;
    constexpr static size_t unmatched = (size_t)-1;
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

}
