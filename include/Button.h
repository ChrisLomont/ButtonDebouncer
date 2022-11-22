#pragma once
#ifndef BUTTON_H
#define BUTTON_H

// Lomont Button system
// Chris Lomont Nov 2022
// Requires C++ 17

#include <memory>
#include <vector>
#include <atomic>
#include "ButtonHelp.h"

namespace Lomont {

    /* USAGE:
     * See TODO: URL, refs?
     * TODO
     * 1. Implement button hardware functions
     * 2. Use the Button class
     * 3. Call each button Update every so often (10-30ms?)
     * 4. Check each button pattern for clicks
     * 5. (Optional) Use Button multi pattern to check cross button patterns
     *
     * Creating new patterns: 
     * 1. Patterns are state machines. 
     * 2. Look at existing patterns until you understand how they work
     * 3. setting dumpStateChangesToConsole to true helps in debugging patterns
     *
     */


    // represent a button.
    // when pressed, buttons go high, default pulled low
    class Button final : public Debouncer
    {
    public:

        // create a button on the given gpio, default is pulls high on down
        // starts button timer interrupts, attaches
        Button(int gpioNum, bool downIsHigh = true);

        // remove button from system, close out 
        // interrupt on last button gone
        ~Button();

        // NOTE: can check with base Debouncer class 
        // is debounced button down?
        // optionally gets time this state was changed
        // bool Debouncer::IsDown(uint64_t * timeInState = nullptr) const;

        // call often to look for button clicks, long presses, etc.
        // often = every 5-20ms or so
        void UpdatePatternMatches();

        // get button GPIO
        int GpioNum() const { return gpioNum_; }

        // check if button is down = high or low voltage
        bool DownIsHigh() const { return downIsHigh_; }

        // see if pattern matched
        // return counter 0 from matches, clear counter
        int Clicks(unsigned int patternIndex = 0)
        {
            if (patternIndex < 0 || patterns.size() <= patternIndex)
                return 0;
            return patterns[patternIndex].Read0();
        }

        // unique button id, 1+
        int buttonId;

        // list of patterns as finite state machines
        // todo - list default ones
        std::vector<ButtonHelpers::FSM::ButtonFSM> patterns;

        // track all active buttons
        static std::vector<Button*> buttonPtrs;

    private:

        int gpioNum_{ -1 };
        bool downIsHigh_{ true }; // button pulls high or pulls low when pressed
    };

    class ButtonMultiPattern
    {
    public:
        // call often to look for multi button patterns
        // often = every 5-20ms or so
        void UpdatePatternMatches()
        {
            for (const auto& b : Button::buttonPtrs)
            {
                // each button state and info
                uint64_t timeStateChangedMs;
                const bool isDown = b->IsDown(&timeStateChangedMs);

                const uint64_t stateTime = ButtonHelpers::ButtonHW::ElapsedMs() - timeStateChangedMs; // for Up/Down

                for (auto& p : patterns)
                    p.Update(b->buttonId, isDown, stateTime);
            }
        }

        // see if pattern matched
        int Clicks(int patternIndex = 0)
        {
            if (patternIndex < 0 || static_cast<int>(patterns.size()) <= patternIndex)
                return 0;
            return patterns[patternIndex].Read0();
        }

        // list of patterns as finite state machines
        std::vector<ButtonHelpers::FSM::ButtonFSM> patterns;

        void AddABABPattern(int T0, int T1)
        {
            using namespace ButtonHelpers::FSM;    // make construction easier

            // 1 counter
            auto& abab = defs.emplace_back(1);
            abab.Build({
                // state 0 - 1?,2? : wait for 2 up
                State({
                    Arrow(1,2, false,0),
                }),

                // state 1 - 1?,2u : wait for 1 up
                State({
                    Arrow(2,1, false,0),
                    Arrow(0,2, true,0)  // 2 down resets
                }),

                // state 2 - 1u,2u : wait for 1 down at least T0
                State({
                    Arrow(3,1, true,T0),
                    Arrow(0,2, true,0)  // 2 down resets
                }),

                // state 3 - 1d,2u : wait for 2 down
                State({
                    Arrow(4,2, true,T0),
                    Arrow(0,1, false,0), // 1 up resets 
                    Arrow(0,1, 2, 3, T1)// 1 down in this state too long resets
                }),

                // state 4 - 1d,2d : wait for 1 up
                State({
                    Arrow(5,1, false,T0),
                    Arrow(0,2, false,0), // 2 up resets 
                    Arrow(0,2, 2, 3, T1)// 2 down in this state too long resets
                }),

                // state 5 - 1u,2d : wait for 2 up
                State({
                    Arrow(0,2, false,0,{IncrementCounter(0)}),
                    Arrow(0,1, true,0), // 1 down resets 
                    Arrow(0,2, 2, 3, T1)// 2 down in this state too long resets
                }),

                });
        }

        // add some default patterns:
        // button 1 down, 2 down, 1 up, 2 up
        void AddTestPatterns()
        {
            if (defs.empty())
            { // make defs
                AddABABPattern(500, 1000); // each click between 0.5 and 1.0 secs
                AddABABPattern(20, 100);   // much faster one, 50 to 150 ms each
            }
            for (const auto& f : defs)
            {
                auto& b = patterns.emplace_back();
                // b.dumpStateChangesToConsole = true;
                b.SetFSM(&f);
            }

        }
        // need these to live as long as needed
        std::vector<ButtonHelpers::FSM::FSMDef> defs;
    };

    using ButtonPtr = std::shared_ptr<Button>;

}

#endif //  BUTTON_H

