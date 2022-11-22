// example button code, C++17
#include <memory> // make_shared
#include <string>
#include <iostream>

#include "Button.h"

using namespace std;
using namespace Lomont;

// some buttons to make later
ButtonPtr button1{ nullptr };
ButtonPtr button2{ nullptr };

// a pattern watcher to make later
ButtonMultiPatternPtr multiPattern{ nullptr };


// Step 0. be sure to have implemented the four system functions in ButtonHW namespace
/*
	// get elapsed time from the button system
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
*/


// Step 1. add some buttons
// pin2 are usually GPIO numbers, downIsHigh is if your button physcially pulls to low voltage when down, or high voltage when down
void StartButtons(int pin1, bool pin1DownIsHigh, int pin2, bool pin2DownIsHigh)
{
	// 1. set any system timings first in ButtonTimings namespace in ButtonHelp.h
	//

	// 2. Make some buttons. Starts the timer interrupt. Button ids start at 1 internally and auto increment
	button1 = make_shared<Button>(pin1,pin1DownIsHigh);
	button2 = make_shared<Button>(pin2, pin2DownIsHigh);


	// 3. Make an optional multi-button pattern watcher if desired
	multiPattern = make_shared<ButtonMultiPattern>();
	// add default testing patterns, or create and add your own (see code)
	multiPattern->AddTestPatterns(); 
}

// Step 2. Sample often. 20-30 ms should be fine
void ProcessButtons()
{
	// default single click patterns
	const string singleClickNames[] = {"ClickN","Medium hold","Long hold","Repeat"};

	// update button patterns, see which patterns have occurred
	for (const auto& b : Button::buttonPtrs)
	{
		// can sample this if desired, no need for pattern updates
		// bool isDown = b->IsDown();

		// update any patterns being watched
		b->UpdatePatternMatches();

		// walk patterns to look for matches
		for (auto patternIndex = 0U; patternIndex < b->patterns.size(); ++patternIndex)
		{
			const auto clicks = b->Clicks(patternIndex);
			if (clicks > 0)
			{
				cout << "button (id:" << b->buttonId << ", pin:" << b->GpioNum() << ") saw ";
				cout << "click type " << singleClickNames[patternIndex] << " with count " << clicks;
				cout << endl;
			}
		}
	}


	// default multi click patterns
	const string multiClickNames[] = {
		"Slow ABAB", // 1 down, 2 down, 1 up, 2 up, slowly
		"Fast ABAB",  // 1 down, 2 down, 1 up, 2 up, quickly
	};

	// update and check optional multi button patterns
	if (multiPattern)
	{
		// update patterns
		multiPattern->UpdatePatternMatches();
		for (auto patternIndex = 0U; patternIndex < multiPattern->patterns.size(); ++patternIndex)
		{
			const auto clicks = multiPattern->Clicks(patternIndex);
				if (clicks > 0)
				{
					cout << "Multi button click type " << multiClickNames[patternIndex] << " with count " << clicks;
					cout << endl;
				}
		}
	}
}

// Step 3. Shut down when desired by calling all item destructors
void StopButtons()
{
	// releases shared pointers, last holder triggers destructor
	multiPattern = nullptr;
	button2 = nullptr;
	button1 = nullptr;
}

