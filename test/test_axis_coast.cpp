// core::Axis: the short way for steps at constant speed. While nothing changes, the axis hands out the same
// step again without deciding it anew. The results have to be exactly those of deciding every step.
#include "sim.h"

#include <random>

using namespace TeensyStep;
using namespace sim;

namespace
{
    struct Decided : Axis // decides every step
    {
        Decided() { coastEnabled = false; }
    };

    struct Open : Axis // lets the test see how many steps are handed out the short way
    {
        int32_t coast() const { return coastVersion == version ? coastLeft : 0; }
    };

    template <class A, class B, class Command>
    void both(A& a, B& b, Command command)
    {
        command(a);
        command(b);
    }

    // runs both axes for a number of events and compares every event
    template <class A, class B>
    long sameEvents(A& a, B& b, long events)
    {
        long different = 0;
        StepEvent ea, eb;
        for (long i = 0; i < events; i++)
        {
            bool ma = a.next(ea), mb = b.next(eb);
            if (ma != mb) return different + 1;
            if (!ma) break;
            if (ea.delay != eb.delay || ea.dir != eb.dir || ea.arrived != eb.arrived || a.position() != b.position() || a.speed() != b.speed()) different++;
        }
        return different;
    }
}

TEST_CASE("coast: steps at constant speed are handed out without a new decision")
{
    Open a;
    a.setMaxSpeed(10'000);
    a.setAcceleration(20'000);
    a.setPullInSpeed(0);

    SUBCASE("a move: from the end of the acceleration to the start of the deceleration")
    {
        a.moveAbs(50'000); // 2500 steps up, 2500 steps down
        run(a, 3'000);
        CHECK(std::abs(a.coast() - (50'000 - 3'000 - 2'500)) <= 1); // all steps up to the start of the deceleration
        Run r = run(a);
        CHECK(a.position() == 50'000);
        CHECK(a.coast() == 0);
    }
    SUBCASE("a rotation: as long as nothing changes")
    {
        a.rotate(-8'000);
        run(a, 3'000);
        CHECK(a.coast() > 1'000'000);
        a.setMaxSpeed(5'000); // a change ends it
        CHECK(a.coast() == 0);
    }
    SUBCASE("a shorter maximum delay takes effect at once")
    {
        a.rotate(5'000); // 200 us per step
        run(a, 3'000);
        CHECK(a.coast() > 0);
        a.setMaxDelay(0.00005f);
        Run r = run(a, 100);
        CHECK(r.maxEventDelay <= 0.0000501);
        CHECK(r.waits == 300);
    }
    SUBCASE("a target with a speed")
    {
        a.moveAbs(50'000, 4'000); // slows down from 10000 to 4000 over 2100 steps
        run(a, 3'000);
        CHECK(std::abs(a.coast() - (50'000 - 3'000 - 2'100)) <= 1);
    }
    SUBCASE("not while the speed changes, and not for periods that are handed out in pieces")
    {
        a.moveAbs(50'000);
        run(a, 1'000);
        CHECK(a.coast() == 0);

        Open slow;
        slow.setMaxSpeed(100); // 10 ms per step, longer than the maximum delay
        slow.setPullInSpeed(100);
        slow.moveAbs(1'000);
        run(slow, 10);
        CHECK(slow.coast() == 0);
    }
}

TEST_CASE("coast: every event is the same as when each step is decided")
{
    for (unsigned seed = 1; seed <= 300; seed++)
    {
        CAPTURE(seed);
        std::mt19937 gen(seed);
        auto integer = [&gen](int32_t lo, int32_t hi) { return std::uniform_int_distribution<int32_t>(lo, hi)(gen); };
        auto logReal = [&gen](float lo, float hi) { return std::exp(std::uniform_real_distribution<float>(std::log(lo), std::log(hi))(gen)); };

        Axis fast;
        Decided slow;
        float vMax = logReal(50, 40'000), acc = logReal(200, 400'000);
        float pull = integer(0, 1) ? 0.0f : logReal(1, 2 * vMax);
        bool ownDec    = integer(0, 1);
        float maxDelay = integer(0, 3) ? 0.001f : 0.0001f;
        both(fast, slow, [&](Axis& a) {
            a.setMaxSpeed(vMax);
            a.setAcceleration(acc);
            if (ownDec) a.setDeceleration(acc * 0.37f);
            a.setPullInSpeed(pull);
            a.setMaxDelay(maxDelay);
        });

        for (int c = 0; c < 8; c++)
        {
            CAPTURE(c);
            int kind      = integer(0, 6);
            int32_t value = integer(-60'000, 60'000);
            float speed   = logReal(20, 40'000) * (integer(0, 1) ? 1.0f : -1.0f);
            both(fast, slow, [&](Axis& a) {
                switch (kind)
                {
                    case 0: a.moveAbs(value); break;
                    case 1: a.moveRel(value); break;
                    case 2: a.moveAbs(value, speed); break;
                    case 3: a.rotate(speed); break;
                    case 4: a.setMaxSpeed(std::fabs(speed)); break;
                    case 5: a.setPosition(value); break;
                    default: a.stop(); break;
                }
            });
            REQUIRE(sameEvents(fast, slow, integer(0, 40'000)) == 0);
        }
        both(fast, slow, [](Axis& a) { a.stop(); });
        REQUIRE(sameEvents(fast, slow, 10'000'000) == 0);
        CHECK_FALSE(fast.isMoving());
        CHECK(fast.position() == slow.position());
    }
}
