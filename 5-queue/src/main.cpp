/*
  Introduction to RTOS Part 5 - Queue by Shawn Hymel
  URL: https://www.youtube.com/watch?v=pHJ3lxOoWeI&list=PLXyB2ILBXW5FLc7j2hLcX6sAGbmH0JxX8&index=5
   
  Efraim Manurung, 4th August 2024
  Version 1.0
  
  Concepts from URL: https://www.digikey.nl/en/maker/projects/introduction-to-rtos-solution-to-part-5-freertos-queue-example/72d2b361f7b94e0691d947c7c29a03c9

  Queue material:
  A queue in a real-time operating system (RTOS) is a kernel object that is capable of passing information between tasks
  without incurring overwrites from other tasks or entering into a race contion. A queue is a first in, first out (FIFO)
  system where items are removed from the queue once read.

  Most multi-threaded operating systems offer a number of kernel objects that assist in creating thread-safe communications between
  threads. Such objects include things like queues, mutexes, and semaphores (which we will cover in a later lecture).

  A queue is a simple FIFO system with atomic reads and writes. "Atomic operations" are those that cannot be interrupted by other tasks
  during their execution. This ensures that another task cannot overwrite our data before it is read by the intended target.

  In this example, Task A writes some data to a queue. No other thread can interrupt Task A during that writing process. After, Task B can
  write some other piece of data to the queue. Task B's data will appear behind Task A's data, as the queue is a FIFO system.

  Note that in FreeRTOS, information is copied into a queue by value and not by reference. That means if you use the xQueueSend()
  function to send a piece of data to a queue, all of the data will be copied into the queue atomically.

  This can be helpful if you believe your data will go out of scope prior it to it being read, but it might require a long copying 
  time if it's large piece of data, like a string. While you can copy pointers to a queue, you will want to be sure that the data that
  is being pointed to is read prior to changing it. 
*/

#include <Arduino.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1; 
#endif

// Settings
static const uint8_t msg_queue_len = 5;

// Globals
static QueueHandle_t msg_queue;

//************************************************************
// Tasks

// Task: wait for item on queue and print it
void printMessages(void *parameters) {

  int item;

  // Loop forever
  while (1) {
    
    // See if there's message in the queue (do not black)
    if (xQueueReceive(msg_queue, (void *)&item, 0) == pdTRUE) {
      Serial.print("xQueueReceive: ");
      Serial.println(item);
    }
    //Serial.println(item);

    // Wait before
    vTaskDelay(500 / portTICK_RATE_MS);
  }
}

void setup() {

  // Configure serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Queue Demo---");

  // Create queue
  msg_queue = xQueueCreate(msg_queue_len, sizeof(int));

  // Start print task
  xTaskCreatePinnedToCore(printMessages,
                          "Print Messages",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
}

void loop() {
  
  static int num = 0;

  // Try to add item to queue for 10 ticks, fail if queue is full
  if (xQueueSend(msg_queue, (void*)&num, 10) != pdTRUE) {
    Serial.println("Queue full");
  }

  // Increment num variable
  num++;

  // Wait before trying again
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}