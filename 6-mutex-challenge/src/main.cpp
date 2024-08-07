/*
  Introduction to RTOS Part 6 - Mutex challenge by Shawn Hymel
  URL: https://www.youtube.com/watch?v=I55auRpbiTs&list=PLEBQazB0HUyQ4hAPU1cJED6t3DU0h34bz&index=6
   
  Efraim Manurung, 6th August 2024
  Version 1.0
  
  Concepts from URL: https://www.digikey.nl/en/maker/projects/introduction-to-rtos-solution-to-part-6-freertos-mutex-example/c6e3581aa2204f1380e83a9b4c3807a6

  Race condition concepts:
  A mutex (short for MUTual EXclusion) is a flag or lock used to allow only one thread
  to access a section of code at a time. It blocks (or locks out) all other threads from accessing 
  the code or resource. This ensures that anything executed in that critical section is thread-safe
  and information will not be corrupted by other threads.

  Example:
  A mutex is like a single key sitting in a basket at a coffee shop. The key can be used to unlock
  a shared public restroom. 

  A person takes the key when they wish to use the shared resource (restroom) and returns it when
  they are done. While they are in the restroom, no one else may enter. Other people (analogous to threads)
  must wait for the key. When it is returned, another person may take the key to to use the restroom.

  Without such a locking mechanism, we are liable to end up with a race condition in our code
  as multiple threads attempt to read and modify a common resource (such as global variable, serial port, etc.).

  To eliminate the race condition, we need some kind of locking mechanism to prevent more than one thread
  from executing the read-modify-write sequence at a time. 

  We can use a mutex to help, as it allows mutual exclusion of thread execution in a critical selection. 
  In an RTOS, a mutex is simply a global (or shared) binary value that can be accessed atomically. That means if a
  thread takes the mutex, it can read and decrement the value without being interrupted by other threads.
  Giving the mutex (incrementing the value by one) is also atomic.

  What makes mutexes work, much like queues, is that a thread is force to wait if a mutex is not available.
  If a thread sees that a mutex cannot be taken, it enters into the block state or it can do some other work before
  checking the mutex again.

  Challenge:
  Starting with the code given below, modify it to protect the task parameter (delay_arg) with a mutex. 
  With the mutex in place, the task should be able to read the parameter (parameters) into the local variable (num) 
  before the calling function’s stack memory goes out of scope (the value given by delay_arg).

  When you run the code above as-is, you should see that regardless of what you enter for the number, the task
  will always read 0. That's because the local variable (delay_arg) goes out of scope before the task can read the
  value given by the parameters pointer.

  After adding a mutex, the task should be able to read the parameters variable before the memory location goes
  out of scope. This is what you should see in the Serial terminal if you did it right.
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
// The first is to create a global mutex handle at the top.
static SemaphoreHandle_t mutex;

//*************************************************************************************************
// Tasks

// Blink LED based on rate passed by parameter
void blinkLED(void *parameters) {

  // Copy the parameters into a local variable
  int num = *(int *)parameters;

  // Release the mutex so that the creating function can finish
  // In this task function, we "give" the mutex back just after copying in the value from the parameters pointer.
  xSemaphoreGive(mutex);

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
  
  // Because a mutex will initialize at 1, it means we need to "take" the mutex in our setup code,
  // which we do just after creating it (before we start the task).
  // Create mutex before starting tasks
  mutex = xSemaphoreCreateMutex();

  // Take the mutex
  xSemaphoreTake(mutex, portMAX_DELAY);

  // Start task 1
  xTaskCreatePinnedToCore(blinkLED,
                          "Blink LED",
                          1024,
                          (void *)&delay_arg,
                          1,
                          NULL,
                          app_cpu);

  // After we start the task, we block the "setup and loop" task until the mutex is given back (which is done in the task).
  // We do this by trying to "take" the mutex and delaying (blocking) for the maximum amount of time.
  // Do nothing until mutex has been returned (maximum delay)
  xSemaphoreTake(mutex, portMAX_DELAY);

  // Show that we accomplished our task of passing the stack-based argument
  Serial.println("Done!");
}

void loop() {
  
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}



