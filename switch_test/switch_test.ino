/*
 * Switch test program
 */

int pin1 = 14;              // Switch connected to digital pin 14, 8, and 6
int pin2 = 13;              // Switch connected to digital pin 14, 8, and 6
int pin3 = 12;              // Switch connected to digital pin 14, 8, and 6

void setup()                    // run once, when the sketch starts
{
  Serial.begin(9600);           // set up Serial library at 9600 bps
  pinMode(pin1, INPUT);    // sets the digital pin as input to read switch
  pinMode(pin2, INPUT);    // sets the digital pin as input to read switch
  pinMode(pin3, INPUT);    // sets the digital pin as input to read switch

}


void loop()                     // run over and over again
{
//  Serial.print("Read switch inputs: ");
  Serial.print("\n\nValues are: "); 
  Serial.println(digitalRead(pin1));    // Read the pin and display the value
  Serial.println(digitalRead(pin2));    // Read the pin and display the value
  Serial.println(digitalRead(pin3));    // Read the pin and display the value

  delay(1000);
}
