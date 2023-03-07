#include "project.h"
#define TRANSMIT_BUFFER_SIZE 16

char TransmitBuffer[TRANSMIT_BUFFER_SIZE];
char TransmitBuffer2[TRANSMIT_BUFFER_SIZE];

uint32 output;
uint32 ch;
uint8_t s_boot = 0;

enum states
{
    SM_POWER_ON,
    SM_INIT,
    SM_WAIT,
    SM_SEND_DATA,
    //SM_ERROR,
}SysState;

 enum events
{
    POWER_ON,
    INIT_OK_EVENT,
    POWER_OFF_EVENT,
    REQ_DATA,
    SEND_DATA,
    ERROR,
}SysEvent;

void stepIntoState(enum events SysEvent)
{
    switch(SysState)
    {
        case SM_POWER_ON :
        switch(SysEvent)
        {
            case POWER_ON:
            UART_1_Start();
            CyDelay(100);
            SysState = SM_INIT;
            UART_1_PutString("\n\rSystem ON\r\nCommunication Enabled\r\nInit State -> 1 for System Initialization\r\n");
            break;
            
            default: 
            break;
        }
        break;
        
        case SM_INIT : 
        switch(SysEvent)
        {
           case INIT_OK_EVENT : 
           UART_1_Wakeup();
           // ADC_SAR_Wakeup();
           PWM_Start();
           CyDelay(100);
           UART_1_PutString("Initialization Done\r\nWait State -> 2 for Request Data\r\n           -> 3 for Sending Data\r\n");
           SysState = SM_WAIT;
           break;
        
           default:
           break;
        }
        break;
        
        case SM_WAIT:
        switch(SysEvent)
        {    
           
            case REQ_DATA : 
            UART_1_PutString("Requesting data... \r\n");
            ADC_SAR_Start();
            ADC_SAR_StartConvert();
            output = ADC_SAR_CountsTo_mVolts(ADC_SAR_GetResult16());
           // PWM = Pin_1_Read();
            SysState = SM_SEND_DATA;
            break;
            
            case POWER_OFF_EVENT :
            PIN_LED_Write(1);
            UART_1_PutString("\n\rData module OFF\r\nInit State -> 1 for System Initialization\r\n");
            CyDelay(100);  
            PWM_Sleep();
            ADC_SAR_Sleep();
            CyDelay(100);
            SysState = SM_INIT;
            break;
            
            default:
            break;
        }
        break;
        
        case SM_SEND_DATA:
        switch(SysEvent)
        {
            case SEND_DATA: 
            sprintf(TransmitBuffer,"[%lu]\r\n",output);
            UART_1_PutString("Sending data...\r\n");
            UART_1_PutString(TransmitBuffer);
            if(output != 4999 && output != 0)
            {
                UART_1_PutString("Erorr \r\n");
                SysState = SM_INIT;
            }
            
            SysState = SM_WAIT;
            break;
            
            default:
            break;
        }
        break;
        
        default:
        break;
    }
}

CY_ISR(PIN_SW_Handler)
{
    if(s_boot == 0)
    {
        PIN_LED_Write(~PIN_LED_Read());
        s_boot++;
    };
    
    if(SysState == SM_WAIT)
    {
        PIN_LED_Write(0);
        CyDelay(500);
        stepIntoState(POWER_OFF_EVENT);
    }
    PIN_SW_ClearInterrupt();
};

int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    PIN_SW_INT_StartEx(PIN_SW_Handler);
    while(!PIN_LED_Read()){}
    stepIntoState(POWER_ON);
    
    for(;;)
    {    
        ch = UART_1_GetChar();
        
        switch(ch)
        {
            case '1' :
            stepIntoState(INIT_OK_EVENT);
            break;
             
            case '2':
            stepIntoState(REQ_DATA);
            break;
            
            case '3':
            stepIntoState(SEND_DATA);
            break;
            
            case '4':
            stepIntoState(POWER_OFF_EVENT);
            break;
            
            default:
            break;
        } 
    }
}

/* [] END OF FILE */
