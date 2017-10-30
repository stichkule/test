#include <stdint.h>
#include "system_MKL25Z4.h"
#include "MKL25Z4.h"
#include "project2.h"
#include "circular_buffer.h"
#include "uart.h"

#define BAUD_RATE 38400
#define OSR 0x0F
# define BUS_CLK (DEFAULT_SYSTEM_CLOCK)
//#define BUS_CLK ((DEFAULT_SYSTEM_CLOCK)/(((SYSTEM_SIM_CLKDIV1_VALUE&SIM_CLKDIV1_OUTDIV4_MASK)>>SIM_CLKDIV1_OUTDIV4_SHIFT)+0x1))
#define BAUD_RATE_DIV ((BUS_CLK)/((OSR+1)*BAUD_RATE))
#define BAUD_RATE_DIV_H ((BAUD_RATE_DIV>>8)&UART0_BDH_SBR_MASK)
#define BAUD_RATE_DIV_L ((BAUD_RATE_DIV&UART0_BDL_SBR_MASK))

UART_status UART_configure(void)
{
  /* Disable Transmit and Receive during configuration */
  //UART0->C2 &= ~(UART0_C2_TE_MASK & UART0_C2_RE_MASK);

  /* Set clock source */
  //MCG->C1 |= MCG_C1_IRCLKEN(1);
  SIM->SOPT2 |= SIM_SOPT2_UART0SRC(0x1);

  SIM->SCGC4 |= SIM_SCGC4_UART0(1);

  /* Enable Port A clock */
  SIM->SCGC5 |= SIM_SCGC5_PORTA(1);

  /* Enable UART0 pins to Alternative Pin 2 */
  PORTA->PCR[1] = PORT_PCR_MUX(0x2);
  PORTA->PCR[2] = PORT_PCR_MUX(0x2);

  /* Set high and low bits of the SBR (Baud rate clock divider) */
  UART0->BDH |= UART0_BDH_SBR(BAUD_RATE_DIV_H);
  UART0->BDL = UART0_BDL_SBR(BAUD_RATE_DIV_L);

  /* Set 1 stop bit */
  UART0->BDH &= ~(UART0_BDH_SBNS_MASK);
  
  /* Set 8 Bit Transmit and No Parity Bit */
  UART0->C1 &= ~(UART0_C1_M_MASK | UART0_C1_PE_MASK);

  /* Set the oversampling ratio */
  UART0->C4 |= (UART0_C4_OSR(OSR));
  
  /* Re-enable transmit and receive */
  UART0->C2 |= (UART0_C2_TE_MASK | UART0_C2_RE_MASK);
  
  /* Enable receive interrupt */
  UART0->C2 |= (UART0_C2_RIE_MASK);

  return UART_SUCCESS;
}

UART_status UART_send(uint8_t * data)
{
  CB_status rv=0;
  while(!(UART0->S1&UART0_S1_TDRE_MASK))
  {
    //NOP
  }
  rv = CB_buffer_remove_item(buffer, data);
  if(rv)
  {
    return UART_FAILED;
  }
  UART0->D = *data;
  return UART_SUCCESS;
}

UART_status UART_send_n(uint8_t * data, uint16_t length)
{
  for(uint16_t i=0; i<length; i++)
  {
    if(UART_send(data+i)==UART_FAILED)
    {
       return UART_FAILED;
    }
  }
  return UART_SUCCESS;
}

UART_status UART_receive(uint8_t * data)
{
  CB_status rv;
  while(!(UART0->S1&UART0_S1_RDRF_MASK))
  {
    //NOP
  }
  *data = UART0->D;
  rv = CB_buffer_add_item(buffer, *data);
  if(rv)
  {
    return UART_FAILED;
  }
  return UART_SUCCESS;
}

UART_status UART_receive_n(uint8_t * data, uint16_t length)
{ 
  for(uint16_t i=0; i<length; i++)
  {
    if(UART_receive(data)==UART_FAILED)
    {
      return UART_FAILED;
    }
  }
  return UART_SUCCESS;
}

void UART0_IRQHandler(void){
  if(UART0->S1 & UART0_S1_TDRE_MASK)
  {
     UART_send(data);
  }
  if(UART0->S1 & UART0_S1_RDRF_MASK)
  {    
    UART_receive(data);
  }
}