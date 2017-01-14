#pragma once

#include <iostream>
#include <stdexcept>
#include <cstdio>

namespace smatch {

inline bool parse(Side& s, char c)
{
    switch (c)
    {
        case 'B' : s = Side::Buy; return true;
        case 'S' : s = Side::Sell; return true;
        default: return false;
    }
}

inline bool read(Input& input, std::istream& in)
{
    std::string line;
    if (!std::getline(in, line))
        return false; // EOF

    if (line.empty() || line[0] == '#')
    {
        input = Input(); // i.e. empty, will be skipped
        return true;
    }
    else switch (line[0])
    {
        case 'O': {
            Order o;
            char dummy, side, sentinel;
            if (std::sscanf(line.c_str(), "%c %c %u %u %u%c", &dummy, &side, &o.id, &o.price, &o.size, &sentinel) != 5
                || !parse(o.side, side))
                throw std::runtime_error("Ill-formed limit order");
            input = Input(o);
            break;
        }
        case 'C': {
            Cancel c;
            char dummy, sentinel;
            if (std::sscanf(line.c_str(), "%c %u%c", &dummy, &c.id, &sentinel) != 2)
                throw std::runtime_error("Ill-formed cancel");
            input = Input(c);
            break;
        }
        default:
            throw std::runtime_error("Unrecognized data");
    }
    return true;
}

struct OstreamWriter
{
    std::ostream& output;
    explicit OstreamWriter(std::ostream& s) : output(s) { }

    void write(const Match& m)
    {
        output << "M " << m.buyId << ' ' << m.sellId << ' ' << m.price << ' ' << m.size << std::endl;
    }

    void write(const Order& o)
    {
        output << "O " << o.side << ' ' << o.id << ' ' << o.price << ' ' << o.size << std::endl;
    }
};

inline OstreamWriter writer(std::ostream& s) { return OstreamWriter(s); }

}
