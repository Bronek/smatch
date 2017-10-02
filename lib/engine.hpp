#pragma once

#include "types.hpp"
#include "book.hpp"
#include "stream.hpp"

#include <vector>

namespace smatch {

class Engine
{
    Book                book_;

public:
    using matches_t = std::vector<Match>;
    constexpr const auto& book() const { return book_; }

    template <Side side>
    bool order(const Order& o, matches_t& matches)
    {
        // Empty collection of matches on input is important precondition for the matching algorithm
        matches.clear();

        // Copy order received, perform matching first
        Order active = o;
        book_.match<side>(active, matches);

        // If there is any remaining liquidity in the active order, add it to the book_
        if (active.add && active.size > 0)
            book_.insert(active);

        return not matches.empty();
    }

    bool cancel(const Cancel& c)
    {
        book_.remove(c.id);
        return false; // No matching performed
    }
};

}
