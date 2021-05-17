



int kern_init(void) __attribute__((noreturn));
void grade_backtrace(void);


int kern_init(){
    extern char edata[], end[];
}