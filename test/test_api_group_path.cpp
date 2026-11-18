// advanced::GroupPath on the simulated backend: moves of a group with a speed at the targets, and moves that
// follow each other without a stop. The tests read the pins.
#include "TeensyStepAdvanced.h"
#include "approx.h"
#include "backend/host/HostBackend.h"
#include "doctest.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace TeensyStep;
using advanced::GroupPath;
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
    GroupPath* path;
    std::vector<double> arrivalTimes;
    std::vector<std::pair<int32_t, int32_t>> arrivalPositions;

    void toNextPoint()
    {
        if (nextPoint < route.size())
        {
            const Waypoint& w = route[nextPoint++];
            path->moveAbsAsync({w.x, w.y}, w.speed);
        }
    }

    void onWaypoint()
    {
        arrivalTimes.push_back(host::now());
        arrivalPositions.push_back({mx->getPosition(), my->getPosition()});
        toNextPoint();
    }

    void noteArrival()
    {
        arrivalTimes.push_back(host::now());
        arrivalPositions.push_back({mx->getPosition(), my->getPosition()});
    }

    // runs until the arrival function has been called `count` times
    bool runUntilArrivals(size_t count, double maxSeconds = 100)
    {
        for (double t = 0; t < maxSeconds && arrivalTimes.size() < count; t += 0.0002) host::run(0.0002);
        return arrivalTimes.size() >= count;
    }

    struct Motors // two motors with the usual settings, and the bookkeeping of the arrival functions
    {
        Stepper x{2, 3}, y{4, 5};
        StepperGroup xy{x, y};
        GroupPath p{xy};
        Motors()
        {
            x.setMaxSpeed(20'000).setAcceleration(50'000);
            y.setMaxSpeed(20'000).setAcceleration(50'000);
            mx = &x, my = &y, path = &p;
            nextPoint = 0;
            route.clear();
            arrivalTimes.clear();
            arrivalPositions.clear();
        }
    };
}

TEST_CASE("group path: the group passes its targets at the speed and runs on along the line")
{
    Board board;
    Motors m;
    m.xy.onArrival(noteArrival);

    CHECK(m.p.moveAbsAsync({10'000, 5'000}, 3'000) == errorCode::OK);
    REQUIRE(runUntilArrivals(1));
    CHECK(arrivalPositions[0] == std::make_pair(10'000, 5'000)); // exactly at the targets when the function is called
    CHECK(m.xy.isMoving());
    CHECK(m.x.isMoving());
    CHECK(m.xy.getSpeed() == approx(3'000).epsilon(0.0001));

    int32_t x0 = m.x.getPosition();
    host::run(0.1); // runs on along the line
    CHECK(std::abs(m.x.getPosition() - (x0 + 300)) <= 1);
    CHECK(std::abs(m.x.getPosition() - 2 * m.y.getPosition()) <= 1);

    CHECK(m.p.moveAbsAsync({11'000, 9'000}, 1'000) == errorCode::OK); // the next move, from where the motors are
    REQUIRE(runUntilArrivals(2));
    CHECK(arrivalPositions[1] == std::make_pair(11'000, 9'000));
    CHECK(m.y.getSpeed() == approx(1'000).epsilon(0.0001));

    CHECK(m.xy.moveAbs({0, 0}) == errorCode::OK); // a move of the group itself ends the path: it stops at its targets
    CHECK_FALSE(m.xy.isMoving());
    CHECK(host::pulses(pinsX).net == 0);
    CHECK(host::pulses(pinsY).net == 0);
    CHECK(reported.empty());
}

TEST_CASE("group path: relative moves, a group of one motor, and the forms with an array")
{
    Board board;
    Motors m;
    m.xy.onArrival(noteArrival);

    StepperGroup one{m.x};
    GroupPath onePath(one);
    one.onArrival(noteArrival);
    CHECK(onePath.moveAbsAsync({1'000}, 500) == errorCode::OK);
    REQUIRE(runUntilArrivals(1));
    CHECK(m.x.getPosition() == 1'000);
    CHECK(one.getSpeed() == approx(500).epsilon(0.0001));
    one.stop();

    int32_t x0      = m.x.getPosition();
    int32_t there[] = {3'000, 1'000}, back[] = {-500, -500};
    CHECK(m.p.moveAbsAsync(there, 2, 400) == errorCode::OK);
    REQUIRE(runUntilArrivals(2));
    CHECK(m.xy.getSpeed() == approx(400).epsilon(0.0001));
    CHECK(m.p.moveRelAsync(back, 2, 300) == errorCode::OK);
    REQUIRE(runUntilArrivals(3));
    CHECK(arrivalPositions[2] == std::make_pair(2'500, 500));
    CHECK(m.p.moveRelAsync({-100, 0}) == errorCode::OK); // stops there
    CHECK(host::runUntilIdle());
    CHECK(m.x.getPosition() == 2'400);
    CHECK(m.p.moveAbsAsync(there, 3, 400) == errorCode::invalidArgument);
    CHECK(x0 > 1'000);
}

TEST_CASE("group path: within a move only the speed at the targets can change")
{
    Board board;
    Motors m;

    m.p.moveAbsAsync({40'000, 10'000}, 5'000);
    host::run(1.0);
    CHECK(m.p.moveAbsAsync({0, 0}) == errorCode::notSupported);
    CHECK(m.xy.moveAbsAsync({0, 0}) == errorCode::notSupported); // also not by the group itself
    CHECK(m.p.moveAbsAsync({40'000, 10'001}, 5'000) == errorCode::notSupported);
    CHECK(reported == std::vector<errorCode>{errorCode::notSupported, errorCode::notSupported, errorCode::notSupported});
    reported.clear();

    CHECK(m.p.setSpeedAtTarget(0) == errorCode::OK); // stop there after all
    CHECK(host::runUntilIdle());
    CHECK(m.x.getPosition() == 40'000);
    CHECK(m.y.getPosition() == 10'000);
    CHECK(host::pulses(pinsX).down == 0); // no overshoot
    CHECK(reported.empty());

    CHECK(m.p.setSpeedAtTarget(100) == errorCode::notSupported); // the group stands still
    m.xy.onArrival(noteArrival);
    m.p.moveAbsAsync({50'000, 10'000});
    host::run(0.2);
    CHECK(m.p.setSpeedAtTarget(2'000) == errorCode::OK); // from a stop to a speed
    REQUIRE(runUntilArrivals(1));
    CHECK(m.xy.getSpeed() == approx(2'000).epsilon(0.0001));
    CHECK(m.p.setSpeedAtTarget(100) == errorCode::notSupported); // the targets are passed
    CHECK(m.p.setSpeedAtTarget(-1) == errorCode::invalidArgument);
    m.xy.emergencyStop();
}

TEST_CASE("group path: a way that is too short is reported, and taken without a run-up")
{
    Board board;
    Motors m;
    m.xy.onArrival(noteArrival);

    CHECK(m.p.moveAbsAsync({20, 10}, 10'000) == errorCode::targetSpeedNotReached);
    CHECK(reported == std::vector<errorCode>{errorCode::targetSpeedNotReached});
    REQUIRE(runUntilArrivals(1));
    CHECK(arrivalPositions[0] == std::make_pair(20, 10));
    CHECK(host::pulses(pinsX).down == 0);
    CHECK(m.xy.getSpeed() < 1'500);
    m.xy.emergencyStop();
}

TEST_CASE("group path: the arrival function gives the next move; the motors pass every point without a gap")
{
    for (double lookahead : {0.0, 0.003})
    {
        CAPTURE(lookahead);
        Board board(lookahead);
        Motors m;
        m.x.setMaxSpeed(15'000).setAcceleration(30'000);
        m.y.setMaxSpeed(15'000).setAcceleration(30'000);

        route = {{10'000, 0, 3'000}, {12'000, 2'000, 3'000}, {12'000, 10'000, 2'000}, {3'000, 4'000, 1'000}, {0, 0, 0}};
        m.xy.onArrival(onWaypoint);
        toNextPoint();
        CHECK(host::runUntilIdle());

        CHECK(m.x.getPosition() == 0);
        CHECK(m.y.getPosition() == 0);
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

TEST_CASE("group path: the speed at which a move starts can be given")
{
    Board board;
    Motors m;

    m.xy.onArrival([] {
        arrivalTimes.push_back(host::now());
        if (nextPoint++ == 0) path->moveAbsAsync({10'000, 8'000}, 0, 1'200); // a corner: y starts at 1200
    });
    m.p.moveAbsAsync({10'000, 0}, 3'000);
    CHECK(host::runUntilIdle());
    REQUIRE(arrivalTimes.size() == 2);
    double t         = arrivalTimes[0];
    host::Pulses py = host::pulses(pinsY, t);
    CHECK(py.times[0] - t == approx(1.0 / 1'200).epsilon(0.05));
    CHECK(m.x.getPosition() == 10'000);
    CHECK(m.y.getPosition() == 8'000);
    CHECK(host::pulses(pinsX).count == 10'000);

    t = host::now();
    CHECK(m.p.moveAbsAsync({10'000, 0}, 0, 2'000) == errorCode::OK); // also from standstill
    CHECK(host::runUntilIdle());
    py = host::pulses(pinsY, t);
    CHECK(py.times[0] - t == approx(1.0 / 2'000).epsilon(0.05));
}

TEST_CASE("group path: stop and emergency stop of a group that runs on")
{
    Board board;
    Motors m;
    m.xy.onArrival(noteArrival);

    m.p.moveAbsAsync({10'000, 5'000}, 4'000);
    REQUIRE(runUntilArrivals(1));
    host::run(0.05);
    CHECK(m.xy.stop() == errorCode::OK);
    CHECK_FALSE(m.xy.isMoving());
    CHECK(m.x.getPosition() > 10'200);
    CHECK(std::abs(m.x.getPosition() - 2 * m.y.getPosition()) <= 1); // still on the line

    m.p.moveAbsAsync({0, 0}, 4'000);
    REQUIRE(runUntilArrivals(2));
    host::run(0.05);
    m.xy.emergencyStop();
    CHECK_FALSE(m.xy.isMoving());
    CHECK(m.x.getPosition() == host::pulses(pinsX).net);
    CHECK(m.y.getPosition() == host::pulses(pinsY).net);
    CHECK(host::openChannels() == 0);
}
