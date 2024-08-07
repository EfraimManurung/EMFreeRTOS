/*
  Introduction to RTOS Part 6 - Mutex by Shawn Hymel
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

*/

#include <Arduino.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Globals
static int shared_var = 0;
static SemaphoreHandle_t mutex;

//******************************************************************************
// Tasks

// Increment shared variable
void incTask(void *parameters) {

  int local_var;

  // Loop forever
  while(1) {

    // Take mutex prior to critical section
    if (xSemaphoreTake(mutex, 0) == pdTRUE) {

      // Critical section (poor demonstration of "shared_var++")
      local_var = shared_var;
      local_var++;
      vTaskDelay(random(100, 500) / portTICK_PERIOD_MS);
      shared_var = local_var;

      // Print out new shared variable
      // This is different than in the video--print shared_var inside the
      // critical section to avoid having it be changed by the other task.
      Serial.println(shared_var);

      // Give mutex after critical section
      xSemaphoreGive(mutex);
    } else {
      // Do something else while waiting the mutex
    }
  }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)

void setup() {

  // Hack to kinda get randomness
  randomSeed(analogRead(0));

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Race Condition Demo---");

  // Create mutex before starting tasks
  mutex = xSemaphoreCreateMutex();

  // Start task 1
  xTaskCreatePinnedToCore(incTask,
                          "Increment Task 1",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
  
  // Start task 2
  xTaskCreatePinnedToCore(incTask,
                          "Increment Task 2",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
  
  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Execution should never get here
}


