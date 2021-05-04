/* 
 * File:   main.c
 * Author: Charlie
 *
 * Created on 2021年5月2日, 下午 8:26
 */

/* Standard includes. */
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"


/*
 * The check task as described at the top of this file.
 */
static void vToggleLEDTask(void *pvParameters);

/*
 * 
 */
int main(void) {
    /* Create the test tasks defined within this file. */
    xTaskCreate(vToggleLEDTask, "ToggleLED", 105, NULL, 1, NULL);
    /* Finally start the scheduler. */
    vTaskStartScheduler();
    // Will not get here unless there is insufficient RAM.
    return 0;
}

static void vToggleLEDTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT(((uint32_t) pvParameters) == 1);
    for (;;) {
        Nop();
        vTaskDelayUntil(&xLastWakeTime, (1 / portTICK_PERIOD_MS));
    }
    vTaskDelete(NULL);
}

void vApplicationIdleHook(void) {
    /* Schedule the co-routines from within the idle task hook. */
    vCoRoutineSchedule();
}

