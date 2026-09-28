#include <reg51.h>

#define LCD_DATA P2
sbit RS = P3^0;
sbit EN = P3^2;
sbit RED_LED = P3^5;
sbit GRN_LED = P3^4;

unsigned char code keypad[4][3] = {{'1','2','3'},{'4','5','6'},{'7','8','9'},{'*','0','#'}};
unsigned char code row_mask[] = {0xFE, 0xFD, 0xFB, 0xF7}; 
unsigned char code col_mask[] = {0x10, 0x20, 0x40};       

void delay(unsigned int ms) {
    unsigned int i, j;
    for(i=0; i<ms; i++) for(j=0; j<120; j++);
}

void uart_init() { TMOD = 0x20; TH1 = 0xFD; SCON = 0x50; TR1 = 1; }
void send_char(char c) { SBUF = c; while(TI == 0); TI = 0; }
void send_log(char *s) { while(*s) send_char(*s++); }

void lcd_cmd(unsigned char cmd) { LCD_DATA = cmd; RS = 0; EN = 1; delay(1); EN = 0; delay(3); }
void lcd_data(unsigned char dat) { LCD_DATA = dat; RS = 1; EN = 1; delay(1); EN = 0; delay(3); }
void lcd_str(char *s) { while(*s) lcd_data(*s++); }

unsigned char getKey() {
    unsigned char r, c;
    while(1) {
        for(r = 0; r < 4; r++) {
            P1 = row_mask[r];
            for(c = 0; c < 3; c++) {
                if((P1 & col_mask[c]) == 0) {
                    delay(20); // Debounce
                    while((P1 & col_mask[c]) == 0); // Wait for release
                    return keypad[r][c];
                }
            }
        }
    }
}

void main() {
    unsigned char pass[4] = {'3','0','6','5'}; 
    unsigned char input[4];
    unsigned char k, attempts = 0, j;
    signed char i;

    uart_init();
    lcd_cmd(0x38); lcd_cmd(0x0C);
    
    send_log("\r\n==========================\r\n");
    send_log("[ SECURE TERMINAL ACTIVE ]\r\n");
    send_log("==========================\r\n");

    while(1) {
        if(attempts >= 3) {
            lcd_cmd(0x01); lcd_str("SYSTEM LOCKED");
            send_log("\r\n!!! SECURITY ALERT !!!\r\n");
            send_log("BRUTE FORCE DETECTED\r\n");
            send_log("LOCKOUT IN PROGRESS: ");

            for(j = 0; j < 10; j++) { 
                RED_LED = 1; delay(500);
                RED_LED = 0; delay(500);
                send_log("."); 
            }
            
            attempts = 0;
            send_log("\r\nSTATUS: System Reset.\r\n\r\n");
        }

        RED_LED = 0; GRN_LED = 0;
        lcd_cmd(0x01); lcd_str("Enter PIN:");
        lcd_cmd(0xC0); 
        i = 0;

        while(1) {
            k = getKey();

            if(k == '*') { 
                if(i > 0) {
                    i--;
                    lcd_cmd(0xC0 + i); lcd_data(' '); lcd_cmd(0xC0 + i);
                    send_log("LOG: [Backspace]\r\n");
                }
            }
            else if(k == '#') { 
                if(i == 4) break;
                else {
                    send_log("ERR: PIN TOO SHORT\r\n");
                }
            }
            else if(i < 4) { 
                input[i] = k;
                lcd_data('*'); 
                i++;
                send_log("KEY REGISTERED: "); send_char(k); send_log("\r\n");
            }
        }

        lcd_cmd(0x01);
        if(input[0]==pass[0] && input[1]==pass[1] && input[2]==pass[2] && input[3]==pass[3]) {
            lcd_str("ACCESS GRANTED");
            GRN_LED = 1; attempts = 0;
            send_log("RESULT: SUCCESS - PIN MATCH\r\n");
        } else {
            lcd_str("ACCESS DENIED");
            RED_LED = 1; attempts++;
            send_log("RESULT: FAILED - WRONG PIN\r\n");
            send_log("ATTEMPTS LEFT: "); send_char((3 - attempts) + '0'); send_log("\r\n");
        }
        delay(2500);
    }
}