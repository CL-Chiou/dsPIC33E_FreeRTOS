// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef COMPORT_H
#define	COMPORT_H

void vAltStartComTasks( UBaseType_t uxPriority, uint32_t ulBaudRate, UBaseType_t uxLED );
BaseType_t xAreComTasksStillRunning( void );

void vParInitialise(void);
void vParSetLED(UBaseType_t uxLED, BaseType_t xValue);
void vParToggleLED(UBaseType_t uxLED);

#endif	/* COMPORT_H */

