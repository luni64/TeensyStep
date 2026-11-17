// core::Axis: random sequences of commands, given at random moments. Whatever the state of the axis is when a
// command arrives, the command is carried out exactly, within the limits, with at most two reversals.
#include "sim.h"

#include <random>

using namespace TeensyStep;
using namespace sim;

namespace
{
    struct Rng
    {
        std::mt19937 gen;
        explicit Rng(unsigned seed) : gen(seed) {}
        float real(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(gen); }
        int32_t integer(int32_t lo, int32_t hi) { return std::uniform_int_distribution<int32_t>(lo, hi)(gen); }
        float logReal(float lo, float hi) { return std::exp(real(std::log(lo), std::log(hi))); } // evenly over the magnitudes
        float sign() { return integer(0, 1) ? 1.0f : -1.0f; }
    };
}

TEST_CASE("random: commands at random moments are carried out exactly and within the limits")
{
    for (unsigned seed = 1; seed <= 400; seed++)
    {
        CAPTURE(seed);
        Rng rng(seed);
        Limits L;
        L.vMax = rng.logReal(50, 40'000);
        L.acc = rng.logReal(200, 400'000);
        L.dec = rng.integer(0, 2) ? L.acc : rng.logReal(200, 400'000);
        L.pullIn = rng.integer(0, 1) ? 0.0f : rng.real(0, 0.5f * L.vMax);

        Axis a;
        a.setMaxSpeed(L.vMax);
        a.setAcceleration(L.acc);
        a.setDeceleration(L.dec);
        a.setPullInSpeed(L.pullIn);
        a.setPosition(rng.integer(-1000, 1000));

        int commands = rng.integer(2, 8);
        bool mid     = false; // the axis is in the middle of a step period
        for (int c = 0; c < commands; c++)
        {
            CAPTURE(c);
            bool last = c == commands - 1;
            bool interrupt = !last && rng.integer(0, 2) == 0; // the next command arrives before this one is done
            long budget = interrupt ? rng.integer(0, 3'000) : 20'000'000;
            int kind = last ? 0 : rng.integer(0, 3);
            CAPTURE(kind);
            CAPTURE(interrupt);
            bool startedMid = mid;
            long stepsMade  = 0;
            mid             = false;

            if (kind == 0) // move to a target and stop there
            {
                int32_t target = a.position() + rng.integer(-150'000, 150'000);
                REQUIRE(a.moveAbs(target) == errorCode::OK);
                Run r = run(a, budget, Until::finished, -1, startedMid);
                stepsMade = r.steps();
                checkKinematics(r, L);
                CHECK(r.reversals() <= 1);
                if (!interrupt)
                {
                    REQUIRE(r.finished);
                    CHECK(a.position() == target);
                    CHECK(a.speed() == 0.0f);
                    CHECK_FALSE(a.isMoving());
                }
            }
            else if (kind == 1) // move to a target and pass it at a speed
            {
                int32_t target = a.position() + rng.integer(-150'000, 150'000);
                float v = rng.sign() * std::min(L.vMax, rng.logReal(std::max(1.0f, 0.001f * L.vMax), L.vMax));
                REQUIRE(a.moveAbs(target, v) == errorCode::OK);
                Run r = run(a, budget, Until::arrived, -1, startedMid);
                stepsMade = r.steps();
                checkKinematics(r, L);
                CHECK(r.reversals() <= 2);
                if (!interrupt)
                {
                    REQUIRE(r.arrival() >= 0);
                    CHECK(a.position() == target);
                    CHECK(a.speed() == approx(v).epsilon(0.0002));
                    Run after = run(a, 20); // keeps that speed
                    REQUIRE(after.steps() == 20);
                    CHECK(a.position() == target + (v > 0 ? 20 : -20));
                    CHECK(a.speed() == approx(v).epsilon(0.0002));
                }
            }
            else if (kind == 2) // rotate
            {
                float v = rng.sign() * std::min(L.vMax, rng.logReal(std::max(1.0f, 0.001f * L.vMax), L.vMax));
                REQUIRE(a.rotate(v) == errorCode::OK);
                long steps = interrupt ? budget : (long)(L.vMax * L.vMax / std::min(L.acc, L.dec)) + 100; // enough to brake and speed up
                Run r = run(a, steps, interrupt ? Until::finished : Until::speed, -1, startedMid, v);
                stepsMade = r.steps();
                checkKinematics(r, L);
                CHECK(r.reversals() <= 1);
                if (!interrupt)
                {
                    CHECK(a.speed() == v);
                    Run after = run(a, 5); // keeps the speed
                    CHECK(a.speed() == v);
                    CHECK(after.reversals() == 0);
                }
            }
            else // stop
            {
                REQUIRE(a.stop() == errorCode::OK);
                Run r = run(a, budget, Until::finished, -1, startedMid);
                stepsMade = r.steps();
                checkKinematics(r, L);
                CHECK(r.reversals() == 0);
                if (!interrupt)
                {
                    REQUIRE(r.finished);
                    CHECK(a.speed() == 0.0f);
                }
            }

            if (interrupt && a.isMoving()) // let the next command arrive at a random moment within a step period
            {
                float v      = std::max(std::fabs(a.speed()), 1.0f);
                Run between  = runFor(a, rng.real(0, 1.5f) / v, startedMid && stepsMade == 0);
                checkKinematics(between, L);
                mid = between.endedInWait;
            }
        }
    }
}

TEST_CASE("random: changing the limits during a move keeps the end position exact")
{
    for (unsigned seed = 1; seed <= 200; seed++)
    {
        CAPTURE(seed);
        Rng rng(seed + 10'000);
        Axis a;
        a.setMaxSpeed(rng.logReal(100, 40'000));
        a.setAcceleration(rng.logReal(500, 400'000));
        int32_t target = rng.integer(-200'000, 200'000);
        a.moveAbs(target);
        for (int i = 0; i < 6; i++)
        {
            run(a, rng.integer(0, 20'000));
            switch (rng.integer(0, 3))
            {
                case 0: a.setMaxSpeed(rng.logReal(100, 40'000)); break;
                case 1: a.setAcceleration(rng.logReal(500, 400'000)); break;
                case 2: a.setDeceleration(rng.logReal(500, 400'000)); break;
                default: a.setPullInSpeed(rng.real(0, 2'000)); break;
            }
        }
        Run r = run(a);
        REQUIRE(r.finished);
        CHECK(a.position() == target);
        CHECK(a.speed() == 0.0f);
    }
}
