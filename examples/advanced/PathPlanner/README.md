# PathPlanner

An example of the advanced functions (`TeensyStepAdvanced.h`). The path is a list of points in mm: a
rectangle and a circle of 72 short segments. The planner in `Planner.h`, a part of this example, decides
how fast each point can be passed and gives the library one move for each segment.

**What you learn**

- How a motion planner is built on `advanced::GroupPath`: the machine (motors, limits, steps per mm), a
  path of points with a feed rate, `plan()`, and `giveMoves()` in `loop()`.
- What sets the speed at a point: `maxJump`, the change of speed a motor takes without a ramp, and the
  angle between the two segments. A higher `maxJump`, or a circle of more segments, is run faster.
- `Planner.h` is example code of about 300 lines, made to be read, copied and changed.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** two stepper drivers,
STEP and DIR on pins 2/3 and 4/5; 100 steps per mm.

**What you see:** with the values of the sketch and a feed rate of 80 mm/s, the corners of the rectangle
are passed at 5 mm/s and the circle is run at 55 mm/s and more.

The example CoreXY runs the same planner on a CoreXY machine.

**Manual:** [Writing a motion planner](https://luni64.github.io/TeensyStep/advanced/motion-planner/)
