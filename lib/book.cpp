#include "book.hpp"

#include <algorithm>

namespace smatch {

Order* Book::insert(const Order& o)
{
    // Build PricePriority for price and priority of the order. Since on each insert
    // we bump priority_, each such constructed PricePriority will be unique.
    PricePriority pp = { o.price , ++priority_ };
    // OrderSide is what we store in ids_, to allow us to quickly find orders by id
    OrderSide os = { buysEnd_, sellsEnd_ };
    // Store an order (limit or iceberg) in an appropriate collection buys_ or sells_
    // and persist the iterator for this element in OrderSide created above.
    Order* result = nullptr;
    if (o.side == Side::Buy)
    {
        os.itb = buys_.emplace(std::make_pair(pp, o)).first;
        result = &os.itb->second;
    }
    else
    {
        os.its = sells_.emplace(std::make_pair(pp, o)).first;
        result = &os.its->second;
    }
    // Here we store the iterators with id, also enforcing that id are unique
    if (!ids_.emplace(std::make_pair(o.id, os)).second)
        throw std::runtime_error("Order id must be unique");

    return result;
}

void Book::remove(uint id)
{
    auto i = ids_.find(id);
    if (i == ids_.end())
        throw std::runtime_error("Order id is invalid");
    if (i->second.itb != buysEnd_)
        buys_.erase(i->second.itb);
    else // if (i->second.its != sellsEnd_ )
        sells_.erase(i->second.its);
    ids_.erase(i);
}

template <Side side>
void Book::match(Order& active, std::vector<Match>& matches)
{
    // Active order is on "this side" and it will be matched against orders on the "opposite side"
    constexpr auto opposite = (side == Side::Buy ? Side::Sell : Side::Buy);
    auto& orders = this->orders<opposite>();
    size_t count = 0; // Partially matched orders
    while (active.size > 0 && !orders.empty())
    {
        const auto begin = orders.begin();
        auto& top = begin->second;
        if (side == Side::Buy && active.price < top.price)
            break;
        else if (side == Side::Sell && active.price > top.price)
            break;

        const uint size = std::min(active.size, top.size);
        if (top.match == Order::unmatched)
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
        // Remove liquidity from active order
        active.size -= size;
        // Remove liquidity from top order
        top.size -= size;
        if (top.size == 0)
        {
            --count;
            ids_.erase(top.id);
            orders.erase(begin);
        }
    }

    // Clear partial matches collected so far
    size_t i = 0;
    for (auto& o : orders)
    {
        if (o.second.match == Order::unmatched)
            continue;
        o.second.match = Order::unmatched;
        if (++i == count)
            break;
    }
    active.match = Order::unmatched;
}

// Explicit instantiations of the above, for Engine::handle() to use
template void Book::match<Side::Buy>(Order&, std::vector<Match>& );
template void Book::match<Side::Sell>(Order&, std::vector<Match>& );

}
