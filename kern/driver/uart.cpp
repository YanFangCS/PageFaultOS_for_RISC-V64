#include "libs/types.h"
#include "libs/param.h"
#include "mm/memlayout.h"
#include "libs/riscv.h"
#include "sync/spinlock.h"
#include "proc/proc.h"
#include "trap/intr.h"
#include "console/console.h"
#include "libs/printf.h"

#define Reg(reg) ((volatile unsigned char *)(UART + reg))

extern volatile int panicked;

#define RHR                 0                 // receive holding register (for input bytes)
#define THR                 0                 // transmit holding register (for output bytes)
#define IER                 1                 // interrupt enable register
#define IER_TX_ENABLE       (1<<0)
#define IER_RX_ENABLE       (1<<1)
#define FCR                 2                 // FIFO control register
#define FCR_FIFO_ENABLE     (1<<0)
#define FCR_FIFO_CLEAR      (3<<1)            // clear the content of the two FIFOs
#define ISR                 2                 // interrupt status register
#define LCR                 3                 // line control register
#define LCR_EIGHT_BITS      (3<<0)
#define LCR_BAUD_LATCH      (1<<7)            // special mode to set baud rate
#define LSR                 5                 // line status register
#define LSR_RX_READY        (1<<0)            // input is waiting to be read from RHR
#define LSR_TX_IDLE         (1<<5)            // THR can accept another character to send

#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

struct spinlock uart_tx_lock;
#define UART_TX_BUF_SIZE    32
char uart_tx_buf[UART_TX_BUF_SIZE];
int uart_tx_w;
int uart_tx_r;


void uartStart();

void uartInit(void){
    // disable interrupts.
    WriteReg(IER, 0x00);

    // special mode to set baud rate.
    WriteReg(LCR, LCR_BAUD_LATCH);

    // LSB for baud rate of 38.4K.
    WriteReg(0, 0x03);

    // MSB for baud rate of 38.4K.
    WriteReg(1, 0x00);

    // leave set-baud mode,
    // and set word length to 8 bits, no parity.
    WriteReg(LCR, LCR_EIGHT_BITS);

    // reset and enable FIFOs.
    WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

    // enable transmit and receive interrupts.
    WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);

    uart_tx_w = uart_tx_r = 0;

    initlock(&uart_tx_lock, (const char *)"uart");
}


void uartputc(int c){
    acquire(&uart_tx_lock);

    if(panicked){
        for(;;) ;
    }

    while(1){
        if(((uart_tx_w + 1) % UART_TX_BUF_SIZE) == uart_tx_r){
            sleep(&uart_tx_r, &uart_tx_lock);
        } else {
            uart_tx_buf[uart_tx_w] = c;
            uart_tx_w = (uart_tx_w + 1) % UART_TX_BUF_SIZE;
            uartStart();
            release(&uart_tx_lock);
            return;
        }
    }
}

void uartputc_sync(int c){
    push_off();

    if(panicked){
        for( ; ; ) ;
    }

    while((ReadReg(LSR) & LSR_TX_IDLE) == 0) ;

    WriteReg(THR, c);

    pop_off();
}

void uartStart(){
    while(1){
        if(uart_tx_w == uart_tx_r){
            return;
        }

        if((ReadReg(LSR) & LSR_TX_IDLE) == 0){
            return;
        }

        int c = uart_tx_buf[uart_tx_r];
        uart_tx_r = (uart_tx_r + 1) % UART_TX_BUF_SIZE;

        wakeup(&uart_tx_r);

        WriteReg(THR, c);
    }
}

int uartgetc(void){
    if(ReadReg(LSR) & 0x01){
        return ReadReg(RHR);
    } else {
        return -1;
    }
}

void uartIntr(void){
    while(1){
        int c = uartgetc();
        if(c == -1) break;
        consoleintr(c);
    }

    acquire(&uart_tx_lock);
    uartStart();
    release(&uart_tx_lock);
}