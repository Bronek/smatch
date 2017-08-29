#include "stream.hpp"
#include "input.hpp"
#include "book.hpp"

namespace smatch {

bool Stream::read(Input &input) {
    std::string line;
    if (not std::getline(in, line))
        return false; // EOF

    if (line.empty() || line[0] == '#') {
        input = Input(); // i.e. empty, will be skipped
        return true;
    } else
    switch (line[0]) {
        case 'M': {
            Order o;
            o.add = false;
            char dummy, side, sentinel;
            if (std::sscanf(line.c_str(), "%c %c %u %u%c", &dummy, &side, &o.id, &o.size, &sentinel) != 4
                || not parse(o.side, side))
                throw bad_input("Ill-formed market order");
            o.peak = o.full = o.size;
            if (o.side == Side::Buy)
                o.price = std::numeric_limits<decltype(o.price)>::max();
            else // if (o.side == Side::Sell)
                o.price = std::numeric_limits<decltype(o.price)>::min();
            input = Input(o);
            break;
        }
        case 'O': {
            Order o;
            o.add = false;
            char dummy, side, sentinel;
            if (std::sscanf(line.c_str(), "%c %c %u %u %u%c", &dummy, &side, &o.id, &o.price, &o.size, &sentinel) != 5
                || not parse(o.side, side))
                throw bad_input("Ill-formed order");
            o.peak = o.full = o.size;
            input = Input(o);
            break;
        }
        case 'L': {
            Order o;
            o.add = true;
            char dummy, side, sentinel;
            if (std::sscanf(line.c_str(), "%c %c %u %u %u%c", &dummy, &side, &o.id, &o.price, &o.size, &sentinel) != 5
                || not parse(o.side, side))
                throw bad_input("Ill-formed limit order");
            o.peak = o.full = o.size;
            input = Input(o);
            break;
        }
        case 'I': {
            Order o;
            o.add = true;
            char dummy, side, sentinel;
            if (std::sscanf(line.c_str(), "%c %c %u %u %u %u%c", &dummy, &side, &o.id, &o.price, &o.full, &o.peak, &sentinel) != 6
                || not parse(o.side, side)
                || o.peak > o.full)
                throw bad_input("Ill-formed iceberg order");
            o.size = o.peak;
            input = Input(o);
            break;
        }
        case 'C': {
            Cancel c;
            char dummy, sentinel;
            if (std::sscanf(line.c_str(), "%c %u%c", &dummy, &c.id, &sentinel) != 2)
                throw bad_input("Ill-formed cancel");
            input = Input(c);
            break;
        }
        default:
            throw bad_input("Unrecognized input type");
    }
    return true;
}

bool Stream::report(const exception& e, bool)
{
    if (const auto* tmp = dynamic_cast<const bad_order_id*>(&e))
        std::cerr << tmp->what() << ' ' << tmp->id << std::endl;
    else
        std::cerr << e.what() << std::endl;
    return true;
}

}
