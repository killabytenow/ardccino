ardccino
========

This is a dual PWM/DCC controller software for model trains.

WARNING: THIS SOFTWARE IS UNDER DEVELOPMENT. THIS DOCUMENT NOW EXPRESSES WHAT I
WANT TO IMPLEMENT, NOT WHAT IT CAN DO.

This Arduino software can control and generate signal for several boosters
attached to it. This program is designed for a LMD18200T based booster,
generating some digital signals:

  - direction
  - motor driving signal
  - reset heat & OCP (over current protection)

It can read also two input signals:

  - overheat detected
  - over current detected (i.e. a shortcircuit)

In an ideal design a booster becomes automatically deactivated if one of these
events (overheat or O.C.) occurs, until it is reactivated by this software.
Anyway, if one of these events is triggered this software deactivates the
affected booster and it stops delivering control signals until its status is
reseted.

It has two working modes:

  - PWM
      Each booster generates a PWM signal for controlling a segment of a model
      train circuit. All PWM signals are synchronized to avoid "glitches" when
      a train travels from one PWM segment to another.

      In this mode 'direction' output depends on the direction selected for
      each segment, and the 'motor driving signal' is a PWM signal.

  - DCC
      The same DCC signal is delivered to all boosters.

      In this mode the 'motor driving signal' is always 1, and 'direction'
      output is modulated for generating an square alternate current on tracks
      (which encodes the DCC commands).

      It has two submodes:

        - stateless - passthrough DCC commands. The DCC decoders state is never
          kept.

        - stateful - DCC decoders state is kept.

CLI COMMANDS
============

GENERIC COMMANDS
----------------

about
  Shows version, copyright and contact info

menu
  Launches the serial menu driven interface

booster list
  Prints booster info

booster <n> power (off|on)
  Activates/deactivates booster #n

booster <n> status
  Print info about booster #n

OFF COMMANDS
------------

off
  Go to off mode. No powered delivered to tracks.

PWM COMMANDS
------------

pwm
  Go to PWM mode for analog locomotives.

booster <n> power [+-][<v>]
  Set booster #n power level. Value 'v' is a signed integer between "minimum
  power" (see below) and 255. Sign determines direction.

booster <n> mode (direct|inertial)
  Enable or deactivate the 'inertial' feature when changing the booster power
  level. In direct mode any change to the power level is applied inmediately.
  In intertial mode the power level is changed applying the 'acceleration' and
  'maximum acceleration' parameters.

booster <n> minimum power [<v>]
  Sets the minimum power delivered by the PWM signal to tracks.

booster <n> acceleration [<a>]
  Sets the acceleration parameter when running in 'inertial' mode.

booster <n> maximum acceleration [<a>]
  Sets the maximum acceleration parameter when running in 'inertial' mode.

DCC COMMANDS
------------

dcc
  Go to DCC mode for digital locomotives.

dcc mode (stateless|stateful)
  Set DCC control mode:
    - stateless - passthrough DCC commands. The DCC decoders state is never
      kept.
    - stateful - DCC decoders state is kept.

dcc <n> speed [4bit|5bit|7bit] [+-]<v> [acked]
  Send speed update to decode <n>. In stateless mode CV#29:5 is assumed to be
  on, and the default selected instruction is 5bit. In stateful mode, selected
  instruction (4bit, 5bit or 7bit) depends on the state configured for deco
  <n>.

dcc <n> f <f> (on|off) [acked]
  Set flag <f> (on|off)

dcc <n> analog <f> <v> [acked]

dcc <n> address (advanced|normal) [acked]
dcc <n> ack (service|railcom|off)
dcc <n> send [service] command <bytes...> [acked]
dcc <n> read <v> [mode (paged|direct)]
