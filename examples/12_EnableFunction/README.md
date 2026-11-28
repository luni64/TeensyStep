# EnableFunction

The enable input of the driver is not connected to a pin of the board but to a port extender. Instead of
a pin, the library gets a function that switches the driver on and off.

**What you learn**

- `setEnable(function, onDelayMs, offDelayMs)`: the library calls the function with `true` before a move
  and with `false` after it, with the same delays as for a pin.
- The function is called from an interrupt: it must not wait, and no motor gets a step while it runs
  (here about 0.3 ms for the I2C transfer).

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** a stepper driver with STEP on pin 2 and DIR on pin 3; an MCP23017 at
the I2C address 0x20 whose pin GPA0 is connected to the enable input of the driver (active low).

**What you see:** the driver is switched on 50 ms before the first move, stays on for the move back, and
is switched off 500 ms after it.

**Manual:** [Motor settings – An enable signal that is not on a pin](https://luni64.github.io/TeensyStep/motor-settings/#an-enable-signal-that-is-not-on-a-pin)
