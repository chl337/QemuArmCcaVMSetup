#include <stdint.h>
#define UART_FREQUENCY (100 * 1000000)
#define UART_BAUDRATE  (230000)

#define UART_SYSCALL_BASE 	(0x9001000)
#define UART_BASE 	(0x9000000)
#define UART_DR		(0x00 ) //16 bit, data is byte 7:0
#define UART_ECR	(0x04 )
#define UART_RSR	(0x04 )
#define UART_FR 	(0x18 )
#define UART_BRD	(0x24 ) 
#define UART_FBRD	(0x28 ) 
#define UART_LCRH	(0x2c )
#define UART_CR		(0x30 ) 
#define UART_IFLS	(0x34 )
#define UART_IMSC	(0x38 )
#define UART_RIS	(0x3c ) 
#define UART_MIS	(0x40 )
#define UART_ICR 	(0x44 )
#define UART_DMACR	(0x48 ) 

__attribute__((__packed__)) struct uart_data_reg 
{
	uint8_t data;
	uint8_t FE:1;
	uint8_t PE:1;
	uint8_t BE:1;
	uint8_t OE:1;
	uint8_t :4;
};

//status register
//clear everything by writing to clear member
__attribute__((__packed__)) struct uart_status_reg 
{
	uint8_t FE:1; //framing error
	uint8_t PE:1; //parity error
	uint8_t OE:1; //break error
	uint8_t BE:1; //overrun error
	uint8_t :4;
	uint8_t clear;
};

struct uart_flag_reg {	
	uint16_t CTS:1; //clear to send
	uint16_t DSR:1; //data set ready
	uint16_t DCD:1;	//data carrier detect
	uint16_t BUSY:1;	//transmiting
	uint16_t RXFE:1;	//rx fife empty
	uint16_t TXFF:1; //tx fifo full
	uint16_t RXFF:1; //rx fifo full
	uint16_t TXFE:1; //tx framing error
	uint16_t RI:1;	//ring indicator
	uint16_t ignore:9;
};

struct uart_ibrd_reg {
	uint16_t baudrate_int;
};

struct uart_fbrd_reg {
	uint8_t baudrate_frac:6;	
};


#define CalcBaudRateInt(baudrate,freq) ( (freq)/(16*(baudrate)))
#define CalcBaudRateFraq(baudrate,freq) (( (((freq)%(16*(baudrate))) * 64) + (8*freq)) / (16 * freq))

struct uart_lcrh_reg {
	uint8_t BRK:1;//send break;
	uint8_t PEN:1;//parity enable
	uint8_t EPS:1;//even parity =1, odd_parity = 0
	uint8_t STP2:1;//2 stop bits
	uint8_t FEN:1;//enable fifos
	uint8_t WLEN:2;//wlen b11-8 bits, b10->7 bits, b01-> 6 bits, b00->5 bits
	uint8_t SPS:1; //enable fifos 
	uint8_t ignore:8;
};


struct uart_cr_reg
{
	uint8_t uarten:1; //uart enable
	uint8_t siren:1;  //for irda
	uint8_t sirlp:1;  //irda
	uint8_t :4;
	uint8_t lbe:1;//loopback enable
	uint8_t txe:1;//transmit enable
	uint8_t rxe:1;//receive enable
	uint8_t dtr:1;//data transmit ready
	uint8_t rts:1;//request to send
	uint8_t out1:1;//can be used as dcd
	uint8_t out2:1;//can be used as ri
	uint8_t RTSEn:1;//hardware flow control en
	uint8_t CTSEn:1;//cts hardware flow control en
};


/*interrupt control start*/
#define RX_FIFO_1_8_FULL (b000)
#define RX_FIFO_1_4_FULL (b001)
#define RX_FIFO_1_2_FULL (b010)
#define RX_FIFO_3_4_FULL (b011)
#define RX_FIFO_7_8_FULL (b100)

#define TX_FIFO_1_8_FULL (b000)
#define TX_FIFO_1_4_FULL (b001)
#define TX_FIFO_1_2_FULL (b010)
#define TX_FIFO_3_4_FULL (b011)
#define TX_FIFO_7_8_FULL (b100)

struct uart_fifo_ifls
{
	uint16_t txifsel:3; //transmit fifo level
	uint16_t rxifsel:3; //receive interrupt fifo level
	uint16_t ignore:10;
};

struct uart_imsc 
{
	uint8_t RIMIM:1;//modem RI interrupt mask
	uint8_t CTSMIM:1;//modem CTS interrupt mask
	uint8_t DCDMIM:1;//modem dcd interrupt mask
	uint8_t DSRMIM:1;//modem dsr interrupt mask
	uint8_t rxim:1;//receive interrupt mask
	uint8_t txim:1;//tx interrupt mask
	uint8_t rtim:1;//receive timeout intterupt mask
	uint8_t feim:1;//framing error interrupt mask
	uint8_t peim:1;//parity error interrupt mask
	uint8_t beim:1;//break ...
	uint8_t oeim:1;//overrun ...
	uint8_t :5;
};
/*interrupt control register end */





