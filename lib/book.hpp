#pragma once

#include "types.hpp"

#include <map>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace smatch {

class Book
{
    // For storing orders in an ordered collection, prioritized by price and order received
    struct Priority {
        uint price;
        uint64_t serial; // Order serial, used to prioritize orders by order received (if price same)
    };

    // For sorting of orders specific to side
    template <Side>
    struct Sort {
        // Since specializations can only be implemented in namespace scope, scroll to bottom
        inline constexpr bool operator()(Priority lh, Priority rh);
    };

    // Two distinct types to store buy and sell orders, because different sort order
    template <Side side> using orders_t = std::map<Priority, Order, Sort<side>>;
    using buys_t = orders_t<Side::Buy>;
    using sells_t = orders_t<Side::Sell>;

    // This structure is to aid finding an order in buys_ or sells_, given an id. We can simply store
    // iterators because these are never invalidated by ordinary operations we do on a map, like insert
    // or erase (of an unrelated element). Only one of these two is ever set, the other is "end" one.
    struct BuySell {
        orders_t<Side::Buy>::iterator buy;
        orders_t<Side::Sell>::iterator sell;
    };

    buys_t                              buys_;
    sells_t                             sells_;
    std::unordered_map<uint, BuySell>   ids_;

    // For sorting of orders by order received. This is only incremented inside insert(), which copies
    // current value into Priority
    uint64_t                            serial_;

    template <Side side> orders_t<side>& orders()
    {
        // Mutable version implemented in terms of immutable one (below)
        return const_cast<orders_t<side>&>(
                (const_cast<const Book&>(*this)).orders<side>()
        );
    }

public:
    Book() : serial_(0)
    { }

    template <Side side> constexpr const orders_t<side>& orders() const;

    Order& insert(const Order& o);
    void remove(uint id);
    template <Side side> void match(Order& active, std::vector<Match>& matches);
};

template <> inline constexpr bool Book::Sort<Side::Buy>::operator()(Book::Priority lh, Book::Priority rh)
{
    // If same price, lower priority (i.e. order placed earlier) wins. Otherwise better (higher bid) price wins
    return (lh.price == rh.price) ? lh.serial < rh.serial
                                  : lh.price > rh.price;
}

template <> inline constexpr bool Book::Sort<Side::Sell>::operator()(Book::Priority lh, Book::Priority rh)
{
    // If same price, lower priority (i.e. order placed earlier) wins. Otherwise better (lower offer) price wins
    return (lh.price == rh.price) ? lh.serial < rh.serial
                                  : lh.price < rh.price;
}

template <> inline constexpr const Book::orders_t<Side::Buy>& Book::orders<Side::Buy>() const
{
    return buys_;
}

template <> inline constexpr const Book::orders_t<Side::Sell>& Book::orders<Side::Sell>() const
{
    return sells_;
}

}
