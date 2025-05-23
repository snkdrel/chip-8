# A Chip-8 interpreter. 
Chip-8 interpreter made in C++, using SDL library. Currently it supports original strict Chip-8 behavior.

## How to run.
Clone repository along with SDL submodule.

Build with:
```
cmake -S . -B build
cmake --build build
```

Run with:
```
cd build
./chip-8 [your program file path]
```
or
```
cd build/Debug
./chip-8 [your program file path]
```
## To do list.
- Make quirks configurable.
- Make resolution configurable.
- Change beep sound so it doesn't clip.
- Adjust game speed.

## Credits.
I used [Guide to making a CHIP-8 emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/) as a reference and
[Timedus' chip-8 test suite](https://github.com/Timendus/chip8-test-suite) for testing.
