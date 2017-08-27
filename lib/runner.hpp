#pragma once

#include "types.hpp"
#include "input.hpp"
#include "engine.hpp"
#include "streams.hpp"

namespace smatch {

struct Runner
{
    template<typename In, typename Out>
    static void run(Engine& e, In& in, Out& out)
    {
        // Use overloading and ADL to construct writer appropriate for Out type
        auto wr = writer(out);
        Input i;

        for (;;) {
            // Handle own exceptions (e.g. bad input or bad order id) per each input
            try {
                // Use overloading and ADL to populate Input from In type
                if (not read(i, in))
                    return;

                handle(i, e, wr);
            }
            catch(const smatch::exception& e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }

    template<typename Writer>
    static void handle(const Input& i, Engine& e, Writer& wr)
    {
        if (i.empty())
            return;

        // Function Input.handle() returns true only if any matches found (and stored in e)
        if (i.handle(e)) {
            for (const auto &m : e.matches())
                wr.write(m);
        }

        for (const auto &b : e.book().orders<Side::Buy>())
            wr.write(b.second);
        for (const auto &s : e.book().orders<Side::Sell>())
            wr.write(s.second);
    }
};

}
