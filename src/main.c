// Sofia Linardou
// Simulation Link https://wokwi.com/projects/481019273412530177

#include <stdint.h>
#include <stdio.h>
#include <stm32l031xx.h>
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void delay(volatile uint32_t dly);
void eputchar(char c);
char egetchar(void);
void eputs(char *String); 
void printDecimal(uint32_t Value);
void ledOn(void);
void ledOff(void); 
void printHex(uint32_t Value);
void initSerial(void);

int main()
{
	initSerial();
	pinMode(GPIOA, 0, 1); // Make GPIOA bit 0 an output
	uint32_t Count = 250;
	char ch;
	while(1)
	{
		// printDecimal(Count);
		printHex(Count);
		// ch = egetchar();
		// if ((ch == 'a') || (ch == 'A'))
		// {
		// 	ledOn();
		// 	delay(500000);
		// }
		// eputchar(ch);
		// eputs("\r\n");
		// ledOff(); 
	}
}

void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
	// This function writes the given Mode bits to the appropriate location for
	// the given BitNumber in the Port specified.  It leaves other bits unchanged
	// Mode values:
	// 0 : digital input
	// 1 : digital output
	// 2 : Alternative function
	// 3 : Analog input
	uint32_t mode_value = Port->MODER; // read current value of Mode register 
	Mode = Mode << (2 * BitNumber);    // There are two Mode bits per port bit so need to shift
																	   // the mask for Mode up to the proper location
	mode_value = mode_value & ~(3u << (BitNumber * 2)); // Clear out old mode bits
	mode_value = mode_value | Mode; // set new bits
	Port->MODER = mode_value; // write back to port mode register
}
void delay(volatile uint32_t dly)
{
	while(dly--);
}
void eputchar(char c)
{
	while( (USART2->ISR & (1 << 7)) == 0); 
	USART2->TDR = c;
}

char egetchar()
{
	// Check if an Overrun Error (ORE) happened (Bit 3 of ISR)
	if (USART2->ISR & (1 << 3)) 
	{
		// Clear the overrun flag so the receiver can breathe again
		// On modern STM32 chips, writing 1 to the OICR bit in ICR clears it
		USART2->ICR |= (1 << 3); 
	}
	// Now wait for a valid character to arrive safely (Bit 5)
	while( (USART2->ISR & (1 << 5)) == 0); 
	return (char)USART2->RDR;
}
void eputs(char *String)
{
	while(*String) 
	{
		eputchar(*String);
		String++;
	}
	// CRITICAL FIX FOR WOKWI: Wait for the last byte to physically leave the chip
	// Bit 6 is TC (Transmission Complete)
	while( (USART2->ISR & (1 << 6)) == 0); 
}
void printDecimal(uint32_t Value)
{
	// create a decimal string array that can store up to 10 digits (the eleventh is the 'stop' bit)
	char DecimalString[11]; // a 32 bit value can go up to about 4 billion
													// That's 10 digits plus a NULL character 
	DecimalString[10] = 0;  // terminate the string // make the last element a zero - ie the stop bit
	int index = 9; // the maximum possible index now is 9.
	while (Value > 0) // counts up to nine
	{
		DecimalString[index] = (Value % 10) + '0'; // get the remainder of the value and add a '0;
		Value = Value / 10; //divide this value by 10 to get decimal value
		index--; // decrement index
	}
	eputs(&DecimalString[index + 1]); // for the index (+1) at which we exit the while, get the location and print
}

void printHex(uint32_t Value)
{
	char HexString[11]; // set up a big array to accomodate hex value
	HexString[10] = 0; // set last elemnt to 0 to terminate - stop bit
	int index = 9; // start with LSB
	while (Value > 0)
	{
		// get remainder of value - this is the first digit
		int remainder = (Value % 16);
		char hex_character;
		if (remainder > 9) // check if that will be an integer or letter
		{ // if remainder > 9, need to map to letters
			switch (remainder)
			{
				case 10:
					hex_character = 'A';
					break;
				case 11:
					hex_character = 'B';
					break;
				case 12:
					hex_character = 'C';
					break;
				case 13:
					hex_character = 'D';
					break;
				case 14:
					hex_character = 'E';
					break;
				case 15:
					hex_character = 'F';
					break;
			}
			// add remainder value to hex string array
			HexString[index] = hex_character; 
		} 
		else
		{
			HexString[index] = remainder; 
		}
		Value = Value / 16;
		index--; // decrement index
	}
	eputs(&HexString[index + 1]); // for the index at which we exit while, get the location and print
}

void initClockHSI16()
{
    // Use the HSI16 clock as the system clock - allows operation down to 1.5V
    RCC->CR &= ~(1 << 24);
    RCC->CR |= (1 << 0); // turn on HSI16 (16MHz clock)
    while ((RCC->CR & (1 << 2))==0); // wait for HSI to be ready
    // set HSI16 as system clock source 
    RCC->CFGR |= (1<<0);
}
void initSerial()
{
	const uint32_t CLOCK_SPEED=16000000;
	const uint32_t BAUD_RATE = 9600;
    initClockHSI16();
	uint32_t BaudRateDivisor;
	RCC->IOPENR |= (1 << 0);  // Turn on GPIOA
	RCC->APB1ENR |= (1 << 17); // Turn on USART2

	pinMode(GPIOA,2,2);  // enable alternative function (UART tx) on PA2
	pinMode(GPIOA,15,2); // enable alternative function (UART rx) on PA15
	
	// turn on pull-ups for PA2 and PA15
	GPIOA->PUPDR |= ( (1 << 4) | (1 << 30));
	GPIOA->PUPDR &= (uint32_t)~(1 << 5);
	GPIOA->PUPDR &= ~((uint32_t)1 << 31);
	// select uart 2 alternative function for PA2 and PA15 
	GPIOA->AFR[0] &= (uint32_t)( (1 << 11) | (1 << 10) | (1 << 9) | (1 << 8));
	GPIOA->AFR[0] |= (1 << 10);
	GPIOA->AFR[1] &= (uint32_t)( ((uint32_t)1 << 31) | (1 << 30) | (1 << 29) | (1 << 28));
	GPIOA->AFR[1] |= (1 << 30);
	BaudRateDivisor = CLOCK_SPEED/BAUD_RATE;
	RCC->APB1RSTR &= (uint32_t)~(1 << 17);
	USART2->CR1 = 0;
	USART2->CR2 = 0;
	USART2->CR3 = (1 << 12); // disable over-run errors
	USART2->BRR = BaudRateDivisor;
	USART2->CR1 = ( (1 << 2) | (1 << 3) );
	USART2->CR1 |= (1 << 0);
}

void ledOn()   
{   
    uint32_t original;   
    original = GPIOA->ODR;   
    original = original | 1;   
    GPIOA->ODR = original;
}

void ledOff()   
{   
    uint32_t original;   
    original = GPIOA->ODR;   
    original = original & ~(1);   
    GPIOA->ODR = original;   
}