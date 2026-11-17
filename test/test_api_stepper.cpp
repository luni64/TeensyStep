// The Stepper class on the simulated backend. The tests look at the pins, as a logic analyzer would: they count
// the step pulses and their directions and compare them with what the library reports. Requirements R12, R14,
// R15, R16, R20, R21; both kinds of backends (R4).
#include "TeensyStep.h"
#include "backend/host/HostBackend.h"
#include "approx.h"
#include "doctest.h"

#include <cmath>
#include <string>
#include <vector>

using namespace TeensyStep;
namespace host = TeensyStep::backend::host;

namespace
{
    const backend::MotorPins pins23{2, 3, true, true};
    const backend::MotorPins pins45{4, 5, true, true};

    std::vector<errorCode> reported;
    void collectErrors(errorCode code) { reported.push_back(code); }

    // every test starts with a fresh simulated board
    struct Board
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

TEST_CASE("stepper: the smallest sketch makes 1000 pulses")
{
    Board board;
    Stepper motor(2, 3);

    CHECK(motor.moveRel(1000) == errorCode::OK); // blocking: returns when the motor has arrived

    host::Pulses p = host::pulses(pins23);
    CHECK(p.count == 1000);
    CHECK(p.net == 1000);
    CHECK(p.minWidth == approx(5e-6).epsilon(0.001));
    CHECK(p.maxWidth == approx(5e-6).epsilon(0.001));
    CHECK(p.minPeriod == approx(1.0 / 800).epsilon(0.001)); // default maximum speed
    CHECK(motor.getPosition() == 1000);
    CHECK(motor.getSpeed() == 0.0f);
    CHECK_FALSE(motor.isMoving());
    CHECK(host::openChannels() == 0); // the timer is given back
    CHECK(reported.empty());

    CHECK(motor.moveRel(-300) == errorCode::OK);
    p = host::pulses(pins23);
    CHECK(p.count == 1300);
    CHECK(p.down == 300);
    CHECK(motor.getPosition() == 700);
    CHECK(host::level(3) == false); // DIR is low after moving down
}

TEST_CASE("stepper: a non-blocking move runs in the background")
{
    Board board;
    Stepper motor(2, 3);
    motor.setMaxSpeed(10'000).setAcceleration(20'000);

    CHECK(motor.moveAbsAsync(20'000) == errorCode::OK);
    CHECK(motor.isMoving());
    CHECK(host::openChannels() == 1);
    CHECK(host::pulses(pins23).count == 0);

    host::run(1.0);
    CHECK(motor.isMoving());
    CHECK(motor.getSpeed() == approx(10'000).epsilon(0.0001));
    long made = host::pulses(pins23).net;
    CHECK(made > 5'000);
    CHECK(std::abs(motor.getPosition() - made) <= 1); // the library is at most the one waiting step ahead of the pins

    CHECK(host::runUntilIdle());
    CHECK(motor.getPosition() == 20'000);
    CHECK(host::pulses(pins23).net == 20'000);
    CHECK_FALSE(motor.isMoving());
}

TEST_CASE("stepper: pulse polarity, pulse width and direction of rotation")
{
    Board board;
    Stepper motor(2, 3);
    motor.setStepPinPolarity(0).setPulseWidth(20).setInverseRotation(true);
    CHECK(host::level(2) == true);  // idle level of an active-low step signal
    CHECK(host::level(3) == false); // "up" is DIR low now

    motor.moveRel(500);
    backend::MotorPins inverted{2, 3, false, false};
    host::Pulses p = host::pulses(inverted);
    CHECK(p.count == 500);
    CHECK(p.net == 500);
    CHECK(p.minWidth == approx(20e-6).epsilon(0.001));
    host::run(0.0001); // let the last pulse end
    CHECK(host::level(2) == true);
    CHECK(host::level(3) == false);

    motor.moveRel(-200);
    CHECK(host::pulses(inverted).net == 300);
    CHECK(host::level(3) == true);
}

TEST_CASE("stepper: rotation, new speed, reversal, stop")
{
    Board board;
    Stepper motor(2, 3);
    motor.setAcceleration(20'000);
    motor.setMaxSpeed(10'000);

    CHECK(motor.rotateAsync(2'000) == errorCode::OK);
    host::run(1.0);
    CHECK(motor.getSpeed() == approx(2'000).epsilon(0.0001));

    motor.rotateAsync(6'000);
    host::run(1.0);
    CHECK(motor.getSpeed() == approx(6'000).epsilon(0.0001));

    motor.rotateAsync(-500);
    host::run(1.0);
    CHECK(motor.getSpeed() == approx(-500).epsilon(0.0001));

    CHECK(motor.stop() == errorCode::OK); // blocking
    CHECK_FALSE(motor.isMoving());
    CHECK(motor.getSpeed() == 0.0f);
    host::Pulses p = host::pulses(pins23, 0, 5e-6);
    CHECK(p.net == motor.getPosition());
    CHECK(p.down > 100);
    CHECK(p.dirTooLate == 0); // DIR was stable for the setup time before every step
    CHECK(host::openChannels() == 0);
}

TEST_CASE("stepper: a new target during a move")
{
    Board board;
    Stepper motor(2, 3);
    motor.setMaxSpeed(10'000).setAcceleration(20'000);
    motor.moveAbsAsync(50'000);
    host::run(2.0);
    motor.moveAbsAsync(-1'000);
    CHECK(host::runUntilIdle());
    CHECK(motor.getPosition() == -1'000);
    CHECK(host::pulses(pins23).net == -1'000);
}

TEST_CASE("stepper: settings change during a move")
{
    Board board;
    Stepper motor(2, 3);
    motor.setMaxSpeed(10'000).setAcceleration(20'000);
    motor.moveAbsAsync(100'000);
    host::run(1.0);
    motor.setMaxSpeed(2'000);
    host::run(1.0);
    CHECK(motor.getSpeed() == approx(2'000).epsilon(0.0001));
    motor.setMaxSpeed(8'000).setAcceleration(100'000);
    host::run(0.5);
    CHECK(motor.getSpeed() == approx(8'000).epsilon(0.0001));

    motor.setPulseWidth(50); // a hardware setting: refused while moving
    CHECK(reported == std::vector<errorCode>{errorCode::notSupported});
    CHECK(host::runUntilIdle());
    CHECK(motor.getPosition() == 100'000);
    CHECK(host::pulses(pins23).maxWidth == approx(5e-6).epsilon(0.001));
}

namespace
{
    struct Waypoint
    {
        int32_t x;
        float v;
    };
    const Waypoint track[] = {{10'000, 2'000}, {40'000, 9'000}, {50'000, 0}};
    Stepper* trackMotor    = nullptr;
    unsigned nextWaypoint  = 0;
    std::vector<float> speedAtWaypoint;
    std::vector<int32_t> positionAtWaypoint;

    void toNextWaypoint() // the arrival function of the sketch api-examples/06_TargetWithSpeed
    {
        if (nextWaypoint > 0)
        {
            speedAtWaypoint.push_back(trackMotor->getSpeed());
            positionAtWaypoint.push_back(trackMotor->getPosition());
        }
        if (nextWaypoint < 3)
        {
            trackMotor->moveAbsAsync(track[nextWaypoint].x, track[nextWaypoint].v);
            nextWaypoint++;
        }
    }
}

TEST_CASE("stepper: targets with a speed are chained from the arrival function")
{
    for (double lookahead : {0.0, 0.003})
    {
        CAPTURE(lookahead);
        Board board(lookahead);
        Stepper motor(2, 3);
        motor.setMaxSpeed(10'000).setAcceleration(20'000).setPullInSpeed(0);
        trackMotor   = &motor;
        nextWaypoint = 0;
        speedAtWaypoint.clear();
        positionAtWaypoint.clear();
        motor.onArrival(toNextWaypoint);

        toNextWaypoint();
        CHECK(host::runUntilIdle());

        CHECK(motor.getPosition() == 50'000);
        host::Pulses p = host::pulses(pins23);
        CHECK(p.net == 50'000);
        CHECK(p.down == 0);
        REQUIRE(speedAtWaypoint.size() == 3);
        CHECK(speedAtWaypoint[0] == approx(2'000).epsilon(0.0001));
        CHECK(speedAtWaypoint[1] == approx(9'000).epsilon(0.0001));
        CHECK(speedAtWaypoint[2] == 0.0f);
        CHECK(positionAtWaypoint == std::vector<int32_t>{10'000, 40'000, 50'000});

        // the motor never stopped on the way: between pulse 100 and pulse 49900 no gap is longer than 1 ms
        double longest = 0;
        for (size_t i = 100; i < 49'900; i++) longest = std::max(longest, p.times[i + 1] - p.times[i]);
        CHECK(longest < 0.001);
    }
}

TEST_CASE("stepper: a blocking move with a speed at the target returns at the target, and the motor runs on")
{
    Board board;
    Stepper motor(2, 3);
    motor.setMaxSpeed(5'000).setAcceleration(20'000);
    CHECK(motor.moveAbs(3'000, 500) == errorCode::OK);
    CHECK(motor.getPosition() >= 3'000);
    CHECK(motor.getPosition() <= 3'002);
    CHECK(motor.isMoving());
    CHECK(motor.getSpeed() == approx(500).epsilon(0.0001));
    host::run(1.0);
    CHECK(motor.getPosition() == approx(3'500).epsilon(0.002));
    motor.stop();
    CHECK(host::pulses(pins23).net == motor.getPosition());
}

TEST_CASE("stepper: a move to the current position makes no step and calls the arrival function")
{
    Board board;
    Stepper motor(2, 3);
    static int calls;
    calls = 0;
    motor.onArrival([] { calls++; });
    motor.setPosition(77);
    CHECK(motor.moveAbs(77) == errorCode::OK);
    CHECK(calls == 1);
    CHECK(host::pulses(pins23).count == 0);
    CHECK(host::openChannels() == 0);
    CHECK(motor.stopAsync() == errorCode::OK); // nothing to stop
    CHECK(motor.rotateAsync(0) == errorCode::OK);
    CHECK_FALSE(motor.isMoving());
}

TEST_CASE("stepper: emergency stop makes no further pulse, and the position matches the pulses made")
{
    for (double lookahead : {0.0, 0.004})
    {
        CAPTURE(lookahead);
        Board board(lookahead);
        Stepper motor(2, 3);
        motor.setMaxSpeed(20'000).setAcceleration(50'000);
        motor.moveAbsAsync(100'000);
        host::run(1.2345);

        motor.emergencyStop();
        long made = host::pulses(pins23).net;
        CHECK_FALSE(motor.isMoving());
        CHECK(motor.getSpeed() == 0.0f);
        CHECK(motor.getPosition() == made); // steps that were calculated but not made are not counted
        CHECK(host::openChannels() == 0);
        host::run(1.0);
        CHECK(host::pulses(pins23).net == made);

        motor.moveAbs(0); // usable at once
        CHECK(motor.getPosition() == 0);
        CHECK(host::pulses(pins23).net == 0);
    }
}

namespace
{
    Stepper* haltMotor = nullptr;
    void haltAtTarget() { haltMotor->emergencyStop(); }
}

TEST_CASE("stepper: emergency stop from the arrival function")
{
    for (double lookahead : {0.0, 0.004})
    {
        CAPTURE(lookahead);
        Board board(lookahead);
        Stepper motor(2, 3);
        motor.setMaxSpeed(5'000).setAcceleration(20'000);
        haltMotor = &motor;
        motor.onArrival(haltAtTarget);
        motor.moveAbsAsync(2'000, 3'000); // would run on at 3000 steps/s
        host::run(5.0);
        CHECK_FALSE(motor.isMoving());
        CHECK(motor.getPosition() == host::pulses(pins23).net);
        CHECK(std::abs(motor.getPosition() - 2'000) <= 20); // at the target; a buffered backend is told a little early
    }
}

TEST_CASE("stepper: no free timer is reported, and the motor is unchanged")
{
    Board board;
    host::setTimers(1);
    Stepper a(2, 3), b(4, 5);

    CHECK(a.rotateAsync(500) == errorCode::OK);
    CHECK(b.moveAbsAsync(1'000) == errorCode::noTimer);
    CHECK(reported == std::vector<errorCode>{errorCode::noTimer});
    CHECK_FALSE(b.isMoving());
    CHECK(b.getPosition() == 0);
    host::run(1.0);
    CHECK(host::pulses(pins45).count == 0);

    a.stop();
    CHECK(b.moveAbs(1'000) == errorCode::OK); // the timer is free again
    CHECK(b.getPosition() == 1'000);
}

TEST_CASE("stepper: values outside the range are limited and reported to the error function")
{
    Board board;
    Stepper motor(2, 3);
    motor.setMaxSpeed(5'000'000).setAcceleration(-1).setPullInSpeed(-5);
    CHECK(reported == std::vector<errorCode>{errorCode::speedLimited, errorCode::accelerationLimited, errorCode::speedLimited});
    reported.clear();

    motor.setMaxSpeed(1'000).setAcceleration(10'000).setPullInSpeed(0);
    CHECK(motor.rotateAsync(4'000) == errorCode::speedLimited); // a warning: the motor runs, at its maximum speed
    host::run(1.0);
    CHECK(motor.getSpeed() == approx(1'000).epsilon(0.0001));
    CHECK(reported == std::vector<errorCode>{errorCode::speedLimited});
}

TEST_CASE("stepper: two motors move independently at the same time")
{
    Board board;
    Stepper a(2, 3), b(4, 5);
    a.setMaxSpeed(10'000).setAcceleration(20'000);
    b.setMaxSpeed(3'000).setAcceleration(5'000);
    a.moveAbsAsync(30'000);
    b.moveAbsAsync(-8'000);
    CHECK(host::openChannels() == 2);
    host::run(1.0);
    CHECK(a.isMoving());
    CHECK(b.isMoving());
    CHECK(host::runUntilIdle());
    CHECK(host::pulses(pins23).net == 30'000);
    CHECK(host::pulses(pins45).net == -8'000);
    CHECK(a.getPosition() == 30'000);
    CHECK(b.getPosition() == -8'000);
}

TEST_CASE("stepper: a buffered backend makes the same pulses at the same times")
{
    std::vector<double> times[2];
    double lookahead[2] = {0.0, 0.005};
    for (int i = 0; i < 2; i++)
    {
        Board board(lookahead[i]);
        Stepper motor(2, 3);
        motor.setMaxSpeed(20'000).setAcceleration(50'000).setPullInSpeed(200);
        motor.moveAbsAsync(30'000);
        CHECK(motor.isMoving());
        CHECK(host::runUntilIdle());
        CHECK_FALSE(motor.isMoving());
        CHECK(motor.getPosition() == 30'000);
        times[i] = host::pulses(pins23).times;
    }
    REQUIRE(times[0].size() == 30'000);
    REQUIRE(times[1].size() == 30'000);
    double worst = 0;
    for (size_t i = 0; i < 30'000; i++) worst = std::max(worst, std::fabs(times[0][i] - times[1][i]));
    CHECK(worst < 1e-9);
}

TEST_CASE("stepper: with a buffered backend the motor counts as moving until the last pulse is made")
{
    Board board(0.010);
    Stepper motor(2, 3);
    motor.setMaxSpeed(1'000).setAcceleration(100'000).setPullInSpeed(1'000);
    motor.moveAbsAsync(20); // 20 ms of pulses; all of them are fetched within the first 10 ms
    host::run(0.015);
    CHECK(host::pulses(pins23).count < 20);
    CHECK(motor.isMoving());
    CHECK(host::openChannels() == 1);
    CHECK(host::runUntilIdle());
    CHECK(host::pulses(pins23).count == 20);
    CHECK_FALSE(motor.isMoving());

    motor.moveAbsAsync(40); // a command during the last pulses of a move continues it
    host::run(0.015);
    motor.moveAbsAsync(100);
    CHECK(host::runUntilIdle());
    CHECK(motor.getPosition() == 100);
    CHECK(host::pulses(pins23).net == 100);
}

TEST_CASE("stepper: at low speed the wait events make no pulses")
{
    Board board;
    Stepper motor(2, 3);
    motor.setMaxSpeed(100).setAcceleration(1'000).setPullInSpeed(0);
    motor.rotateAsync(2); // one step every 0.5 s, handed out in pieces of 1 ms
    host::run(3.2);
    host::Pulses p = host::pulses(pins23);
    CHECK(p.count >= 5);
    CHECK(p.count <= 7);
    CHECK(p.maxPeriod == approx(0.5).epsilon(0.01));

    motor.stopAsync(); // takes effect within a millisecond, without a further step
    host::run(0.002);
    CHECK_FALSE(motor.isMoving());
    CHECK(host::pulses(pins23).count == p.count);
}

TEST_CASE("stepper: a motor that goes out of scope stops")
{
    Board board;
    {
        Stepper motor(2, 3);
        motor.rotateAsync(500);
        host::run(0.5);
        CHECK(host::openChannels() == 1);
    }
    CHECK(host::openChannels() == 0);
    long made = host::pulses(pins23).count;
    host::run(1.0);
    CHECK(host::pulses(pins23).count == made);
}

TEST_CASE("stepper: more motors than channels")
{
    Board board;
    std::vector<Stepper*> motors;
    for (unsigned i = 0; i < backend::maxChannels + 1; i++) motors.push_back(new Stepper(2 * (int)i + 10, 2 * (int)i + 11));
    for (unsigned i = 0; i < backend::maxChannels; i++) CHECK(motors[i]->rotateAsync(100) == errorCode::OK);
    CHECK(motors[backend::maxChannels]->rotateAsync(100) == errorCode::noTimer);
    motors[0]->emergencyStop();
    CHECK(motors[backend::maxChannels]->rotateAsync(100) == errorCode::OK);
    for (Stepper* m : motors) delete m;
    CHECK(host::openChannels() == 0);
}

TEST_CASE("stepper: a relative move during a move counts from the current position")
{
    Board board;
    Stepper motor(2, 3);
    motor.setMaxSpeed(10000).setAcceleration(20000);
    motor.moveAbsAsync(50000);
    host::run(2.0);
    int32_t here = motor.getPosition();
    CHECK(here > 10000);
    motor.moveRelAsync(-4000);
    CHECK(host::runUntilIdle());
    CHECK(motor.getPosition() == here - 4000);
    CHECK(host::pulses(pins23).net == here - 4000);
}

TEST_CASE("emergencyStopAll stops single motors and groups")
{
    Board board;
    Stepper a(2, 3), b(4, 5), c(6, 7);
    for (Stepper* m : {&a, &b, &c}) m->setMaxSpeed(10000).setAcceleration(20000);
    a.rotateAsync(5000);
    StepperGroup g{b, c};
    g.moveAbsAsync({50000, -20000});
    host::run(1.0);
    CHECK(host::openChannels() == 2);

    emergencyStopAll();
    CHECK(host::openChannels() == 0);
    CHECK_FALSE(a.isMoving());
    CHECK_FALSE(g.isMoving());
    CHECK(a.getPosition() == host::pulses(pins23).net);
    CHECK(b.getPosition() == host::pulses(pins45).net);
    long made = host::pulses(pins23).count;
    host::run(0.5);
    CHECK(host::pulses(pins23).count == made);
}

TEST_CASE("every code has a text")
{
    for (errorCode c : {errorCode::OK, errorCode::speedLimited, errorCode::accelerationLimited, errorCode::invalidArgument, errorCode::motorBusy,
                        errorCode::noTimer, errorCode::notSupported, errorCode::internal})
    {
        CHECK(describe(c)[0] != 0);
        CHECK(std::string(describe(c)) != "Unknown code");
    }
    CHECK(std::string(describe(static_cast<errorCode>(77))) == "Unknown code");
}
