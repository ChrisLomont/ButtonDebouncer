#pragma once

// Helper stuff for Button.h

#include <cstdint>

namespace Lomont {
	namespace ButtonHelpers
	{

        // global timing of button items
        // Set before creating any buttons
        namespace ButtonTimings
        {
            // this can be set globally, preferably before any debouncers started
            // Should divide debouncerInterruptMs
            // default to 5 ms
            extern uint8_t debounceMs;
            // you can set the interrupt rate before creating any buttons
            // default 1ms rate
            extern uint8_t debouncerInterruptMs;

            // global timing settings - change before making any buttons
            extern int clickUpLowMs; // click up time ms, low range
            extern int clickUpHighMs; // click up time ms, high range

            extern int clickDownLowMs; // click down time ms, low range
            extern int clickDownHighMs; // click down time ms, high range

            extern int repeatClickDelayMs; // repeat click start time, low end

            extern int mediumPressMs; // medium hold
            extern int longPressMs; // long hold

        };

        // support stuff button system
        namespace ButtonHW
        {
            // get elapsed time from the button system
            // ensure no possibility of getting data in wrong order or data tearing!
            uint64_t ElapsedMs();

            // Start/Stop the button interrupt
            // interrupt should
            // 1 - get elapsed time in ms from Button::ElapsedMs
            // 2 - for each button in system (via buttonPtrs),
            //     1 - read pin, process whether pins high or low means button down
            //     2 - call base class Debouncer DebounceInput with isDown and elapsedMs
            void StartDebouncerInterrupt();
            void StopDebouncerInterrupt();

            // prepare hardware as needed for a new pin to watch
            void SetPinHardware(int gpioPinNumber, bool downIsHigh);
        };




        // all FSM stuff
        namespace FSM
        {
            // An action to perform on an arrow match       
            struct Action
            {
                // values p,q to use
                int p{ 0 };
                int q{ 0 };

                // Action
                // 1 = add p to counter q
                // 2 = sub p from counter q
                // 3 = copy counter p to counter q
                // 4 = set counter q to value in p
                int action{ 0 };

                // apply action to counters and timer offset
                void DoAction(std::vector<int>& counters) const
                {
                    if (action == 1) counters[q] += p;
                    else if (action == 2) counters[q] -= p;
                    else if (action == 3) counters[q] = counters[p];
                    else if (action == 4) counters[q] = p;
                    else { printf("ERROR - invalid button action"); }
                }

            };

            // simpler ways to make common actions
            inline Action IncrementCounter(int counter)
            {
                return Action{ 1,counter,1 };
            }
            inline Action SetCounter(int counter, int value)
            {
                return Action{ value,counter,4 };
            }
            inline Action CopyCounter(int dst, int src)
            {
                return Action{ src,dst,3 };

            }

            // hold a state transition and pattern to match
            struct Arrow
            {
                Arrow(int dst, int buttonId, int action, int tm, int t0)
                    : destState(dst)
                    , buttonId_(buttonId)
                    , buttonAction(action)
                    , timeAction(tm)
                    , timeBoundMs(t0)
                {
                }

                Arrow(int dst, int buttonId, int action, int tm, int t0,
                    std::initializer_list<Action> actions
                ) : Arrow(dst, buttonId, action, tm, t0)
                {
                    for (auto& a : actions)
                        actions_.push_back(a);
                }

                // simple arrow interface, default to any button, button up or down, time > timeBoundMs
                Arrow(int dst, bool buttonDown, int timeBoundMs)
                    : Arrow(dst, 0, buttonDown ? 2 : 1, 2, timeBoundMs)

                {

                }
                Arrow(int dst, bool buttonDown, int timeBoundMs,
                    std::initializer_list<Action> actions
                )
                    : Arrow(dst, 0, buttonDown ? 2 : 1, 2, timeBoundMs)
                {
                    for (auto& a : actions)
                        actions_.push_back(a);
                }

                Arrow(int dst, int buttonId, bool buttonDown, int timeBoundMs) :
                    Arrow(dst, buttonId, buttonDown ? 2 : 1, 2, timeBoundMs)
                {
                }
                Arrow(int dst, int buttonId, bool buttonDown, int timeBoundMs,
                    std::initializer_list<Action> actions) :
                    Arrow(dst, buttonId, buttonDown ? 2 : 1, 2, timeBoundMs)
                {
                    for (auto& a : actions)
                        actions_.push_back(a);
                }



                // dst state on match
                int destState{ 0 };

                // button to match 
                // 0 for ignore
                int buttonId_{ 0 };

                // action to match 
                // 0 for ignore
                // 1 for up
                // 2 for down
                int buttonAction{ 0 };

                // time to match
                // 0 for ignore
                // 1 for buttonTimeInState <= timeBound
                // 2 for buttonTimeInState >= timeBound
                // 3 for timeInState >= timeBound
                int timeAction{ 0 };

                // time for comparisons
                int timeBoundMs{ 0 };


                // actions to do on match
                std::vector<Action> actions_;


                [[nodiscard]] bool Matches(int buttonId, bool buttonDown, uint64_t timeInButtonState, uint64_t stateTimeMs) const
                {
                    if (buttonId_ != 0 && buttonId_ != buttonId)
                        return false;
                    if (buttonAction != 0)
                    {
                        const auto bt = buttonDown ? 2 : 1;
                        if (bt != buttonAction)
                            return false;
                    }
                    if (timeAction != 0)
                    {
                        if (timeAction == 1 && static_cast<int>(timeInButtonState) > timeBoundMs)
                            return false;
                        if (timeAction == 2 && static_cast<int>(timeInButtonState) < timeBoundMs)
                            return false;
                        if (timeAction == 3 && static_cast<int>(stateTimeMs) < timeBoundMs)
                            return false;
                    }
                    return true;
                }
            };

            // a state is a list of exiting arrows
            struct State
            {
                std::vector<Arrow> arrows_;
                State(std::initializer_list<Arrow> arrows)
                {
                    for (auto& a : arrows)
                        arrows_.push_back(a);
                }
                State() = default;
            };


            // defines a finite state machine
            struct FSMDef
            {
                FSMDef(int counters) : counters_(counters) {}

                int counters_; // number of counters needed

                // finite state machine states, 0 indexed
                std::vector<State> states_;

                // call these to build up states
                void AddState()
                {
                    states_.emplace_back();
                }

                // add arrow to most recent state
                void AddArrow(int dst, int buttonId, int action, int tm, int t0)
                {
                    auto& s = states_.back();
                    s.arrows_.emplace_back(dst, buttonId, action, tm, t0);
                }

                // add action to most recent arrow in most recent state
                void AddAction(int p, int q, int action)
                {
                    auto& s = states_.back();
                    auto& a = s.arrows_.back();
                    a.actions_.push_back(Action{ p,q,action });
                }

                // allows building states in nicer formatted hierarchies
                void Build(std::initializer_list<State> states)
                {
                    for (auto& s : states)
                        states_.push_back(s);
                }
            };

            // holds a finite state machine
            class ButtonFSM
            {

            public:

                ButtonFSM(const FSMDef* fsm)
                {
                    fsm_ = fsm;
                    const auto c = fsm->counters_;
                    counters_.resize(c);
                    // zero values
                    for (auto i = 0; i < c; ++i)
                        counters_[i] = 0;
                }

                // read counter j, set to 0
                int Read0(int j = 0)
                {
                    if (j < 0 || static_cast<int>(counters_.size()) <= j) return 0;
                    const auto v = counters_[j];
                    counters_[j] = 0;
                    return v;
                }

                // call this often to monitor state
                void Update(int buttonId, bool buttonDown, uint64_t timeInStateMs)
                {
                    if (!fsm_) return; // null, no items
                    //printf("Check state %d %d %d\n",buttonId,buttonDown,(int)timeInStateMs);
                    const auto& state = fsm_->states_[stateIndex_];
                    const auto stateDt = ButtonHW::ElapsedMs() - stateTimeChangedMs_;
                    for (auto arrowIndex = 0U; arrowIndex < state.arrows_.size(); ++arrowIndex)
                    {
                        const Arrow& arrow = state.arrows_[arrowIndex];
                        if (arrow.Matches(buttonId, buttonDown, timeInStateMs, stateDt))
                        {
                            for (auto& action : arrow.actions_)
                            {
                                action.DoAction(counters_);
                            }

                            // useful debugging statement
                            if (dumpStateChangesToConsole)
                            {
                                static int cnt = 1;
                                printf("State change #%d: button:%d, states %d->%d via arrow %d, time %llu, actions %zu\n",
                                    cnt++,
                                    buttonId,
                                    stateIndex_, arrow.destState,
                                    arrowIndex,
                                    timeInStateMs,
                                    arrow.actions_.size()
                                );
                            }
                            // go to dest
                            stateIndex_ = arrow.destState;
                            if (stateIndex_ < 0 || static_cast<int>(fsm_->states_.size()) <= stateIndex_)
                                stateIndex_ = 0; // reset, todo - log error?

                            stateTimeChangedMs_ = ButtonHW::ElapsedMs();

                            break; // done, we have a match
                        }
                    }
                }


                // set to true to get printf of state changes
                // useful for debugging 
                bool dumpStateChangesToConsole{ false };

            private:
                // current state
                int stateIndex_{ 0 };
                const FSMDef* fsm_{ nullptr };
                // counters used in FSM
                std::vector<int> counters_;
                // last time state changed
                uint64_t stateTimeChangedMs_{ 0 };
            };

	        
        }
		
	}

    // a button debouncer
    // gives current debounced state IsDown
    // gives elapsed US in current state
    // gives time in last state
    class Debouncer
    {

    public:

        // called from interrupt
        // give button down state, and elapsed milliseconds in the system
        void DebounceInput(bool buttonDown, uint64_t elapsedMs)
        {
            using namespace ButtonHelpers::ButtonTimings;

            const bool localDown = IsDown(); // read once for routine
            if (buttonDown && integrator_ + debouncerInterruptMs <= debounceMs)
            {
                integrator_ = static_cast<int8_t>(integrator_ + debouncerInterruptMs);
                if (integrator_ >= debounceMs && buttonDown != localDown)
                    SetStateAtomically(buttonDown, elapsedMs);
            }
            else if (!buttonDown && integrator_ - debouncerInterruptMs >= 0)
            {
                integrator_ = static_cast<int8_t>(integrator_ - debouncerInterruptMs);
                if (integrator_ <= 0 && buttonDown != localDown)
                    SetStateAtomically(buttonDown, elapsedMs);
            }
        }


        // is debounced button down?
        // optionally gets time this state was changed
        bool IsDown(uint64_t* stateChangeTimeMs = nullptr) const
        {

            bool isDown;
            uint64_t time;
            GetStateAtomically(&isDown,&time);
            if (stateChangeTimeMs)
                *stateChangeTimeMs = time;
            return isDown;
        }
    private:
        // 0          = button up
        // debounceMs = button down
        // tallies milliseconds in a state
        int8_t integrator_{ 0 };

        /**************************Atomic read/write of state ***********************************/
        // many systems won't have atomic 64 bit, or structs. ESP32 Xtensa is one
        // Others pay a heavy price for larger than native int atomics
        // Assume the current one has atomic 32 bit read/write/exchanges
        // TODO - test this higher in code, warn/error out if not
        // 
        // idea is: 
        //  - put up/down bit in low bit, top 31 bits are low part of time
        //  - fill in rest of time carefully when outer level code requests

        std::atomic<uint32_t> atomicState_;

        uint32_t PackState(bool buttonDown, uint64_t elapsedMs) const
        {
            uint32_t val = (uint32_t)elapsedMs; // lower bits
            val <<= 1;
            val |= buttonDown ? 1 : 0;
            return val;
        }

        // change visible state to the given one
        // must be atomic in case of concurrent reads of state        
        void SetStateAtomically(bool buttonDown, uint64_t elapsedMs)
        {
            atomicState_ = PackState(buttonDown, elapsedMs); // atomic set
        }
        
        mutable bool buttonDown_{ false };
        mutable uint64_t changeTimeMs_{ 0 };
        void GetStateAtomically(bool * buttonDown, uint64_t * changedTimeMs) const
        {
            uint32_t state = atomicState_; // read it
            if (state != PackState(buttonDown_, changeTimeMs_))
            { // update internals....
                const uint64_t bit32 = (1ULL << 32); // bit 32 set bits mask
                const uint64_t mask = bit32-1; // low 31 bits mask

                const uint64_t curTime     = ButtonHelpers::ButtonHW::ElapsedMs();      // current time
                const uint64_t hiCurTime   = curTime & (~mask);          // zero out low 31 bits
                const uint32_t loCurTime   = (uint32_t)(curTime & mask); // low 31 bits
                const uint32_t stateTime = (state >> 1);

                // subtract one from top if there was wraparound
                // This requires calling within 2^31 ms = ~24 days.
                const uint64_t hiCorrectTime = hiCurTime - (stateTime < loCurTime?bit32:0);

                buttonDown_ = (state & 1) == 1;
                changeTimeMs_ = hiCorrectTime | stateTime; // high bits and low bits 
            }
            *buttonDown = buttonDown_;
            *changedTimeMs = changeTimeMs_;
        }
    }; // Debouncer 

}

