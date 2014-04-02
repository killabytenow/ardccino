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


BUILDING
========

You will need 'arduino-mk' package (it includes the Arduino.mk makefile) and
the 'arduino-core' package (command line arduino tools).

You need also the UTFT library. You can download it from
  http://henningkarlsen.com/electronics/library.php?id=52

You must unzip it into the ./libraries/utft directory.

Edit the included 'Makefile' and adjust the following parameters to your
requirements:

  * `ARDUINO_DIR`
      Things which are always in the Arduino distribution e.g. boards.txt,
      libraries, &c.
      Default: /usr/share/arduino/
  * `ARDMK_DIR`
      Things which are included in this distribution e.g. ard-parse-boards
      Default: /usr/
  * `AVR_TOOLS_DIR`
      Things which might be bundled with the Arduino distribution, but might
      come from the system. Most of the toolchain is like this: on Linux it's
      supplied by the system.
      Default: /usr/
  * `BOARD_TAG`
      Your Arduino board. Mine is 'mega2560'. See 'boards.txt' in your Arduino
      distribution and choose the most appropiate for you.
  * `MCU, F_CPU`
      If you use a BOARD_TAG from 'boards.txt' you don't need to set these.
  * `ARDUINO_PORT`
      Where you arduino is plug ... I use here an automatic one-liner for
      discovering it.

Now run:
    $ ./parse_tokens.sh
    $ make


CLI COMMANDS
============

GENERIC COMMANDS
----------------

`about`
  Shows version, copyright and contact info

`booster list`
  Prints booster info

`booster <n> power (off|on)`
  Activates/deactivates booster #n

`booster <n> reset`
  Resets booster #n state

`booster <n> status`
  Print info about booster #n

OFF COMMANDS
------------

`off`
  Go to off mode. No powered delivered to tracks.

PWM COMMANDS
------------

`pwm`
  Go to PWM mode for analog locomotives.

`booster <n> power <v>`
  Set booster #n power level. Value 'v' is a signed integer between "minimum
  power" (see below) and 255. Sign determines direction.

`booster <n> mode (direct|inertial)`
  Enable or deactivate the 'inertial' feature when changing the booster power
  level. In direct mode any change to the power level is applied inmediately.
  In intertial mode the power level is changed applying the 'acceleration' and
  'maximum acceleration' parameters.

`booster <n> minimum power [<v>]`
  Sets the minimum power delivered by the PWM signal to tracks.

`booster <n> acceleration [<a>]`
  Sets the acceleration parameter when running in 'inertial' mode.

`booster <n> maximum acceleration [<a>]`
  Sets the maximum acceleration parameter when running in 'inertial' mode.

DCC COMMANDS
------------

### GENERAL COMMANDS

`dcc`
  Go to DCC mode for digital locomotives.

`dcc address (normal|advanced|auto)`
  Use by default
    - normal addresses (7bit/2-digit), or
    - advanced addresses (14bit/4-digit), or
    - auto mode (for any address above 127 use advanced adresses, elsewhere use
      7bit addresses).

### DIRECTED COMMANDS

DCC directed commands result in a DCC message sent, so they are send against a
DCC decoder, a broadcast address or a set of DCC decoders.

General syntax is:

    [@!] (@|[+-]<address>) ...command...

Where:

  - First character (@ or !) decides if the message is sent to the operations
    track (@) or the service track (!).

  - Second component is the numerical target address. Use @ for broadcasting a
    message.

  - An address can be forced in 7bit (2-digit) or 14bit (4-digit) format using
    the minus (-) and plus (+) operators.

  - If the address type is not specified it will be choosed using the algorithm
    set by the `dcc address` configuration parameter.

We call the exposed syntax `<<trg_spec>>`, which stands for "target
specification". All the following described commands are preceeded by this
directives.

`<<trg_spec>> speed [4bit|5bit|7bit] [light (on|off)] [+-]<v>`
  Send speed update. In stateless mode CV#29:5 is assumed to be on, and the
  default selected instruction is 5bit. In stateful mode, selected instruction
  (4bit, 5bit or 7bit) depends on the state configured for deco <n>.

### TODO

`dcc mode (pass_through|stateful)`
  Set DCC control mode:
    - stateless - pass through DCC commands. The DCC decoders state is never
      kept.
    - stateful - DCC decoders state is kept.

`dcc [!] [<n>] f <f> (on|off) [acked]`
  Set flag <f> (on|off)

`dcc [!] [<n>] analog <f> <v> [acked]`

`dcc [!] [<n>] ack (service|railcom|off)`
`dcc [!] [<n>] send [service] command <bytes...> [acked]`
`dcc [!] [<n>] read <v> [mode (paged|direct)]`
