// button support for ESP32 code
#ifdef _MSC_VER // __WIN32__ || _CONSOLE // Win32 types

#include <windows.h>
#include <thread>
#include <memory>
using namespace std;

#include "Button.h"

using namespace Lomont;
using namespace Lomont::ButtonHelpers;

namespace {

    // timer interrupt for buttons
    void ButtonISR(void*)
    {
	    const uint64_t elapsedMs = ButtonHW::ElapsedMs();
        // process each button
        for (const auto& b : Button::buttonPtrs)
        {
	        const SHORT k = GetAsyncKeyState(b->GpioNum());
	        const auto isDown = (k & 0x8000) != 0; // high bit down
            // ignore pull direction in windows, since keyboard handled it
            //if (!b->DownIsHigh()) 
            //    isDown = !isDown;
            b->DebounceInput(isDown, elapsedMs); // call debouncer code
        }
    }

	shared_ptr<thread> th{nullptr};
    atomic<bool> stopThread;
void ThreadLoop()
{
    while (!stopThread)
        ButtonISR(nullptr);
}

}

void ButtonHW::StartDebouncerInterrupt()
{
    if (th) return; // already running
    stopThread = false;
    th = make_shared<thread>(ThreadLoop);
}

void ButtonHW::StopDebouncerInterrupt()
{
    stopThread = true;
    th->join();
    th = nullptr;
}

// set pin
void ButtonHW::SetPinHardware(int gpioPinNumber, bool downIsHigh)
{

}

// get elapsed time from the button system
uint64_t ButtonHW::ElapsedMs()
{
    static bool firstPass = true;
    static uint64_t startTicks = 0;
    static uint64_t ticksPerSecond = 0;

    LARGE_INTEGER tick;   // A point in time

    // what time is it?
    QueryPerformanceCounter(&tick);
    const uint64_t ticks64 = tick.QuadPart;

    if (firstPass)
    {
        LARGE_INTEGER ticksPerSecond2;
        // get the high resolution counter's accuracy
        QueryPerformanceFrequency(&ticksPerSecond2);

        ticksPerSecond = ticksPerSecond2.QuadPart;
        startTicks = ticks64;

        firstPass = false; // no more

        // cout << "Start ticks " << ticks64 << " ticks/sec " << ticksPerSecond << endl;
    }
    // convert to milliseconds
    return 1'000ULL * (ticks64 - startTicks) / ticksPerSecond;
}


#endif