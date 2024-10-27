#ifndef _UART_H
#define _UART_H
#include <stdint.h>
void uart_init(uint64_t _base);
uint8_t  uart_send_byte(uint8_t c);
uint8_t  uart_has_char();
uint8_t  uart_get_byte();//uint8_t *error)
uint32_t uart_send_byte_array(uint8_t array[], uint32_t len);
uint64_t get_uart_base();
void uart_syscall_init(uint64_t _base);
uint64_t get_uart_syscall_base();
uint64_t uart_syscall_write(uint64_t value);
#endif
