// button support for ESP32 code

#include <cstdint>

#include "driver/gpio.h"
#include "esp_timer.h" // esp_timer_get_time
#include "esp_log.h"

#include "Button.h"

using namespace Lomont;
using namespace Lomont::ButtonHelpers;


namespace {

// timer task for ESP32
esp_timer_handle_t periodic_timer;

// timer interrupt for buttons
void ButtonISR(void*)
{
    uint64_t elapsedMs = ButtonHW::ElapsedMs();
    // process each button
    for (auto & b : Button::buttonPtrs)
    {
        auto level = gpio_get_level((gpio_num_t)b->GpioNum());
        auto isDown = level!=0; // assume down is high
        if (!b->DownIsHigh())
            isDown = !isDown;
        b->DebounceInput(isDown, elapsedMs); // call debouncer code
    }
}

} // anonymous namespace

namespace Lomont { namespace ButtonHelpers { namespace ButtonHW {

void StartDebouncerInterrupt()
{

    const esp_timer_create_args_t periodic_timer_args = {
                .callback = ButtonISR,
                .arg = nullptr, // no arguments
                .dispatch_method = ESP_TIMER_TASK, // task or ISR
                /* name is optional, but may help identify the timer when debugging */
                .name = "buttonTimer",
                .skip_unhandled_events = true
        };

    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    /* The timer has been created but is not running yet */	

    /* Start the timer */
    // timer in microseconds
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, ButtonTimings::debouncerInterruptMs*1000));
}

void StopDebouncerInterrupt()
{
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
    ESP_ERROR_CHECK(esp_timer_delete(periodic_timer));	
}

// set pin
void SetPinHardware(int gpioPinNumber, bool downIsHigh)
{
	// zero-initialize the config structure.
    gpio_config_t io_conf = {};

    // no interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // bit mask of the pins
    io_conf.pin_bit_mask = 1ULL<< gpioPinNumber;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;

    // NOTE: ESP32 pins 34,35,36,39 don't have internal pullups/pulldowns
    // you need to pull them with ~10K resistor, else will be noisy
	if (!downIsHigh)
	{
		// disable pull-down mode
		io_conf.pull_down_en = (gpio_pulldown_t)0;
		// enable pull-up mode
		io_conf.pull_up_en = (gpio_pullup_t)1;
	}
	else
	{
		// enable pull-down mode
		io_conf.pull_down_en = (gpio_pulldown_t)1;
		// disable pull-up mode
		io_conf.pull_up_en = (gpio_pullup_t)0;
	}
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    }

// get elapsed time from the button system
uint64_t ElapsedMs()
{
    auto t =  esp_timer_get_time(); // get 64 bit signed time in us
    return t/1000;// into milliseconds
}

}}} // nested namespaces
