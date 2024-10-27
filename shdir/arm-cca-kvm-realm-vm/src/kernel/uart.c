#include "uart.h"
#include "pl011.h"
extern uint64_t get_shared_bit();
uint64_t pa = UART_BASE;
uint64_t base = UART_BASE;
void uart_init(uint64_t _base)
{
	struct uart_cr_reg control_reg = {0};	
	struct uart_lcrh_reg lcrh_reg  = {0};	
	struct uart_ibrd_reg int_baud_reg = {0};
	struct uart_fbrd_reg  frac_baud_reg = {0};

	//set baud rate	
	int_baud_reg.baudrate_int = CalcBaudRateInt(UART_BAUDRATE, UART_FREQUENCY);
	frac_baud_reg.baudrate_frac = CalcBaudRateFraq(UART_BAUDRATE, UART_FREQUENCY);

	//setup the control register
	control_reg.txe = 1; 
	control_reg.rxe = 1; 
	control_reg.uarten = 1; 

	//setup 8 bits, no parity, 1 stop bit
	lcrh_reg.WLEN = 0b11;//8 bits
	lcrh_reg.STP2 = 0; // 1 stop bit

	base = _base ;

	((struct uart_status_reg*) (base +UART_RSR))->clear = 0;
	*((struct uart_lcrh_reg *) (base +UART_LCRH)) = lcrh_reg;	
	*((struct uart_ibrd_reg *) (base +UART_BRD)) = int_baud_reg;
	*((struct uart_fbrd_reg *) (base +UART_FBRD)) = frac_baud_reg;
	*((struct uart_cr_reg*) (base+ UART_CR)) = control_reg;	
	//clear status register
};

/*return 0 on failure, 1 on success*/

uint8_t  uart_send_byte(uint8_t c)
{
	while(1) {
		if( !(((struct uart_flag_reg*)(base + UART_FR))->TXFF) ){
			*((uint8_t*)(base + UART_DR)) = c;
			return 1;
		}
	}
	return 0;
};

uint8_t uart_has_char()
{
	return ((struct uart_flag_reg*)(base + UART_FR))->RXFE;
}
uint8_t uart_get_byte()//uint8_t *error)
{
	while(uart_has_char() == 0) {
	}
	return *((uint8_t*)(base + UART_DR));	
};

uint32_t uart_send_byte_array(uint8_t array[], uint32_t len)
{
	uint32_t count = 0;
	for(;count < len;count++) {
		if( uart_send_byte(array[count]) == 0)
			break;
	}
	return count;
};

uint64_t get_uart_base()
{
	return pa | get_shared_bit();
}
