# Targets with a speed

A move command normally ends with the motor standing at its target. The optional second parameter of the
move commands is the speed the motor shall have **at** the target:

```cpp
motor.moveAbsAsync(10'000);        // stop at 10000
motor.moveAbsAsync(10'000, 2'000); // pass 10000 at 2000 steps/s and run on at that speed
```

The motor reaches the position exactly at that speed and then keeps it, like after `rotateAsync()`, until
the next command. The sign of the speed is the direction in which the position is passed.

If the target is too close to reach the speed, or lies behind the motor, the motor takes a run-up: it
first moves away from the target, reverses, and then passes the target at the required speed. This can
happen when a target is given or changed while the motor runs, and when a motor that stands still is
given a target that is too close for the speed. A target far enough ahead needs no run-up.

## The arrival function

`onArrival()` registers a function of the sketch that is called when the motor reaches a target. The
function can give the next command, which then follows without a stop. In this way a motor runs through a
series of positions, each with its own speed:

```cpp
--8<-- "06_TargetWithSpeed/06_TargetWithSpeed.ino"
```

The motor passes position 10,000 at 2000 steps/s, accelerates, passes 40,000 at 12,000 steps/s and stops
at 50,000.

!!! warning "The arrival function is called from an interrupt"
    Keep it short, and do not use `Serial`, `delay()` or blocking move commands in it (1). The `Async`
    commands, the settings and the getters of the library may be used. Variables that the function shares
    with `loop()` have to be declared `volatile`.
    { .annotate }

    1.  **Interrupt context**

        While the arrival function runs, the step pulses of the motors that use the same timer wait. A
        function that takes long delays these steps.

The arrival function is also called when a move without a speed ends at its target. It is not called
after `stop()` or `emergencyStop()`. `onArrival(nullptr)` removes the function.

## Blocking moves with a speed

`moveAbs(pos, speed)` and `moveRel(delta, speed)` return when the motor has reached the target. The motor
is then still running at `speed`:

```cpp
motor.moveAbs(10'000, 2'000); // returns when the motor passes 10000
motor.moveAbs(50'000);        // goes on from there and stops at 50000
```

## Following a calculated path

A target with a speed is also the way to let a motor follow a path that is calculated while it moves:
a timer calculates at short intervals where the motor has to be next and how fast it moves there. Because
the motor arrives at each point with the right speed, it runs through the points smoothly. In the
following sketch a motor swings back and forth like a pendulum:

```cpp
--8<-- "13_Follower/13_Follower.ino"
```

The timer runs every 10 ms and gives the motor the position and the speed of the curve for the time of
its next call. The sketch first moves the motor to the start of the curve, where the speed is 0, so that
the motor does not have to catch up with a curve that is already moving.

The motor reaches every point at the right speed, but the library does not control when it gets there:
it is on its way with the acceleration it is allowed. Set the acceleration of the motor well above what
the curve needs (here about 50,000 steps/s²); the motor then stays close to the curve, in this sketch
within about 70 of the 5000 steps.

The example `15_RoseFunction` uses the same method for a slide that follows the angle of a rotating
spindle and draws a rose curve.
