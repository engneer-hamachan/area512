# Pin

Breadboard circuit simulator for Area512/Cardputer. Place parts on a 30-column breadboard and watch the circuit run.

## Controls

- `h` / `j` / `k` / `l` or arrows: move cursor
- `a` / Enter: add a part (choose a category, then a part)
- Enter / space while placing: set the next pin; lead parts take two pins, modules drop at the cursor
- `ESC` while placing: cancel
- space: press a tact switch / flip a slide switch
- `+` / `-`: change resistor value, volume knob or CdS light
- `x` / BS: remove the part under the cursor
- `c`: clear the board
- `?`: help
- `q`: quit

## Parts

Jumper wire, resistor, LED (red/yellow/green/blue), RGB LED, 7-segment LED, diode, tact switch, slide switch, 5V battery, buzzer, motor, NPN transistor, relay, 10k volume, CdS, 1000uF capacitor.

## Notes

- Nothing is powered until you place a battery. The rails are only marked `+` / `-`.
- An LED without a series resistor burns out. Remove it and place a new one.
- The header shows the pin under the cursor and its current or setting. `SHORT!` means a battery is shorted.
