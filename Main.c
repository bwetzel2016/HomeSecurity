#include <msp430.h>
#include <inttypes.h>


#define BUZZER_LED_PIN BIT0
#define PIR_PIN BIT1


#define CMD         0
#define DATA        1
#define LCD_OUT     P2OUT
#define LCD_DIR     P2DIR
#define D4          BIT4
#define D5          BIT5
#define D6          BIT6
#define D7          BIT7
#define RS          BIT2
#define EN          BIT3

#define ROWS    4
#define COLS    3

#define ROW_PORT    P1IN
#define COL_PORT    P1OUT

// Define the pin configuration for the keypad rows and columns
#define ROW1    BIT0
#define ROW2    BIT1
#define ROW3    BIT2
#define ROW4    BIT3
#define COL1    BIT4
#define COL2    BIT5
#define COL3    BIT6


// Delay function for producing delay in 0.1 ms increments
void delay(uint8_t t)
{
    uint8_t i;
    for (i = t; i > 0; i--) {
        __delay_cycles(100);
    }
}

// Function to pulse EN pin after data is written
void pulseEN(void)
{
    LCD_OUT |= EN;   delay(1); LCD_OUT &= ~EN;  delay(1);
}

// Function to write data/command to LCD
void lcd_write(uint8_t value, uint8_t mode)
{
    if (mode == CMD) {
        LCD_OUT &= ~RS;
    } else {
        LCD_OUT |= RS;
    }

    LCD_OUT = ((LCD_OUT & 0x0F) | (value & 0xF0));
    pulseEN();
    delay(1);
    LCD_OUT = ((LCD_OUT & 0x0F) | ((value << 4) & 0xF0));
    pulseEN();
    delay(1);
}

// Function to print a string on LCD
void lcd_print(char *s)
{
    while (*s) {
        lcd_write(*s, DATA);
        s++;
    }
}

// Function to move cursor to desired position on LCD
void lcd_setCursor(uint8_t row, uint8_t col)
{
    const uint8_t row_offsets[] = {0x00, 0x40};
    lcd_write(0x80 | (col + row_offsets[row]), CMD);
    delay(1);
}

// Initialize LCD
void lcd_init()
{
    P2SEL &= ~(BIT6 + BIT7);
    LCD_DIR |= (D4 + D5 + D6 + D7 + RS + EN);
    LCD_OUT &= ~(D4 + D5 + D6 + D7 + RS + EN);

    delay(150);                             // Wait for power up ( 15ms )
    lcd_write(0x33, CMD);
    delay(50);                              // Initialization Sequence 1
    lcd_write(0x32, CMD);
    delay(1);                               // Initialization Sequence 2
    lcd_write(0x28, CMD);
    delay(1);                               // 4 bit mode, 2 line
    lcd_write(0x0F, CMD);
    delay(1);
    lcd_write(0x01, CMD);
    delay(20);                              // Clear screen
    lcd_write(0x06, CMD);
    delay(1);
    lcd_setCursor(0, 0);
}



// Define the keypad layout
char keypad[ROWS][COLS] =
{
  {'3', '2', '1'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
  };


// Function to initialize the keypad
void keypad_init()
{
    // Set the rows as inputs with internal pull-ups
    P1DIR &= ~(ROW1 | ROW2 | ROW3 | ROW4);
    P1REN |= (ROW1 | ROW2 | ROW3 | ROW4);
    P1OUT |= (ROW1 | ROW2 | ROW3 | ROW4);

    // Set the columns as outputs and drive them low
    P1DIR |= (COL1 | COL2 | COL3);
    P1OUT &= ~(COL1 | COL2 | COL3);
}


// Function to read the keypad and return the pressed key
char keypad_read()
{
    uint8_t r, c;

    for (c = 0; c < COLS; c++) {
        // Set the current column as output low (grounding the column)
        P1DIR |= (COL1 | COL2 | COL3);
        P1OUT &= ~(COL1 | COL2 | COL3);

        // Set other columns as inputs with pull-up resistors
        if (c == 0) {
            P1DIR &= ~COL1;
            P1OUT |= COL1;
        } else if (c == 1) {
            P1DIR &= ~COL2;
            P1OUT |= COL2;
        } else if (c == 2) {
            P1DIR &= ~COL3;
            P1OUT |= COL3;
        }

        // Delay briefly to allow the port to settle
        __delay_cycles(5000);

        // Check the rows for a pressed key
        for (r = 0; r < ROWS; r++) {
            if (!(ROW_PORT & (1 << r))) {

                while (!(ROW_PORT & (1 << r)))
                    ;
                // Return the corresponding character
                return keypad[r][c];
            }
        }
    }

    return '\0'; // No key pressed
}

void initialize()
{
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer

    P2DIR |= BUZZER_LED_PIN; // Set P1.0 (buzzer and LED pin) as output
    P2DIR &= ~PIR_PIN;       // Set P1.3 (PIR sensor pin) as input

    P2REN |= PIR_PIN;  // Enable pull-up/pull-down resistor
    P2OUT |= PIR_PIN;  // Set pull-up resistor

    P2IES |= PIR_PIN;  // Set P1.3 interrupt to trigger on falling edge
    P2IE |= PIR_PIN;   // Enable P1.3 interrupt
    P2IFG &= ~PIR_PIN; // Clear P1.3 interrupt flag

    __enable_interrupt(); // Enable interrupts
}

void tone(unsigned int frequency, unsigned int duration)
{
    unsigned int i;
    unsigned int half_period = (1000000 / frequency) / 2;

    for (i = 0; i < duration * frequency / 1000; i++)
    {
        P2OUT |= BUZZER_LED_PIN; // Turn on the buzzer/LED pin
        __delay_cycles(5000);
        P2OUT &= ~BUZZER_LED_PIN; // Turn off the buzzer/LED pin
        __delay_cycles(5000);
    }
}

#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
    if (P2IFG & PIR_PIN) // PIR motion detected
    {
        int i;
        for (i = 1; i > 0; i++)
        {
            tone(5000, 50000);    // Play a tone at 1000Hz for 200ms
            __delay_cycles(1000); // Delay for 1 second
        }

        P2OUT |= BUZZER_LED_PIN; // Turn on the buzzer/LED continuously

        P2IFG &= ~PIR_PIN; // Clear P1.3 interrupt flag
    }
}




void main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // stop watchdog
    initialize();
    lcd_init();
    lcd_setCursor(0, 0);
    lcd_print("Home Security");
    lcd_setCursor(1, 2);
    lcd_print("Code: ");

    keypad_init();

    char input_buffer[2]; // Buffer to store the keypad input
    uint8_t input_length = 0; // Keeps track of the number of digits entered

    while (1) {
        // Check for keypad input
        char key = keypad_read();
        if (key != '\0') {
            // Handle the keypad input
            // For example, display the pressed key on the LCD
            lcd_setCursor(1, 8 + input_length); //Position after "P: "
            lcd_write(key, DATA);

            if (key == '#') {

                input_buffer[input_length] = '\0'; // Null-terminate the string
                input_length = 0; // Reset the input length
            } else if (input_length < sizeof(input_buffer) - 1) {
                // Add the entered digit to the buffer if it is not full yet
                input_buffer[input_length++] = key;
            }

            __delay_cycles(100000);


        }
    }
}"