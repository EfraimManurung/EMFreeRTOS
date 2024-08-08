/*
  Introduction to RTOS Part 7 - Semaphore by Shawn Hymel
  URL: https://www.youtube.com/watch?v=5JcMtbA9QEE&list=PLXyB2ILBXW5FLc7j2hLcX6sAGbmH0JxX8&index=12
   
  Efraim Manurung, 8th August 2024
  Version 1.0
  
  Concepts from URL: https://www.digikey.nl/en/maker/projects/introduction-to-rtos-solution-to-part-7-freertos-semaphore-example/51aa8660524c4daba38cba7c2f5baba7
  
  Semaphore concepts:
  In programming, a semaphore is a variable used to control access to a common, shared resource that needs
  to be accessed by multiple threads or processes. It is similar to a mutex in that it can prevent other threads
  from accessing a shared resource or critical section.

  However, where a mutex implies a level of ownership of the lock (i.e. a single thread is said to have the lock
  while executing in a critical section), a semaphore is a counter that can allow multiple threads to enter a critical
  section.

  In theory, a semaphore is a shared counter that can be incremented and decremented atomically. For example, Tasks A, B, and C 
  wish to enter the critical section in the image above. They each call semaphore take(), which decrements the counting semaphore.
  At this point, all 3 tasks are inside the critical section and the semaphore's value is 0.

  If another task, like Task D, attempts to enter the critical section, it must first call semaphoreTake() as well. 
  However, since the semaphore is 0, semaphoreTake() will tell Task D to wait. When any of the other tasks leave 
  the critical section, they must call semaphoreGive(), which atomically increments the semaphore. 
  At this point, Task D may call semaphoreTake() again and enter the critical section.

  This demonstrates that semaphores work like a generalized mutex capable of counting to numbers greater than 1.
  However, in practice, you rarely use semaphores like this, as even within the critical section, there is no way to protect
  individual resources with just that one semaphore; you would need to use other protection mechanisms,
  like queues or mutexes to supplement the semaphore.

  In practice, semaphores are often used to signal to other threads that some common resource is 
  available to use. This works well in producer/consumer scenarios where one or more tasks generate data
  and one or more tasks use that data.

  Example of producers and consumers:
  Tasks A dan B (producers) create data to be put into a shared resource, such as a buffer or linked list.
  Each time they do, they call semaphoreGive() to increment the semaphore. Tasks C and D can read values from that resource,
  removing the values as they do so (consumers). Each time a task reads from the resource, it calls semaphoreTake(), which
  decrements the semaphore value.

  Note that the shared resource must still be protected to prevent overwrites from the producer threads. A semaphore is then
  used as an additional signal to the consumer tasks that values are ready. By limiting the maximum value of the semaphore,
  we can control how much information can be put into or read from the resource at a time (i.e. the size of the buffer or
  maximum length of the linked list).

  You can have a binary semaphore, which only counst to 1. Hopefully, you can see how a binary
  semaphore differs from a mutex in its use cases. While they might have similar implementations, 
  the use cases for mutexes and semaphores differ.

  sources:
  Atomic operations: https://stackoverflow.com/questions/52196678/what-are-atomic-operations-for-newbies


*/

#include <Arduino.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Pins
static const int led_pin = LED_BUILTIN;

// Globals
static SemaphoreHandle_t bin_sem;

//*************************************************************************************************
// Tasks

// Blink LED based on rate passed by parameter
void blinkLED(void *parameters) {

  // Copy the parameters into a local variable
  int num = *(int *)parameters;

  // Release the binary semaphore so that the creating function can finish
  xSemaphoreGive(bin_sem);

  // Print the parameter
  Serial.print("Received: ");
  Serial.println(num);

  // Configure the LED pin
  pinMode(led_pin, OUTPUT);

  // Blink forever and ever
  while (1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(num / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(num / portTICK_PERIOD_MS);
  }
}

//*************************************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  long int delay_arg;

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Mutex Challenge---");
  Serial.println("Enter a number for delay (milliseconds)");

  // Wait for input from serial
  while (Serial.available() <= 0);

  // Read integer value
  delay_arg = Serial.parseInt();
  Serial.print("Sending: ");
  Serial.println(delay_arg);
  
  // Create binary sempahore before starting tasks
  bin_sem = xSemaphoreCreateBinary();

  // Start task 1
  xTaskCreatePinnedToCore(blinkLED,
                          "Blink LED",
                          1024,
                          (void *)&delay_arg,
                          1,
                          NULL,
                          app_cpu);

  // Do nothing until binary semaphore has been returned
  xSemaphoreTake(bin_sem, portMAX_DELAY);

  // Show that we accomplished our task of passing the stack-based argument
  Serial.println("Done!");
}

void loop() {
  
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}





