#include "book.hpp"

#include <algorithm>

namespace smatch {

namespace {
    // Special value for Order.match, set at the end of Book::insert(). 0 is not good because the
    // purpose of Order.match is to identify index of Match in vector passed to Book::match(), and
    // of course first match added to this collection will have index 0
    static constexpr size_t unmatched = std::numeric_limits<size_t>::max();
}

Order& Book::insert(const Order& o)
{
    // BuySell is what we store in ids_, to allow us to quickly find orders by id
    const auto it = ids_.emplace(
        o.id , BuySell { buys_.end() , sells_.end() }
    );

    // Enforce that ids are unique
    if (not it.second)
        throw bad_order_id("Duplicate order id", o.id);

    // A shortcut for storing buy or sell iterator into BuySell we just inserted into ids_
    auto& os = it.first->second;

    // Build Priority for price and priority of the order. Since on each insert
    // we bump serial_, each such constructed Priority will be unique.
    const Priority pp { o.price , ++serial_ };

    // Store an order (limit or iceberg) in an appropriate collection buys_ or sells_
    // and persist the iterator for this element in BuySell created above.
    Order* ret = nullptr;
    if (o.side == Side::Buy) {
        os.buy = buys_.emplace(pp, o).first;
        ret = &os.buy->second;
    }
    else {
        os.sell = sells_.emplace(pp, o).first;
        ret = &os.sell->second;
    }
    ret->match = unmatched;
    return *ret;
}

void Book::remove(uint id)
{
    const auto i = ids_.find(id);
    if (i == ids_.end())
        throw bad_order_id("Invalid order id", id);

    if (i->second.buy != buys_.end())
        buys_.erase(i->second.buy);
    else // if (i->second.sell != sells_.end() )
        sells_.erase(i->second.sell);

    ids_.erase(i);
}

template <Side side>
void Book::match(Order& active, std::vector<Match>& matches)
{
    // Active order is on "this side" and it will be matched against orders on the "opposite side"
    constexpr auto opposite = (side == Side::Buy ? Side::Sell : Side::Buy);
    auto& orders = this->orders<opposite>();
    size_t count = 0; // Partially matched orders
    while (active.size > 0 && not orders.empty())
    {
        auto& top = orders.begin()->second;
        if (side == Side::Buy && active.price < top.price)
            break;
        else if (side == Side::Sell && active.price > top.price)
            break;

        const uint size = std::min(active.size, top.size);
        if (top.match == unmatched)
        {
            ++count;
            Match match;
            match.price = top.price;
            match.size = 0; // Increased below
            match.buyId = (side == Side::Buy ? active.id : top.id);
            match.sellId = (side == Side::Sell ? active.id : top.id);
            top.match = matches.size();
            matches.push_back(match);
        }
        matches[top.match].size += size;

        // Remove liquidity from active order, reset size if it is an iceberg
        active.full -= size;
        active.size = std::min(active.full, active.peak);

        // Remove liquidity from top order
        top.size -= size;
        top.full -= size;

        // This section could be slightly optimized by replacing calls to remove() and insert()
        // with custom tailored modifications of both ids_ and orders collections
        if (top.size == 0)
        {
            const Order copy = top;
            remove(copy.id);
            // Must not use top below this point
            if (copy.full > 0)
            {
                auto& renew = insert(copy);
                renew.match = copy.match;
                renew.size = std::min(copy.full, copy.peak);
            }
            else
                --count;
        }
    }

    // Clear partial matches collected so far
    size_t i = 0;
    for (auto& o : orders)
    {
        if (o.second.match == unmatched)
            continue;
        o.second.match = unmatched;
        if (++i == count)
            break;
    }
}

// Explicit instantiations of the above, for Engine::handle() to use
template void Book::match<Side::Buy>(Order&, std::vector<Match>& );
template void Book::match<Side::Sell>(Order&, std::vector<Match>& );

}
