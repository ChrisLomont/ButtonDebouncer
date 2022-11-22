# Button debouncer and button pattern recognizer

Chris Lomont, Nov 2022

This is a simple C++17 library that supports a nice method of handling button patterns in embedded systems. It supports:

* Click, Double-Click, Triple-Click, N-Click
* Medium and Long presses
* Repeat click
* User definable timings
* Arbitrary patterns defined via a Finite State Machine
* Arbitrary patterns for inter-button patterns, e.g., A down, B down, A up, B up
* System based on milliseconds, so easily portable to any hardware. All you need is a timer interrupt that can trigger every millisecond.
* Buttons can pull high or low voltage.

## Usage

1. Add files `Button.cpp`, `Button.h`, and `ButtonHelp.h` to your project
2. From `ButtonHelp.h`, implement the functions in the `ButtonHW `namespace:
   1. Implement the interrupt. Requirements are in the header
   2. Implement functions `StartButtonISR` and `StopButtonISR`
   3. Implement `SetPinHardware` to prepare any hardware for the button
   4. Implement `ElapsedMs` to get elapsed system time. If your environment does not support it, you can add it to the interrupt.
3. Use the `Button` class to make as many buttons as you need
4. You can now read the button `IsDown` function to see if the button is down from your user (non-interrupt) code.
5. If you poll buttons somewhat frequently (15-30 ms or so), you can also check for patterns matched:
   1. Call button `UpdatePatternMatches` frequently
   2. for each pattern index, check the button`Clicks` to get the number of clicks obtained. Reading this resets the click counter. Default patterns are: 0 = ClickN pattern, 1 = Medium Press Pattern, 2 = Long Pattern, 3 = RepeatClicks
6. If you want to use multiple button patterns, or modify or add or delete existing patterns, dig through the example patterns in `Buttons.cpp`



Hopefully I'll update a blog post soon and also provide more details in header comments.

Send fixes/changes as pull requests

## TODO

* Finish and link to blog post at [lomont.org](http://www.lomont.org)
* Better header comments