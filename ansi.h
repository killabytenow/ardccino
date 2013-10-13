#ifndef __ANSI_H__
#define __ANSI_H__

#define ANSI_ESC "\x1B"
#define ANSI_CSI "\x1B["

// ED – Erase Display (cursor does not move)
//   Clears part of the screen. If n is zero (or missing), clear from cursor to
//   end of screen. If n is one, clear from cursor to beginning of the screen.
//   If n is two, clear entire screen (and moves cursor to upper left on DOS
//   ANSI.SYS).
#define ANSI_ED(x)	ANSI_CSI #x "J"

// EL  - Erase in Line (cursor does not move)
//   [K [0K Erase from current position to end (inclusive)
//   [1K    Erase from beginning to current position
//   [2K    Erase entire current line
//   [?0K   Selective erase to end of line ([?1K, [?2K similar)
#define ANSI_EL(x)	ANSI_CSI #x "K"

// SCP – Save Cursor Position
// RCP – Restore Cursor Position
//   Saves/Restores the cursor position.
#define ANSI_SCP	ANSI_CSI "s"
#define ANSI_RCP	ANSI_CSI "u"

// CUU – Cursor Up
// CUD – Cursor Down
// CUF – Cursor Forward
// CUB – Cursor Back
//   Moves the cursor n (default 1) cells in the given direction. If the cursor
//   is already at the edge of the screen, this has no effect.
#define ANSI_CUU(n)	ANSI_CSI #n "A"
#define ANSI_CUD(n)	ANSI_CSI #n "B"
#define ANSI_CUF(n)	ANSI_CSI #n "C"
#define ANSI_CUB(n)	ANSI_CSI #n "D"

// CUP – Cursor Position
// HVP – Horizontal and Vertical Position (same as CUP)
//   Moves the cursor to row n, column m. The values are 1-based, and default
//   to 1 (top left corner) if omitted. A sequence such as CSI ;5H is a synonym
//   for CSI 1;5H as well as CSI 17;H is the same as CSI 17H and CSI 17;1H
#define ANSI_CUP(x, y)	ANSI_CSI #y ";" #x "H"
#define ANSI_HVP(x, y)	ANSI_CSI #y ";" #x "f"

// SGR – Select Graphic Rendition
//   Sets SGR parameters, including text color. After CSI can be zero or more
//   parameters separated with ;. With no parameters, CSI m is treated as CSI 0
//   m (reset / normal), which is typical of most of the ANSI escape sequences.
#define ANSI_SGR(x)		ANSI_CSI #x "m"

// SGR COMMANDS
//   Some macros based on SGR commands
#define ANSI_SGR_RESET		ANSI_SGR(0)
#define ANSI_SGR_BOLD		ANSI_SGR(2)
#define ANSI_SGR_BOLD_OFF	ANSI_SGR(22)

// EXTRAS
#define ANSI_CLS		ANSI_ED(1) ANSI_HVP(1,1)

#endif
