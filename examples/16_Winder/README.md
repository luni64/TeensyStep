# Winder

A string or coil winder: the spindle rotates the core, the feeder moves the wire along it. The pitch (mm
of feeder travel for each turn of the spindle) is the ratio of the two speeds. It has to stay constant
while the spindle speeds up or slows down, and it shall be trimmable while winding. (In version 2 this was
the example Winder.)

A group keeps a ratio exactly but cannot change it while it runs. The two motors therefore run
independently:

- **The spindle speed changes:** both motors get their new speed at the same moment, and the acceleration
  of the feeder is set to that of the spindle times the ratio. Both ramps then take the same time, and
  the pitch stays constant during the ramp.
- **The pitch changes:** only the feeder gets a new speed, with an acceleration of its own.

**What you learn**

- How two independent motors keep a ratio through a ramp: accelerations in the ratio of the speeds.
- That speed and acceleration of a rotating motor can be changed at any time.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** two stepper drivers: the spindle with STEP on pin 0 and DIR on pin 1,
the feeder with STEP on pin 2 and DIR on pin 3.

**What you see:** the sketch winds at 1125 rpm with a pitch of 0.10 mm, trims the pitch three times
(0.12, 0.08, 0.15 mm), slows down to 300 rpm and stops. It prints the time, both speeds and the actual
pitch every 25 ms, for a diagram.

**Manual:** [Examples – Coil winder](https://luni64.github.io/TeensyStep/examples/#coil-winder-16_winder)
