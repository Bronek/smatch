#pragma once

#include "types.hpp"
#include "input.hpp"
#include "book.hpp"
#include "streams.hpp"

#include <vector>

namespace smatch {

class Engine
{
    Book    book_;
    Order   active_;

    // The two functions below are not meant to be public interface of Engine, so we make them private.
    // However Input needs to access them, so make it a friend. Paradoxically this improves encapsulation
    // because it means we leak less outside, but at the cost of having to maintain discipline in Input.
    friend class Input;

    bool order(const Order& o)
    {
        // Copy order received to active_ order
        active_ = o;
        return true;
    }

    bool cancel(const Cancel& c)
    {
        book_.remove(c.id);
        return false; // No matching to be performed
    }

public:
    Engine() { }

    template <typename In, typename Out>
    void run(In& in, Out& out)
    {
        // Use overloading to construct writer appropriate for Out type
        auto wr = writer(out);
        Input i;
        // Use overloading to populate Input from In type
        while (read(i, in))
        {
            handle(i, wr);
        }
    }

    template <typename Writer>
    void handle(const Input& i, Writer& wr)
    {
        if (i.empty())
            return;

        // This vector is used as-if on stack, but created only once as performance optimization
        static thread_local std::vector<Match> matches;

        // The return value of Input.update() is either from order() or cancel() above
        if (i.handle(*this))
        {
            if (active_.side == Side::Buy)
                book_.match<Side::Buy>(active_, matches);
            else
                book_.match<Side::Sell>(active_, matches);

            // If there is any remaining liquidity in the active_ order, add it to the book
            if (active_.size > 0)
                book_.insert(active_);

            for (const auto& m : matches)
                wr.write(m);

            matches.clear();
        }
        // else no matching needed

        for (const auto& b : book_.orders<Side::Buy>())
            wr.write(b.second);
        for (const auto& s : book_.orders<Side::Sell>())
            wr.write(s.second);
    }
};

}
