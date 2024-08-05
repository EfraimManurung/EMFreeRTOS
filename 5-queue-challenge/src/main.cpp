/*
  Introduction to RTOS Part 5 - Queue Challenge by Shawn Hymel
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

  Challenge:
  Task A should print any new message it receives from Queue 2. Additionally, it should read any Serial input from the user and echo
  back this input to the serial input. If the user enters "delay" followed by a space and a number, it should send that number to Queue 1.

  Task B should read any messages from Queue 1. If it contains a number, it should update its delay rate to that number (milliseconds). It
  should also blink an LED at rate specified by that delay. Additionally, every time the LED blinks 100 times, it should send the string
  "Blinked" to Queue 2. You can also optionally send the number of times the LED blinked (e.g. 100) as part of struct that encapsulates the
  string and this number.

  The solution provided by Shawn Hymel
  https://github.com/ShawnHymel/introduction-to-rtos/blob/main/05-queue/esp32-freertos-05-solution-queue/esp32-freertos-05-solution-queue.ino
*/

#include <Arduino.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint8_t buf_len = 255;     // Size of buffer to look for command
static const char command[] = "delay "; // Note that space!
static const int delay_queue_len = 5;   // Size of delay_queue
static const int msg_queue_len = 5;     // Size of msg_queue
static const uint8_t blink_max = 100;   // Num times to blink before message

// Pins (change this if your Arduino board does not have LED_BUILTIN defined)
static const int led_pin = LED_BUILTIN;

// Message struct: used to wrap strings (not necessary, but it's useful to see
// how to use structs here)

/*
The settings and LED pin should look pretty familiar at this point. We used a struct as a way
to encapsulate multiple, related pieces of data that need to be copied to Queue 2.
*/

typedef struct Message {
  char body[20];
  int count;
} Message;

// Globals
/*
Two separate queues, so we create those as global variables
*/
static QueueHandle_t delay_queue;
static QueueHandle_t msg_queue;

//******************************************************************************
// Tasks

/*
In Task A (which I call with the doCLI() function), I use a char buffer to record things
from the Serial terminal. You're welcome to implement this in a number of different ways,
but I examine serial input character-by-character.

The important thing there is that if we see something in Queue 2, it should be printed to the
console. Notice that I do not block witing for something in Queue 2. If nothing is there, the task
simply moves on.
*/

// Task: command line interface (CLI)
void doCLI(void *parameters) {

  // Initialize struct and variables
  Message receive_message;
  char c;
  char buf[buf_len];
  uint8_t idx = 0;

  /*
  `strlen` is a function in the C standard library that computes the length of a C-style null-terminated string.
  The functino is defined in  the `<cstring>` header in C++ and `<string.h>` in C.
  */
  uint8_t cmd_len = strlen(command);
  int led_delay;

  // Clear whole buffer
  memset(buf, 0, buf_len);

  // Loop forever
  while (1) {

    // See if there's a message in the queue (do not block)
    if (xQueueReceive(msg_queue, (void *)&receive_message, 0) == pdTRUE) {
      Serial.print(receive_message.body);
      Serial.println(receive_message.count);
    }

    // Read characters from serial
    if (Serial.available() > 0) {
      c = Serial.read();

      // Store received character to buffer if not over buffer limit
      if (idx < buf_len - 1) {
        buf[idx] = c;
        idx++;
      }

      // Print newline and check input on 'enter'
      if ((c == '\n') || (c == '\r')) {

        // Print newline to terminal
        Serial.print("\r\n");

        /*
        We checked and echoed any characters received on the serial port. If the first string is "delay", I parse it
        and then send the number after it to Queue 1. 

        The C library memcmp() function can be used to compare two blocks of memory. In general,
        it is used compare the binary data or raw data.

        Here, memcmp() accepts three variables as parameters that compares the first n bytes of memory
        area str1 and memory area str2

        int memcmp(const void *str1, const void *str2, size_t n)
        */

        // Check if the first 6 characters are "delay "
        if (memcmp(buf, command, cmd_len) == 0) {

          // Convert last part to positive integer (negative int crashes)
          char* tail = buf + cmd_len;
          led_delay = atoi(tail);
          led_delay = abs(led_delay);

          // Send integer to other task via queue
          if (xQueueSend(delay_queue, (void *)&led_delay, 10) != pdTRUE) {
            Serial.println("ERRORL Could not put item on delay queue.");
          }
        }

        // Reset receive buffer and index counter
        memset(buf, 0, buf_len);
        idx = 0;

      // Otherwise, echo character back to serial terminal
      } else {
        Serial.print(c);
      }
    }
  }
}

/*
In Task B, I set up the LED pin as output. In the while loop, I first check to see if there are any
messages in Queue 1. Once again, it is non-bloking to allow the rest of the task to execute if 
nothing is present in the queue.
*/

// Task: flash LED based on delay provided, notify other task every 100 blinks
void blinkLED(void *parameters) {

  Message msg;
  int led_delay = 500;
  uint8_t counter = 0;

  // Set up pin
  pinMode(LED_BUILTIN, OUTPUT);

  // Loop forever
  while (1) {

    // See if there's a message in the queue (do not block)
    if (xQueueReceive(delay_queue, (void*)&led_delay, 0) == pdTRUE) {
      /*
      The function prototype of strcpy() is
      char* strcpy(char* destination, const char* source);
      - The strcpy() function copies the string pointer by source (including the null character) to the destination.
      - The strcpy() function also return the copied string.
      */

      // Best practice: use only one task to manage serial comms
      strcpy(msg.body, "Message received ");
      msg.count = 1;
      xQueueSend(msg_queue, (void *)&msg, 10);
    }

    // Blink
    digitalWrite(led_pin, HIGH);
    vTaskDelay(led_delay / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(led_delay / portTICK_PERIOD_MS);

    /*
    If something is in the queue, we read it, and it updates the led_delay variable. Note that if nothing is in
    the queue, led_delay is not changed. 

    Additionally, I send a message back to Task A using Queue 2. This is not required, but I did it as a 
    test to show that a message was received in Queue 1. I send a simple string and set the count
    member to 1 (some arbitrary value).

    The task blinks the LED once and updates a counter. When the counter hits 100, it is reset, and a message is 
    sent back to Task A using Queue 2. 
    */

    // If we've blinked 100 times, send a message to the other task
    counter++;
    if (counter >= blink_max) {

      // Construct message and send
      strcpy(msg.body, "Blinked: ");
      msg.count = counter;
      xQueueSend(msg_queue, (void *)&msg, 10);

      // Reser counter
      counter = 0;
    }
  }
}

/*
The setup() is straightforward: we create two queues, assign them to the global handles, and 
then start the two tasks.

There is one big flaw with this design: the blink section is blocking! Task B must wait for an
entire blink cycle before sending or reading from the queues. This can be devastating if a user
has to wait a long time before seeing their input take some aciton.

Perhaps, afterall, having a global, shared delay variable was the way to go. 
However, this was just an exercise in using queues, nothing more. Using shared resources is still a viable method, 
but we might need different ways to protect them from being overwritten by other threads. 
That’s where mutexes and semaphores come in, which we’ll discuss in future episodes.
*/

void setup() {

  // Configure Serial
  Serial.begin(115200);

  // Wait a momentto start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Queue Solution---");
  Serial.println("Enter the command 'delay xxx' where xxx is your desired ");
  Serial.println("LED blink delay time in milliseconds");

  // Create queues
  delay_queue = xQueueCreate(delay_queue_len, sizeof(int));
  msg_queue = xQueueCreate(msg_queue_len, sizeof(Message));

  // Start CLI task
  xTaskCreatePinnedToCore(doCLI,
                          "CLI",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);

  // Start blink task
  xTaskCreatePinnedToCore(blinkLED,
                          "Blink LED",
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
