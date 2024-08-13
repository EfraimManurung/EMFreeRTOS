/*
    Introduction to RTOS Part 9 - Hardware Interrupts by Shawn Hymel
    URL: https://www.youtube.com/watch?v=b1f1Iex0Tso&list=PLEBQazB0HUyQ4hAPU1cJED6t3DU0h34bz&index=9

    Efraim Manurung, 13th August 2024
    Version 1.0
    
    Concepts from URL: https://www.digikey.nl/en/maker/projects/introduction-to-rtos-solution-to-part-9-hardware-interrupts/3ae7a68462584e1eb408e1638002e9ed

    Hardware interrupts concept:
    Hardware interrupts are an important part of many embedded systems. They allow events to occur asynchronously 
    (not as part of any executing program) and notify the CPU that it should take some action.
    These types of interrupts can cause the CPU to stop whatever it was doing and execute some other function,
    known as an "interrupt service routine" (ISR).

    Such hardware interrupts can include things like button presses (input pin voltage change), a
    hardware timer expiring, or a communication buffer being filled.

    In most RTOSes (FreeRTOS included), hardware interrupts have a higher priority than any task (unless
    we purposely disable hardware interrupts). When working with hardware interrupts, there are few things to keep in mind.

    First, an ISR should never block itself. ISRs are not executed as part of a task, and as a reulst, cannot
    be blocked. Because of this, you should only use FreeRTOS function calls that end in *FromISR inside an ISR. You cannot wait 
    for a queue, mutex, or semaphore, so you will need to come up with alternatives if writing to one fails!

    Second, you should keep ISRs as short as possible. You probably do not want to delay the execution of all your waiting
    tasks!

    Third, if a variable (such as a global) is updated inside an ISR, you likely need to declare it with the 
    "volatile" qualifier. This lets the compiler know that the "volatile" variable can change outside the current thread 
    of execution. Without it, compilers may (depending on their optimization settings) simply remove the variable,
    as they don't think it is being used (anywhere inside main() or various tasks).

    Finally, one of the easiest ways to synchronize a task to an ISR is to use what is known as a "deferred interrupt".
    Here, we defer processing the data captured inside the ISR to another task. Whenever such data has been captured,
    we can give a semaphore (or send a task notification https://www.freertos.org/RTOS-task-notification-API.html) to let 
    some other task know that data is ready for processing.

    In the diagram above, Task B is blocked waiting for semaphore. Only the ISR gives the semaphore. So, as soon as the ISR runs 
    (and, say, collects some data from a sensor), it gives the semaphore. When the ISR has finished executing, Task B is immediately
    unblocked and runs to process the newly collected data.

    
    Description:
    ESP32 ISR SEmaphore Demo
    Read ADC values in ISR at 1 Hz and defer printing them out in a task.
*/

#include<Arduino.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
    static const BaseType_t app_cpu = 0;
#else
    static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint16_t timer_divider = 80;   // count at 1 Mhz
static const uint64_t timer_max_count = 1000000; 

// Pins
static const int adc_pin = A0;

// Globals
static hw_timer_t *timer = NULL;
static volatile uint16_t val;
static SemaphoreHandle_t bin_sem = NULL;

//*****************************************************************************
// Interrupt Service Routines (ISRs)

// This function executes when timer reaches max (and resets)
void IRAM_ATTR onTimer() {

    BaseType_t task_woken = pdFALSE;

    // Perform action (read from ADC)
    val = analogRead(adc_pin);

    // Give semaphore to tell task that new value is ready
    xSemaphoreGiveFromISR(bin_sem, &task_woken);

    // Exit from ISR (ESP-IDF)
    if (task_woken) {
        portYIELD_FROM_ISR();
    }
}

//*****************************************************************************
// Tasks

// Wait for semaphore and print out ADC value when received
void printValues(void *parameters) {

    // Loop forever, wait for semaphore, and print value
    while (1) {
        xSemaphoreTake(bin_sem, portMAX_DELAY);
        Serial.println(val);
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
  Serial.println("---FreeRTOS ISR Buffer Demo---");

  // Create semaphore before it is used (in task or ISR)
  bin_sem = xSemaphoreCreateBinary();

  // Force reboot if we can't create the semaphore
  if (bin_sem == NULL) {
    Serial.println("Could not create semaphore");
    ESP.restart();
  }

  // Start task to print out results (higher priority!)
  xTaskCreatePinnedToCore(printValues,
                          "Print values",
                          1024,
                          NULL,
                          2,
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
}

void loop() {
  // Do nothing, forever
}