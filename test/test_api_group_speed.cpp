// StepperGroup on the simulated backend: speed and acceleration of a group, moves with a speed at the target,
// and moves that follow each other without a stop. The tests read the pins.
#include "TeensyStep.h"
#include "approx.h"
#include "backend/host/HostBackend.h"
#include "doctest.h"

#include <algorithm>
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

    // time from a point in time to the next pulse of any of the two motors
    double nextPulseAfter(double t)
    {
        double next = 1e9;
        for (const backend::MotorPins& pins : {pinsX, pinsY})
            for (double p : host::pulses(pins).times)
                if (p > t + 1e-9)
                {
                    next = std::min(next, p);
                    break;
                }
        return next - t;
    }

    // a path of waypoints, driven from the arrival function as a sketch does it
    struct Waypoint
    {
        int32_t x, y;
        float speed;
    };
    std::vector<Waypoint> route;
    size_t nextPoint;
    Stepper* mx;
    Stepper* my;
    StepperGroup* group;
    std::vector<double> arrivalTimes;
    std::vector<std::pair<int32_t, int32_t>> arrivalPositions;

    void toNextPoint()
    {
        if (nextPoint < route.size())
        {
            const Waypoint& w = route[nextPoint++];
            group->moveAbsAsync({w.x, w.y}, w.speed);
        }
    }

    void onWaypoint()
    {
        arrivalTimes.push_back(host::now());
        arrivalPositions.push_back({mx->getPosition(), my->getPosition()});
        toNextPoint();
    }
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

// ------------------------------------------------------------------------------------------- speed at the target

TEST_CASE("group target with a speed: a blocking move returns when the targets are passed; the group runs on")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setMaxSpeed(20'000).setAcceleration(50'000);
    y.setMaxSpeed(20'000).setAcceleration(50'000);
    StepperGroup xy{x, y};

    // The blocking move returns when the targets are reported; until the sketch looks, a few steps more are made.
    CHECK(xy.moveAbs({10'000, 5'000}, 3'000) == errorCode::OK);
    CHECK(xy.isMoving());
    int32_t x0 = x.getPosition(), y0 = y.getPosition();
    CHECK(x0 >= 10'000);
    CHECK(x0 <= 10'006);
    CHECK(std::abs(x0 - 2 * y0) <= 1);
    CHECK(xy.getSpeed() == approx(3'000).epsilon(0.0001));
    CHECK(x.isMoving());

    host::run(0.1); // runs on along the line
    CHECK(std::abs(x.getPosition() - (x0 + 300)) <= 1);
    CHECK(std::abs(x.getPosition() - 2 * y.getPosition()) <= 1);

    CHECK(xy.moveAbs({11'000, 9'000}, 1'000) == errorCode::OK); // the next segment, from where the motors are
    CHECK(y.getPosition() >= 9'000);
    CHECK(y.getPosition() <= 9'003);
    CHECK(x.getPosition() >= 11'000);
    CHECK(x.getPosition() <= 11'001);
    CHECK(y.getSpeed() == approx(1'000).epsilon(0.0001));

    x.setTargetAbs(0);
    y.setTargetAbs(0);
    CHECK(xy.move() == errorCode::OK); // to the targets of the members, and stop
    CHECK_FALSE(xy.isMoving());
    CHECK(host::pulses(pinsX).net == 0);
    CHECK(host::pulses(pinsY).net == 0);
    CHECK(reported.empty());
}

TEST_CASE("group target with a speed: a group of one motor, and the forms with an array")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    StepperGroup one{x};
    CHECK(one.moveAbs({1'000}, 500) == errorCode::OK);
    CHECK(x.getPosition() == 1'000);
    CHECK(one.getSpeed() == approx(500).epsilon(0.0001));
    one.stop();

    StepperGroup xy{x, y};
    int32_t there[] = {3'000, 1'000}, back[] = {-500, -500};
    CHECK(xy.moveAbs(there, 2, 400) == errorCode::OK);
    CHECK(xy.moveRel(back, 2, 0, 300) == errorCode::OK);
    CHECK(x.getPosition() == 2'500);
    CHECK(y.getPosition() == 500);
    CHECK(xy.move(250) == errorCode::OK); // already there: nothing to pass
    CHECK_FALSE(xy.isMoving());

    x.setTargetRel(2'000);
    y.setTargetRel(1'000);
    CHECK(xy.move(600) == errorCode::OK); // the targets of the members, passed at a speed
    CHECK(xy.isMoving());
    CHECK(xy.getSpeed() == approx(600).epsilon(0.0001));
    xy.emergencyStop();
}

TEST_CASE("group target with a speed: within a segment only the speed at the target can change")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setMaxSpeed(20'000).setAcceleration(50'000);
    y.setMaxSpeed(20'000).setAcceleration(50'000);
    StepperGroup xy{x, y};

    xy.moveAbsAsync({40'000, 10'000}, 5'000);
    host::run(1.0);
    CHECK(xy.moveAbsAsync({0, 0}) == errorCode::notSupported);
    CHECK(xy.moveAbsAsync({40'000, 10'001}, 5'000) == errorCode::notSupported);
    CHECK(reported == std::vector<errorCode>{errorCode::notSupported, errorCode::notSupported});
    reported.clear();

    CHECK(xy.moveAbsAsync({40'000, 10'000}) == errorCode::OK); // the same targets: stop there after all
    CHECK(host::runUntilIdle());
    CHECK(x.getPosition() == 40'000);
    CHECK(y.getPosition() == 10'000);
    CHECK(host::pulses(pinsX).down == 0); // no overshoot
    CHECK(reported.empty());
}

TEST_CASE("group target with a speed: a way that is too short is reported, and taken without a run-up")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setMaxSpeed(20'000).setAcceleration(50'000);
    y.setMaxSpeed(20'000).setAcceleration(50'000);
    StepperGroup xy{x, y};

    CHECK(xy.moveAbs({20, 10}, 10'000) == errorCode::targetSpeedNotReached);
    CHECK(reported == std::vector<errorCode>{errorCode::targetSpeedNotReached});
    CHECK(x.getPosition() >= 20);
    CHECK(x.getPosition() <= 23);
    CHECK(host::pulses(pinsX).down == 0);
    CHECK(xy.getSpeed() < 1'500);
    xy.emergencyStop();
}

// ------------------------------------------------------------------------------------------- segments from the arrival function

TEST_CASE("group segments: the arrival function gives the next move; the motors pass every point without a gap")
{
    for (double lookahead : {0.0, 0.003})
    {
        CAPTURE(lookahead);
        Board board(lookahead);
        Stepper x(2, 3), y(4, 5);
        x.setMaxSpeed(15'000).setAcceleration(30'000);
        y.setMaxSpeed(15'000).setAcceleration(30'000);
        StepperGroup xy{x, y};

        route     = {{10'000, 0, 3'000}, {12'000, 2'000, 3'000}, {12'000, 10'000, 2'000}, {3'000, 4'000, 1'000}, {0, 0, 0}};
        nextPoint = 0;
        mx = &x, my = &y, group = &xy;
        arrivalTimes.clear();
        arrivalPositions.clear();

        xy.onArrival(onWaypoint);
        toNextPoint();
        CHECK(host::runUntilIdle());

        CHECK(x.getPosition() == 0);
        CHECK(y.getPosition() == 0);
        host::Pulses px = host::pulses(pinsX), py = host::pulses(pinsY);
        CHECK(px.net == 0);
        CHECK(py.net == 0);
        CHECK(px.count == 10'000 + 2'000 + 9'000 + 3'000); // no step more than the route needs
        CHECK(py.count == 2'000 + 8'000 + 6'000 + 4'000);
        REQUIRE(arrivalTimes.size() == route.size());
        for (size_t i = 0; i + 1 < route.size(); i++)
        {
            CAPTURE(i);
            if (lookahead == 0) // the function is called when the point is reached (earlier, if the backend buffers)
            {
                CHECK(arrivalPositions[i].first == route[i].x);
                CHECK(arrivalPositions[i].second == route[i].y);
                CHECK(nextPulseAfter(arrivalTimes[i]) <= 1.02 / route[i].speed);
            }
        }
        CHECK(reported.empty());
    }
}

TEST_CASE("group segments: the speed at which a segment starts can be given")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setMaxSpeed(20'000).setAcceleration(50'000);
    y.setMaxSpeed(20'000).setAcceleration(50'000);
    StepperGroup xy{x, y};

    route     = {{10'000, 8'000, 0}};
    nextPoint = 0;
    mx = &x, my = &y, group = &xy;
    arrivalTimes.clear();
    arrivalPositions.clear();
    xy.onArrival([] {
        arrivalTimes.push_back(host::now());
        if (nextPoint++ == 0) group->moveAbsAsync({10'000, 8'000}, 0, 1'200); // a corner: y starts at 1200
    });
    xy.moveAbsAsync({10'000, 0}, 3'000);
    CHECK(host::runUntilIdle());
    REQUIRE(arrivalTimes.size() == 2);
    double t         = arrivalTimes[0];
    host::Pulses py = host::pulses(pinsY, t);
    CHECK(py.times[0] - t == approx(1.0 / 1'200).epsilon(0.05));
    CHECK(x.getPosition() == 10'000);
    CHECK(y.getPosition() == 8'000);
    CHECK(host::pulses(pinsX).count == 10'000);
}

TEST_CASE("group segments: stop and emergency stop of a group that runs on")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setMaxSpeed(20'000).setAcceleration(50'000);
    y.setMaxSpeed(20'000).setAcceleration(50'000);
    StepperGroup xy{x, y};

    xy.moveAbs({10'000, 5'000}, 4'000);
    host::run(0.05);
    CHECK(xy.stop() == errorCode::OK);
    CHECK_FALSE(xy.isMoving());
    CHECK(x.getPosition() > 10'200);
    CHECK(std::abs(x.getPosition() - 2 * y.getPosition()) <= 1); // still on the line

    xy.moveAbs({0, 0}, 4'000);
    host::run(0.05);
    xy.emergencyStop();
    CHECK_FALSE(xy.isMoving());
    CHECK(x.getPosition() == host::pulses(pinsX).net);
    CHECK(y.getPosition() == host::pulses(pinsY).net);
    CHECK(host::openChannels() == 0);
}
