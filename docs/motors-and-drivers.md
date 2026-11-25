# Motors and drivers

This chapter collects what is needed around the library: which motors and drivers fit, how they are
connected to a Teensy, and how to choose a motor. It is independent of the functions of the library.

## What the library needs

TeensyStep produces two logic signals per motor:

| Signal | |
|---|---|
| STEP | one pulse per step (or microstep) |
| DIR | the direction: one level while the position counts up, the other while it counts down |

and optionally a third one, ENABLE, which switches the motor current on and off
([enable pin](motor-settings.md#the-enable-pin)). Everything else is the job of a stepper driver: it
turns the pulses into currents through the motor coils.

Drivers with a STEP and DIR interface are available for motors of all sizes, from small modules for 1 A
to industrial drivers for several amperes. Practically all of them are constant current drivers (1) for
bipolar motors, and they can divide the steps of the motor into microsteps.
{ .annotate }

1.  **Constant current driver**

    A driver that measures the current through each motor coil and switches the supply voltage on and off
    many thousand times per second, so that the current keeps the value that was set (a *chopper*
    driver). The supply voltage can therefore be much higher than the rated voltage of the motor, which
    lets the current rise quickly and gives the motor torque at high speeds.

## Drivers

Typical drivers, with values from their data sheets. The data sheet of the driver in use decides.

| Driver | Supply | Current per coil | Microsteps | Inputs | Shortest step pulse |
|---|---|---|---|---|---|
| [A4988](https://www.pololu.com/product/1182) module | 8 to 35 V | about 1 A, 2 A with cooling | up to 16 | 3.3 V and 5 V | 1 µs |
| [DRV8825](https://www.pololu.com/product/2133) module | 8.2 to 45 V | about 1.5 A, 2.2 A with cooling | up to 32 | 3.3 V and 5 V | 1.9 µs |
| TMC2209 module | 4.75 to 29 V | up to 2 A | up to 64, interpolated to 256 | 3.3 V and 5 V | 0.1 µs |
| Industrial drivers in a housing (for example DM542, DM556, TB6600 types) | 20 to 50 V and more | 1 to 6 A | up to 128 and more, set with switches | opto-couplers, made for 5 V | 2.5 to 10 µs |

Suppliers such as Pololu list many more [driver modules](https://www.pololu.com/category/120/stepper-motor-drivers)
with their data.

### Connecting a driver module

The small modules (A4988, DRV8825, TMC2209 and similar) share the same pin layout. With 3.3 V compatible
inputs they are connected directly to the Teensy:

| Driver | Connect to |
|---|---|
| STEP | the STEP pin of the motor in the sketch, for example pin 2 for `Stepper motor(2, 3)` |
| DIR | the DIR pin, pin 3 in this example |
| GND (logic) | GND of the Teensy |
| VDD or VIO (logic supply, if the module has it) | 3.3 V of the Teensy |
| EN or ENABLE | leave open (the driver is then on), or connect it to a pin for [`setEnable()`](motor-settings.md#the-enable-pin) |
| MS1, MS2, ... (microsteps) | to GND or to the logic supply, according to the table in the data sheet |
| RESET and SLEEP (A4988, DRV8825) | A4988: connect the two to each other. DRV8825: connect both to 3.3 V |
| VMOT and GND (motor supply) | the motor supply, with a capacitor of 100 µF or more close to the module |
| 1A, 1B, 2A, 2B (or A1, A2, B1, B2) | the two coils of the motor, one coil per pin pair |

Before the first move:

- **Set the current** of the driver to the rated current of the motor or below, with the potentiometer of
  the module as described in its data sheet. A current that is too high overheats motor and driver.
- **Never connect or disconnect a motor while the driver is powered.** The voltage peak can destroy the
  driver.

### Connecting an industrial driver

These drivers have opto-coupler inputs with two terminals per signal, named PUL+ / PUL− (step),
DIR+ / DIR− and ENA+ / ENA−. Connect PUL−, DIR− and ENA− to GND and drive PUL+ and DIR+ with the
signals. The inputs are made for 5 V. Some drivers also work with the 3.3 V of a Teensy 4, but most data
sheets do not promise it: use a level shifter, as shown in the next section. ENA can stay unconnected;
the driver is then on.

Opto-couplers are slow. Set the step pulse to the width the data sheet asks for, often 5 to 10 µs
([hardware settings](motor-settings.md#hardware-settings)); the width of the pulse limits the step rate.

## Logic levels

A Teensy 4 has 3.3 V outputs, and its pins do not tolerate 5 V.

- Drivers with 3.3 V compatible inputs can be connected directly: STEP, DIR and GND.
- Drivers with 5 V inputs or opto-coupler inputs need a level shifter. A 74HCT125 contains four buffers
  that accept 3.3 V input signals and produce 5 V output signals, enough for the STEP and DIR signals of
  two motors:

![Level shifter with a 74HCT125 for two drivers](images/74hct125.png){ width="55%" }

Supply the 74HCT125 from 5 V (pin 14), and connect its enable inputs (pins 1, 4, 10, 13) to GND. A
74HCT245 contains eight buffers and does the same for four motors.

Check the data sheet of the driver for the minimum width of the step pulse and for the time it needs
between a change of DIR and the next step, and set both in the sketch
([hardware settings](motor-settings.md#hardware-settings)). Opto-coupler inputs are slow and often need
pulses of 10 µs or more.

## Motor types

![Wiring of stepper motors with 4, 5, 6 and 8 wires](images/wiring.png)

- **Bipolar motors** have two coils and four wires (a). The current through the coils is reversed from
  step to step. Most modern steppers are of this type, and the usual drivers are made for them.
- **Unipolar motors** have a centre tap on both coils. With five wires (b) they can only be driven by
  a unipolar driver, for example four transistors or a ULN2003. The small geared 28BYJ-48 is a popular
  motor of this kind. Such drivers have no STEP and DIR inputs, so these motors cannot be used with the
  library.
- Motors with **six or eight wires** (c, d) can be driven by a bipolar driver. Leave the centre taps of a
  six wire motor unconnected. The coil halves of an eight wire motor can be connected in series (more
  torque at low speed) or in parallel (more torque at high speed):

![Series and parallel connection of the coil halves](images/configurations.png)

### Motors for constant current and motors for constant voltage

Bipolar motors are made in two kinds, which look alike:

| | Made for a constant current driver | Made for a constant voltage |
|---|---|---|
| Rated voltage | low: 2 to 5 V | high: often 5 V, 12 V or 24 V |
| Rated current | 1 A and more | low: 0.1 to 0.5 A |
| Resistance of a coil | about 1 to 5 Ω | 20 Ω and more |
| Inductance of a coil | about 1 to 5 mH | 20 to 50 mH and more |
| Example | [NEMA 17, 2.8 V, 1.7 A, 1.65 Ω](https://www.pololu.com/product/2267) | [NEMA 14, 10 V, 0.5 A, 20 Ω](https://www.pololu.com/product/1208) |

A motor of the second kind is made to be connected to its rated voltage through simple switches, without
a control of the current: its high resistance limits the current. Its high inductance lets the current
rise slowly, so that the torque falls off already at a few hundred steps per second.

**For the drivers described here, choose a motor of the first kind.** A motor with a high rated voltage
is not damaged by a constant current driver that is set to its rated current, but it stays slow, and
the driver gains nothing from a supply voltage that is not well above the rated voltage of the motor.

## Choosing a motor

Motors are usually named by the size of their flange according to the NEMA standard: the number is the
flange size in tenths of an inch.

| NEMA | Flange | Shaft | Typical holding torque |
|:---:|:---:|:---:|:---:|
| 8 | 20 mm | 4 mm | 0.02 to 0.04 Nm |
| 11 | 28 mm | 5 mm | 0.03 to 0.14 Nm |
| 14 | 35 mm | 5 mm | 0.05 to 0.4 Nm |
| 17 | 42 mm | 5 mm | 0.15 to 1.0 Nm |
| 23 | 56.4 mm | 6.35 or 8 mm | 0.4 to 3.4 Nm |
| 34 | 86 mm | 14 mm | 2.5 to 14 Nm |

The data sheet of a motor states, among others:

| Parameter | |
|---|---|
| Rated current | the current per coil for which the motor is made; set the driver to it |
| Resistance, inductance | of one coil. The inductance limits how fast the current can change, and with it the speed. Typical values of motors for constant current drivers: 1 to 5 Ω and 1 to 5 mH; see [the two kinds of motors](#motors-for-constant-current-and-motors-for-constant-voltage) |
| Rated voltage | resistance times rated current: the voltage for the rated current at standstill. A constant current driver is supplied with a much higher voltage |
| Holding torque | the torque of the motor at standstill with the rated current |
| Detent torque | the torque without current; it can be felt when turning the shaft by hand. Seldom stated |

How to choose:

- **Torque.** Estimate the torque the application needs. A motor with 0.1 Nm lifts a mass of 1 kg on a
  lever of 1 cm, or 0.1 kg on a lever of 10 cm. The torque of a stepper falls with the speed, so leave a
  wide margin.
- **Speed.** For high speeds choose a motor with a low inductance. Such a motor needs a higher current
  for the same torque, so the driver has to be able to deliver it. A higher supply voltage of the driver
  also raises the speed the motor can reach.
- **Microstepping.** More microsteps give a smoother run and a finer resolution, but they multiply the
  step rate: see [Step rates](index.md#speed-and-acceleration).

## Finding speed and acceleration

The values for `setMaxSpeed()` and `setAcceleration()` depend on the motor, its load, the driver and its
supply voltage. They are found by trial:

1. Start with a low acceleration and raise the maximum speed until the motor stalls or loses steps. Use
   about two thirds of that speed.
2. Then raise the acceleration in the same way.
3. Test with the real load, and in both directions if the load is not the same in both, as with a
   vertical axis.

A motor that loses steps is not damaged, but the position the library counts is then wrong until the
machine has found its reference position again ([homing](moving-a-motor.md#homing)).
