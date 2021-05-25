/*
 * File:   comport.c
 * Author: user
 *
 * Created on 2021年5月24日, 上午 10:08
 */

/* Scheduler include files. */
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"

/* Demo program include files. */
#include "serial.h"
#include "comport.h"

#define ptOUTPUT 	0x3C
#define ptALL_OFF	0x3C

unsigned portBASE_TYPE uxOutput;

#define comSTACK_SIZE				configMINIMAL_STACK_SIZE
#define comTX_LED_OFFSET			( 0 )
#define comRX_LED_OFFSET			( 1 )
#define comTOTAL_PERMISSIBLE_ERRORS ( 2 )

/* The Tx task will transmit the sequence of characters at a pseudo random
interval.  This is the maximum and minimum block time between sends. */
#define comTX_MAX_BLOCK_TIME		( ( TickType_t ) 0x96 )
#define comTX_MIN_BLOCK_TIME		( ( TickType_t ) 0x32 )
#define comOFFSET_TIME				( ( TickType_t ) 3 )

/* We should find that each character can be queued for Tx immediately and we
don't have to block to send. */
#define comNO_BLOCK					( ( TickType_t ) 0 )

/* The Rx task will block on the Rx queue for a long period. */
#define comRX_BLOCK_TIME			( ( TickType_t ) 0xffff )

/* The sequence transmitted is from comFIRST_BYTE to and including comLAST_BYTE. */
#define comFIRST_BYTE				( 'A' )
#define comLAST_BYTE				( 'Z' )

#define comBUFFER_LEN				( ( UBaseType_t ) ( comLAST_BYTE - comFIRST_BYTE ) + ( UBaseType_t ) 1 )
#define comINITIAL_RX_COUNT_VALUE	( 0 )

/* Handle to the com port used by both tasks. */
static xComPortHandle xPort = NULL;

/* The transmit task as described at the top of the file. */
static portTASK_FUNCTION_PROTO(vComTxTask, pvParameters);

/* The receive task as described at the top of the file. */
static portTASK_FUNCTION_PROTO(vComRxTask, pvParameters);

/* The LED that should be toggled by the Rx and Tx tasks.  The Rx task will
toggle LED ( uxBaseLED + comRX_LED_OFFSET).  The Tx task will toggle LED
( uxBaseLED + comTX_LED_OFFSET ). */
static UBaseType_t uxBaseLED = 0;

/* Check variable used to ensure no error have occurred.  The Rx task will
increment this variable after every successfully received sequence.  If at any
time the sequence is incorrect the the variable will stop being incremented. */
static volatile UBaseType_t uxRxLoops = comINITIAL_RX_COUNT_VALUE;

static const uint8_t xString[] = "This is UART1 Tx Test!";

/*-----------------------------------------------------------*/

void vAltStartComTestTasks(UBaseType_t uxPriority, uint32_t ulBaudRate, UBaseType_t uxLED) {
    /* Initialise the com port then spawn the Rx and Tx tasks. */
    uxBaseLED = uxLED;
    xSerialPortInitMinimal(ulBaudRate, comBUFFER_LEN);

    /* The Tx task is spawned with a lower priority than the Rx task. */
    xTaskCreate(vComTxTask, "COMTx", comSTACK_SIZE, NULL, uxPriority - 1, (TaskHandle_t *) NULL);
    //xTaskCreate(vComRxTask, "COMRx", comSTACK_SIZE, NULL, uxPriority, (TaskHandle_t *) NULL);
}

/*-----------------------------------------------------------*/

static portTASK_FUNCTION(vComTxTask, pvParameters) {
    char cByteToSend;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t *pU1String;

    /* Just to stop compiler warnings. */
    (void) pvParameters;

    for (;;) {
        /* Simply transmit a sequence of characters from comFIRST_BYTE to
        comLAST_BYTE. */
        for (pU1String = xString; *pU1String != '\0'; pU1String++) {
            if (xSerialPutChar(xPort, *pU1String, comNO_BLOCK) == pdPASS) {
                vParToggleLED(uxBaseLED + comTX_LED_OFFSET);
            }
        }

        /* Turn the LED off while we are not doing anything. */
        vParSetLED(uxBaseLED + comTX_LED_OFFSET, pdFALSE);

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
} /*lint !e715 !e818 pvParameters is required for a task function even if it is not referenced. */

/*-----------------------------------------------------------*/

static portTASK_FUNCTION(vComRxTask, pvParameters) {
    signed char cExpectedByte, cByteRxed;
    BaseType_t xResyncRequired = pdFALSE, xErrorOccurred = pdFALSE;
    
    /* Just to stop compiler warnings. */
    (void) pvParameters;

    for (;;) {
        /* We expect to receive the characters from comFIRST_BYTE to
        comLAST_BYTE in an incrementing order.  Loop to receive each byte. */
        for (cExpectedByte = comFIRST_BYTE; cExpectedByte <= comLAST_BYTE; cExpectedByte++) {
            /* Block on the queue that contains received bytes until a byte is
            available. */
            if (xSerialGetChar(xPort, &cByteRxed, comRX_BLOCK_TIME)) {
                /* Was this the byte we were expecting?  If so, toggle the LED,
                otherwise we are out on sync and should break out of the loop
                until the expected character sequence is about to restart. */
                if (cByteRxed == cExpectedByte) {
                    vParToggleLED(uxBaseLED + comRX_LED_OFFSET);
                } else {
                    xResyncRequired = pdTRUE;
                    break; /*lint !e960 Non-switch break allowed. */
                }
            }
        }

        /* Turn the LED off while we are not doing anything. */
        vParSetLED(uxBaseLED + comRX_LED_OFFSET, pdFALSE);

        /* Did we break out of the loop because the characters were received in
        an unexpected order?  If so wait here until the character sequence is
        about to restart. */
        if (xResyncRequired == pdTRUE) {
            while (cByteRxed != comLAST_BYTE) {
                /* Block until the next char is available. */
                xSerialGetChar(xPort, &cByteRxed, comRX_BLOCK_TIME);
            }

            /* Note that an error occurred which caused us to have to resync.
            We use this to stop incrementing the loop counter so
            sAreComTestTasksStillRunning() will return false - indicating an
            error. */
            xErrorOccurred++;

            /* We have now resynced with the Tx task and can continue. */
            xResyncRequired = pdFALSE;
        } else {
            if (xErrorOccurred < comTOTAL_PERMISSIBLE_ERRORS) {
                /* Increment the count of successful loops.  As error
                occurring (i.e. an unexpected character being received) will
                prevent this counter being incremented for the rest of the
                execution.   Don't worry about mutual exclusion on this
                variable - it doesn't really matter as we just want it
                to change. */
                uxRxLoops++;
            }
        }
    }
} /*lint !e715 !e818 pvParameters is required for a task function even if it is not referenced. */

/*-----------------------------------------------------------*/

BaseType_t xAreComTasksStillRunning(void) {
    BaseType_t xReturn;

    /* If the count of successful reception loops has not changed than at
    some time an error occurred (i.e. a character was received out of sequence)
    and we will return false. */
    if (uxRxLoops == comINITIAL_RX_COUNT_VALUE) {
        xReturn = pdFALSE;
    } else {
        xReturn = pdTRUE;
    }

    /* Reset the count of successful Rx loops.  When this function is called
    again we expect this to have been incremented. */
    uxRxLoops = comINITIAL_RX_COUNT_VALUE;

    return xReturn;
}

void vParInitialise(void) {
    /* The explorer 16 board has LED's on port A.  All bits are set as output
    so LATA is read-modified-written directly.  Two pins have change 
    notification pullups that need disabling. */
#if !defined(__dsPIC33EP512MU810__)
    CNPU2bits.CN22PUE = 0;
    CNPU2bits.CN23PUE = 0;
#endif
    TRISA &= ~(ptOUTPUT);
    LATA &= ~(ptALL_OFF);
    
}

/*-----------------------------------------------------------*/

void vParSetLED(unsigned portBASE_TYPE uxLED, signed portBASE_TYPE xValue) {
    unsigned portBASE_TYPE uxLEDBit;

    /* Which port A bit is being modified? */
    uxLEDBit = 1 << uxLED;

    if (xValue) {
        /* Turn the LED on. */
        portENTER_CRITICAL();
        {
            LATA |= uxLEDBit;
            /*uxOutput |= uxLEDBit;
            LATA = uxOutput;*/
        }
        portEXIT_CRITICAL();
    } else {
        /* Turn the LED off. */
        portENTER_CRITICAL();
        {
            LATA &= ~uxLEDBit;
            /*uxOutput &= ~uxLEDBit;
            LATA = uxOutput;*/
        }
        portEXIT_CRITICAL();
    }
}

/*-----------------------------------------------------------*/

void vParToggleLED(unsigned portBASE_TYPE uxLED) {
    unsigned portBASE_TYPE uxLEDBit;

    uxLEDBit = 1 << uxLED;
    portENTER_CRITICAL();
    {
        /* If the LED is already on - turn it off.  If the LED is already
        off, turn it on. */
        if (LATA & uxLEDBit) {
            LATA &= ~uxLEDBit;
            /*uxOutput &= ~uxLEDBit;
            LATA = uxOutput;*/
        } else {
            LATA |= uxLEDBit;
            /*uxOutput |= uxLEDBit;
            LATA = uxOutput;*/
        }
    }
    portEXIT_CRITICAL();
}