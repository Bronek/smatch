#pragma once

#include "types.hpp"
#include "input.hpp"
#include "engine.hpp"
#include "stream.hpp"

namespace smatch {

struct Runner
{
    template<typename In, typename Out>
    static void run(Engine& e, In& in, Out& out)
    {
        // Use overloading and ADL to construct communication channel wrapper appropriate for In/Out
        auto&& ch = channel(in, out);
        Input i;

        for (;;) {
            // Handle own exceptions (e.g. bad input or bad order id) per each input
            try {
                if (not ch.read(i))
                    return;

                handle(i, e, ch);
            }
            catch(const smatch::exception& e) {
                if (not ch.report(e, true))
                    throw;
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
