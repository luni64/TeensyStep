#pragma once

/*==========================================================================
 * Planner.h: a small motion planner for TeensyStep
 *
 * Example code, made to be read, copied and changed. It is not a part of
 * the library.
 *
 * A path is a list of points in mm with a feed rate. The planner turns it
 * into moves of the motors and decides how fast each point can be passed,
 * so that the path is run without a stop where the motors allow it. The
 * library runs the moves (advanced::GroupPath).
 *
 *   axes        the coordinates of the points (x, y, ...), in mm
 *   motors      what the library moves, in steps; as many as axes
 *   kinematics  how the positions of the motors follow from the axes
 *   segment     the straight piece of the path between two points
 *   junction    the point where two segments meet. The direction of the
 *               path changes there at once, and with it the speed of
 *               every motor. A junction is passed as fast as the motors
 *               can follow such a jump (MotorLimits::maxJump).
 *
 * Use:
 *   planner::Path path(buffer, size, kinematics, limits);
 *   path.start({0, 0});            the point where the motors stand
 *   path.add({50, 0}, 80);         a segment to this point at 80 mm/s
 *   ...
 *   path.plan();                   calculates the speeds
 *   runner.start(path);            planner::PathRunner runner(group);
 *   runner.giveMoves();            in loop(): hands the moves to the library
 *
 * Limits of this planner: straight segments only (cut an arc into short
 * ones), acceleration without a limit of the jerk, and the whole path is
 * planned before it starts.
 *==========================================================================*/

#include "TeensyStepAdvanced.h"
#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace planner
{
    constexpr unsigned maxAxes = 4;

    //----------------------------------------------------------------------------------------------------
    // Kinematics: steps of motor i = sum over the axes j of m[i][j] * mm of axis j. A machine whose motors
    // follow from the axes like this moves its motors on a straight line when the axes move on a straight
    // line, which is what a group of the library does.

    struct Kinematics
    {
        unsigned axes             = 0;
        float m[maxAxes][maxAxes] = {};

        /// One motor for each axis, with its steps per mm.
        static Kinematics plain(std::initializer_list<float> stepsPerMm)
        {
            Kinematics k;
            for (float s : stepsPerMm)
            {
                if (k.axes == maxAxes) break;
                k.m[k.axes][k.axes] = s;
                k.axes++;
            }
            return k;
        }

        /// CoreXY: motor A = x + y, motor B = x - y. A negative value reverses an axis.
        static Kinematics coreXY(float stepsPerMmX, float stepsPerMmY)
        {
            Kinematics k;
            k.axes    = 2;
            k.m[0][0] = stepsPerMmX; // A = x + y
            k.m[0][1] = stepsPerMmY;
            k.m[1][0] = stepsPerMmX; // B = x - y
            k.m[1][1] = -stepsPerMmY;
            return k;
        }

        void toMotors(const double* mm, double* steps) const
        {
            for (unsigned i = 0; i < axes; i++)
            {
                steps[i] = 0;
                for (unsigned j = 0; j < axes; j++) steps[i] += (double)m[i][j] * mm[j];
            }
        }
    };

    /// What a motor can do. Give the motors of the library the same maximum speed and acceleration.
    struct MotorLimits
    {
        float maxSpeed;     ///< steps/s
        float acceleration; ///< steps/s^2
        float maxJump;      ///< steps/s: the largest change of its speed without a ramp, about its pull-in speed
    };

    /// A segment, as planned. Speeds are along the path, in mm/s.
    struct Segment
    {
        int32_t target[maxAxes]; ///< positions of the motors at its end, steps
        int32_t delta[maxAxes];  ///< ways of the motors, steps
        unsigned lead;           ///< the motor with the longest way
        float length;            ///< mm
        float feed;              ///< the speed that was asked for
        float vMax;              ///< the speed it may have: the feed rate, or less if a motor would be too fast
        float acceleration;      ///< mm/s^2: as much as the motors allow in its direction
        float vIn, vOut;         ///< speed at its start and at its end
        float vPeak;             ///< the highest speed on its way

        /// Seconds the segment takes: a ramp from vIn to vPeak, a piece at vPeak, a ramp to vOut.
        float duration() const
        {
            double a = acceleration, up = (vPeak - vIn) / a, down = (vPeak - vOut) / a;
            double ramps = (2.0 * vPeak * vPeak - (double)vIn * vIn - (double)vOut * vOut) / (2 * a); // mm
            double rest  = length - ramps;
            return (float)(up + down + (rest > 0 ? rest / vPeak : 0));
        }

        /// Steps of the motor with the longest way per mm of the path.
        float leadStepsPerMm() const { return (float)(delta[lead] < 0 ? -delta[lead] : delta[lead]) / length; }
    };

    //----------------------------------------------------------------------------------------------------
    // The path: start(), add() for every point, plan(). Then the segments can be read and run.

    class Path
    {
     public:
        /// @param buffer room for the segments, `capacity` of them; it has to exist as long as the path
        /// @param limits one entry for each motor
        Path(Segment* buffer, unsigned capacity, const Kinematics& kinematics, const MotorLimits* limits)
            : seg(buffer), capacity(capacity), kin(kinematics)
        {
            for (unsigned i = 0; i < kin.axes; i++) lim[i] = limits[i];
        }

        /// Begins a new path at this point (mm). The motors stand still there.
        void start(std::initializer_list<float> mm) { start(mm.begin()); }
        void start(const float* mm)
        {
            n = 0;
            double steps[maxAxes];
            for (unsigned j = 0; j < kin.axes; j++) lastMm[j] = mm[j];
            kin.toMotors(lastMm, steps);
            for (unsigned i = 0; i < kin.axes; i++) lastSteps[i] = (int32_t)std::lround(steps[i]);
        }

        /// Adds a segment from the last point to this one, at this feed rate (mm/s). A point that is less
        /// than one step from the last one is left out (and returns true).
        /// @return false: the buffer is full, or the feed rate is not positive
        bool add(std::initializer_list<float> mm, float feed) { return add(mm.begin(), feed); }
        bool add(const float* mm, float feed);

        /// Calculates the speeds of all segments. The path ends at standstill.
        void plan();

        unsigned size() const { return n; }
        const Segment& operator[](unsigned i) const { return seg[i]; }

        float duration() const ///< seconds for the whole path, after plan()
        {
            double t = 0;
            for (unsigned k = 0; k < n; k++) t += seg[k].duration();
            return (float)t;
        }

     private:
        Segment* seg;
        unsigned capacity, n = 0;
        Kinematics kin;
        MotorLimits lim[maxAxes];
        double lastMm[maxAxes]     = {}; // the end of the last segment, as it was given
        int32_t lastSteps[maxAxes] = {}; // and where the motors are there
    };

    inline bool Path::add(const float* mm, float feed)
    {
        if (!(feed > 0)) return false;

        // The targets of the motors come from the point itself, not from the last targets plus a way, so
        // that rounding to steps does not add up along the path.
        double point[maxAxes] = {}, steps[maxAxes] = {};
        int32_t target[maxAxes];
        bool moves = false;
        for (unsigned j = 0; j < kin.axes; j++) point[j] = mm[j];
        kin.toMotors(point, steps);
        for (unsigned i = 0; i < kin.axes; i++)
        {
            target[i] = (int32_t)std::lround(steps[i]);
            if (target[i] != lastSteps[i]) moves = true;
        }
        if (!moves) return true; // less than a step: the next segment starts where this one would have
        if (n == capacity) return false;

        Segment& s    = seg[n];
        double square = 0;
        for (unsigned j = 0; j < kin.axes; j++) square += (point[j] - lastMm[j]) * (point[j] - lastMm[j]);
        double length = std::sqrt(square);

        // The speed of a motor is the speed along the path times the steps the motor makes per mm of the
        // path. From this: how fast the path may be, and how much it may accelerate, without a motor
        // exceeding its limits.
        double vMax = feed, acc = 1e30;
        int32_t longest = 0;
        s.lead          = 0;
        for (unsigned i = 0; i < kin.axes; i++)
        {
            s.target[i] = target[i];
            s.delta[i]  = target[i] - lastSteps[i];
            int32_t way = s.delta[i] < 0 ? -s.delta[i] : s.delta[i];
            if (way > longest)
            {
                longest = way;
                s.lead  = i;
            }
            if (way == 0) continue;
            double perMm = way / length;
            if (lim[i].maxSpeed / perMm < vMax) vMax = lim[i].maxSpeed / perMm;
            if (lim[i].acceleration / perMm < acc) acc = lim[i].acceleration / perMm;
        }
        s.length       = (float)length;
        s.feed         = feed;
        s.vMax         = (float)vMax;
        s.acceleration = (float)acc;
        s.vIn = s.vOut = s.vPeak = 0;

        for (unsigned j = 0; j < kin.axes; j++) lastMm[j] = point[j];
        for (unsigned i = 0; i < kin.axes; i++) lastSteps[i] = target[i];
        n++;
        return true;
    }

    inline void Path::plan()
    {
        if (n == 0) return;

        // 1. The highest speed at each junction: neither of the two segments is faster, and no motor
        //    changes its speed by more than its jump limit.
        for (unsigned k = 0; k + 1 < n; k++)
        {
            const Segment &a = seg[k], &b = seg[k + 1];
            double v = a.vMax < b.vMax ? a.vMax : b.vMax;
            for (unsigned i = 0; i < kin.axes; i++)
            {
                // the jump of motor i in steps/s, for each mm/s along the path
                double change = std::fabs(b.delta[i] / (double)b.length - a.delta[i] / (double)a.length);
                if (change * v > lim[i].maxJump) v = lim[i].maxJump / change;
            }
            seg[k].vOut = (float)v;
        }
        seg[n - 1].vOut = 0; // the path ends at standstill

        // 2. Backward, from the end: a junction is not faster than the segment behind it can brake from.
        for (unsigned k = n - 1; k > 0; k--)
        {
            const Segment& s = seg[k];
            double reachable = std::sqrt((double)s.vOut * s.vOut + 2.0 * s.acceleration * s.length);
            if (seg[k - 1].vOut > reachable) seg[k - 1].vOut = (float)reachable;
        }

        // 3. Forward, from the start: a junction is not faster than the segment before it can accelerate
        //    to. Then the highest speed on the way: that of the segment, or the top of a triangle if the
        //    segment is too short to reach it.
        for (unsigned k = 0; k < n; k++)
        {
            Segment& s       = seg[k];
            s.vIn            = k ? seg[k - 1].vOut : 0;
            double energy    = 2.0 * s.acceleration * s.length;
            double reachable = std::sqrt((double)s.vIn * s.vIn + energy);
            if (s.vOut > reachable) s.vOut = (float)reachable;
            double top = std::sqrt((energy + (double)s.vIn * s.vIn + (double)s.vOut * s.vOut) / 2);
            s.vPeak    = (float)(s.vMax < top ? s.vMax : top);
        }
    }

    //----------------------------------------------------------------------------------------------------
    // Runs a planned path with a group of the library: one move for each segment, given ahead while the
    // one before it runs. The motors of the group are the motors of the path, in the same order, and they
    // stand at the start of the path (Stepper::setPositionCounter()).

    class PathRunner
    {
     public:
        explicit PathRunner(TeensyStep::StepperGroup& group) : moves(group), group(group) {}

        /// Starts the path: giveMoves() hands it to the library from now on. The path has to exist until
        /// isRunning() returns false.
        void start(const Path& p)
        {
            path  = &p;
            given = 0;
            error = TeensyStep::errorCode::OK;
        }

        /// Gives the library as many moves as it takes. Call it from loop(), more often than the shortest
        /// segment takes: the next move has to be there before the one that runs arrives.
        void giveMoves()
        {
            while (hasMore() && moves.room() > 0) giveNext();
        }

        /// Gives the next move. Speeds and acceleration go as they were planned, in mm; the library
        /// converts them for the motors. @return the code of GroupPath::add()
        TeensyStep::errorCode giveNext()
        {
            const Segment& s = (*path)[given];
            TeensyStep::advanced::PathMove move;
            move.length        = s.length;
            move.speed         = s.vMax;
            move.speedAtTarget = s.vOut;
            move.acceleration  = s.acceleration;
            TeensyStep::errorCode code = moves.add(s.target, group.size(), move);
            if (TeensyStep::isError(code))
                error = code; // the path is given up; the moves that were given are still run
            else
                given = given + 1;
            return code;
        }

        bool hasMore() const { return path && given < path->size() && !TeensyStep::isError(error); } ///< moves are left to give
        bool isRunning() const { return hasMore() || group.isMoving(); }
        unsigned movesGiven() const { return given; }
        TeensyStep::errorCode lastError() const { return error; } ///< an error ended the path

        /// The moves in the library: overrideSpeed() runs the path slower (0: hold), getSpeed() returns the
        /// speed along the path in mm/s.
        TeensyStep::advanced::GroupPath moves;

     private:
        TeensyStep::StepperGroup& group;
        const Path* path        = nullptr;
        volatile unsigned given = 0;
        TeensyStep::errorCode error = TeensyStep::errorCode::OK;
    };
}
