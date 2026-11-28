# Errors

No command of the library fails silently. The sketch starts a group while one of its motors is still
moving in another group; the command is refused, and the sketch shows two ways to notice it.

**What you learn**

- Every command returns an `errorCode`: `OK`, a warning (the command was executed with a restriction) or
  an error (the command was not executed, and the motor is unchanged).
- `attachErrFunc()` sets a function that is called for every warning and every error.
- The ready-made handler for development: `attachErrFunc(ErrorHandler(Serial));` prints warnings and
  errors, and at an error it stops all motors and halts the sketch with a blinking LED.

**Boards:** Teensy 4.0, 4.1, MicroMod, 3.5 and 3.6. **Hardware:** none needed; the output goes to the serial monitor. With three stepper
drivers (pins 2/3, 4/5, 6/7) the motors move.

**Output:**

```
TeensyStep reported 1001
yz could not start, waiting for xy
```

**Manual:** [Error handling](https://luni64.github.io/TeensyStep/error-handling/)
