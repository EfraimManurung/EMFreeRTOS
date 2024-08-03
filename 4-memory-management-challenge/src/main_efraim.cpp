/*
   Introduction to RTOS Part 4 - Memory Management with FreeRTOS by Shawn Hymel
   URL: https://youtu.be/Qske3yZRW5I
   https://www.digikey.nl/en/maker/projects/introduction-to-rtos-solution-to-part-4-memory-management/6d4dfcaa1ff84f57a2098da8e6401d9c

   Efraim Manurung, 3rd August 2024
   Version 1.0

   Concept of memory management 

   Volatile memory (e.g. RAM) in most microcontroller systems is divided up into 3 sections:
   static, stack and heap.

   - Static memory is used for storing global variables and variables designed as "static" in code
     (they persist between function calls).
   - The stack is used for automatic allocation of local variables. Stack memory is organize as a 
     last-in-first-out (LIFO) system so that the variables of one function can be "pushed" to the stack
     when a new function is called. Upon  returning to the first function, that functions' variables
     can be "popped" off, which the function can use to continue running where it let off.
   - Heap must be allocated explicitly by the programmer. Most often in C, you will use the malloc()
     function to allocate heap for your variables, buffers, etc. This is known as "dynamic allocation".
    
    Note that in languages without a garbage collection system (e.g. C and C++), you must deallocate
    heap memory when it's no longer used. Failing to do so will result in a memory leak, and could cause 
    undefined effects, such as corrupting other parts of memory. 

    In most systems, the stack and heap will grow toward each other, taking up unallocated memory
    where needed. If left unchecked, they could collide and begin overwriting each other's data.

    When you create a task in FreeRTOS (e.g. with xTaskCreate()), the operating system will allocate
    a section of heap memory for the task.

    Challenge:
    Using FreeRTOS, create two separate tasks. One listens for input over UART (from the Serial monitor).
    Upon receiveng a newline character ('\n'), the task allocate a new section of heap memory (using pvPortMalloc())
    and stores the string up to the newline character in that section of heap. It then notifies the second task
    that a message is ready.

    The second task watis for notification from the first task. When it receives that notification,
    it prints the message in heap memory to the Serial monitor. Finally, it deletes the allocated heap memory
    (using vPortFree())
*/

#include <Arduino.h>

// Import FreeRTOS library
#include <freertos/FreeRTOS.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Set global variables
static const uint8_t string_len = 255;

// Handle for the second task notification
TaskHandle_t secondTaskHandle = NULL;

// First task: Listen for input over UART (from the Serial Monitor).
void firstTask(void *parameter) {
  // Initiate local variables
  char c;
  char string_input[string_len];
  uint8_t idx = 0;

  /*
  `memset` is a function from the C standard library that sets a block of memory to a specific value. In this code, it is used
  to clear the buffer `buf` by setting all its elements to 0. This ensures that buffer is empty before storing new
  input data from the serial port.
  */
 
  // Clear whole buffer
  memset(string_input, 0, string_len);

  // Loop forever
  while(1) {

    // Read characters from serial
    if (Serial.available() > 0) {
      c = Serial.read();

      // Update the string_input and reset buffer if we get a newline character
      if (c == '\n') {
        
        // Allocate memory for the string
        char *string_send = (char *)pvPortMalloc((idx + 1) * sizeof(char));

        // Check the allocated memory
        if (string_send != NULL) {

          // char *strncpy(char *dest, const char *src, size_t n)
          strncpy(string_send, string_input, idx);
          string_send[idx] = '\0';

          // Notify the second task
          xTaskNotify(secondTaskHandle, (uint32_t)string_send, eSetValueWithOverwrite);

          // string_send = string_input;
          Serial.print("Update send string message: ");
          Serial.println(string_send);
        } else {
          Serial.println("Memory allocation failed!");
        }

        // Clear whole buffer
        memset(string_input, 0, string_len);
        idx = 0;
      } else if (idx < string_len - 1) {
        // Append character to the string
        string_input[idx++] = c;
      }
    }
  } 
}

// Second task: Wait and receives the notification, it prints the message in heap memory to the Serial monitor.
void secondTask(void *parameter) {
  // Wait for notification
  char *received_string;
  if (xTaskNotifyWait(0, 0, (uint32_t *)&received_string, portMAX_DELAY)) {

    // Check again the received string 
    if (received_string != NULL) {
        Serial.print("Received string message: ");
        Serial.println(received_string);
      
        // Free the memory of microcontroller
        vPortFree(received_string);
    }
  }
}

void setup() {

  // Initialize Serial Communication
  Serial.begin(115200);

  // Create tasks
  // Task to run forever
  xTaskCreatePinnedToCore(firstTask,
                          "First Task",
                          2048,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
  
  // Task to run once with higher priority
  xTaskCreatePinnedToCore(secondTask,
                          "Second Task",
                          2048,
                          NULL,
                          1,
                          &secondTaskHandle,
                          app_cpu);

}

void loop() {
  // The program should not run in here
}