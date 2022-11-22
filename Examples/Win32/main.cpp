#include <windows.h>
#include "Button.h"

using namespace std;
using namespace Lomont;

// link with example.cpp
extern void StartButtons(int pin1, bool pin1DownIsHigh, int pin2, bool pin2DownIsHigh);
extern void StopButtons();
extern void ProcessButtons();

int main()
{
	printf("keys x & z for patterns, q to quit\n");

	// buttons 1 and 2
	// set pin pull directions to match your hardware!
	StartButtons('Z', true, 'X', true);

	auto t = ButtonHelpers::ButtonHW::ElapsedMs();
	bool done = false;
	while (!done)
	{
		// wait till change
		while (t == ButtonHelpers::ButtonHW::ElapsedMs())
		{

		}
		t = ButtonHelpers::ButtonHW::ElapsedMs();

		ProcessButtons();

		// see if done
		const SHORT k = GetAsyncKeyState('Q');
		done = (k & 0x8000) != 0; // high bit down
	}

	StopButtons();

	Sleep(500); // hope threads let go

}

