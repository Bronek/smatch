#pragma once

#include "types.hpp"
#include "input.hpp"
#include "book.hpp"
#include "streams.hpp"

#include <vector>

namespace smatch {

class Engine
{
    Book                book_;
    std::vector<Match>  matches_;

public:
    constexpr const auto& book() const { return book_; }
    constexpr const auto& matches() const { return matches_; }

    template <Side side>
    bool order(const Order& o)
    {
        // Vector matches_ is used as-if on stack, but must be accessible after exit from this function
        matches_.clear();

        // Copy order received, perform matching first
        Order active = o;
        book_.match<side>(active, matches_);

        // If there is any remaining liquidity in the active order, add it to the book_
        if (active.size > 0)
            book_.insert(active);

        return not matches_.empty();
    }

    bool cancel(const Cancel& c)
    {
        book_.remove(c.id);
        return false; // No matching performed
    }
};

}
