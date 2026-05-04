/*****************************************************************************
* \file matrix_keypad.c
*
* \brief
*    	Implementation of the matrix keypad library for PSoC 6 devices.
*
*    	This file provides the functions to initialize, scan, and manage matrix
*    	keypads. It allows users to interface with matrix keypads and detect
*    	key presses in a simple and efficient manner.
*
********************************************************************************/

#include <matrix_keypad.h>

/**
 * \brief Declare a pointer to a matrix keypad struct
 */
matrix_keypad_t *keypad;

/**
 * \brief Callback data structures for each GPIO interrupt
 */
cyhal_gpio_callback_data_t gpio_col0_callback_data;
cyhal_gpio_callback_data_t gpio_col1_callback_data;
cyhal_gpio_callback_data_t gpio_col2_callback_data;
cyhal_gpio_callback_data_t gpio_col3_callback_data;

/**
 * \brief Initializes a matrix keypad.
 *
 * \details A matrix keypad will have some pins or wires that you have to connect to
 * digital inputs of the development board. You will need to sort out which pins are
 * connected to the rows, which are connected to the columns and their ordering.
 * Then define a matrix and initialize it with the ordered row pin number. Define
 * another matrix for the columns.
 *
 * To create the key mapping, define a bi-dimensional array and initialize with
 * the character to be returned when the key on it's place is pressed. Note
 * that the key mapping ordering is directly related to the pin mapping ordering.
 *
 * The library don't make a copy of mappings to use less storage. The library
 * references the mappings defined in the main sketch, so they can't be
 * re-purposed or edited.
 *
 * As an example, consider a 4x3 keypad:
 *
\code{.c}
uint8_t row_n = 4; //4 rows
uint8_t column_n = 4; //4 columns
uint8_t row_pins[row_n] = {10, 9, 8, 7};  //first row is connect to pin 10, second to 9...
uint8_t column_pins[column_n] = {6, 5, 4, 3};  //first column is connect to pin 6, second to 5...
char keymap[row_n][column_n] =
  {{'1','2','3','A'}, //key of the first row first column is '1', first row second column column is '2'
   {'4','5','6','B'}, //key of the second row first column is '4', second row second column column is '5'
   {'7','8','9','C'},
   {'*','0','#','D'}};

//keypad is the variable that you will need to pass to the other functions
matrix_keypad_t *keypad = matrix_keypad_init((char*)keymap, row_pins, column_pins, row_n, column_n);

\endcode
 *
 * \param [in] keymap Key mapping for the keypad. Its a bi-dimensional matrix with "row_n" rows and "column_n" columns.
 * 				You can define a variable as "char keymap [row_n][column_n]" and cast it as "(char*)keymap".
 * \param [in] row_pins Pin mapping for the rows. Is a uni-dimensional matrix with length "row_n".
 * \param [in] column_pins Pin mapping for the columns. Is a uni-dimensional matrix with length "column_n".
 * \param [in] row_n Number of rows. Must be greater than zero.
 * \param [in] column_n Number of columns. Must be greater than zero.
 *
 * This function initializes a matrix keypad by storing the keymap, row and column pins,
 * number of rows and columns in the keypad structure. It also initializes the row pins
 * as output pins and sets them high, and the column pins as input pins with pull-down
 * resistors and sets them low. These pins are used for scanning the keypad matrix to
 * detect button presses.
 */
void matrix_keypad_init(char *keymap, cyhal_gpio_t *row_pins, cyhal_gpio_t *column_pins, uint8_t row_n, uint8_t column_n){

    uint8_t i;

    keypad = malloc(sizeof(matrix_keypad_t));
    keypad->row_n = row_n;
    keypad->column_n = column_n;
    keypad->row_pins = malloc(row_n * sizeof(cyhal_gpio_t));
    keypad->column_pins = malloc(column_n * sizeof(cyhal_gpio_t));
    keypad->keymap = keymap;
    keypad->lastkey = '\0';
    keypad->buffer = '\0';

    /* How the hardware works
     *
     * The keypad is a matrix which each row and column is a wire. All wires are disconnected
     * from each other. When a button is pressed the corresponding row and column wires are shorted.
     * To detect that, the rows are set as outputs and held high and the columns are set as inputs
     * with pull down resistors.
     *
     * To scan a row, the row is set to low. If a key is pressed, the corresponding column will
     * read as low.
     */
    for(i = 0; i < keypad->row_n; i++){
         keypad->row_pins[i] = row_pins[i];
         cyhal_gpio_init(keypad->row_pins[i], CYHAL_GPIO_DIR_OUTPUT,
                        CYHAL_GPIO_DRIVE_STRONG, HIGH);
    }
    for(i = 0; i < keypad->column_n; i++){
        keypad->column_pins[i] = column_pins[i];
        cyhal_gpio_init(keypad->column_pins[i] , CYHAL_GPIO_DIR_INPUT,
                    CYHAL_GPIO_DRIVE_PULLDOWN, LOW);
    }

}

/**
 * \brief Scans a matrix keypad to detect key presses.
 *
 * \details This function scans a matrix keypad to detect which key (if any) has been pressed.
 * It does this by setting each row of the keypad to low and reading each column to determine
 * whether a key press has occurred. If a key press is detected, the function saves the
 * corresponding character value of the key in the keypad buffer, which can later be retrieved
 * by calling the matrix_keypad_getKey function.
 *
 * The function also stores the last key press in the last key variable to ensure that the
 * buffer is not overwritten when a key is released (important when the scan interval is
 * higher than the time of the key press).
 *
 * If the keypad is not properly initialized (i.e., keypad is null), the function does nothing.
 */
void matrix_keypad_scan (){
	
	uint8_t row, col;
	char key = '\0'; /* the "not detected" key */
	
	if(keypad != NULL) {

		/* How the hardware works
		 *
		 * The keypad is a matrix which each row and column is a wire. All wires
		 * are disconnected from each other. When a button is pressed the corresponding
		 * row and column wires are shorted. To detect that, the rows are set as outputs
		 * and held high and the columns are set as inputs with pull up resistors. To scan a
		 * row, the row is set to low. If a key is pressed, the corresponding column will
		 * read as low.
		 *
		 * On the other side, if none key is pressed, the corresponding column will read as high.
		 *
		 */
	    for(col = 0; col < keypad->column_n; col++){
			if(cyhal_gpio_read(keypad->column_pins[col]) == HIGH) {
				for(row = 0; row < keypad->row_n; row++){
					cyhal_gpio_write(keypad->row_pins[row], LOW);
					if(cyhal_gpio_read(keypad->column_pins[col]) == LOW) {
						 /* imagine as keyMap[row][col] */
						key = keypad->keymap[row * keypad->column_n + col];
					}
					cyhal_gpio_write(keypad->row_pins[row], HIGH);
				}
			}
		}
	    cyhal_system_delay_ms(5);
	    /* saves the key in the buffer only if the last key was released */
		if(keypad->lastkey != key) {
			/* because the buffer is flushed after a reading */
			keypad->lastkey = key;
			/*
			 * don't overwrite the buffer when the key is released. Important
			 * when the scan interval is higher than the time of the key press
			 */
			if(key != '\0') {
				keypad->buffer = key;
			}
		}
	}

}

/**
 * \brief Checks if a key press has been detected by a matrix keypad.
 *
 * \details This function checks whether a key press has been detected by a
 * matrix keypad. If the keypad is not properly initialized (i.e., keypad is null),
 * the function returns 0. If a key press has been detected and stored in the
 * keypad buffer, the function returns 1. Otherwise, the function returns 0.
 *
 * \return 1 if a key press has been detected, 0 otherwise.
 *
 */
uint8_t matrix_keypad_hasKey (){
	
	if(keypad == NULL) {
		return 0;
	}
	
	if(keypad->buffer != '\0'){
		return 1;
	}
	
	return 0;
}

/**
 * \brief Reads the last key press from a matrix keypad.
 *
 * \details This function returns the character value of the last key press that was
 * stored in the keypad buffer. If no key was pressed or the keypad is not properly
 * initialized (i.e., keypad is null), the function returns a null character ('\0').
 * The keypad buffer is cleared after the key press is read to prevent the same key
 * press event from being read multiple times.
 *
 * \return The character value of the last key press.
 *
 */
char matrix_keypad_getKey (){
	
	char key;
	
	if(keypad == NULL) {
		return '\0';
	}
	
	key = keypad->buffer;

	keypad->buffer = '\0'; /* a key press event can be read only one time.
						    * the buffer is cleared after its read to avoid reading
							* the same key press event many times
							*/
	
	return key;
}

/**
 * \brief Waits for a key press on a matrix keypad.
 *
 * \details This function waits until a key is pressed on a matrix keypad and then
 * returns the character value of the pressed key. If no key is pressed, the function
 * will keep scanning the keypad until a key is detected. If the keypad is not properly
 * initialized (i.e., keypad is null), the function will return a null character ('\0').
 *
 * \return The character value of the key that was pressed.
 *
 */
char matrix_keypad_waitForKey (){
	
	char key;
	
	if(keypad == NULL) {
		return '\0';
	}
	
	/* scans the keypad until a key is pressed */
	while(!matrix_keypad_hasKey()) {
		matrix_keypad_scan();
	}
	key = matrix_keypad_getKey();
	
	return key;
}

/**
 * \brief Flushes the unread keys buffer of a matrix keypad.
 *
 * \details This function clears the unread keys buffer of a matrix keypad, allowing
 * you to discard any queued key presses that were not yet read by the matrix_keypad_getKey
 * function. It does this by setting the keypad buffer to a null character, effectively
 * erasing any previously stored key presses.
 *
 */
void matrix_keypad_flush (){
 
	if(keypad != NULL) {

		keypad->buffer = '\0';
		keypad->lastkey = '\0';
	} 
}

/**
 * \brief Adds a listener function for keypad button events.
 *
 * \details This function registers a listener function to be called whenever
 * a keypad button is pressed. It uses the cyhal_gpio_register_callback and
 * cyhal_gpio_enable_event functions to enable GPIO interrupts for each
 * keypad column pin (GPIO_KEYPAD_COL0-3). The provided listener function
 * is set as the callback for all keypad column interrupts.
 *
 * \param[in] listener: A function pointer to the listener function to be
 * called when a keypad button is pressed.
 *
 */
void matrix_keypad_add_event_listener(void (*listener)(void*, cyhal_gpio_event_t)){

   /**
	 * \brief Set the callback function and keypad struct for all GPIO interrupts
	 */
    gpio_col0_callback_data.callback = listener;
    gpio_col0_callback_data.callback_arg = keypad;

    gpio_col1_callback_data.callback = listener;
    gpio_col1_callback_data.callback_arg = keypad;

    gpio_col2_callback_data.callback = listener;
    gpio_col2_callback_data.callback_arg = keypad;

    gpio_col3_callback_data.callback = listener;
    gpio_col3_callback_data.callback_arg = keypad;

    /**
     * \brief Register the callback for each keypad column pin and enable
     *  interrupts for rising edges.
     */
    cyhal_gpio_register_callback( keypad->column_pins[0],
								 &gpio_col0_callback_data);
	cyhal_gpio_enable_event(keypad->column_pins[0], CYHAL_GPIO_IRQ_RISE,
								 GPIO_INTERRUPT_PRIORITY, true);

	cyhal_gpio_register_callback(keypad->column_pins[1],
								 &gpio_col1_callback_data);
	cyhal_gpio_enable_event(keypad->column_pins[1], CYHAL_GPIO_IRQ_RISE,
								 GPIO_INTERRUPT_PRIORITY, true);

	cyhal_gpio_register_callback(keypad->column_pins[2],
								 &gpio_col2_callback_data);
	cyhal_gpio_enable_event(keypad->column_pins[2], CYHAL_GPIO_IRQ_RISE,
								 GPIO_INTERRUPT_PRIORITY, true);

	cyhal_gpio_register_callback(keypad->column_pins[3],
								 &gpio_col3_callback_data);
	cyhal_gpio_enable_event(keypad->column_pins[3], CYHAL_GPIO_IRQ_RISE,
								 GPIO_INTERRUPT_PRIORITY, true);
}