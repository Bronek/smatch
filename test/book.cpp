#include "catch.hpp"

#include "book.hpp"

TEST_CASE("insert and remove orders", "[exceptions][book]") {
    using namespace smatch;
    Book book;
    const Book& cbook = book; // shortcut for 'const_cast<const Book&>(book)'

    REQUIRE(cbook.orders<Side::Buy>().empty());
    REQUIRE(cbook.orders<Side::Sell>().empty());

    auto& o1 = book.insert(Order{Side::Buy, 1, 1020, 30, 50, 40, true, 0});
    REQUIRE(o1.side == Side::Buy);
    REQUIRE(o1.id == 1);
    REQUIRE(o1.price == 1020);
    REQUIRE(o1.size == 30);
    REQUIRE(o1.full == 50);
    REQUIRE(o1.peak == 40);
    REQUIRE(o1.add == true);
    // REQUIRE(o1.match == 0); not subject to testing, it is internal to book

    REQUIRE(not cbook.orders<Side::Buy>().empty());
    REQUIRE(cbook.orders<Side::Buy>().size() == 1);
    REQUIRE(cbook.orders<Side::Sell>().empty());

    const auto os1 = cbook.orders<Side::Buy>().begin();
    REQUIRE(os1->first.price == o1.price);
    REQUIRE(os1->first.serial == 1);
    REQUIRE(&os1->second == &o1);

    auto& o2 = book.insert(Order{Side::Buy, 2, 1030, 20, 20, 20, true, 0});
    REQUIRE(o2.side == Side::Buy);
    REQUIRE(o2.id == 2);
    REQUIRE(o2.price == 1030);
    REQUIRE(o2.size == 20);
    REQUIRE(o2.full == 20);
    REQUIRE(o2.peak == 20);
    REQUIRE(o2.add == true);

    REQUIRE(cbook.orders<Side::Buy>().size() == 2);
    REQUIRE(cbook.orders<Side::Sell>().empty());

    // New order with more aggressive buy price of 1030 should become the new top order
    const auto os2 = cbook.orders<Side::Buy>().begin();
    REQUIRE(os2->first.price == o2.price);
    REQUIRE(os2->first.serial == 2);
    REQUIRE(&os2->second == &o2);

    // Iterator to os1 is still valid, and is now the second order from top
    REQUIRE((++cbook.orders<Side::Buy>().begin()) == os1);
    REQUIRE(&os1->second == &o1);

    // Insert duplicate order id on same and opposite side
    REQUIRE_THROWS_AS(book.insert(Order{Side::Buy, 1, 1020, 30, 50, 40, true, 0}), smatch::bad_order_id);
    REQUIRE_THROWS_AS(book.insert(Order{Side::Sell, 2, 0, 0, 0, 0, false, 0}), smatch::bad_order_id);

    book.remove(1);
    REQUIRE(cbook.orders<Side::Buy>().size() == 1);
    REQUIRE(cbook.orders<Side::Sell>().empty());

    const auto os2b = cbook.orders<Side::Buy>().begin();
    REQUIRE(os2b->second.side == Side::Buy);
    REQUIRE(os2b->second.id == 2);
    REQUIRE(os2b->second.price == 1030);
    REQUIRE(os2b->second.size == 20);
    REQUIRE(&os2b->second == &o2);

    // Remove order which was already removed
    REQUIRE_THROWS_AS(book.remove(1), smatch::bad_order_id);

    book.remove(2);
    REQUIRE(cbook.orders<Side::Buy>().empty());
    REQUIRE(cbook.orders<Side::Sell>().empty());

    // Add another order, reuse old id (we do not remember ids of removed orders)
    auto& o3 = book.insert(Order{Side::Sell, 1, 1010, 20, 50, 20, true, 0});
    REQUIRE(o3.side == Side::Sell);
    REQUIRE(o3.id == 1);
    REQUIRE(o3.price == 1010);
    REQUIRE(o3.size == 20);
    REQUIRE(o3.full == 50);
    REQUIRE(o3.peak == 20);
    REQUIRE(o3.add == true);

    REQUIRE(cbook.orders<Side::Buy>().empty());
    REQUIRE(cbook.orders<Side::Sell>().size() == 1);

    // New order is now top of the book, with bumped serial
    const auto os3 = cbook.orders<Side::Sell>().begin();
    REQUIRE(os3->first.price == o3.price);
    REQUIRE(os3->first.serial == 3);
    REQUIRE(&os3->second == &o3);

    // More orders
    auto& o4 = book.insert(Order{Side::Sell, 2, 1020, 20, 20, 20, true, 0});
    REQUIRE(cbook.orders<Side::Sell>().size() == 2);
    REQUIRE(cbook.orders<Side::Buy>().empty());

    auto& o5 = book.insert(Order{Side::Sell, 3, 1000, 20, 20, 20, true, 0});
    REQUIRE(cbook.orders<Side::Sell>().size() == 3);
    REQUIRE(cbook.orders<Side::Buy>().empty());

    auto os5 = cbook.orders<Side::Sell>().begin();
    // This funny notation means "create temporary of the same type as os5 and then increment it"
    REQUIRE(++(decltype(os5) (os5)) == os3);
    // Simplar as above, except the temporary is incremented twice
    auto os4 = ++(++(decltype(os5) (os5)));

    // Most aggressive sell price at top
    REQUIRE(os5->first.price == 1000);
    REQUIRE(&os5->second == &o5);
    REQUIRE(os3->first.price == 1010);
    REQUIRE(&os3->second == &o3);
    REQUIRE(os4->first.price == 1020);
    REQUIRE(&os4->second == &o4);
}

namespace {
    smatch::Order buy(unsigned int id, unsigned int price, unsigned int size) {
        return smatch::Order{smatch::Side::Buy, id, price, size, size, size, true, 0};
    }

    smatch::Order sell(unsigned int id, unsigned int price, unsigned int size) {
        return smatch::Order{smatch::Side::Sell, id, price, size, size, size, true, 0};
    }

    bool operator==(const smatch::Order& lh, const smatch::Order& rh) {
        return lh.side == rh.side
               && lh.id == rh.id
               && lh.price == rh.price
               && lh.size == rh.size
               && lh.full == rh.full
               && lh.peak == rh.peak
               && lh.add == rh.add;
    }

    template <smatch::Side side>
    bool same_orders(const std::vector<smatch::Order>& lh, const decltype((static_cast<const smatch::Book*>(nullptr))->orders<side>())& rh) {
        auto i = rh.begin();
        for (const auto& l : lh){
            if (i == rh.end())
                return false;
            if (not (l == i->second))
                return false;
            ++i;
        }
        return i == rh.end();
    }

    bool operator==(const smatch::Match& lh, const smatch::Match& rh) {
        return lh.buyId == rh.buyId
               && lh.sellId == rh.sellId
               && lh.price == rh.price
               && lh.size == rh.size;
    }
}

TEST_CASE("matching and sorting of buy orders", "[book][sorting][matching]") {
    using namespace smatch;
    Book book;
    const Book& cbook = book; // shortcut for 'const_cast<const Book&>(book)'

    const auto& o1 = book.insert(buy(1, 1010, 200));
    const auto& o2 = book.insert(buy(2, 1010, 200));
    const auto& o3 = book.insert(buy(3, 1030, 200));
    const auto& o4 = book.insert(buy(4, 1010, 200));
    const auto& o5 = book.insert(buy(5, 1000, 200));
    REQUIRE(cbook.orders<Side::Buy>().size() == 5);

    // Check sort order of new orders : first by price, then by serial (which coincides with id)
    std::vector<Order> buys;
    buys.push_back(Order{Side::Buy, 3, 1030, 200, 200, 200, true, 0});
    buys.push_back(Order{Side::Buy, 1, 1010, 200, 200, 200, true, 0});
    buys.push_back(Order{Side::Buy, 2, 1010, 200, 200, 200, true, 0});
    buys.push_back(Order{Side::Buy, 4, 1010, 200, 200, 200, true, 0});
    buys.push_back(Order{Side::Buy, 5, 1000, 200, 200, 200, true, 0});
    REQUIRE(same_orders<Side::Buy>(buys, cbook.orders<Side::Buy>()));

    // Replace top order 3 at 1030 with another top order 6 at 1020
    book.remove(3);
    buys.erase(buys.begin());
    REQUIRE(same_orders<Side::Buy>(buys, cbook.orders<Side::Buy>()));

    const auto& o6 = book.insert(buy(6, 1020, 200));
    buys.insert(buys.begin(), Order{Side::Buy, 6, 1020, 200, 200, 200, true, 0});
    REQUIRE(same_orders<Side::Buy>(buys, cbook.orders<Side::Buy>()));

    // Remove order 4 in the middle
    book.remove(4);
    auto i = buys.begin();
    std::advance(i, 3);
    buys.erase(i);
    REQUIRE(same_orders<Side::Buy>(buys, cbook.orders<Side::Buy>()));

    // Match sell order 7 at 1010
    auto&& o7 = sell(7, 1010, 450);
    std::vector<Match> matches;
    book.match<Side::Sell>(o7, matches);

    REQUIRE(matches.size() == 3);
    REQUIRE(matches[0] == (Match{6, 7, 1020, 200}));
    REQUIRE(matches[1] == (Match{1, 7, 1010, 200}));
    REQUIRE(matches[2] == (Match{2, 7, 1010, 50}));

    // Only two orders left, of which order 2 is partially filled now
    buys.clear();
    buys.push_back(Order{Side::Buy, 2, 1010, 150, 150, 200, true, 0});
    buys.push_back(Order{Side::Buy, 5, 1000, 200, 200, 200, true, 0});
    REQUIRE(same_orders<Side::Buy>(buys, cbook.orders<Side::Buy>()));

    // Add new top order
    const auto& o8 = book.insert(buy(8, 1020, 200));
    buys.insert(buys.begin(), Order{Side::Buy, 8, 1020, 200, 200, 200, true, 0});
    REQUIRE(same_orders<Side::Buy>(buys, cbook.orders<Side::Buy>()));

    // Add second level order at 1010 - must be last at this price level
    const auto& o9 = book.insert(buy(9, 1010, 200));
    i = buys.begin();
    std::advance(i, 2);
    buys.insert(i, Order{Side::Buy, 9, 1010, 200, 200, 200, true, 0});
    REQUIRE(same_orders<Side::Buy>(buys, cbook.orders<Side::Buy>()));

    // We currently have orders 8, 2, 9 and 5. Check the references are still valid.
    REQUIRE(buys[0] == o8);
    REQUIRE(buys[1] == o2);
    REQUIRE(buys[2] == o9);
    REQUIRE(buys[3] == o5);
}
