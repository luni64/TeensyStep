// StepperGroup on the simulated backend: speed and acceleration of a group. The tests read the pins.
#include "TeensyStep.h"
#include "approx.h"
#include "backend/host/HostBackend.h"
#include "doctest.h"

#include <cmath>
#include <vector>

using namespace TeensyStep;
namespace host = TeensyStep::backend::host;

namespace
{
    const backend::MotorPins pinsX{2, 3, true, true};
    const backend::MotorPins pinsY{4, 5, true, true};

    std::vector<errorCode> reported;
    void collectErrors(errorCode code) { reported.push_back(code); }

    struct Board // every test starts with a fresh simulated board
    {
        explicit Board(double lookahead = 0)
        {
            host::reset();
            host::setLookahead(lookahead);
            reported.clear();
            attachErrFunc(collectErrors);
        }
        ~Board()
        {
            attachErrFunc(nullptr);
            CHECK_MESSAGE(host::fetchesWhileLocked() == 0, "the backend was called back while the library held the lock");
        }
    };
}

// ------------------------------------------------------------------------------------------- speed and acceleration

TEST_CASE("group speed: set for the group, measured at the motor with the longest way")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setMaxSpeed(20'000).setAcceleration(50'000).setPullInSpeed(0);
    y.setMaxSpeed(20'000).setAcceleration(50'000).setPullInSpeed(0);
    StepperGroup xy{x, y};
    CHECK(xy.getSpeed() == 0.0f);

    xy.setMaxSpeed(8'000).setAcceleration(16'000);
    CHECK(xy.moveAbsAsync({40'000, -10'000}) == errorCode::OK);
    host::run(0.25);
    CHECK(xy.getSpeed() == approx(4'000).epsilon(0.01)); // accelerating at 16000
    host::run(0.75);
    CHECK(xy.getSpeed() == approx(8'000).epsilon(0.0001));
    CHECK(x.getSpeed() == approx(8'000).epsilon(0.0001));
    CHECK(y.getSpeed() == approx(-2'000).epsilon(0.0001));

    xy.setMaxSpeed(2'000); // during the move
    host::run(1.0);
    CHECK(xy.getSpeed() == approx(2'000).epsilon(0.0001));
    xy.setMaxSpeed(0); // no speed of the group: as fast as the members allow, at the acceleration of the group
    host::run(1.0);
    CHECK(xy.getSpeed() == approx(18'000).epsilon(0.01));
    host::run(0.25);
    CHECK(xy.getSpeed() == approx(20'000).epsilon(0.0001));

    CHECK(host::runUntilIdle());
    CHECK(x.getPosition() == 40'000);
    CHECK(y.getPosition() == -10'000);
    CHECK(host::pulses(pinsX).net == 40'000);
    CHECK(host::pulses(pinsY).net == -10'000);
    CHECK(xy.getSpeed() == 0.0f);
    CHECK(reported.empty());
}

TEST_CASE("group speed: the settings of the motors are ceilings; more is limited and reported")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setMaxSpeed(20'000).setAcceleration(50'000);
    y.setMaxSpeed(5'000).setAcceleration(50'000);
    StepperGroup xy{x, y};

    xy.setMaxSpeed(15'000);
    CHECK(reported.empty()); // not moving: nothing to compare with
    CHECK(xy.moveAbsAsync({40'000, 20'000}) == errorCode::speedLimited); // y allows 10000 on this line
    CHECK(reported == std::vector<errorCode>{errorCode::speedLimited});
    host::run(1.0);
    CHECK(xy.getSpeed() == approx(10'000).epsilon(0.0001));

    reported.clear();
    xy.setAcceleration(500'000); // during the move: the motors allow 50000
    CHECK(reported == std::vector<errorCode>{errorCode::accelerationLimited});
    xy.setDeceleration(20'000);
    CHECK(reported.size() == 1);
    CHECK(host::runUntilIdle());
    CHECK(x.getPosition() == 40'000);
    CHECK(y.getPosition() == 20'000);
}

TEST_CASE("group speed: it belongs to the group object, also to a temporary one")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setMaxSpeed(20'000).setAcceleration(1'000'000);
    y.setMaxSpeed(20'000).setAcceleration(1'000'000);

    CHECK(StepperGroup{x, y}.setMaxSpeed(2'000).moveAbs({4'000, 1'000}) == errorCode::OK); // about 2 s
    host::Pulses slow = host::pulses(pinsX);
    CHECK(slow.last - slow.first == approx(2.0).epsilon(0.01));

    double t = host::now();
    CHECK(StepperGroup{x, y}.moveAbs({0, 0}) == errorCode::OK); // another group of the same motors, without a speed
    host::Pulses back = host::pulses(pinsX, t);
    CHECK(back.last - back.first == approx(0.2).epsilon(0.15));

    x.setPullInSpeed(0);
    y.setPullInSpeed(0);
    StepperGroup xy{x, y};
    xy.setMaxSpeed(1'000).setAcceleration(2'000).setDeceleration(500);
    for (int i = 0; i < 2; i++) // the settings stay for the following moves
    {
        t = host::now();
        xy.moveRel({3'000, 0});
        host::Pulses p = host::pulses(pinsX, t);
        CHECK(p.last - p.first == approx(0.5 + 1.75 + 2.0).epsilon(0.01)); // 250 steps up, 1750 at 1000 steps/s, 1000 down
    }
}
