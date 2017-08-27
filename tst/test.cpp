#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <sstream>
#include <cassert>

#include "engine.hpp"

TEST(utest_basic, test_not_infinite_loop)
{
    using namespace smatch;
    std::istringstream in;
    std::ostringstream out;
    Engine engine;
    engine.run(in, out);
    EXPECT_TRUE(true); // i.e. not infinite loop above
}

namespace {
    struct Dummy
    {
        std::vector<smatch::Match> fills;
        std::vector<smatch::Order> book;
        void write(const smatch::Match& m) {
            if (!book.empty())
                throw std::runtime_error("Book must be empty");
            fills.push_back(m);
        }
        void write(const smatch::Order& o) {
            book.push_back(o);
        }
        void reset() {
            book.clear();
            fills.clear();
        }
    };

    struct TestException { };

    smatch::OstreamWriter writer(Dummy& ) { throw TestException(); }
    bool read(smatch::Input&, Dummy& ) { return false; }
}

TEST(utest_basic, test_exception_handling)
{
    using namespace smatch;
    Dummy d1, d2;
    bool thrown = false;
    try {
        Engine engine;
        engine.run(d1, d2);
    } catch (TestException) {
        thrown = true;
    }
    EXPECT_TRUE(thrown);
}

TEST(utest_basic, test_parsing)
{
    using namespace smatch;
    std::istringstream in ("\n"
        "# Test\n"
        "\n"
        "L B 1 1020 100\n"
        "O B 1 1020 100\n"); // unrecognized, should be L

    Input t;
    EXPECT_TRUE(read(t, in)); // empty line
    EXPECT_TRUE(t.empty());
    EXPECT_TRUE(read(t, in)); // comment
    EXPECT_TRUE(t.empty());
    EXPECT_TRUE(read(t, in)); // another empty line
    EXPECT_TRUE(t.empty());

    EXPECT_TRUE(read(t, in));
    EXPECT_TRUE(not t.empty());

    bool thrown = false;
    try {
        read(t, in);
    } catch (std::exception&) {
        thrown = true;
    }
    EXPECT_TRUE(thrown);
    EXPECT_TRUE(not read(t, in));
}

TEST(utest_order_cancel, test_order_and_cancel)
{
    using namespace smatch;
    const unsigned int id1 = 1;
    Engine me;

    Input i1 { Order { Side::Buy, id1, 1020, 100, 100, 100, 0 } };
    Dummy d;
    me.handle(i1, d);
    EXPECT_TRUE(d.fills.empty());
    ASSERT_EQ(d.book.size(), 1);
    const auto& o1 = d.book.front();
    EXPECT_EQ(o1.side, Side::Buy);
    EXPECT_EQ(o1.id, id1);
    EXPECT_EQ(o1.price, 1020);
    EXPECT_EQ(o1.size, 100);

    Input i2 { Cancel { id1 } };
    d.reset();
    me.handle(i2, d);
    EXPECT_TRUE(d.fills.empty());
    EXPECT_TRUE(d.book.empty());

    const unsigned int id2 = 2;
    Input i3 { Order { Side::Sell, id2, 1030, 50, 100, 50, 0 } };
    d.reset();
    me.handle(i3, d);
    EXPECT_TRUE(d.fills.empty());
    ASSERT_EQ(d.book.size(), 1);
    const auto& o2 = d.book.front();
    EXPECT_EQ(o2.side, Side::Sell);
    EXPECT_EQ(o2.id, id2);
    EXPECT_EQ(o2.price, 1030);
    EXPECT_EQ(o2.size, 50);

    Input i4 { Cancel { id2 } };
    d.reset();
    me.handle(i4, d);
    EXPECT_TRUE(d.fills.empty());
    EXPECT_TRUE(d.book.empty());
}

TEST(utest_order_cancel, test_order_and_cancel_from_stream)
{
    using namespace smatch;
    Engine me;
    const unsigned int id1 = 1;
    const unsigned int id2 = 2;

    std::istringstream in ("# Test\n"
        "L B 1 1020 100\n"
        "L S 2 1030 100\n"
        "C 1\n"
        "C 2\n");

    Input t;
    EXPECT_TRUE(read(t, in)); // comment
    EXPECT_TRUE(t.empty());

    EXPECT_TRUE(read(t, in));
    EXPECT_TRUE(not t.empty());
    Dummy d;
    me.handle(t, d);
    EXPECT_TRUE(d.fills.empty());
    ASSERT_EQ(d.book.size(), 1);
    const auto& o1 = d.book.front();
    EXPECT_EQ(o1.side, Side::Buy);
    EXPECT_EQ(o1.id, id1);
    EXPECT_EQ(o1.price, 1020);
    EXPECT_EQ(o1.size, 100);

    EXPECT_TRUE(read(t, in));
    EXPECT_TRUE(not t.empty());
    d.reset();
    me.handle(t, d);
    EXPECT_TRUE(d.fills.empty());
    ASSERT_EQ(d.book.size(), 2);
    const auto& o2 = d.book[0];
    EXPECT_EQ(o2.side, Side::Buy);
    EXPECT_EQ(o2.id, id1);
    EXPECT_EQ(o2.price, 1020);
    EXPECT_EQ(o2.size, 100);
    const auto& o3 = d.book[1];
    EXPECT_EQ(o3.side, Side::Sell);
    EXPECT_EQ(o3.id, id2);
    EXPECT_EQ(o3.price, 1030);
    EXPECT_EQ(o3.size, 100);

    EXPECT_TRUE(read(t, in));
    EXPECT_TRUE(not t.empty());
    d.reset();
    me.handle(t, d);
    EXPECT_TRUE(d.fills.empty());
    ASSERT_EQ(d.book.size(), 1);
    const auto& o4 = d.book.front();
    EXPECT_EQ(o4.side, Side::Sell);
    EXPECT_EQ(o4.id, id2);
    EXPECT_EQ(o4.price, 1030);
    EXPECT_EQ(o4.size, 100);

    EXPECT_TRUE(read(t, in));
    EXPECT_TRUE(not t.empty());
    d.reset();
    me.handle(t, d);
    EXPECT_TRUE(d.fills.empty());
    EXPECT_TRUE(d.book.empty());

    EXPECT_TRUE(not read(t, in));
}

TEST(utest_matching, test_order_and_match)
{
    using namespace smatch;
    Engine me;
    std::istringstream in (
        "I B 1 1020 170 50\n"
        "L S 2 1020 100\n"
        "I S 3 1010 100 40\n");

    const unsigned int id1 = 1;
    const unsigned int id3 = 3;

    Input t;
    EXPECT_TRUE(read(t, in));
    EXPECT_TRUE(not t.empty());
    Dummy d;
    me.handle(t, d);
    EXPECT_TRUE(d.fills.empty());
    ASSERT_EQ(d.book.size(), 1);
    const auto& o1 = d.book.front();
    EXPECT_EQ(o1.side, Side::Buy);
    EXPECT_EQ(o1.id, id1);
    EXPECT_EQ(o1.price, 1020);
    EXPECT_EQ(o1.size, 50);

    // Add limit order at the same price as iceberg - expect match
    EXPECT_TRUE(read(t, in));
    EXPECT_TRUE(not t.empty());
    d.reset();
    me.handle(t, d);
    ASSERT_EQ(d.fills.size(), 1);
    const auto& f1 = d.fills.front();
    EXPECT_EQ(f1.buyId, 1);
    EXPECT_EQ(f1.sellId, 2);
    EXPECT_EQ(f1.price, 1020);
    EXPECT_EQ(f1.size, 100);
    // There is still some of buy iceberg order left
    ASSERT_EQ(d.book.size(), 1);
    const auto& o2 = d.book.front();
    EXPECT_EQ(o2.side, Side::Buy);
    EXPECT_EQ(o2.id, id1);
    EXPECT_EQ(o2.price, 1020);
    EXPECT_EQ(o2.size, 50);

    // Add iceberg order at better price than older iceberg - expect match
    EXPECT_TRUE(read(t, in));
    EXPECT_TRUE(not t.empty());
    d.reset();
    me.handle(t, d);
    ASSERT_EQ(d.fills.size(), 1);
    const auto& f2 = d.fills.front();
    EXPECT_EQ(f2.buyId, 1);
    EXPECT_EQ(f2.sellId, 3);
    EXPECT_EQ(f2.price, 1020);
    EXPECT_EQ(f2.size, 70);
    // There is still some of sell iceberg order left
    ASSERT_EQ(d.book.size(), 1);
    const auto& o3 = d.book.front();
    EXPECT_EQ(o3.side, Side::Sell);
    EXPECT_EQ(o3.id, id3);
    EXPECT_EQ(o3.price, 1010);
    EXPECT_EQ(o3.size, 30);
}

TEST(utest_matching, test_comp_sorting_buys)
{
    using namespace smatch;
    std::istringstream in (
        "L B 1 1010 200\n"
        "L B 2 1010 200\n"
        "L B 3 1030 200\n"
        "L B 4 1010 200\n");
    std::ostringstream out;
    Engine engine;
    engine.run(in, out);

    out.str("");
    std::istringstream in2 (
        "L B 5 1000 200\n");
    engine.run(in2, out);
    EXPECT_EQ(out.str(), std::string(
        "O B 3 1030 200\n"
        "O B 1 1010 200\n"
        "O B 2 1010 200\n"
        "O B 4 1010 200\n"
        "O B 5 1000 200\n"));

    out.str("");
    std::istringstream in3 (
        "C 3\n");
    engine.run(in3, out);
    EXPECT_EQ(out.str(), std::string(
        "O B 1 1010 200\n"
        "O B 2 1010 200\n"
        "O B 4 1010 200\n"
        "O B 5 1000 200\n"));

    out.str("");
    std::istringstream in4 (
        "L B 6 1020 200\n");
    engine.run(in4, out);
    EXPECT_EQ(out.str(), std::string(
        "O B 6 1020 200\n"
        "O B 1 1010 200\n"
        "O B 2 1010 200\n"
        "O B 4 1010 200\n"
        "O B 5 1000 200\n"));

    out.str("");
    std::istringstream in5 (
        "C 4\n");
    engine.run(in5, out);
    EXPECT_EQ(out.str(), std::string(
        "O B 6 1020 200\n"
        "O B 1 1010 200\n"
        "O B 2 1010 200\n"
        "O B 5 1000 200\n"));

    out.str("");
    std::istringstream in6 (
        "L S 7 1010 450\n");
    engine.run(in6, out);
    EXPECT_EQ(out.str(), std::string(
        "M 6 7 1020 200\n"
        "M 1 7 1010 200\n"
        "M 2 7 1010 50\n"
        "O B 2 1010 150\n"
        "O B 5 1000 200\n"));

    out.str("");
    std::istringstream in7 (
        "L B 8 1020 200\n");
    engine.run(in7, out);
    EXPECT_EQ(out.str(), std::string(
        "O B 8 1020 200\n"
        "O B 2 1010 150\n"
        "O B 5 1000 200\n"));

    out.str("");
    std::istringstream in8 (
        "L B 9 1010 200\n");
    engine.run(in8, out);
    EXPECT_EQ(out.str(), std::string(
        "O B 8 1020 200\n"
        "O B 2 1010 150\n"
        "O B 9 1010 200\n"
        "O B 5 1000 200\n"));
}

TEST(utest_matching, test_comp_sorting_sells)
{
    using namespace smatch;
    std::istringstream in (
        "L S 1 1010 200\n"
        "L S 2 1010 200\n"
        "L S 3 1000 200\n"
        "L S 4 1010 200\n");
    std::ostringstream out;
    Engine engine;
    engine.run(in, out);

    out.str("");
    std::istringstream in2 (
        "L S 5 1020 200\n");
    engine.run(in2, out);
    EXPECT_EQ(out.str(), std::string(
        "O S 3 1000 200\n"
        "O S 1 1010 200\n"
        "O S 2 1010 200\n"
        "O S 4 1010 200\n"
        "O S 5 1020 200\n"));

    out.str("");
    std::istringstream in3 (
        "C 3\n");
    engine.run(in3, out);
    EXPECT_EQ(out.str(), std::string(
        "O S 1 1010 200\n"
        "O S 2 1010 200\n"
        "O S 4 1010 200\n"
        "O S 5 1020 200\n"));

    out.str("");
    std::istringstream in4 (
        "L S 6 1000 200\n");
    engine.run(in4, out);
    EXPECT_EQ(out.str(), std::string(
        "O S 6 1000 200\n"
        "O S 1 1010 200\n"
        "O S 2 1010 200\n"
        "O S 4 1010 200\n"
        "O S 5 1020 200\n"));

    out.str("");
    std::istringstream in5 (
        "C 4\n");
    engine.run(in5, out);
    EXPECT_EQ(out.str(), std::string(
        "O S 6 1000 200\n"
        "O S 1 1010 200\n"
        "O S 2 1010 200\n"
        "O S 5 1020 200\n"));

    out.str("");
    std::istringstream in6 (
        "L B 7 1010 450\n");
    engine.run(in6, out);
    EXPECT_EQ(out.str(), std::string(
        "M 7 6 1000 200\n"
        "M 7 1 1010 200\n"
        "M 7 2 1010 50\n"
        "O S 2 1010 150\n"
        "O S 5 1020 200\n"));

    out.str("");
    std::istringstream in7 (
        "L S 8 1000 200\n");
    engine.run(in7, out);
    EXPECT_EQ(out.str(), std::string(
        "O S 8 1000 200\n"
        "O S 2 1010 150\n"
        "O S 5 1020 200\n"));

    out.str("");
    std::istringstream in8 (
        "L S 9 1010 200\n");
    engine.run(in8, out);
    EXPECT_EQ(out.str(), std::string(
        "O S 8 1000 200\n"
        "O S 2 1010 150\n"
        "O S 9 1010 200\n"
        "O S 5 1020 200\n"));
}

TEST(utest_matching, test_comp_iceberg_match1)
{
    using namespace smatch;
    std::istringstream in (
    "L S 1 1000 50\n"
    "I S 2 1010 200 50\n"
    "L S 3 1010 200\n");
    std::ostringstream out;
    Engine engine;
    engine.run(in, out);
    EXPECT_EQ(out.str(), std::string(
    "O S 1 1000 50\n"
    "O S 1 1000 50\n"
    "O S 2 1010 50\n"
    "O S 1 1000 50\n"
    "O S 2 1010 50\n"
    "O S 3 1010 200\n"));

    out.str("");
    std::istringstream in2 (
    "L B 4 1010 70\n");
    engine.run(in2, out);
    EXPECT_EQ(out.str(), std::string(
    "M 4 1 1000 50\n"
    "M 4 2 1010 20\n"
    "O S 2 1010 30\n" // Partial fill, order maintains high priority
    "O S 3 1010 200\n"));

    out.str("");
    std::istringstream in3 (
    "L B 4 1010 30\n");
    engine.run(in3, out);
    EXPECT_EQ(out.str(), std::string(
    "M 4 2 1010 30\n"
    "O S 3 1010 200\n"
    "O S 2 1010 50\n"));
}

TEST(utest_matching, test_comp_iceberg_match2)
{
    using namespace smatch;
    std::istringstream in (
    "L S 3 101 20000\n"
    "L S 1 100 10000\n"
    "L S 2 100 7500\n"
    "L B 4 99 50000\n");
    std::ostringstream out;
    Engine engine;
    engine.run(in, out);

    out.str("");
    std::istringstream in2 (
    "L B 5 98 25500\n");
    engine.run(in2, out);
    EXPECT_EQ(out.str(), std::string(
    "O B 4 99 50000\n"
    "O B 5 98 25500\n"
    "O S 1 100 10000\n"
    "O S 2 100 7500\n"
    "O S 3 101 20000\n"));

    // Enter aggressive B iceberg order, which will match against orders S orders 1 and 2 above
    // This matching will remove 17,500 from 100,000 full iceberg size, leaving 82,500 liquidity
    out.str("");
    std::istringstream in3 (
    "I B 6 100 100000 10000\n");
    engine.run(in3, out);
    EXPECT_EQ(out.str(), std::string(
    "M 6 1 100 10000\n"
    "M 6 2 100 7500\n"
    "O B 6 100 10000\n"
    "O B 4 99 50000\n"
    "O B 5 98 25500\n"
    "O S 3 101 20000\n"));

    // Enter limit S order, which will match against iceberg. The iceberg will automatically renew
    // with 72,500 liquidity remaining
    out.str("");
    std::istringstream in4 (
    "L S 7 100 10000\n");
    engine.run(in4, out);
    EXPECT_EQ(out.str(), std::string(
    "M 6 7 100 10000\n"
    "O B 6 100 10000\n"
    "O B 4 99 50000\n"
    "O B 5 98 25500\n"
    "O S 3 101 20000\n"));

    // Enter limit S order which will match against iceberg again, but will hit it twice - removing
    // 10,000 and then 1,000. These are presented as single match, but new peak is 9,000
    out.str("");
    std::istringstream in5 (
    "L S 8 100 11000\n");
    engine.run(in5, out);
    EXPECT_EQ(out.str(), std::string(
    "M 6 8 100 11000\n"
    "O B 6 100 9000\n"
    "O B 4 99 50000\n"
    "O B 5 98 25500\n"
    "O S 3 101 20000\n"));

    // Add another B iceberg
    out.str("");
    std::istringstream in6 (
    "I B 9 100 50000 20000\n");
    engine.run(in6, out);
    EXPECT_EQ(out.str(), std::string(
    "O B 6 100 9000\n"
    "O B 9 100 20000\n"
    "O B 4 99 50000\n"
    "O B 5 98 25500\n"
    "O S 3 101 20000\n"));

    // Enter limit S order, which will match against both icebergs
    out.str("");
    std::istringstream in7 (
    "L S 10 100 35000\n");
    engine.run(in7, out);
    EXPECT_EQ(out.str(), std::string(
    "M 6 10 100 15000\n"
    "M 9 10 100 20000\n"
    "O B 6 100 4000\n"
    "O B 9 100 20000\n"
    "O B 4 99 50000\n"
    "O B 5 98 25500\n"
    "O S 3 101 20000\n"));
    // Specification example ends here

    // Enter limit S order, which will match against one iceberg only. The matched order will drop
    // its priority
    out.str("");
    std::istringstream in8 (
    "L S 11 100 4000\n");
    engine.run(in8, out);
    EXPECT_EQ(out.str(), std::string(
    "M 6 11 100 4000\n"
    "O B 9 100 20000\n"
    "O B 6 100 10000\n"
    "O B 4 99 50000\n"
    "O B 5 98 25500\n"
    "O S 3 101 20000\n"));

    // Enter another limit S order, this time to match both iceber orders
    out.str("");
    std::istringstream in9 (
    "L S 12 100 30000\n");
    engine.run(in9, out);
    EXPECT_EQ(out.str(), std::string(
    "M 9 12 100 20000\n"
    "M 6 12 100 10000\n"
    "O B 9 100 10000\n"
    "O B 6 100 10000\n"
    "O B 4 99 50000\n"
    "O B 5 98 25500\n"
    "O S 3 101 20000\n"));
}
