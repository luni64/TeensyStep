# Follower

A motor swings back and forth like a pendulum: its position follows a cosine with an amplitude of 5000
steps and a period of 2 s. A timer calculates every 10 ms where the motor has to be at the next call and
how fast it moves there, and gives both to the motor.

**What you learn**

- A target with a speed lets a motor follow a path that is calculated while it moves: it runs through the
  points without stopping at them.
- The commands of the library can be called from a timer interrupt. The timer gets a low priority, so
  that it does not delay the step pulses.
- The acceleration of the motor has to be well above what the path needs.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** a stepper driver with STEP on pin 2 and DIR on pin 3.

**What you see:** the motor swings smoothly between +5000 and -5000 steps.

**Manual:** [Targets with a speed – Following a calculated path](https://luni64.github.io/TeensyStep/targets-with-speed/#following-a-calculated-path)
