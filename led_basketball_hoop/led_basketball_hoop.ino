/**************************************************
  J. Zambreno and J. Zambreno
  LED basketball hoop
  January 2018

  - Arduino connected to switches, 4x7-segment LED, IR LED and sensor, and Adafruit NeoPixel LED array
  - Tested with Adafruit Bluefruit nRF52 Feather board
  - Inspired by https://learn.adafruit.com/neopixel-mini-basketball-hoop
  **************************************************/


#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"


// Physical pins, see https://learn.adafruit.com/bluefruit-nrf52-feather-learning-guide/device-pinout 
// for details. 
#define NEOPIXEL_PIN         16
#define IR_LED_PIN           11
#define IR_SENSOR_PIN        15    


// Some tweakable parameters
#define BASKET_CHECK_us   1000  // How long a given IR pulse is to check to see if there is a ball.
#define LOOP_UPDATE_ms    50    // How long each loop should take to iterate
#define ANIMATE_UPDATE_ms 200    // How long to advance an animation
#define DEBOUNCE_WAIT_ms 1     // How long to wait for a switch to settle down
#define CALIBRATION_COUNT 25   // Number of consecutive successful IR pulses for calibration
#define MAX_CALIBRATION_ATTEMPTS 15 // Number of attempts to calibrate the sensor
#define MAX_SCORE 19 // Max score until game over
 
Adafruit_7segment matrix = Adafruit_7segment();
Adafruit_NeoPixel strip = Adafruit_NeoPixel(30, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// "Teams" are the different color choices for the LED array
// https://teamcolorcodes.com/category/ncaa-team-color-codes/
#define NUM_TEAMS 19
int team_color1[NUM_TEAMS][3] = {
  {167, 25, 48}, // Iowa State
  {255, 205, 0}, // Iowa
  {75, 17, 111}, // Northern Iowa
  {0, 86, 150},  // Drake  
  {78, 42, 132}, // Northwestern
  {255, 204, 51}, // Minnesota
  {200, 16, 46}, // Nebraska
  {153, 0, 0}, // Indiana  
  {0, 81, 186}, // Kansas  
  {206, 17, 65}, // Chicago Bulls
  {36, 62, 144}, // Golden State Warriors
  {111, 38, 51}, // Cleveland Cavailers
  {0, 43, 92}, // Minnesota Timberwolves
  {0, 21, 51}, // Seattle Seahawks
  {14, 51, 134}, // Chicago Cubs
  {17, 33, 75}, // Tottenham
  {0, 0, 255}, // Big beast team (Johnny)
  {255, 0, 154}, // Unicorn team (Tessa)
  {100, 0, 100} // Awesome Sauce team (Joey+Tessa)
};


int team_color2[NUM_TEAMS][3] = {
  {253, 200, 47}, // Iowa State
  {0, 0, 0}, // Iowa
  {255, 204, 0}, // Northern Iowa
  {174, 182, 187},  // Drake  
  {255, 255, 255}, // Northwestern
  {122, 0, 25}, // Minnesota
  {255, 255, 255}, // Nebraska
  {238, 237, 235}, // Indiana  
  {232, 0, 13}, // Kansas  
  {255, 255, 255}, // Chicago Bulls
  {255, 205, 52}, // Golden State Warriors
  {255, 184, 28}, // Cleveland Cavailers
  {0, 81, 131}, // Minnesota Timberwolves
  {77, 255, 0}, // Seattle Seahawks
  {204, 52, 51}, // Chicago Cubs
  {255, 255, 255}, // Tottenham
  {128, 128, 128}, // Big beast team (Johnny)
  {100, 0, 100}, // Unicorn team (Tessa)
  {255, 8, 0} // Awesome Sauce team (Joey+Tessa)
};

// It's a 2-player basketball game
#define NUM_PLAYERS 2
int scores[NUM_PLAYERS] = {0, 0};
int teams[NUM_PLAYERS] = {0, 1};
int cur_player = 0;

// Button pins, states, timestamps
#define NUM_SWITCHES 3
int switch_pins[NUM_SWITCHES] = {14, 12, 13};
int switch_pressed[NUM_SWITCHES] = {0, 0, 0};

long animation_timer_ms = 0;
boolean basket_detection_mode = false;

// Setup function. This is called once on reset. 
void setup() {

  // Initialize the serial port (debug messages)
  Serial.begin(9600);

  // Initialize the 7-seg matrix
  matrix.begin(0x70);

  // Setup the 3 input switches
  pinMode(switch_pins[0], INPUT); 
  pinMode(switch_pins[1], INPUT); 
  pinMode(switch_pins[2], INPUT); 

  // Setup the IR LED and senor
  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(IR_SENSOR_PIN, INPUT);

  // Initialize and clear the NeoPixel
  strip.begin();
  strip.show();

  // Team select mode. Cycle through L+R button presses, updating colors as you go
  boolean start_game = false;  

  while (start_game == false) {

    // Read all 3 switches
    for (int i = 0; i < NUM_SWITCHES; i++) {
      switch_pressed[i] = debouncedRead(i);
    }    

    // Left switch is team 1 select
    if (switch_pressed[0] == 1) {
       teams[0]++;     

       // Check for overflow
       if (teams[0] == NUM_TEAMS) {
        teams[0] = 0;
       }

       // No same-team games - increment and overflow again
       if (teams[0] == teams[1]) {
        teams[0]++;
        if (teams[0] == NUM_TEAMS) {
          teams[0] = 0;
        }
       }
    }

    // Right switch is team 2 select
    if (switch_pressed[1] == 1) {
       teams[1]++;     

       // Check for overflow
       if (teams[1] == NUM_TEAMS) {
        teams[1] = 0;
       }

       // No same-team games - increment and overflow again
       if (teams[0] == teams[1]) {
        teams[1]++;
        if (teams[1] == NUM_TEAMS) {
          teams[1] = 0;
        }
       }
    }


    // Middle switch is our start game
    if (switch_pressed[2] == 1) {
      start_game = true;
    }

    // Display current teams
    display_teams();

    // Animate current teams
    animate_teams(true);

  }  

  IR_calibration();

  // Initialize scoreboard
  display_scores();
   
}


// IR calibration mode. Flash a white pattern on the LEDs while you wait for the IR sensor to reach a stable value.
// Give up eventually , and just ignore all the basket detection code. 
void IR_calibration() {
  boolean calibration_done = false;
  int calibration_attempts = 0;  
  while (calibration_done == false) {
    theaterChase(strip.Color(127, 127, 127), 50); // White

    int pulse_count = 0;
    for (int i = 0; i < CALIBRATION_COUNT; i++) {
      pulseIR(1000);
      
      // Check if the IR sensor picked up the pulse (i.e. output wire went to ground).
      if (digitalRead(IR_SENSOR_PIN) == LOW) {
        pulse_count++;
      }

    }

    if (pulse_count == CALIBRATION_COUNT) {
      colorWipe(strip.Color(0, 255, 0), 50); // Green
      calibration_done = true;
      basket_detection_mode = true;
    } 

    else {
      calibration_attempts++;
      if (calibration_attempts == MAX_CALIBRATION_ATTEMPTS) {
        colorWipe(strip.Color(255, 0, 0), 50); // Red
        calibration_done = true;
        basket_detection_mode = false;
      }
    }
    
  }
}


// SW reset hack
void(* resetFunc) (void) = 0;


// Loop function - main polling loop. 
void loop() {


  boolean update_scoreboard = false;
  boolean switch_teams = false;

  // Read all 3 buttons
    for (int i = 0; i < NUM_SWITCHES; i++) {
      switch_pressed[i] = debouncedRead(i);
    } 

  // Update score as needed (L, R button press)
  // Left switch is add basket
  if (switch_pressed[0] == 1) {  
    update_scoreboard = true;
    switch_teams = true;
    scores[cur_player]++;
  }

  // Right switch is remove basket
  else if (switch_pressed[1] == 1) {  
    switch_teams = false;
    if (scores[cur_player] > 0) {
      update_scoreboard = true;
      scores[cur_player]--;  
    }
  }

  // Update team as needed (M button press)
  // Right switch is remove basket
  else if (switch_pressed[2] == 1) {  
    switch_teams = true;
    update_scoreboard = true;
  }


  // Check for shot made, play effect and update score
  if (basket_detection_mode == true) {
    if (isBallInHoop()) {
      update_scoreboard = true;
      switch_teams = true;
      scores[cur_player]++;    
      display_scores();
      timedRainbowCycle(1250, 1);
      IR_calibration();
    }
  }
 
  // Delay for 100 milliseconds so the ball in hoop check happens 10 times a second.
  // DO THIS IWTH PROPER TIMER AND DELAY
 

  // Update current LED pattern (this function should be done SIMPLY for now, and updated later)
  animate_teams(false);



  // Switch teams if needed
  if (switch_teams == true) {
    cur_player = !cur_player;
  }

  // Update scoreboard
  if (update_scoreboard == true) {
    display_scores();
  }


  // If max score reached, do a game over condition
  if ((scores[0] >= MAX_SCORE) || (scores[1] >= MAX_SCORE)) {
    
    while(1) {
      for (int i = 0; i < 20; i++) {
        theaterChase(strip.Color(200, 200, 200), 100); // White
        theaterChase(strip.Color(200, 0, 0), 100); // Red
        theaterChase(strip.Color(0, 0, 200), 100); // Blue
      }
      for (int i = 0; i < 20; i++) {
        theaterChaseRainbow(50);
      }
    }

  }

}


///////////////////////////////////////////////////////
// isBallInHoop function
//
// Returns true if a ball is blocking the sensor.
///////////////////////////////////////////////////////
boolean isBallInHoop() {
  // Pulse the IR LED at 38khz for 1 millisecond
  pulseIR(1000);

  // Check if the IR sensor picked up the pulse (i.e. output wire went to ground).
  if (digitalRead(IR_SENSOR_PIN) == LOW) {
    return false; // Sensor can see LED, return false.
  }

  return true; // Sensor can't see LED, return true.
}


///////////////////////////////////////////////////////
// pulseIR function
//
// Pulses the IR LED at 38khz for the specified number
// of microseconds.
///////////////////////////////////////////////////////
void pulseIR(long microsecs) {
  // 38khz IR pulse function from Adafruit tutorial: http://learn.adafruit.com/ir-sensor/overview
  
  // we'll count down from the number of microseconds we are told to wait
 
  noInterrupts();  // this turns off any background interrupts
 
  while (microsecs > 0) {
    // 38 kHz is about 13 microseconds high and 13 microseconds low
   digitalWrite(IR_LED_PIN, HIGH);  // this takes about 3 microseconds to happen
   delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
   digitalWrite(IR_LED_PIN, LOW);   // this also takes about 3 microseconds
   delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
 
   // so 26 microseconds altogether
   microsecs -= 26;
  }
 
  interrupts();  // this turns them back on

}



// Displays each team's colors on half of the NeoPixel strip. Animate slightly
void animate_teams(boolean both_teams) {

  long current_time_ms;
  static boolean swap_colors = false;
  
  // Set the first half of the strip to team1, the second half to team2
  for (uint16_t i = 0; i < strip.numPixels(); i+=2) {

    uint8_t cur_team = 0;
    if (i >= strip.numPixels()/2) {
      cur_team = 1;
    }

    // Use the same function to animate just 1 team as needed
    if (both_teams == false) {
      cur_team = cur_player;
    }

    uint16_t pixel1 = i;
    uint16_t pixel2 = i+1;
    if (swap_colors == true) {
      pixel1 = i+1;
      pixel2 = i;
    }
    strip.setPixelColor(pixel1, strip.Color(team_color1[teams[cur_team]][0], team_color1[teams[cur_team]][1], team_color1[teams[cur_team]][2]));
    strip.setPixelColor(pixel2, strip.Color(team_color2[teams[cur_team]][0], team_color2[teams[cur_team]][1], team_color2[teams[cur_team]][2]));
    
  }


  strip.show();

  // Check the timer, if ANIMATE_UPDATE_ms has passed, swap the colors
  current_time_ms = millis();
  if ((current_time_ms - animation_timer_ms) >= ANIMATE_UPDATE_ms) {
    animation_timer_ms = current_time_ms;
    swap_colors= !swap_colors;
  }

  delay(1);
  
}

// Displays the current teams on the 7-segment matrix. Requires a bit of math
void display_teams() {

    int digits[4];
    int team1 = teams[0]+1;
    int team2 = teams[1]+1;
   
    // If it's a double-digit team, write both digits, otherwise just write the single digit
    if (team1 > 9) {
      matrix.writeDigitNum(1, team1 % 10, false);
      matrix.writeDigitNum(0, team1 / 10, false);
    }
    else {
      matrix.writeDigitRaw(1, 0);
      matrix.writeDigitNum(0, team1, false);
    }

    matrix.drawColon(true);

    if (team2 > 9) {
      matrix.writeDigitNum(4, team2 % 10, false);
      matrix.writeDigitNum(3, team2 / 10, false);
    }
    else {
      matrix.writeDigitRaw(3, 0);
      matrix.writeDigitNum(4, team2, false);
    }

    
    matrix.writeDisplay();

}


// Displays the current score on the 7-segment matrix. Requires a bit of math
void display_scores() {

    int digits[4];
    int score1 = scores[0];
    int score2 = scores[1];

    boolean team1_possession = false;
    boolean team2_possession = false;

    if (cur_player == 0) {
      team1_possession = true;
    }
    else {
      team2_possession = true;
    }
   
    // If it's a double-digit score, write both digits, otherwise just write the single digit
    if (score1 > 9) {
      matrix.writeDigitNum(1, score1 % 10, team1_possession);
      matrix.writeDigitNum(0, score1 / 10, team1_possession);
    }
    else {
      matrix.writeDigitRaw(0, 0 | (team1_possession << 7));
      matrix.writeDigitNum(1, score1, team1_possession);
    }

    matrix.drawColon(false);

    if (score2 > 9) {
      matrix.writeDigitNum(4, score2 % 10, team2_possession);
      matrix.writeDigitNum(3, score2 / 10, team2_possession);
    }
    else {
      matrix.writeDigitRaw(3, 0 | (team2_possession << 7));
      matrix.writeDigitNum(4, score2, team2_possession);
    }

    
    matrix.writeDisplay();

}



// Helper function - reads and returns value, sleeping a bit if it is positive
int debouncedRead(int switchNumber) {
  int readValue = digitalRead(switch_pins[switchNumber]);

  if (readValue == 1) {
    delay(DEBOUNCE_WAIT_ms);
    while (readValue == 1) {
      readValue = digitalRead(switch_pins[switchNumber]);    
    }
    return 1;
  }
  return 0;
  
}







// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}


void timedRainbowCycle(uint32_t milliseconds, uint8_t wait) {
  // Get the starting time in milliseconds.
  uint32_t start = millis();
  // Use a counter to increment the current color position.
  uint32_t j = 0;
  // Loop until it's time to stop (desired number of milliseconds have elapsed).
  while (millis() - start < milliseconds) {
    // Change all the light colors.
    for (int i = 0; i < strip.numPixels(); ++i) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    // Wait the deisred number of milliseconds.
    delay(wait);
    // Increment counter so next iteration changes.
    j += 1;
  }
  // Turn all the pixels off after the animation is done.
  for (int i = 0; i < strip.numPixels(); ++i) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}




