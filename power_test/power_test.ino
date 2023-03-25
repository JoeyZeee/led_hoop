
int pin1 = 7;              // Switch connected to digital pin 7
int pin2 = 15;

void setup()                    // run once, when the sketch starts
{
  Serial.begin(9600);           // set up Serial library at 9600 bps
  pinMode(pin1, INPUT);    // sets the digital pin as input to read switch
  pinMode(pin2, INPUT);    // sets the digital pin as input to read switch

}


void loop()                     // run over and over again
{
//  Serial.print("Read switch input: ");
  Serial.println(digitalRead(pin1));    // Read the pin and display the value
  Serial.println(digitalRead(pin2));    // Read the pin and display the value

  delay(1000);
}
