// ----------------------------------------------------------------------------
// Rotary Encoder Driver with Acceleration
// Supports Click, DoubleClick, Long Click
//
// (c) 2010 karl@pitrich.com
// (c) 2014 karl@pitrich.com
//
// Timer-based rotary encoder logic by Peter Dannegger
// http://www.mikrocontroller.net/articles/Drehgeber
//
// Modified for ESP32 IDF
// (c) 2017 KaRadio
// ----------------------------------------------------------------------------

#ifndef __have__ClickEncoder_h__
#define __have__ClickEncoder_h__

#include "driver/gpio.h"
// ---Button defaults-------------------------------------------------------------
#define ENC_BUTTONINTERVAL    10  // check enc->button every x milliseconds, also debouce time
#define BTN_DOUBLECLICKTIME  600  // second click within 400ms
#define BTN_HOLDTIME        1000  // report held button after 1s

// ----------------------------------------------------------------------------




//

// ----------------------------------------------------------------------------
typedef gpio_mode_t pinMode_t;
#undef INPUT
#define INPUT	GPIO_MODE_INPUT
#undef INPUT_PULLUP
#define INPUT_PULLUP GPIO_MODE_INPUT
#undef LOW
#define LOW 0
#undef HIGH
#define HIGH 1
#define digitalRead(x) gpio_get_level((gpio_num_t)x)
#ifndef __have__ClickButton_h__
  typedef enum Button_e {
    Open = 0,
    Closed,
    Pressed,
    Held,
    Released,
    Clicked,
    DoubleClicked
  } Button;
#endif

  typedef struct {
  int8_t pinA;
  int8_t pinB;
  int8_t pinBTN;
  bool pinsActive;
  volatile int16_t delta;
  volatile int16_t last;
  volatile uint8_t steps;
  volatile uint8_t accel_inc;
  volatile int16_t acceleration;
  bool accelerationEnabled;
  volatile Button button;
  bool doubleClickEnabled;
  bool buttonHeldEnabled;
  uint16_t keyDownTicks ;
  uint16_t doubleClickTicks ;
  unsigned long lastButtonCheck ;

  //printf("diff: %d  cur: %d  last: %d  delta: %d\n",diff,curr,enc->last,enc->delta);
/*  int8_t pcurr;
  int16_t plast;
  int8_t pdiff;
  uint16_t count;
  uint16_t icount;
  uint16_t dcount;
  int16_t pdelta;
*/
  } Encoder_t;


  Encoder_t* encoder_init(int8_t A, int8_t B, int8_t BTN , bool half);
  void encoder_set_half_step(Encoder_t *enc, bool value);
  bool encoder_get_half_step(Encoder_t *enc);
  void encoder_service(Encoder_t *enc);
  int16_t encoder_get_value(Encoder_t *enc);
  Button encoder_get_button(Encoder_t *enc);
  bool encoder_get_pin_state(Encoder_t *enc);
  bool encoder_get_pins_active(Encoder_t *enc);

// ----------------------------------------------------------------------------

#endif // __have__ClickEncoder_h__
