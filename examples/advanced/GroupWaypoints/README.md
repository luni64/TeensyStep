# GroupWaypoints

An example of the advanced functions (`TeensyStepAdvanced.h`). A group always stops at its targets.
`advanced::GroupPath` takes moves ahead: one runs, one waits, and at the targets of a move the group goes
on with the move that waits, without a stop. The sketch runs an x-y table through four waypoints.

**What you learn**

- How moves are given ahead: `path.add(targets, move)` as long as `path.room()` says there is room.
- Each move says how long it is in the unit of the sketch (here: mm). Its speeds are then in that unit
  per second, whichever motor has the longest way. The targets are positions of the motors in steps.
- Within each move the motors stay on a straight line. At a waypoint the direction of the line changes,
  so the speeds of the single motors jump there: the speed at the waypoints has to be low enough for the
  motors to follow.
- The library does not plan ahead across the moves, and it does not round corners. A corner is cut into
  short moves, as in the path of the sketch.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** two stepper drivers, STEP and DIR on pins 2/3 and 4/5; 100 steps per mm.

**What you see:** the table runs to (100, 0), cuts the corner to (104, 4), runs to (104, 80) and back to
the start, at 80 mm/s with 5 mm/s at the waypoints.

The example PathPlanner calculates the speeds at the points from the limits of the motors instead of
taking them from a table.

**Manual:** [Paths of a group](https://luni64.github.io/TeensyStep/advanced/group-paths/)
