/*========================================================*/
/*
  
  Project 1
  Student: Maria Luisa Carrion
  
  Objective: display OK in the LED matrix.
  
  ---------------------------------------------------------------------------------------------------------------------------------
  
  ACKNOWLEDGEMENTS:
    - Eiji Hayashi's code (http://www.cs.cmu.edu/~ehayashi/projects/lasercommand/) was used as an example for building this project.
    - The code for the scrolling text was found here: http://oomlout.com/8X8M/8X8M-ScrollMessage.txt
    - The operations for reverting bits was found in  Bit Twiddling Hacks, By Sean Eron Anderson: http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv
  
  ---------------------------------------------------------------------------------------------------------------------------------

  LED matrix:
    - Placed in breadboard slots: from row 33 to row 40.

  ---------------------------------------------------------------------------------------------------------------------------------
  
  Pin distribution:
    The following table presnts how the matrix pins are connected to the arduino pins. Here are the explanations of the headers:
      * mat = number of the pin in the matrix
      * col/row = matrix col/row to which the mat pin is connected
      * pin = arduino pin to which the mat (matrix pin) is connected
  
    mat  col  pin
    9    1    2*
    14   2    3*
    8    3    4*
    12   4    5*
    1    5    6*
    7    6    7*
    2    7    8*
    5    8    9*
    
    mat  row  pin
    16   1    14 A0 *
    15   2    15 A1 *
    11   3    16 A2 *
    6    4    17 A3 *
    10   5    18 A4 *
    4    6    19 A5 *
    3    7    11 *    
    13   8    12 *
    
    Memory Registers:
    
      D – 0..7  (D0..D7)   (D0, D1 are com to PC)
      B – 8..13 (D8..D13)            (6 bit port)
      C – 14..19 (A0..A5) (D14..D19) (6 bit port)    
*/
/*========================================================*/

#include <avr/delay.h>

// Set the pins of the rows and columns
const int rows[8] = { 14, 15, 16, 17, 18, 19, 11, 12 };
const int cols[8] = { 2, 3, 4, 5, 6, 7, 8, 9 };
const int pinX = A7;                               // Input pin for the joystick x coordinate
const int pinY = A6;                               // Input pin for the joystick y coordinate
const int pinS = 10;                               // Input pin for the switch

// Control variables
const int PLAY = 0;
const int OVER = 1;
const int CLEARED = 2;
const int ENDED = 3;
int     current_level = 0;                         // Current level being played
int     lives = 3;
int     hero_row = 0;                              // Row in which the hero is currently located inside the hero[] buffer
long    base_time_input = 0;                       // Time at which the loop for reading the input begins. This is updated everytime the desired target time is achieved.
long    base_time_shots = 0;                       // Time at which the loop for updating the shots begins. This is updated everytime the desired target time is achieved.
long    base_time_enemies = 0;                     // Base time for updating the enemies movement
long    base_time_stage_end = 0;                   // Base time for turning on/off the stage goal or end
int     target_time_input = 100;                   // Desired target time for reading the input in mili seconds
int     target_time_shots = 100;                   // Desired target time for updating the shots in mili seconds
int     target_time_enemies = 1000;
int     target_time_stage_end = 800;
int     stage_end_status = 1;                      // 1 means on; 0 means off.
boolean unpressed = true;                          // Tells if the switch has been disconnected
boolean enemies_to_right[8] = { true, true, true, true, true, true, true, true };                   // Tells if the enemies should keep moving to the right or not. There is one for each row of the enemies_left_right buffer
int game_state = OVER;

// Image buffers
int hero[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };          // Contains the position of the hero
int shots_right[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };   // Contains all the shots going to the right
int shots_left[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };    // Contains all the shots going to the left
int shots_up[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };      // Contains all the shots going up
int shots_down[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };    // Contains all the shots going down
int shots_all[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };     // Contains all the shots
int enemy_shots_right[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };   // Contains all the shots going to the right
int enemy_shots_left[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };    // Contains all the shots going to the left
int enemy_shots_up[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };      // Contains all the shots going up
int enemy_shots_down[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };    // Contains all the shots going down
int enemy_shots_all[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };     // Contains all the shots
int stage_ends_row[] = { 0, 0 };                            // Indicates in which row the stage end row is located
int stage_ends[] = {      
    B00000001,
    B00000001
  };
int lvl0[] = {                                     // Contains the scenario for level 0
    B00000000,
    B00000000,
    B00011111,
    B00000000,
    B00000000,
    B00011000,
    B00010000,
    B00011000
  };
int lvl1[] = {                                     // Contains the scenario for level 0
    B00000000,
    B00011111,
    B00000000,
    B00000100,
    B00000100,
    B00000110,
    B00000100,
    B00000000
  };

int* stages[8] = { lvl0, lvl1 };                         // Contains all the scenarios for the levels  
int enemies_left_right[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int enemies_all[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };



// Code for the scrolling text (found at: http://oomlout.com/8X8M/8X8M-ScrollMessage.txt )
//An array holding the powers of 2 these are used as bit masks when calculating what to display
const int powers[] = {1,2,4,8,16,32,64,128};
int speed = 15; //number of times to repeat each frame
int pauseDelay = 500;  //microseconds to leave each row  on before moving to the next

char requestString[] = " ARST07 ";  //The string to display
                                           //to change the message in code you right yourself simply 
                                           //change this data and reset index and offset to 0
//Variables used for scrolling (both start at 0
int index = 0;  //this is the current charachter in the string being displayed
int offset = 0; //this is how many columns it is offset by
//Constants defining each charachters position in an array of integer arrays
//Letters
const int A = 0;  const int R = 1; const int S = 2; const int T = 3; const int ZERO = 4;
const int SEVEN = 5; const int PISTOL = 6; const int SPACE = 7;

//The array used to hold a bitmap of the display 
//(if you wish to do something other than scrolling marque change the data in this
//variable then display)
byte data[] = {0,0,0,0,0,0,0,0};        

//The alphabet
//Each Charachter is an 8 x 7 bitmap where 1 is on and 0 if off
const int _A[] = {B00000000,
                  B00110011,
                  B01010101,
                  B01010101,
                  B01010101,
                  B01100110,
                  B00000000,
                  B00000000};
                  
const int _R[] = {B00000000,
                  B01110111,
                  B00010111,
                  B00100010,
                  B01001100,
                  B01000000,
                  B00000000,
                  B00000000};

const int _S[] = {B00000000,
                  B11100111,
                  B00000100,
                  B00000111,
                  B00000001,
                  B00000111,
                  B00000000,
                  B00000000};

const int _T[] = {B00000000,
                  B01110111,
                  B00100101,
                  B00100111,
                  B00100101,
                  B00100101,
                  B00000000,
                  B00000000};

const int _0[] = {B00000000,
                  B01100111,
                  B01010010,
                  B01100010,
                  B01010010,
                  B01010010,
                  B00000000,
                  B00000000};

const int _7[] = {B00000000,
                  B00000000,
                  B00000000,
                  B00011000,
                  B00011000,
                  B00000000,
                  B00000000,
                  B00000000};

const int _PISTOL[] = {B0000000,
                  B0000000,
                  B0000000,
                  B0000000,
                  B0000000,
                  B0000000,
                  B0000000,
                  B0000000};

const int __[] = {B0000000,
                  B0000000,
                  B0000000,
                  B0000000,
                  B0000000,
                  B0000000,
                  B0000000,
                  B0000000};
//Load the bitmap charachters into an array (each charachters position corresponds to its previously defined index (ie _A (a's bitmap) 
//is at index 0 and A = 0 so letters[A] will return the 'A' bitmap)
const int* letters[] = {_A,_R,_S,_T, _0, _7, _PISTOL,__};

// End of Code for the scrolling text


// Initializization code
void setup() {
  
  // open the serial port at 9600 bps:
  Serial.begin(9600); 
  //Serial.println( 2, BIN);
  
  // Turn off all the LEDs
  DDRC &= ~B00111111;   // Set rows 0 to 5 as INPUT
  DDRB &= ~B00011000;   // Set rows 6 and 7 as INPUT
  DDRD |=  B11111100;   // Set cols 0 to 5 as OUTPUT
  DDRB |=  B00000011;   // Set cols 6 and 7 as OUTPUT
  PORTD &=  ~B11111100; // Set cols 0 to 5 as LOW
  PORTB &=  ~B00000011; // Set cols 6 and 7 as LOW
  
  // Load initial position of the hero
  init_hero(hero, current_level);
  // Load initial position of enemies
  init_enemies_left_right(enemies_left_right, current_level);

  // Set the base time for the first time we enter the loop
  base_time_input = base_time_shots = base_time_enemies = base_time_stage_end = millis();  
}



/* Main routine (called repeated by from the Arduino framework) */
void loop() {

  if ( game_state == PLAY ) {
            // Read input
            if ( millis() - base_time_input >= target_time_input ) {    // Add delay for reading the input. If we read the input too fast, the game environment would change too quickly
              // Update base time 
              base_time_input = millis();
              
              // Read input from the joystick 
              int valX = analogRead(pinX);  
              int valY = analogRead(pinY);
              //Serial.println( valY );
              
              // Move left or right
              if ( !( hero[hero_row] == B10000000 ) && valX < 450 && !( stages[current_level][hero_row] &  hero[hero_row] << 1 ) ) {        // Check if the hero is in the left border of the screen, if X is for the left and if there is collision
                hero[hero_row] <<= 1;
              }
              else if ( !( hero[hero_row] == B00000001 ) && valX > 650  && !( stages[current_level][hero_row] &  hero[hero_row] >> 1 ) ) {   // Check if the hero is in the right border of the screen
                hero[hero_row] >>= 1;
              }
              
              //Move up or down
              if ( !( hero_row == 0 ) && valY < 450 && !( stages[current_level][hero_row - 1] &  hero[hero_row] ) ) {
                hero[hero_row - 1] = hero[hero_row];
                hero[hero_row] = B00000000;
                hero_row -= 1;
              }
              else if ( !( hero_row == 7 ) && valY > 650 && !( stages[current_level][hero_row + 1] &  hero[hero_row] ) ) {
                hero[hero_row + 1] = hero[hero_row];
                hero[hero_row] = B00000000;
                hero_row += 1;
              }
              
              // Read input from the switch
              int valS = digitalRead(pinS);
              
              // Shoot only if the switch has been pressed (and not kept pressed since the beginning)
              if ( valS == HIGH &&  (  unpressed ) ) {
                unpressed = false;
                // Shoot to the right, if there is not a wall to the right of the hero
                if ( ( ( hero[hero_row] >> 1) & stages[current_level][hero_row] ) ^ ( hero[hero_row] >> 1 ) ) {    // Check if the bullet ( hero[hero_row] >> 1) collides with the scenario ( stages[current_level][hero_row] ). 
                  shots_right[hero_row] |= (hero[hero_row] >> 1);                                                  //   This gives us 1s for the places where collision happens, so we XOR these result with the bullet itself ( hero[hero_row] >> 1 ), 
                                                                                                                   //   to know if this particular bullet will cause a collision
                }
                // Shoot to the left, if there is not a wall to the left of the hero
                if ( ( ( (hero[hero_row] << 1) & B11111111 ) & stages[current_level][hero_row] ) ^ ( (hero[hero_row] << 1) & B11111111 ) ) {
                  shots_left[hero_row] |= (hero[hero_row] << 1) & B11111111; 
                }
                // Shoot up, , if there is not a wall above the hero
                if ( !( hero_row == 0 ) && ( ( hero[hero_row] & stages[current_level][hero_row - 1] ) ^ hero[hero_row] ) ) {
                  shots_up[hero_row - 1] |= hero[hero_row];
                }
                // Shoot down, if there is not a wall below the hero
                if ( !( hero_row == 7 ) && ( ( hero[hero_row] & stages[current_level][hero_row + 1] ) ^ hero[hero_row] ) ) {
                  shots_down[hero_row + 1] |= hero[hero_row];
                }
              }
              if ( valS == LOW ) {
                unpressed = true;
              }
          
            } // End of Read input
          
          
            load_shots_buffers( shots_all );
            load_enemy_shots_buffers( enemy_shots_all );
          
              
            // Show the content of the image buffers in the LED matrix
            for( int row = 0; row < 8; row++ ){
            
              // Turn on the row
              pinMode( rows[row], OUTPUT );
              digitalWrite( rows[row], LOW);
              
              // Show the content of the stage for 100 microseconds if that row has content
              if (stages[current_level][row]) { 
                turn_on_columns(stages[current_level][row], 100, true);
              }
              
              // Show the content of the stage for 100 microseconds if that row has content
              if (enemies_left_right[row]) { 
                turn_on_columns(enemies_left_right[row], 100, true);
              }
              
              // Show the content of shots_all (shots from the hero) if that row has content
              if (shots_all[row]) { 
                turn_on_columns(shots_all[row], 250, true);
              }   
              
              
              // Show the content of the hero buffer
              if ( row == hero_row ) {
                turn_on_columns(hero[hero_row], 3250, true);
              }
              
             // Make enemies shoot
               // If the hero is in the indicated direction
                int hero_up = ( hero_row < row ) && ( enemies_left_right[row] & hero[hero_row] );
                int hero_down = ( hero_row > row ) && ( enemies_left_right[row] & hero[hero_row] );
                int hero_right = ( hero_row == row ) && ( enemies_left_right[row] > hero[hero_row] );
                int hero_left = ( hero_row == row ) && ( enemies_left_right[row] < hero[hero_row] );
                
                // Shoot to the right if the hero is at the right, if there is not already a shot in that direction and if there is not a wall to the right
                if ( hero_right && !( enemy_shots_right[row] ) && ( ( ( enemies_left_right[row] >> 1) & stages[current_level][row] ) ^ ( enemies_left_right[row] >> 1 ) ) ) {    // Check if the bullet ( hero[hero_row] >> 1) collides with the scenario ( stages[current_level][hero_row] ). 
                  enemy_shots_right[row] |= (enemies_left_right[row] >> 1);                                                  //   This gives us 1s for the places where collision happens, so we XOR these result with the bullet itself ( hero[hero_row] >> 1 ), 
                                                                                                                   //   to know if this particular bullet will cause a collision
                }
                // Shoot to the left, if there is not a wall to the left
                if ( hero_left && !( enemy_shots_left[row] ) && ( ( ( (enemies_left_right[row] << 1) & B11111111 ) & stages[current_level][row] ) ^ ( (enemies_left_right[row] << 1) & B11111111 ) ) ) {
                  enemy_shots_left[row] |= (enemies_left_right[row] << 1) & B11111111; 
                }
                
                int shots_up_exist = 0;
                int shots_down_exist = 0;
                // See if there is an existing shot up or down, going towards the hero
                for ( int j = 0; j < 8; j++ ) {
                  if ( j > hero_row ) {
                    shots_up_exist |= ( enemy_shots_up[j] & hero[hero_row] );
                  }
                  else if ( j < hero_row ) {
                    shots_down_exist |= ( enemy_shots_down[j] & hero[hero_row] );
                  }
                }
                
                // Shoot up, , if there is not a wall above
                if ( hero_up && !( shots_up_exist ) && ( !( row == 0 ) && ( ( enemies_left_right[row] & stages[current_level][row - 1] ) ^ enemies_left_right[row] ) ) ) {
                  enemy_shots_up[row - 1] |= enemies_left_right[row];
                }
                // Shoot down, if there is not a wall below
                if ( hero_down && !( shots_down_exist ) && ( !( row == 7 ) && ( ( enemies_left_right[row] & stages[current_level][row + 1] ) ^ enemies_left_right[row] ) ) ) {
                  enemy_shots_down[row + 1] |= enemies_left_right[row];
                }
              
              // Show the content of enemy_shots_all (shots from the enemies) if that row has content
              if (enemy_shots_all[row]) { 
                turn_on_columns(enemy_shots_all[row], 250, true);
              }
              
              // Manage timer and status for the blinking light that indicates the end of the stage
              if ( ( millis() - base_time_stage_end ) >= target_time_stage_end ) {
                base_time_stage_end = millis();
                stage_end_status = ~stage_end_status;
              }
              
              // Show blinking light that indicates the end of the stage
              if ( row == stage_ends_row[current_level] && stage_end_status == 1 ) {
                turn_on_columns(stage_ends[current_level], 1000, true);
              }
              
              // Turn off the row 
              pinMode( rows[row], INPUT );
              
              // Check if it kills an enemy
              int hits = enemies_left_right[row] & shots_right[row];                      // find the enemies hit 
              enemies_left_right[row] = hits ^ enemies_left_right[row];    // keep only those not hit
              shots_right[row] = hits ^ shots_right[row];
              
              hits = enemies_left_right[row] & shots_left[row];
              enemies_left_right[row] = hits ^ enemies_left_right[row];
              shots_left[row] = hits ^ shots_left[row];
              
              hits = enemies_left_right[row] & shots_up[row];
              enemies_left_right[row] = hits ^ enemies_left_right[row];
              shots_up[row] = hits ^ shots_up[row];
              
              hits = enemies_left_right[row] & shots_down[row];
              enemies_left_right[row] = hits ^ enemies_left_right[row];
              shots_down[row] = hits ^ shots_down[row];
              
            }
            
            
          
            // Update shots
            if ( millis() - base_time_shots >= target_time_shots ) {      
              base_time_shots = millis();
              
              // Move shots right, left or up
              for( int row = 0; row < 8; row++ ){      
                    //and con el stage me da la máscara de 1s (q no deben ir) y luego hago un xor de eso
                    // Move right shots one position to the right, but check if the shot hits a wall. If that is the case, then it is eliminated
                    shots_right[row] = ( ( shots_right[row] >> 1) & stages[current_level][row] ) ^ ( shots_right[row] >> 1 );
                    enemy_shots_right[row] = ( ( enemy_shots_right[row] >> 1) & stages[current_level][row] ) ^ ( enemy_shots_right[row] >> 1 );
                    
                    // Move left shots one position to the left, but check if the shot hits a wall. If that is the case, then it is eliminated
                    shots_left[row] = ( ( (shots_left[row] << 1) & B11111111 ) & stages[current_level][row] ) ^ ( (shots_left[row] << 1) & B11111111 ) ;
                    enemy_shots_left[row] = ( ( (enemy_shots_left[row] << 1) & B11111111 ) & stages[current_level][row] ) ^ ( (enemy_shots_left[row] << 1) & B11111111 ) ;
                   
                    // Move up shots one position up, but check if the shot hits a wall. If that is the case, then it is eliminated
                    if ( row < 7 ) {
                      shots_up[row] = ( shots_up[row + 1] & stages[current_level][row] ) ^ shots_up[row + 1];
                      enemy_shots_up[row] = ( enemy_shots_up[row + 1] & stages[current_level][row] ) ^ enemy_shots_up[row + 1];
                    }
                    else {
                      shots_up[row] = 0;
                      enemy_shots_up[row] = 0;
                    }
                     
              }
              
              // Move shots down
              for( int row = 7; row >= 0; row-- ){  
                    // Move down shots one position down, but check if the shot hits a wall. If that is the case, then it is eliminated
                    if ( row > 0 ) {
                      shots_down[row] = ( shots_down[row - 1] & stages[current_level][row] ) ^ shots_down[row - 1];
                      enemy_shots_down[row] = ( enemy_shots_down[row - 1] & stages[current_level][row] ) ^ enemy_shots_down[row - 1];
                    }
                    else {
                      shots_down[row] = 0; 
                      enemy_shots_down[row] = 0; 
                    }
              
              }
              
            } 
            
            // Update enemies
            if ( millis() - base_time_enemies >= target_time_enemies ) {
              base_time_enemies = millis();
              
              // Move the enemies
              for( int row = 0; row < 8; row++ ){
                
                // Move enemies to the right
                if ( enemies_to_right[row] ) {
                  // Check collision to the right
                  if ( ( !( enemies_left_right[row] & B00000001 ) && !( stages[current_level][row] &  enemies_left_right[row] >> 1 ) ) ) {
                    enemies_left_right[row] >>= 1;
                  }
                  else {
                    enemies_to_right[row] = false;
                    //Check collision to the left
                    if ( (!( enemies_left_right[row] & B10000000 ) && !( stages[current_level][row] &  enemies_left_right[row] << 1 ) ) ) {
                      enemies_left_right[row] <<= 1;
                    } 
                  }
                }
                // Move enemies to the left
                else if ( !(enemies_to_right[row]) ) {
                  //Check collision to the left
                  if ( (!( enemies_left_right[row] & B10000000 ) && !( stages[current_level][row] &  enemies_left_right[row] << 1 ) ) ) {
                    enemies_left_right[row] <<= 1;
                  }
                  else {
                    enemies_to_right[row] = true;
                    // Check collision to the right
                    if ( ( !( enemies_left_right[row] & B00000001 ) && !( stages[current_level][row] &  enemies_left_right[row] >> 1 ) ) ) {
                      enemies_left_right[row] >>= 1;
                    }
                  }
                }
                
                
            
                
                
                
                
              }
            }
            
            // Check Game Over
            // If the hero reaches the point where the stage ends, then the level is cleared
            if ( ( hero_row == stage_ends_row[current_level] ) && ( hero[hero_row] & stage_ends[current_level] ) ) {
              if ( current_level < 1 ) {
                game_state = CLEARED;
              }
              else {
                game_state = ENDED;
              }
            }
            // If the hero touches an enemy, or is hit by an enemy shot, it's game over
            else if ( ( hero[hero_row] & enemies_left_right[hero_row] ) || ( enemy_shots_all[hero_row] & hero[hero_row] ) ) {
              if ( lives > 0 ) {
                lives--;
                init_hero(hero, current_level);
                init_enemies_left_right(enemies_left_right, current_level);
    
                // Initialize game state arrays
                for ( int i = 0; i < 8; i++ ) {
                  shots_right[i] = 0;
                  shots_left[i] = 0;
                  shots_up[i] = 0;
                  shots_down[i] = 0;
                  shots_all[i] = 0;
                  enemy_shots_right[i] = 0;
                  enemy_shots_left[i] = 0;
                  enemy_shots_up[i] = 0;
                  enemy_shots_down[i] = 0;
                  enemy_shots_all[i] = 0;
                }
              }
              else {
                game_state = OVER;
              }
              
            }

  }
  else if ( game_state == CLEARED ) {
    lives = 3;
    current_level++;
    init_hero(hero, current_level);
    init_enemies_left_right(enemies_left_right, current_level);
    
    // Initialize game state arrays
    for ( int i = 0; i < 8; i++ ) {
      shots_right[i] = 0;
      shots_left[i] = 0;
      shots_up[i] = 0;
      shots_down[i] = 0;
      shots_all[i] = 0;
      enemy_shots_right[i] = 0;
      enemy_shots_left[i] = 0;
      enemy_shots_up[i] = 0;
      enemy_shots_down[i] = 0;
      enemy_shots_all[i] = 0;
    }
    
    game_state = PLAY;
  }
  else if ( game_state == OVER || game_state == ENDED ) {
    
    loadSprite();
    showSprite(speed);
    

      
//    for( int i=0; i<8; i++ ){
//      // Light a LED
//      pinMode( rows[i], OUTPUT );
//      digitalWrite( rows[i], LOW);
//      digitalWrite( cols[i], HIGH);
//      delay(100);
//      // Turn the LED off
//      digitalWrite( cols[i], LOW );
//      pinMode( rows[i], INPUT );
//      
//      // Read input from the switch
//      int valS = digitalRead(pinS);
//      
//      // Start game when switch is pressed
//      if ( valS == HIGH ) {
//        game_state = PLAY;
//        break;
//      }
//    }
    

             
  }
 
  
}  // end loop()





// Show the content of the columns
void turn_on_columns(int row, int duration, boolean reverse) {
  
  if (reverse) {
    // Reverse bits to show the correct image (and not the mirror image)
    row = (row * 0x0202020202ULL & 0x010884422010ULL) % 1023;
  }
  PORTD =  (row & B00111111) << 2;   // Write the output values for cols 0 to 5
  PORTB =  (row & B11000000) >> 6;   // Write the output values for cols 6 and 7
  _delay_us( duration );
  PORTD &= ~B11111100;  // Turn all columns off
  PORTB &= ~B00000011;
}

// Load initial position of the hero in the array passed as argument
void init_hero(int buf[], int level) {
  if ( level == 0 ) {
    buf[0] = B00000000;
    buf[1] = B00000000;
    buf[2] = B00000000;
    buf[3] = B00000000;
    buf[4] = B00000000;
    buf[5] = B00000000;
    buf[6] = B00000000;
    buf[7] = B00000001;
    
    hero_row = 7;
  }
  else if ( level == 1 ) {
        buf[0] = B00000000;
    buf[1] = B00000000;
    buf[2] = B00000000;
    buf[3] = B00000000;
    buf[4] = B00000000;
    buf[5] = B00000000;
    buf[6] = B00000010;
    buf[7] = B00000000;
    
    hero_row = 6;
  }
}

void init_enemies_left_right(int buf[], int level) {
  if ( level == 0 ) {
    buf[0] = B00000000;
    buf[1] = B00000001;
    buf[2] = B00000000;
    buf[3] = B10000000;
    buf[4] = B00000000;
    buf[5] = B00000000;
    buf[6] = B00000000;
    buf[7] = B00000000;
  }
  else if ( level == 1 ) {
    buf[0] = B00000000;
    buf[1] = B00000000;
    buf[2] = B00100000;
    buf[3] = B00000000;
    buf[4] = B00000000;
    buf[5] = B00000000;
    buf[6] = B00000000;
    buf[7] = B10000000;
  }
}

void load_shots_buffers( int buf[] ) {
  buf[0] = ( ( shots_right[0] | shots_left[0] ) | shots_up[0] ) | shots_down[0];
  buf[1] = ( ( shots_right[1] | shots_left[1] ) | shots_up[1] ) | shots_down[1];
  buf[2] = ( ( shots_right[2] | shots_left[2] ) | shots_up[2] ) | shots_down[2];
  buf[3] = ( ( shots_right[3] | shots_left[3] ) | shots_up[3] ) | shots_down[3];
  buf[4] = ( ( shots_right[4] | shots_left[4] ) | shots_up[4] ) | shots_down[4];
  buf[5] = ( ( shots_right[5] | shots_left[5] ) | shots_up[5] ) | shots_down[5];
  buf[6] = ( ( shots_right[6] | shots_left[6] ) | shots_up[6] ) | shots_down[6];
  buf[7] = ( ( shots_right[7] | shots_left[7] ) | shots_up[7] ) | shots_down[7];
}

void load_enemy_shots_buffers( int buf[] ) {
  buf[0] = ( ( enemy_shots_right[0] | enemy_shots_left[0] ) | enemy_shots_up[0] ) | enemy_shots_down[0];
  buf[1] = ( ( enemy_shots_right[1] | enemy_shots_left[1] ) | enemy_shots_up[1] ) | enemy_shots_down[1];
  buf[2] = ( ( enemy_shots_right[2] | enemy_shots_left[2] ) | enemy_shots_up[2] ) | enemy_shots_down[2];
  buf[3] = ( ( enemy_shots_right[3] | enemy_shots_left[3] ) | enemy_shots_up[3] ) | enemy_shots_down[3];
  buf[4] = ( ( enemy_shots_right[4] | enemy_shots_left[4] ) | enemy_shots_up[4] ) | enemy_shots_down[4];
  buf[5] = ( ( enemy_shots_right[5] | enemy_shots_left[5] ) | enemy_shots_up[5] ) | enemy_shots_down[5];
  buf[6] = ( ( enemy_shots_right[6] | enemy_shots_left[6] ) | enemy_shots_up[6] ) | enemy_shots_down[6];
  buf[7] = ( ( enemy_shots_right[7] | enemy_shots_left[7] ) | enemy_shots_up[7] ) | enemy_shots_down[7];
}

void toggle(int buf[]) {
  buf[0] = buf[0] ^ B11111111;
  buf[1] = buf[1] ^ B11111111;
  buf[2] = buf[2] ^ B11111111;
  buf[3] = buf[3] ^ B11111111;
  buf[4] = buf[4] ^ B11111111;
  buf[5] = buf[5] ^ B11111111;
  buf[6] = buf[6] ^ B11111111;
  buf[7] = buf[7] ^ B11111111;
}


// Code for the scrolling text (found at: http://oomlout.com/8X8M/8X8M-ScrollMessage.txt )

//Loads the current scroll state frame into the data[] display array
void loadSprite(){
  int currentChar = getChar(requestString[index]);
  int nextChar = getChar(requestString[index+1]);
  
  for(int row=0; row < 8; row++){                    //iterate through each row
    data[row] = 0;                                   //reset the row we're working on
    for(int column=0; column < 8; column++){         //iterate through each column
     data[row] = data[row] + ((powers[column] & (letters[currentChar][row] << offset)));   //loads the current charachter offset by offset pixels 
     data[row] = data[row] + (powers[column] & (letters[nextChar][row] >> (8-offset) ));   //loads the next charachter offset by offset pixels
    }
  }
  offset++;                                          //increment the offset by one row
  if(offset==8){offset = 0; index++; if(index==sizeof(requestString)-2){index=0;}}         //if offset is 8 load the next charachter pair for the next time through
}

void showSprite(int speed2){  
  for(int iii = 0; iii < speed2; iii++){
    for( int row = 0; row < 8; row++ ){
    
      // Turn on the row
      pinMode( rows[row], OUTPUT );
      digitalWrite( rows[row], LOW);
      // Show the content of the message buffer
      turn_on_columns(data[row], pauseDelay, true);
      // Turn off the row 
      pinMode( rows[row], INPUT );
      
        // Read input from the switch
        int valS = digitalRead(pinS);
        
        // Start game when switch is pressed
        if ( valS == HIGH ) {
          game_state = PLAY;
          break;
        }
    
    }
  }
  
  //    for( int i=0; i<8; i++ ){
//      // Light a LED
//      pinMode( rows[i], OUTPUT );
//      digitalWrite( rows[i], LOW);
//      digitalWrite( cols[i], HIGH);
//      delay(100);
//      // Turn the LED off
//      digitalWrite( cols[i], LOW );
//      pinMode( rows[i], INPUT );
//      
//      // Read input from the switch
//      int valS = digitalRead(pinS);
//      
//      // Start game when switch is pressed
//      if ( valS == HIGH ) {
//        game_state = PLAY;
//        break;
//      }
//    }
  
  
//  
// for(int iii = 0; iii < speed2; iii++){                 //show the current frame speed2 times
//    for(int column = 0; column < 8; column++){            //iterate through each column
//    
//       // Turn on the column
//       
//       
//        
//       for(int i = 0; i < 8; i++){           
//           pinMode( rows[i], OUTPUT );     
//           digitalWrite(rows[i], LOW);                      //turn off all row pins  
//       }
//       
//       for(int i = 0; i < 8; i++) { //Set only the one pin
//         //turns the current column on
//         if ( i == column ) {     
//           digitalWrite(cols[i], LOW);
//         }  
//         //turns the rest of the rows off
//         else {                
//           digitalWrite(colA[i], HIGH); 
//         }
//       }
//    
//       for(int row = 0; row < 8; row++){                    //iterate through each pixel in the current column
//          int bit = (data[column] >> row) & 1;
//          if ( bit == 1 ) { 
//             digitalWrite(rowA[row], HIGH);                   //if the bit in the data array is set turn the LED on
//          }
//       }
//       
//       delayMicroseconds(pauseDelay);                       //leave the column on for pauseDelay microseconds (too high a delay causes flicker)
//    }
// }
}

//returns the index of a given charachter
//for converting from a string to a lookup in our array of charachter bitmaps
int getChar(char charachter){
 int returnValue = ZERO;
 switch(charachter){
  case 'A': returnValue = A; break;
  case 'a': returnValue = A; break;
  case 'R': returnValue = R; break;
  case 'r': returnValue = R; break;
  case 'S': returnValue = S; break;
  case 's': returnValue = S; break;
  case 'T': returnValue = T; break;
  case 't': returnValue = T; break;
  case '0': returnValue = ZERO; break;
  case '7': returnValue = SEVEN; break;
  case '>': returnValue = PISTOL; break;
  case ' ': returnValue = SPACE; break;
   
  }
  return returnValue;
}



// End of Code for the scrolling text
