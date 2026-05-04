/*****************************************************************************
* \file gui.h
*
* \brief
* 		This file contains declarations for functions related to matrix keypad
* 		driver operations.
*
********************************************************************************/

#ifndef MATRIXKEYPAD_H
#define MATRIXKEYPAD_H

#include "cyhal.h"
#include <stdlib.h>
#include "cybsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Represents the HIGH logic level.
 */
#define HIGH 1

/**
 * \brief Represents the LOW logic level.
 */
#define LOW 0

/**
 * \brief The priority level for GPIO interrupts.
 */
#define GPIO_INTERRUPT_PRIORITY (7u)

/**
 * \struct matrix_keypad_t
 * \brief structure that holds the physical parameters of the keypad, the pin mapping,
 *  the key mapping and the state variables.
 */
typedef struct {
	uint8_t row_n; 				/** \brief  Number of rows. Must be greater than zero */
	uint8_t column_n; 			/** \brief  Number of columns. Must be greater than zero */
	cyhal_gpio_t *row_pins; 	/** \brief Pin mapping for the rows. These pins are set as output.*/
	cyhal_gpio_t *column_pins; 	/** \brief Pin mapping for the columns. These pins are set as inputs. */
	char *keymap; 				/** \brief Key mapping for the keypad. Its a bi-dimentional matrix with "row_n" rows and "column_n" columns.
									When a key press is detect at row R and column C, the returned key is the one at keyMap[R][C].
									The key mapping is directly related to the pin mappings. Don't use '\0' as a mapped key  */
	char lastkey; 				/** \brief Holds the last key detected. Used to avoid the same key press to be read multiple times */
	char buffer; 				/** \brief Holds the last key accepted. Is cleared after the key press is requested.
									Its overwritten if a new key is pressed before the old one is requested */
} matrix_keypad_t;

/**
 * \brief Initializes the matrix keypad.
 *
 * \param[in] keymap Pointer to a character array containing the keymap for the matrix keypad.
 * \param[in] row_pins Pointer to an array of GPIO pins connected to the keypad rows.
 * \param[in] column_pins Pointer to an array of GPIO pins connected to the keypad columns.
 * \param[in] row_n Number of rows in the keypad.
 * \param[in] column_n Number of columns in the keypad.
 */
void matrix_keypad_init(char *keymap, cyhal_gpio_t *row_pins, cyhal_gpio_t *column_pins, uint8_t row_n, uint8_t column_n);

/**
 * \brief Scans the matrix keypad to check if any key is being pressed.
 */
void matrix_keypad_scan ();

/**
 * \brief Checks if any key is being pressed.
 * \return 1 if a key is being pressed, 0 otherwise.
 */
uint8_t matrix_keypad_hasKey ();

/**
 * \brief Gets the key that is being pressed.
 * \return The character corresponding to the pressed key.
 */
char matrix_keypad_getKey ();

/**
 * \brief Waits for a key to be pressed and returns it.
 *
 * \return The character corresponding to the pressed key.
 */
char matrix_keypad_waitForKey ();

/**
 * \brief Flushes the keypad buffer.
 */
void matrix_keypad_flush ();

/**
 * \brief Adds an event listener for keypad buttons.
 *
 * \param[in] listener The function pointer to the listener.
 */
void matrix_keypad_add_event_listener(void (*listener)(void*, cyhal_gpio_event_t));

#ifdef __cplusplus
}
#endif

#endif // MATRIXKEYPAD_H