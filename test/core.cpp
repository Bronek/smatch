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
    smatch::OstreamWriter writer(DummyFail1& ) { throw TestException1(); }
}

TEST_CASE("exception creating writer is passed through", "[core][exceptions]") {
    using namespace smatch;
    std::istringstream in;
    DummyFail1 d1;
    Engine en;
    REQUIRE_THROWS_AS(Runner::run(en, in, d1), TestException1);
}

namespace {
    struct DummyFail2 {
        int i;
    };

    struct TestException2 : smatch::exception {
        using smatch::exception::exception;
    };
    bool read(smatch::Input&, DummyFail2& d) {
        if (--d.i == 0)
            return false;
        throw TestException2("");
    }
}

TEST_CASE("exceptions derived from smatch::exception when reading are swallowed", "[core][exceptions]") {
    using namespace smatch;
    std::ostringstream out;
    DummyFail2 d2 = {3};
    Engine en;
    REQUIRE_NOTHROW(Runner::run(en, d2, out));
}

TEST_CASE("exceptions when parsing or handling bad input are swallowed", "[core][exceptions][parsing]") {
    using namespace smatch;
    std::istringstream in (
        "L B 1 100\n" // too few inputs
        "I B 1 1020 100 50 50\n" // too many inputs
        "F B 1 1020 100\n" // unrecognized
        "C 100\n" // correct format, but invalid order id
        "L S 1 1020 100\n" // want to see this one on output
        "L S 1 1020 100\n" // duplicate order id
        "M B 2 100\n" // market order
    );
    std::ostringstream out;
    Engine en;
    REQUIRE_NOTHROW(Runner::run(en, in, out));
    REQUIRE(out.str() ==
        "O S 1 1020 100\n"
        "M 2 1 1020 100\n"
    );
}

TEST_CASE("parsing stream inputs", "[core][parsing]") {
    using namespace smatch;
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

    Input t;
    REQUIRE(read(t, in)); // empty line
    REQUIRE(t.empty());

    REQUIRE(read(t, in)); // comment
    REQUIRE(t.empty());

    REQUIRE(read(t, in)); // another empty line
    REQUIRE(t.empty());

    REQUIRE_THROWS_AS(read(t, in), bad_input); // too few inputs

    REQUIRE_THROWS_AS(read(t, in), bad_input); // unrecognized side 'U'

    REQUIRE(read(t, in)); // limit order
    REQUIRE(not t.empty());

    REQUIRE(read(t, in)); // iceberg order
    REQUIRE(not t.empty());

    REQUIRE_THROWS_AS(read(t, in), bad_input); // too many inputs

    REQUIRE_THROWS_AS(read(t, in), bad_input); // too few inputs

    REQUIRE_THROWS_AS(read(t, in), bad_input); // wrong number format

    REQUIRE(read(t, in)); // cancel

    REQUIRE_THROWS_AS(read(t, in), bad_input); // too few inputs

    REQUIRE(read(t, in)); // market order

    REQUIRE(read(t, in)); // aggress order

    REQUIRE_THROWS_AS(read(t, in), bad_input); // unrecognized

    REQUIRE(not read(t, in)); // EOF
}
