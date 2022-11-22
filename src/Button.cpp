#include <cstdio>
#include <vector>
#include <algorithm>
#include "Button.h"


/*
TODO: 
- make timing items more user-settable, unified, clean
- make FSM smaller (template sizes?), list limitations
- FSM builds to a table, exportable, useful as const in ROM
- constexpr FSM
- simplify FSM, only patterns used so far are u/d > some time
- cleaner way to make patterns
- clean up pattern comments
- lower C++ level? to 14? 11?
- more pattern matches, such as "All buttons up"
- or anything not matching X

- pattern like 1 down, 2d, 1u, 2u, needs some FSM features to make nicer...

*/

using namespace std;
using namespace Lomont::ButtonHelpers;
using namespace Lomont;

// these based on some empirical work
// median up/down times 150ms, gaussian, std dev 120ms
// so 1.5 std dev gives range of [60,240]
// shhorten a little for faster clickers:

// system timing items
int ButtonTimings::clickUpLowMs = 40; // wait to start looking for down
int ButtonTimings::clickUpHighMs = 210; // 
int ButtonTimings::clickDownLowMs = 40; // length for min down click
int ButtonTimings::clickDownHighMs = 210; // max down click

int ButtonTimings::repeatClickDelayMs = 300; // repeat        
int ButtonTimings::mediumPressMs = 600; // medium hold
int ButtonTimings::longPressMs = 2500; // long hold

// this can be set globally, preferably before any debouncers started
uint8_t ButtonTimings::debounceMs = 5;

uint8_t ButtonTimings::debouncerInterruptMs = 1; // 1 ms default


namespace {


/********************** default button patterns *****************************************/
using namespace Lomont::ButtonHelpers::FSM;
using namespace Lomont::ButtonHelpers::ButtonTimings;

vector<FSMDef> defaultFSM;

// add default click-N FSM
void AddClickN_FSM()
{
    // counter for hidden clicks, published clicks
    auto & f = defaultFSM.emplace_back(2);

    f.Build({
        State({
            Arrow(1,false,clickUpLowMs),
            }),
        
        State({
            Arrow(2,true,clickDownLowMs,{
				SetCounter(1,0)})}), // clear hidden counter 1

        State({
        	Arrow(0,true,clickDownHighMs,{
				CopyCounter(0,1)}), // publish clicks
			Arrow(3,false,clickUpLowMs,{
				IncrementCounter(1)})}), // increment private click counter

        State({
			Arrow(2,true,clickDownLowMs),
            Arrow(0,false,clickUpHighMs,{
				CopyCounter(0,1)})})            
        });
}

// add default med or long click
void AddClickLongerFSM(int minLenMs)
{
    // single counter
    auto & f = defaultFSM.emplace_back(1);

    f.Build({
        State({
        	Arrow(1,false,clickUpLowMs)}),

    	State({
			Arrow(2,true,minLenMs,{
                    IncrementCounter(0)})}), // increment counter

        State({
        	Arrow(0,false,0)})});
            
}

// add default repeat clicker
void AddClickRepeatFSM()
{
    // single counter
    auto & f = defaultFSM.emplace_back(1);

    // average click time
    const auto clickDelay =
        (clickUpLowMs + clickDownLowMs +
            clickUpHighMs + clickDownHighMs) / 2;

    // clickUpLowMs
    f.Build({
        // state 0 - ensure up some time
        State({
            Arrow(1,false,clickUpLowMs)}), 

        // state 1 - held long enough to trigger repeat
        State({
            Arrow(2,true,repeatClickDelayMs,{
                IncrementCounter(0)})}), // increment click counter

		// state 2 - repeat click until button up
        State({
            Arrow(2, 0, 2, 3, clickDelay, {
            IncrementCounter(0)}),
            Arrow(0,false,0)}) // on up, start over
    });
}

void EnsureDefaultFSM()
{
    if (!defaultFSM.empty()) return; // already done
    
    // add default buttons
    // AddClickFSM(); // single clicker lowest button
    AddClickN_FSM(); // N clicker, lowest button
    AddClickLongerFSM(mediumPressMs);
    AddClickLongerFSM(longPressMs);
    AddClickRepeatFSM();
}

// set up default button patterns
void InitFSM(Button * b)
{
    EnsureDefaultFSM();

    for (auto& fsmDef : defaultFSM)
        b->patterns.emplace_back(&fsmDef); // add pattern
}

// global next button
int nextButtonId{1};

} // namespace

// buttons in play
vector<Button*> Button::buttonPtrs;


Button::Button(int gpioNum, bool downIsHigh)
    : buttonId(nextButtonId++)
    , gpioNum_(gpioNum)
    , downIsHigh_(downIsHigh)
{
    InitFSM(this);

    // add button
    // pause task, update internals, restart task
    if (!buttonPtrs.empty())
        ButtonHW::StopDebouncerInterrupt();
    
    buttonPtrs.push_back(this);
    ButtonHW::SetPinHardware(gpioNum, downIsHigh);
    ButtonHW::StartDebouncerInterrupt();
}

Button::~Button()
{
    // remove button
    if (buttonPtrs.empty()) return;

    // pause task, update internals, restart task
    ButtonHW::StopDebouncerInterrupt();

    buttonPtrs.erase(std::remove(buttonPtrs.begin(), buttonPtrs.end(), this), buttonPtrs.end());
    if (!buttonPtrs.empty())
        ButtonHW::StartDebouncerInterrupt();
}

// call often to look for button clicks, long presses, etc.
void Button::UpdatePatternMatches()
{
    // current button state and info
    uint64_t timeStateChangedMs;
    const bool isDown = IsDown(&timeStateChangedMs);

    const uint64_t stateTime = ButtonHW::ElapsedMs() - timeStateChangedMs; // for Up/Down

    for (auto & p : patterns)
        p.Update(buttonId, isDown, stateTime);
}


