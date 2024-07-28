/*
   Introduction to RTOS Part 3 - Task Scheduling with FreeRTOS by Shawn Hymel
   URL: https://www.youtube.com/watch?v=95yUbClyf3E&list=PLEBQazB0HUyQ4hAPU1cJED6t3DU0h34bz&index=3

   Efraim Manurung, 09th March 2024
   Version 1.0

   Efraim Manurung, 09th March 2024
   Version 1.1 
   Challenge:
   Using FreeRTOS, creat two separate tasks. One listens for an integer over UART (from the Serial Monitor) and sets
   a variable when it sees an integer. The other task blinks the onboard LED (or other connected LED) at a rate specified by that integer.
   In effect, you want to create a multi-threaded system that allows for the user interface to run concurrently with 
   the control task (the blinking LED).
*/

/*
    Learning about task scheduling in FreeRTOS. We also will implement multi-threaded program. 

    Toggles LED and prints "Hello, World" to console in separate tasks. 
*/

#include <Arduino.h>
// #include <FreeRTOSConfig.h>

// Needed for atoi()
#include <stdlib.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint8_t buf_len = 20;

// Pins
static const int led_pin = LED_BUILTIN;

// Globals
static int led_delay = 500;

//***********************************************************************************************
// Tasks

// Task: Blink LED at rate set by global variable
void toggleLED(void *parameter) {
  while(1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(led_delay / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(led_delay / portTICK_PERIOD_MS);
  }
}

void readSerial(void *parameter) {

  char c;
  char buf[buf_len];
  uint8_t idx = 0;

  /*
  `memset` is a function from the C standard library that sets a block of memory to a specific value. In this code, it is used
  to clear the buffer `buf` by setting all its elements to 0. This ensures that buffer is empty before storing new
  input data from the serial port.
  */

  // Clear whole buffer
  memset(buf, 0, buf_len); 

  // Loop forever
  while(1) {

    // Read characters from serial
    if (Serial.available() > 0) {
      c = Serial.read();

      // Update delay variable and reset buffer if we get a newline character
      if (c == '\n') {
        /*
        `atoi` stands for "ASCII to integer". It is a function that converts a string representing a number into
        an integer. In this code, it is used to convert the string in the buffer `buf` to an integer,
        which is then used to set the `led_delay` variable.
        */
        led_delay = atoi(buf); 
        Serial.print("Updated LED delay to: ");
        Serial.println(led_delay);
        memset(buf, 0, buf_len);
        idx = 0;
      } else {

        // Only append if index is not over message limit
        if (idx < buf_len - 1) {
          buf[idx] = c;
          idx++;
        }
      }
    }
  }
}

//********************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  // Configure pin
  pinMode(led_pin, OUTPUT);

  // Configure serial and wait a second
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println("Multi-task LED Demo");
  Serial.println("Enter a number in milliseconds to change the LED delay.");

  // Task to run forever
  xTaskCreatePinnedToCore(toggleLED,
                          "Toggle LED",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
  
  // Task to run once with higher priority
  xTaskCreatePinnedToCore(readSerial,
                          "Read Serial",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
}

void loop() {
    // Execution should never get here
}