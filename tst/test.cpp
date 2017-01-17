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
        "O B 1 1020 100\n"
        "L B 1 1020 100\n"); // unrecognized, should be O

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

    Input i1 { Order { Side::Buy, id1, 1020, 100, 100 } };
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
    Input i3 { Order { Side::Sell, id2, 1030, 50, 100 } };
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
        "O B 1 1020 100\n"
        "O S 2 1030 100\n"
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
        "O B 1 1020 170\n"
        "O S 2 1020 100\n"
        "O S 3 1010 100\n");

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
    EXPECT_EQ(o1.size, 170);

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
    EXPECT_EQ(o2.size, 70);

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
        "O B 1 1010 200\n"
        "O B 2 1010 200\n"
        "O B 3 1030 200\n"
        "O B 4 1010 200\n");
    std::ostringstream out;
    Engine engine;
    engine.run(in, out);

    out.str("");
    std::istringstream in2 (
        "O B 5 1000 200\n");
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
        "O B 6 1020 200\n");
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
        "O S 7 1010 450\n");
    engine.run(in6, out);
    EXPECT_EQ(out.str(), std::string(
        "M 6 7 1020 200\n"
        "M 1 7 1010 200\n"
        "M 2 7 1010 50\n"
        "O B 2 1010 150\n"
        "O B 5 1000 200\n"));

    out.str("");
    std::istringstream in7 (
        "O B 8 1020 200\n");
    engine.run(in7, out);
    EXPECT_EQ(out.str(), std::string(
        "O B 8 1020 200\n"
        "O B 2 1010 150\n"
        "O B 5 1000 200\n"));

    out.str("");
    std::istringstream in8 (
        "O B 9 1010 200\n");
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
        "O S 1 1010 200\n"
        "O S 2 1010 200\n"
        "O S 3 1000 200\n"
        "O S 4 1010 200\n");
    std::ostringstream out;
    Engine engine;
    engine.run(in, out);

    out.str("");
    std::istringstream in2 (
        "O S 5 1020 200\n");
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
        "O S 6 1000 200\n");
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
        "O B 7 1010 450\n");
    engine.run(in6, out);
    EXPECT_EQ(out.str(), std::string(
        "M 7 6 1000 200\n"
        "M 7 1 1010 200\n"
        "M 7 2 1010 50\n"
        "O S 2 1010 150\n"
        "O S 5 1020 200\n"));

    out.str("");
    std::istringstream in7 (
        "O S 8 1000 200\n");
    engine.run(in7, out);
    EXPECT_EQ(out.str(), std::string(
        "O S 8 1000 200\n"
        "O S 2 1010 150\n"
        "O S 5 1020 200\n"));

    out.str("");
    std::istringstream in8 (
        "O S 9 1010 200\n");
    engine.run(in8, out);
    EXPECT_EQ(out.str(), std::string(
        "O S 8 1000 200\n"
        "O S 2 1010 150\n"
        "O S 9 1010 200\n"
        "O S 5 1020 200\n"));
}
