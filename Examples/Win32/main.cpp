#include <thread>
#include <windows.h>

#include "Button.h"

using namespace std;
using namespace std::chrono;
using namespace Lomont;

void Run()
{
	auto bz = Button('Z'); // button 1
	auto bx = Button('X'); // button 2
	auto by = Button('Y'); // button 3

	auto mp = ButtonMultiPattern();
	mp.AddTestPatterns(); // default patterns to check

	auto t = ButtonHelpers::ButtonHW::ElapsedMs();
	bool done = false;
	while (!done)
	{
		// wait till change
		while (t == ButtonHelpers::ButtonHW::ElapsedMs())
		{
			
		}
		t = ButtonHelpers::ButtonHW::ElapsedMs();

		mp.UpdatePatternMatches();
		for (auto k = 0U; k < mp.patterns.size(); ++k)
		{
			const auto c = mp.Clicks(k);
			if (c > 0)
				printf("Multi pattern type %d, count %d\n", k, c);
		}

#if 1
		for (const auto & b : Button::buttonPtrs)
		{
			b->UpdatePatternMatches();
			for (auto k = 0U; k < b->patterns.size(); ++k)
			{
				const auto c = b->Clicks(k);
				if (c > 0)
					printf("%c click type %d, count %d\n", b->GpioNum(), k, c);
			}
		}
#endif

		const SHORT k = GetAsyncKeyState('Q');
		done = (k & 0x8000) != 0; // high bit down

	}
}
int main()
{
	Run();
	Sleep(500); // hope threads let go

}

