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

