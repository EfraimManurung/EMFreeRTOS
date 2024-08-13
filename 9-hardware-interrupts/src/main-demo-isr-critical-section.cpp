/*
    Introduction to RTOS Part 9 - Hardware Interrupts by Shawn Hymel
    URL: https://www.youtube.com/watch?v=b1f1Iex0Tso&list=PLEBQazB0HUyQ4hAPU1cJED6t3DU0h34bz&index=9

    Efraim Manurung, 12th August 2024
    Version 1.0
    
    Concepts from URL: https://www.digikey.nl/en/maker/projects/introduction-to-rtos-solution-to-part-9-hardware-interrupts/3ae7a68462584e1eb408e1638002e9ed

    Hardware interrupts concept:

    ESP32 ISR Critical Section Demo

    Increment global variable in ISR.
*/

#include<Arduino.h>

// USe only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
    static const BaseType_t app_cpu = 0;
#else
    static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint16_t timer_divider = 8;
static const uint64_t timer_max_count = 1000000;
static const TickType_t task_delay = 2000 / portTICK_PERIOD_MS;

// Globals
static hw_timer_t *timer = NULL;
static volatile int isr_counter;
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

//*****************************************************************************
// Interrupt Service Routines (ISRs)

void IRAM_ATTR onTimer() {

    // ESP-IDF version of a critical sectioni (in an ISR)
    portENTER_CRITICAL_ISR(&spinlock);
    isr_counter++;
    portEXIT_CRITICAL_ISR(&spinlock);
}

//*****************************************************************************
// Tasks

// Wait for semaphore and print out ADC value when received 
void printValues(void *parameters) {

    // Loop forever
    while (1) {

        // Count down and print out counter value
        while (isr_counter > 0) {

        // Print value of counter
        Serial.println(isr_counter);

        // ESP-IDF version of a critical section (in a task)
        portENTER_CRITICAL(&spinlock);
        isr_counter--;
        portEXIT_CRITICAL(&spinlock);
        }

        // Wait 2 seconds while ISR increments counter a few times
        vTaskDelay(task_delay);
    }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

    // Configure Serial
    Serial.begin(115200);

    // Wait a moment to start (so we don't miss Serial output)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.println();
    Serial.println("---FreeRTOS ISR Critical Section Demo---");

    xTaskCreatePinnedToCore(printValues,
                            "Print values",
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);
    
    // Create and start timer (num, divider, countUp)
    timer = timerBegin(0, timer_divider, true);

    // Provide ISR to timer (timer, function, edge)
    timerAttachInterrupt(timer, &onTimer, true);

    // At what count should ISR trigger (timer, count, autoreload)
    timerAlarmWrite(timer, timer_max_count, true);

    // Allow ISR to trigger
    timerAlarmEnable(timer);

    // Delete "setup and loop" task
    vTaskDelete(NULL);
}

void loop() {
    // Execution should never get here
}