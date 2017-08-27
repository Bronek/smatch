#pragma once

#include "types.hpp"

#include <map>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace smatch {

class Book
{
    // For storing orders in an ordered collection
    struct PricePriority {
        uint price;
        uint64_t priority; // As-if timestamp, used to prioritize orders by order received (if price same)
    };

    // For sorting of orders specific to side
    template <Side>
    struct SortOrder {
        // Since specializations can only be implemented in namespace scope, scroll to bottom
        inline constexpr bool operator()(PricePriority lh, PricePriority rh);
    };

    // Two distinct types to store buy and sell orders, because different sort order
    using buys_t = std::map<PricePriority, Order, SortOrder<Side::Buy>> ;
    using sells_t = std::map<PricePriority, Order, SortOrder<Side::Sell>>;

    // This structure is to aid finding an order in buys_ or sells_, given an id. We can simply store
    // iterators because these are never invalidated by ordinary operations we do on a map, like insert
    // or erase (of an unrelated element). Only one of these two is ever set, the other is "end" one.
    struct OrderSide {
        std::map<PricePriority, Order, SortOrder<Side::Buy>>::iterator itb;
        std::map<PricePriority, Order, SortOrder<Side::Sell>>::iterator its;
    };

    buys_t                              buys_;
    sells_t                             sells_;
    std::unordered_map<uint, OrderSide> ids_;

    // For sorting of orders by time priority, used as-if it was monotonically increasing timestamp.
    // This is only incremented inside insert(), which copies current value into PricePriority
    uint64_t                            priority_;

public:
    Book() : priority_(0)
    { }

    Order& insert(const Order& o);
    void remove(uint id);
    template <Side side> void match(Order& active, std::vector<Match>& matches);
    template <Side side> std::map<PricePriority, Order, SortOrder<side>>& orders();
};

template <> inline constexpr bool Book::SortOrder<Side::Buy>::operator()(Book::PricePriority lh, Book::PricePriority rh)
{
    // If same price, lower priority (i.e. order placed earlier) wins. Otherwise better (higher bid) price wins
    return (lh.price == rh.price) ? lh.priority < rh.priority :
                                    lh.price > rh.price;
}

template <> inline constexpr bool Book::SortOrder<Side::Sell>::operator()(Book::PricePriority lh, Book::PricePriority rh)
{
    // If same price, lower priority (i.e. order placed earlier) wins. Otherwise better (lower offer) price wins
    return (lh.price == rh.price) ? lh.priority < rh.priority :
                                    lh.price < rh.price;
}

template <> inline std::map<Book::PricePriority, Order, Book::SortOrder<Side::Buy>>& Book::orders<Side::Buy>()
{
    return buys_;
}

template <> inline std::map<Book::PricePriority, Order, Book::SortOrder<Side::Sell>>& Book::orders<Side::Sell>()
{
    return sells_;
}

}
