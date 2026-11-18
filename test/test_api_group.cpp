// StepperGroup and the enable pin on the simulated backend. The tests read the pins: pulse counts, directions and
// times. Requirements R1, R11, R14, R20, R21, R24.
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
    const backend::MotorPins pinsZ{6, 7, true, true};

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

    // Largest distance of a follower from the line, in steps, read from the pins: at each pulse of the follower,
    // its count is compared with its share of the pulses the lead motor has made up to that time.
    // changes of a pin from time `since` on
    std::vector<double> changesOf(uint8_t pin, double since)
    {
        std::vector<double> times;
        for (const host::Edge& e : host::edges())
            if (e.pin == pin && e.t >= since - 1e-9) times.push_back(e.t);
        return times;
    }

    double worstOffLine(const host::Pulses& lead, const host::Pulses& follower, double share)
    {
        double worst = 0;
        size_t made  = 0;
        for (size_t i = 0; i < follower.times.size(); i++)
        {
            while (made < lead.times.size() && lead.times[made] <= follower.times[i] + 1e-12) made++;
            worst = std::max(worst, std::fabs((double)(i + 1) - share * (double)made));
        }
        return worst;
    }
}

TEST_CASE("group: the members move on a straight line and arrive together")
{
    Board board;
    Stepper x(2, 3), y(4, 5), z(6, 7);
    x.setMaxSpeed(20'000).setAcceleration(50'000);
    y.setMaxSpeed(20'000).setAcceleration(50'000);
    z.setMaxSpeed(5'000).setAcceleration(10'000);
    StepperGroup xyz{x, y, z};

    CHECK(xyz.moveAbs({40'000, -10'000, 1'000}) == errorCode::OK); // blocking
    CHECK_FALSE(xyz.isMoving());
    CHECK(x.getPosition() == 40'000);
    CHECK(y.getPosition() == -10'000);
    CHECK(z.getPosition() == 1'000);

    host::Pulses px = host::pulses(pinsX), py = host::pulses(pinsY), pz = host::pulses(pinsZ);
    CHECK(px.net == 40'000);
    CHECK(py.net == -10'000);
    CHECK(pz.net == 1'000);
    CHECK(py.count == 10'000);
    CHECK(worstOffLine(px, py, 0.25) <= 0.5 + 1e-9);
    CHECK(worstOffLine(px, pz, 0.025) <= 0.5 + 1e-9);
    CHECK(py.last <= px.last + 1e-12); // the last pulse of all is one of the motor with the longest way
    CHECK(pz.last <= px.last + 1e-12);
    CHECK(px.minPeriod == approx(1.0 / 20'000).epsilon(0.001)); // z allows 5000 / 0.025: no limit for the path
    CHECK(host::openChannels() == 0);
    CHECK(reported.empty());
}

TEST_CASE("group: targets from the members, groups created on the fly, a temporary group that moves on")
{
    Board board;
    Stepper x(2, 3), y(4, 5), z(6, 7);
    StepperGroup xyz{x, y, z};

    x.setTargetAbs(300);
    y.setTargetRel(-500);
    xyz.move(); // z has no target: it stays
    CHECK(x.getPosition() == 300);
    CHECK(y.getPosition() == -500);
    CHECK(z.getPosition() == 0);
    CHECK(host::pulses(pinsZ).count == 0);

    xyz.move(); // the targets are positions, not distances: nothing left to do
    CHECK(host::pulses(pinsX).count == 300);

    xyz.moveAbs({100, -500, 0}); // targets given to the group replace the targets of the members
    xyz.move();
    CHECK(x.getPosition() == 100);
    CHECK(host::pulses(pinsX).count == 500);
    xyz.moveAbs({300, -500, 0});

    StepperGroup{y, z}.moveAbs({0, 250});
    CHECK(y.getPosition() == 0);
    CHECK(z.getPosition() == 250);

    StepperGroup{x, z}.moveAbsAsync({0, 0}); // the group object is gone, the move goes on
    CHECK(x.isMoving());
    CHECK(host::runUntilIdle());
    CHECK(x.getPosition() == 0);
    CHECK(z.getPosition() == 0);
    CHECK(host::pulses(pinsX).net == 0);
    CHECK(host::pulses(pinsZ).net == 0);

    z.moveAbs(40); // a member moves on its own again
    CHECK(z.getPosition() == 40);
}

TEST_CASE("group: relative moves")
{
    Board board;
    Stepper x(2, 3), y(4, 5), z(6, 7);
    StepperGroup xyz{x, y, z};
    x.setPosition(1'000);
    y.setPosition(-200);

    CHECK(xyz.moveRel({400, -100, 0}) == errorCode::OK); // blocking; distances from where the members stand
    CHECK_FALSE(xyz.isMoving());
    CHECK(x.getPosition() == 1'400);
    CHECK(y.getPosition() == -300);
    CHECK(z.getPosition() == 0);
    host::Pulses px = host::pulses(pinsX), py = host::pulses(pinsY);
    CHECK(px.net == 400);
    CHECK(py.net == -100);
    CHECK(host::pulses(pinsZ).count == 0);
    CHECK(worstOffLine(px, py, 0.25) <= 0.5 + 1e-9);

    CHECK(xyz.moveRel({400, -100, 0}) == errorCode::OK); // the same command again moves again
    CHECK(x.getPosition() == 1'800);
    CHECK(y.getPosition() == -400);

    CHECK(xyz.moveRelAsync({-800, 200, 50}) == errorCode::OK); // non-blocking
    CHECK(xyz.isMoving());
    CHECK(host::runUntilIdle());
    CHECK(x.getPosition() == 1'000);
    CHECK(y.getPosition() == -200);
    CHECK(z.getPosition() == 50);

    const int32_t d[] = {10, 20, 30}; // distances from an array
    CHECK(xyz.moveRel(d, 3) == errorCode::OK);
    CHECK(z.getPosition() == 80);

    xyz.move(); // a relative move sets the targets of the members, like an absolute one: nothing left to do
    CHECK(x.getPosition() == 1'010);

    CHECK(xyz.moveRel({1, 2}) == errorCode::invalidArgument); // wrong number of distances
    CHECK(x.getPosition() == 1'010);
    CHECK(reported == std::vector<errorCode>{errorCode::invalidArgument});
}

TEST_CASE("group: the last target wins; a refused command changes no target")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    for (Stepper* m : {&x, &y}) m->setMaxSpeed(10'000).setAcceleration(20'000);
    StepperGroup xy{x, y};

    x.setTargetAbs(700); // targets on the members ...
    y.setTargetAbs(900);
    xy.moveAbs({100, 200}); // ... are replaced by the targets given to the group
    CHECK(x.getPosition() == 100);
    CHECK(y.getPosition() == 200);
    xy.move(); // the targets of the members are now those of the group: nothing to do
    CHECK(x.getPosition() == 100);

    x.setTargetAbs(700); // and the other way round: a target set afterwards replaces the one of the group
    xy.move();
    CHECK(x.getPosition() == 700);
    CHECK(y.getPosition() == 200);

    x.setTargetAbs(300); // a command that is refused leaves the targets as they were
    y.moveAbsAsync(50'000);
    host::run(0.5);
    CHECK(xy.moveAbsAsync({1, 2}) == errorCode::motorBusy);
    CHECK(xy.moveRelAsync({1, 2}) == errorCode::motorBusy);
    CHECK(xy.moveAbsAsync({1}) == errorCode::invalidArgument);
    y.stop();
    y.setTargetAbs(y.getPosition());
    xy.move();
    CHECK(x.getPosition() == 300);
}

TEST_CASE("group: a member that never got a target stays where it is")
{
    Board board;
    Stepper a(2, 3), b(4, 5);
    b.setPosition(123);
    a.setTargetAbs(50);
    StepperGroup{a, b}.move();
    CHECK(a.getPosition() == 50);
    CHECK(b.getPosition() == 123);
    CHECK(host::pulses(pinsY).count == 0);
}

TEST_CASE("group: a group is not moving while one of its members moves on its own")
{
    Board board;
    Stepper m(2, 3);
    StepperGroup g{m}; // a group of one
    m.rotateAsync(500);
    host::run(0.5);
    CHECK_FALSE(g.isMoving());
    CHECK(g.stopAsync() == errorCode::OK); // nothing to stop for the group
    host::run(0.5);
    CHECK(m.isMoving());
    CHECK(g.moveAbsAsync({100}) == errorCode::motorBusy);
    m.stop();
    CHECK(g.moveAbs({100}) == errorCode::OK);
    CHECK(m.getPosition() == 100);
}

TEST_CASE("group: state of the members during a move")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setMaxSpeed(20'000).setAcceleration(50'000);
    y.setMaxSpeed(20'000).setAcceleration(50'000);
    StepperGroup xy{x, y};
    xy.moveAbsAsync({100'000, -25'000});
    host::run(1.0);
    CHECK(xy.isMoving());
    CHECK(x.isMoving());
    CHECK(y.isMoving());
    CHECK(x.getSpeed() == approx(20'000).epsilon(0.0001));
    CHECK(y.getSpeed() == approx(-5'000).epsilon(0.0001));
    CHECK(std::abs(x.getPosition() - host::pulses(pinsX).net) <= 1);
    CHECK(std::abs(y.getPosition() - host::pulses(pinsY).net) <= 1);
    CHECK(host::openChannels() == 1); // one timer for the group
}

TEST_CASE("group: a busy member; a stop of a member stops its group")
{
    Board board;
    Stepper x(2, 3), y(4, 5), z(6, 7);
    for (Stepper* m : {&x, &y, &z}) m->setMaxSpeed(10'000).setAcceleration(20'000);
    StepperGroup xy{x, y}, yz{y, z};

    xy.moveAbsAsync({50'000, 20'000});
    host::run(1.0);
    CHECK(yz.moveAbsAsync({0, 3'000}) == errorCode::motorBusy); // y is moving in xy
    CHECK(y.moveAbsAsync(0) == errorCode::motorBusy);
    CHECK(y.rotateAsync(100) == errorCode::motorBusy);
    CHECK(reported == std::vector<errorCode>{errorCode::motorBusy, errorCode::motorBusy, errorCode::motorBusy});
    CHECK(xy.moveAbsAsync({0, 0}) == errorCode::notSupported); // new targets only when the group stands still
    CHECK_FALSE(z.isMoving());

    CHECK(y.stopAsync() == errorCode::OK);
    CHECK(host::runUntilIdle());
    CHECK_FALSE(xy.isMoving());
    CHECK(x.getPosition() < 50'000);
    host::Pulses px = host::pulses(pinsX), py = host::pulses(pinsY);
    CHECK(px.net == x.getPosition());
    CHECK(py.net == y.getPosition());
    CHECK(worstOffLine(px, py, 0.4) <= 0.5 + 1e-9); // stopped on the line

    CHECK(yz.moveAbs({0, 3'000}) == errorCode::OK); // free again
    CHECK(z.getPosition() == 3'000);
}

TEST_CASE("group: the limits of a member can change during a move")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setMaxSpeed(20'000).setAcceleration(50'000);
    y.setMaxSpeed(20'000).setAcceleration(50'000);
    StepperGroup xy{x, y};
    xy.moveAbsAsync({200'000, 100'000});
    host::run(1.0);
    CHECK(x.getSpeed() == approx(20'000).epsilon(0.0001));
    y.setMaxSpeed(2'500); // y makes half the steps of x: the group slows down to 5000 on x
    host::run(1.0);
    CHECK(x.getSpeed() == approx(5'000).epsilon(0.0001));
    CHECK(y.getSpeed() == approx(2'500).epsilon(0.0001));
    CHECK(host::runUntilIdle());
    CHECK(x.getPosition() == 200'000);
    CHECK(y.getPosition() == 100'000);
    CHECK(worstOffLine(host::pulses(pinsX), host::pulses(pinsY), 0.5) <= 0.5 + 1e-9);
}

TEST_CASE("group: emergency stop, the positions match the pulses made")
{
    for (double lookahead : {0.0, 0.004})
    {
        CAPTURE(lookahead);
        Board board(lookahead);
        Stepper x(2, 3), y(4, 5);
        x.setMaxSpeed(20'000).setAcceleration(50'000);
        y.setMaxSpeed(20'000).setAcceleration(50'000);
        StepperGroup xy{x, y};
        xy.moveAbsAsync({100'000, -33'333});
        host::run(1.2345);
        xy.emergencyStop();
        CHECK_FALSE(xy.isMoving());
        CHECK(x.getPosition() == host::pulses(pinsX).net);
        CHECK(y.getPosition() == host::pulses(pinsY).net);
        CHECK(host::openChannels() == 0);
        host::run(0.5);
        CHECK(x.getPosition() == host::pulses(pinsX).net);

        xy.moveAbs({0, 0});
        CHECK(host::pulses(pinsX).net == 0);
        CHECK(host::pulses(pinsY).net == 0);
    }
}

TEST_CASE("group: rotation with a fixed ratio, speed factor, stop and restart")
{
    Board board;
    Stepper a(2, 3), b(4, 5);
    a.setMaxSpeed(20'000).setAcceleration(20'000);
    b.setMaxSpeed(20'000).setAcceleration(60'000);
    StepperGroup g{a, b};

    CHECK(g.overrideSpeed(0.5f) == errorCode::notSupported); // not rotated yet
    reported.clear();

    CHECK(g.rotateAsync({5'000, -15'000}) == errorCode::OK);
    host::run(2.0);
    CHECK(a.getSpeed() == approx(5'000).epsilon(0.0001));
    CHECK(b.getSpeed() == approx(-15'000).epsilon(0.0001));

    CHECK(g.overrideSpeed(0.5f) == errorCode::OK);
    host::run(1.0);
    CHECK(b.getSpeed() == approx(-7'500).epsilon(0.0001));

    CHECK(g.overrideSpeed(-1.0f) == errorCode::OK); // all reverse
    host::run(2.0);
    CHECK(a.getSpeed() == approx(-5'000).epsilon(0.0001));
    CHECK(b.getSpeed() == approx(15'000).epsilon(0.0001));

    CHECK(g.rotateAsync({5'000, 14'000}) == errorCode::notSupported); // another ratio
    reported.clear();

    CHECK(g.overrideSpeed(0) == errorCode::OK); // stop
    CHECK(host::runUntilIdle());
    CHECK_FALSE(g.isMoving());
    host::Pulses pa = host::pulses(pinsX), pb = host::pulses(pinsY);
    CHECK(pa.net == a.getPosition());
    CHECK(pb.net == b.getPosition());
    CHECK(std::fabs(pb.net + 3.0 * pa.net) <= 2); // b made three times the way of a, backwards

    CHECK(g.overrideSpeed(0.2f) == errorCode::OK); // starts again with the same ratio
    host::run(1.0);
    CHECK(a.getSpeed() == approx(1'000).epsilon(0.0001));
    CHECK(b.getSpeed() == approx(-3'000).epsilon(0.0001));
    CHECK(g.stop() == errorCode::OK);
    CHECK(reported.empty());
}

TEST_CASE("group: wrong use is reported")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    StepperGroup xy{x, y}, none;
    CHECK(xy.moveAbsAsync({1, 2, 3}) == errorCode::invalidArgument);
    CHECK(xy.rotateAsync({100}) == errorCode::invalidArgument);
    CHECK(none.moveAsync() == errorCode::invalidArgument);
    CHECK(xy.add(x) == errorCode::invalidArgument); // already a member
    CHECK(xy.size() == 2);
    CHECK(reported.size() == 4);
    CHECK(host::openChannels() == 0);

    xy.remove(x);
    CHECK(xy.size() == 1);
    CHECK(xy.moveAbs({77}) == errorCode::OK);
    CHECK(y.getPosition() == 77);
}

namespace
{
    int groupArrivals = 0;
    void countArrival() { groupArrivals++; }
}

TEST_CASE("group: the arrival function is called when the targets are reached")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    StepperGroup xy{x, y};
    groupArrivals = 0;
    xy.onArrival(countArrival);
    xy.moveAbs({500, 200});
    CHECK(groupArrivals == 1);
    xy.moveAbs({500, 200}); // already there
    CHECK(groupArrivals == 2);
    xy.moveAbsAsync({0, 0});
    host::run(0.01);
    xy.stop(); // stopped on the way: the targets were not reached
    CHECK(groupArrivals == 2);
}

// ------------------------------------------------------------------------------------------- enable pin

TEST_CASE("enable pin: the driver is switched on before a move and off after it")
{
    Board board;
    Stepper motor(2, 3);
    motor.setMaxSpeed(5'000).setAcceleration(50'000).setPullInSpeed(2'000);
    motor.setEnablePin(8, 0, 50, 500); // active low, on 50 ms before the first step, off 500 ms after the last
    CHECK(host::level(8) == true);     // off
    host::run(0.1);

    double start = host::now();
    CHECK(motor.moveRel(2'000) == errorCode::OK); // blocking
    double returned  = host::now();
    host::Pulses p   = host::pulses(pinsX);
    CHECK(p.count == 2'000);
    CHECK(p.first - start >= 0.050 - 1e-6);       // the driver had its 50 ms
    CHECK(p.first - start < 0.052);
    CHECK(returned - p.last < 0.002);             // the command returns when the motor stands still, not 500 ms later
    CHECK_FALSE(motor.isMoving());
    CHECK(host::level(8) == false);               // still on

    host::run(0.499 - (host::now() - p.last));
    CHECK(host::level(8) == false);
    host::run(0.003);
    CHECK(host::level(8) == true); // off, 500 ms after the last step
    CHECK(host::openChannels() == 0);

    std::vector<double> changes = changesOf(8, start);
    REQUIRE(changes.size() == 2);
    CHECK(changes[0] == approx(start).epsilon(1e-6));
    CHECK(changes[1] - p.last == approx(0.5).epsilon(0.005));
}

TEST_CASE("enable pin: a new move within the off-delay starts at once and keeps the driver on")
{
    Board board;
    Stepper motor(2, 3);
    motor.setMaxSpeed(5'000).setAcceleration(50'000).setPullInSpeed(1'000);
    motor.setEnablePin(8, 1, 50, 500); // active high this time
    CHECK(host::level(8) == false);

    motor.moveRel(1'000);
    host::run(0.2); // within the 500 ms
    double second = host::now();
    motor.moveRel(-1'000);
    host::Pulses p = host::pulses(pinsX, second);
    CHECK(p.count == 1'000);
    CHECK(p.first - second < 0.002); // no on-delay: the driver was still on
    CHECK(motor.getPosition() == 0);

    host::run(1.0);
    CHECK(changesOf(8, 0).size() == 2); // on once, off once
    CHECK(host::level(8) == false);
}

TEST_CASE("enable pin: two motors on one enable signal")
{
    Board board;
    Stepper a(2, 3), b(4, 5);
    a.setEnablePin(8, 0, 10, 100);
    b.setEnablePin(8, 0, 10, 100);
    host::run(0.1);
    double start = host::now();
    a.moveRelAsync(100);  // about 0.3 s
    b.moveRelAsync(1'000); // about 1.4 s
    host::run(0.6);        // a is done and its off-delay has passed
    CHECK_FALSE(a.isMoving());
    CHECK(b.isMoving());
    CHECK(host::level(8) == false); // still on for b
    CHECK(host::runUntilIdle());
    CHECK(host::level(8) == true);
    CHECK(changesOf(8, start).size() == 2); // on with the first motor, off after the last
}

TEST_CASE("enable pin: after an emergency stop the driver stays on for the off-delay")
{
    Board board;
    Stepper motor(2, 3);
    motor.setMaxSpeed(5'000).setAcceleration(50'000);
    motor.setEnablePin(8, 0, 0, 300);
    motor.rotateAsync(5'000);
    host::run(0.5);
    motor.emergencyStop();
    double stopped = host::now();
    CHECK_FALSE(motor.isMoving());
    CHECK(motor.getPosition() == host::pulses(pinsX).net);
    CHECK(host::level(8) == false);
    host::run(0.29);
    CHECK(host::level(8) == false);
    host::run(0.02);
    CHECK(host::level(8) == true);
    CHECK(host::now() - stopped < 0.32);
    CHECK(host::pulses(pinsX).net == motor.getPosition());
    CHECK(host::openChannels() == 0);
}

TEST_CASE("enable pin: a group switches the drivers of all members")
{
    Board board;
    Stepper x(2, 3), y(4, 5);
    x.setEnablePin(8, 0, 20, 100);
    y.setEnablePin(9, 0, 60, 300); // the longer delays apply to the group
    StepperGroup xy{x, y};
    double start = host::now();
    xy.moveAbs({400, 100});
    host::Pulses p = host::pulses(pinsX);
    CHECK(p.first - start >= 0.060 - 1e-6);
    CHECK(host::level(8) == false);
    CHECK(host::level(9) == false);
    host::run(0.35);
    CHECK(host::level(8) == true);
    CHECK(host::level(9) == true);
    CHECK(x.getPosition() == 400);
    CHECK(y.getPosition() == 100);
}

TEST_CASE("enable pin: without one, the library touches only the STEP and DIR pins")
{
    Board board;
    Stepper motor(2, 3);
    motor.moveRel(100);
    for (const host::Edge& e : host::edges()) CHECK((e.pin == 2 || e.pin == 3));
}
