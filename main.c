/*******************************************************************************
* Header Files
*******************************************************************************/
#if defined (CY_USING_HAL)
#include "cyhal.h"
#endif

#include "cybsp.h"
#include "cy_retarget_io.h"
#include "matrix_keypad.h"
#include <stdio.h>

/*******************************************************************************
* Macros
*******************************************************************************/
#define KEYPAD_ROWS     (4UL)
#define KEYPAD_COLUMNS  (4UL)

// TM1637 Pin Definitions
#define TM1637_CLK  P10_0
#define TM1637_DIO  P10_1

// TM1637 Commands
#define TM1637_I2C_COMM1    0x40
#define TM1637_I2C_COMM2    0xC0
#define TM1637_I2C_COMM3    0x80

/*******************************************************************************
* Global Variables
*******************************************************************************/

char keymap[KEYPAD_ROWS][KEYPAD_COLUMNS] =
{
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'=','0','E','D'}
};

// TM1637 Globals
const uint8_t digitToSegment[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, // 0-7
    0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, // 8-9, A-F
    0x00, 0x40                                      // blank, minus
};

static uint8_t _brightness = 0x0f; // Default max brightness

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

int validate_sequence(int seq[]);
void compute_next(int seq[], int next[]);

// TM1637 Prototypes
void TM1637_Init(void);
void TM1637_DisplayDecimal(int v, int displaySeparator);
void TM1637_SetBrightness(uint8_t brightness);
void TM1637_Clear(void);
void TM1637_ShowNumber(int num);

/*******************************************************************************
* TM1637 Implementation
*******************************************************************************/

/*******************************************************************************
* TM1637 Implementation
*******************************************************************************/

static void _tm1637_delay(void)
{
    cyhal_system_delay_us(50); // Increased delay for stability
}

static void _tm1637_start(void)
{
    cyhal_gpio_write(TM1637_DIO, 1);
    cyhal_gpio_write(TM1637_CLK, 1);
    _tm1637_delay();
    cyhal_gpio_write(TM1637_DIO, 0);
    _tm1637_delay();
    cyhal_gpio_write(TM1637_CLK, 0);
    _tm1637_delay();
}

static void _tm1637_stop(void)
{
    cyhal_gpio_write(TM1637_CLK, 0);
    _tm1637_delay();
    cyhal_gpio_write(TM1637_DIO, 0);
    _tm1637_delay();
    cyhal_gpio_write(TM1637_CLK, 1);
    _tm1637_delay();
    cyhal_gpio_write(TM1637_DIO, 1);
    _tm1637_delay();
}

static int _tm1637_writeByte(uint8_t b)
{
    uint8_t data = b;

    // Write 8 bits
    for(uint8_t i = 0; i < 8; i++)
    {
        cyhal_gpio_write(TM1637_CLK, 0);
        _tm1637_delay();
        cyhal_gpio_write(TM1637_DIO, (data & 0x01) ? 1 : 0);
        _tm1637_delay();
        cyhal_gpio_write(TM1637_CLK, 1);
        _tm1637_delay();
        data >>= 1;
    }

    // Wait for ACK
    cyhal_gpio_write(TM1637_CLK, 0);
    _tm1637_delay();
    cyhal_gpio_write(TM1637_DIO, 1); // Release DIO (High-Z)
    _tm1637_delay();
    cyhal_gpio_write(TM1637_CLK, 1);
    _tm1637_delay();
    
    // Ideally read ACK here
    
    cyhal_gpio_write(TM1637_CLK, 0);
    _tm1637_delay();
    
    return 1;
}

void TM1637_Init(void)
{
    // CLK: Strong Drive (Push-Pull) for better edges
    cyhal_gpio_init(TM1637_CLK, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    // DIO: Open Drain for bidirectional data
    cyhal_gpio_init(TM1637_DIO, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_OPENDRAINDRIVESLOW, 1);
    
    _tm1637_delay();
    _tm1637_delay();
}

void TM1637_SetBrightness(uint8_t brightness)
{
    _brightness = (brightness & 0x07) | 0x08;
}

void TM1637_DisplayDecimal(int v, int displaySeparator)
{
    uint8_t data[4] = {0, 0, 0, 0};
    
    if (v > 9999) v = 9999;
    if (v < -999) v = -999;

    int temp = v;
    if (temp < 0) temp = -temp;

    for (int i = 3; i >= 0; i--)
    {
        data[i] = digitToSegment[temp % 10];
        temp /= 10;
        if (temp == 0 && i > (v < 0 ? 1 : 0)) break; // Stop leading zeros
    }
    
    if (v < 0) data[0] = digitToSegment[17]; // Minus sign

    // Start
    _tm1637_start();
    _tm1637_writeByte(TM1637_I2C_COMM1);
    _tm1637_stop();

    _tm1637_start();
    _tm1637_writeByte(TM1637_I2C_COMM2);
    
    for (int i = 0; i < 4; i++)
    {
        _tm1637_writeByte(data[i]);
    }
    _tm1637_stop();

    _tm1637_start();
    _tm1637_writeByte(TM1637_I2C_COMM3 + (_brightness & 0x0f));
    _tm1637_stop();
}

void TM1637_Clear(void)
{
    _tm1637_start();
    _tm1637_writeByte(TM1637_I2C_COMM1);
    _tm1637_stop();

    _tm1637_start();
    _tm1637_writeByte(TM1637_I2C_COMM2);
    for (int i = 0; i < 4; i++) _tm1637_writeByte(0x00);
    _tm1637_stop();

    _tm1637_start();
    _tm1637_writeByte(TM1637_I2C_COMM3 + (_brightness & 0x0f));
    _tm1637_stop();
}

void TM1637_ShowNumber(int num)
{
    TM1637_DisplayDecimal(num, 0);
}

/*******************************************************************************
* MAIN
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

#if defined (CY_DEVICE_SECURE) && defined (CY_USING_HAL)
    cyhal_wdt_t wdt_obj;

    result = cyhal_wdt_init(&wdt_obj, cyhal_wdt_get_max_timeout_ms());
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    cyhal_wdt_free(&wdt_obj);
#endif

    /* Initialize Board */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    __enable_irq();

    /* UART for printf */
    cy_retarget_io_init_fc(
        CYBSP_DEBUG_UART_TX,
        CYBSP_DEBUG_UART_RX,
        CYBSP_DEBUG_UART_CTS,
        CYBSP_DEBUG_UART_RTS,
        CY_RETARGET_IO_BAUDRATE);

    /* MATRIX KEYPAD PINS */
    cyhal_gpio_t rowPins[] = {P13_5, P13_4, P13_0, P13_3};
    cyhal_gpio_t colPins[] = {P9_6, P9_7, P9_4, P9_1};

    matrix_keypad_init((char*)keymap, rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLUMNS);

    /* TM1637 DISPLAY INIT */
    TM1637_Init();
    TM1637_SetBrightness(0x0f);
    TM1637_Clear();

    printf("\n=== Sequence Validator ===\n");
    printf("Press 6 keys using S1-S4...\n");

    int seq[6];
    int count = 0;

    /*************** GET 6 KEYPRESSES ***************/
    while (count < 6)
    {
        matrix_keypad_scan();

        if (matrix_keypad_hasKey())
        {
            char k = matrix_keypad_getKey();

            if (k >= '0' && k <= '9')   // only numbers allowed
            {
                seq[count] = k - '0';

                printf("%d ", seq[count]);
                
                // Display current number on 7-segment
                TM1637_ShowNumber(seq[count]);
                
                count++;
            }
        }
    }

    printf("\n\nValidating...\n");

    /*************** VALIDATE ***************/
    if (validate_sequence(seq))
    {
        int next[3];
        compute_next(seq, next);

        printf("Sequence VALID!\n");
        printf("Next 3 numbers: %d %d %d\n", next[0], next[1], next[2]);

        // Display sequence with delay
        TM1637_Clear();
        cyhal_system_delay_ms(500);
        
        // Show input sequence
        for(int i=0; i<6; i++) {
            TM1637_ShowNumber(seq[i]);
            cyhal_system_delay_ms(1000);
        }
        
        TM1637_Clear();
        cyhal_system_delay_ms(500);

        // Show next 3 numbers
        for(int i=0; i<3; i++) {
            TM1637_ShowNumber(next[i]);
            cyhal_system_delay_ms(1000);
        }
        
        TM1637_ShowNumber(8888); // Done indicator
    }
    else
    {
        printf("Sequence INVALID (-1)\n");
        TM1637_Clear();
        // Blink 0000 or similar to indicate error
        for(int i=0; i<3; i++) {
             TM1637_ShowNumber(0); // Show 0
             cyhal_system_delay_ms(500);
             TM1637_Clear();
             cyhal_system_delay_ms(500);
        }
    }

    while (1)
    {
        matrix_keypad_scan();
    }
}

/*******************************************************************************
* VALIDATE SEQUENCE
*******************************************************************************/
int validate_sequence(int seq[])
{
    for (int n = 3; n < 6; n++)
    {
        int expected = seq[n - 2] + seq[n - 3];
        if (seq[n] != expected)
            return 0;
    }
    return 1;
}

/*******************************************************************************
* COMPUTE NEXT 3
*******************************************************************************/
void compute_next(int seq[], int next[])
{
    next[0] = seq[3] + seq[4];
    next[1] = seq[5] + seq[4];
    next[2] = next[0] + seq[5];
}

/* END OF FILE */