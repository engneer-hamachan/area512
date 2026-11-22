# Space Lander

A lunar landing game for AREA512, written in Python.

Land the ship entirely on the pad without exceeding the safe vertical or
horizontal speed. Later levels have a narrower pad, less fuel, moving debris,
and lower safe landing speeds.

## Screen

- `L`: current level
- `F`: remaining fuel
- `V`: vertical velocity
- `H`: horizontal velocity

## Controls

- `space` / Enter: start, use the main thruster, or continue after landing
- `w`: use the main thruster
- `a` / `h`: steer left
- `d` / `l`: steer right
- `r` / `n`: restart from level 1
- `q`: quit

## Notes

- The main thruster uses 1.2 units of fuel per input.
- A side thruster uses 0.5 units of fuel per input.
- Touching the ground outside the pad or above the safe landing speed crashes
  the ship.
- Contact with moving debris crashes the ship.
