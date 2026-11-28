# RoseFunction

A spindle rotates at constant speed. A slide moves in and out depending on the angle of the spindle:
r = amplitude · cos(k · angle). A pen on the slide draws a rose curve on a disk that turns with the
spindle. (In version 2 this was the example RoseFunctionFollower.)

Every 25 ms a timer reads the angle of the spindle, calculates where the slide has to be 25 ms later and
how fast it moves there, and gives both to the slide: `slide.moveAbsAsync(position, speed);`

**What you learn**

- Because the target includes the speed, the slide does not stop at each point but runs through the
  points smoothly.
- The speed is given without a sign: the slide passes each point in the direction in which it comes to it.
- The commands of the library can be called from a timer interrupt. The timer gets the lowest priority,
  so that it does not delay the step pulses.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** two stepper drivers: the spindle with STEP on pin 1 and DIR on pin 2,
the slide with STEP on pin 3 and DIR on pin 4.

**Output:** every 20 ms a line with the angle of the spindle in rad and the position of the slide in
steps, separated by a tab, for a plot.

**Manual:** [Examples – Rose curve](https://luni64.github.io/TeensyStep/examples/#rose-curve-15_rosefunction),
[Targets with a speed – Following a calculated path](https://luni64.github.io/TeensyStep/targets-with-speed/#following-a-calculated-path)
