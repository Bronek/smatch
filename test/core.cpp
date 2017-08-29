#include "catch.hpp"

#include "runner.hpp"

TEST_CASE("not infinite loop on empty input", "[core]") {
    using namespace smatch;
    std::istringstream in;
    std::ostringstream out;
    Engine en;
    Runner::run(en, in, out);
    REQUIRE(out.str() == "");
}

namespace {
    struct DummyFail1 { };
    struct TestException1 { };
    smatch::Stream channel(std::istream&, DummyFail1& ) { throw TestException1(); }
}

TEST_CASE("exception creating channel is passed through", "[core][exceptions]") {
    using namespace smatch;
    std::istringstream in;
    DummyFail1 d1;
    Engine en;
    REQUIRE_THROWS_AS(Runner::run(en, in, d1), TestException1);
}

namespace {
    struct TestException2 : smatch::exception {
        using smatch::exception::exception;
    };

    struct DummyFail2 {
        int i;
        bool r;

        bool read(smatch::Input& ) {
            if (--i == 0)
                return false;
            throw TestException2("");
        }

        bool report(const smatch::exception&, bool) { return r; }
        void write(const smatch::Match&) { }
        void write(const smatch::Order&) { }
    };

    DummyFail2 channel(DummyFail2& d, std::ostream&) { return d; }
}

TEST_CASE("exceptions derived from smatch::exception when reading are swallowed, or not", "[core][exceptions]") {
    using namespace smatch;
    std::ostringstream out;
    DummyFail2 d2 = {3 , true};
    Engine en;
    REQUIRE_NOTHROW(Runner::run(en, d2, out));

    DummyFail2 d3 = {2 , false};
    REQUIRE_THROWS_AS(Runner::run(en, d3, out), TestException2);
}

namespace {
    struct TestException3 : std::runtime_error {
        TestException3() : runtime_error("") { }
    };

    struct DummyFail3 {
        bool read(smatch::Input& ) {
            throw TestException3();
        }

        bool report(const smatch::exception&, bool) { return true; }
        void write(const smatch::Match&) { }
        void write(const smatch::Order&) { }
    };

    DummyFail3 channel(DummyFail3& d, std::ostream&) { return d; }
}


TEST_CASE("exceptions not derived from smatch::exception are not swallowed", "[core][exceptions]") {
    using namespace smatch;
    std::ostringstream out;
    DummyFail3 d3;
    Engine en;
    REQUIRE_THROWS_AS(Runner::run(en, d3, out), TestException3);
}

namespace {
    struct DummyFail4 : smatch::Stream {
        DummyFail4(std::istream& in, std::ostream& out) : Stream(in, out) {}
        DummyFail4(std::istream& , DummyFail4& src) : Stream(src) { }

        bool report(const smatch::exception&, bool) { return false; }
    };

    DummyFail4 channel(std::istream&, DummyFail4& d) { return d; }
}

TEST_CASE("exceptions on bad input are passed through, when requested", "[core][exceptions][parsing]") {
    using namespace smatch;
    std::istringstream in (
        "L S 1 1020 100\n"
        "L S 1 1020 100\n" // duplicate id
        "L B 2 1010 100\n" // dropped due to exception
    );
    std::ostringstream out;
    DummyFail4 d4 {in, out};
    Engine en;
    REQUIRE_THROWS_AS(Runner::run(en, in, d4), bad_order_id);
    REQUIRE(out.str() == "O S 1 1020 100\n");
}

TEST_CASE("parsing bad stream inputs", "[core][parsing]") {
    using namespace smatch;
    std::ostringstream dummy;
    std::istringstream in (
        "\n"
        "# Test\n"
        "\n"
        "L B 1 100\n" // too few inputs
        "L U 1 1020 100\n"
        "L S 1 1020 100\n"
        "I B 1 1020 100 50\n"
        "I B 1 1020 100 50 50\n" // too many inputs
        "C\n" // too few inputs
        "C A\n" // wrong number format (decimal expected)
        "C 1\n"
        "M S 1\n" // too few inputs
        "M S 1 200\n"
        "O B 1 1020 100\n"
        "F B 1 1020 100\n" // unrecognized
    );

    Stream s(in, dummy);
    Input t;
    REQUIRE(s.read(t)); // empty line
    REQUIRE(t.empty());

    REQUIRE(s.read(t)); // comment
    REQUIRE(t.empty());

    REQUIRE(s.read(t)); // another empty line
    REQUIRE(t.empty());

    REQUIRE_THROWS_AS(s.read(t), bad_input); // too few inputs

    REQUIRE_THROWS_AS(s.read(t), bad_input); // unrecognized side 'U'

    REQUIRE(s.read(t)); // limit order
    REQUIRE(not t.empty());

    REQUIRE(s.read(t)); // iceberg order
    REQUIRE(not t.empty());

    REQUIRE_THROWS_AS(s.read(t), bad_input); // too many inputs

    REQUIRE_THROWS_AS(s.read(t), bad_input); // too few inputs

    REQUIRE_THROWS_AS(s.read(t), bad_input); // wrong number format

    REQUIRE(s.read(t)); // cancel

    REQUIRE_THROWS_AS(s.read(t), bad_input); // too few inputs

    REQUIRE(s.read(t)); // market order

    REQUIRE(s.read(t)); // aggress order

    REQUIRE_THROWS_AS(s.read(t), bad_input); // unrecognized

    REQUIRE(not s.read(t)); // EOF
}
