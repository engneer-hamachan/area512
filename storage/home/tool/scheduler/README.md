# Scheduler

Monthly calendar with per-day events for Area512/Cardputer.
Data is saved as `Area512_data/scheduler.txt` on the microSD card and loaded on start.

## Calendar screen

- `h` / `l`: previous / next day
- `k` / `j`: previous / next week
- `[` / `]`: previous / next month
- `space` / Enter: open the day view for the selected date

## Day screen

- `h` / `l`: previous / next day
- `j` / `k`: select an event
- `a`: add an event — enter a time as `HH:MM`, then the text
  (Enter confirms, Esc cancels each entry)
- `d`: delete the selected event
- `c` / Esc: back to the calendar

## Both screens

- `s`: save to `Area512_data/scheduler.txt`
- `q`: quit

## Notes

- Saving needs a microSD card; it is mounted while the app runs.
- The device has no clock, so the selected date is stored in the save
  file and restored on the next start.
- Quitting does not save automatically — press `s` first.
