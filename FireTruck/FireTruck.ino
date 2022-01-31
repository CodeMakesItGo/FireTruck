/// CodeMakesItGo Jan 2022
/// Using the Mother Clucker Eggxit Door PCB (Arduino Nano)

/*-----( Includes )-----*/
#include <RCSwitch.h> //sui77,fingolfin 

/*-----( Digital Pins )-----*/
#define BTN2_IN 12      //Button 2 on dashboard
#define BTN1_IN 11      //Button 3 on dashboard
#define BTN3_IN 9       //Button 1 on dashboard
#define RLY1_OUT 4      //Relay 1 on PCB 
#define RLY2_OUT 7      //Relay 2 on PCB
#define LED_OUT 5       //Heartbeat indicator on PCB
#define SIDELED_OUT A5  //Output to turn on solid state relay for side lights
#define SPEAKER_OUT 8   //Square wave output to HORN_IN
#define HORN_IN 2       //HORN_IN input button on steering wheel
#define FLASHYLIGHTS_OUT 10 //Flashing Red and blue lights

/*-----( Configuration )-----*/
#define PRESS_COUNT 5   //5*25 = 125ms debounce

/*-----( typedefs )-----*/
typedef enum lightStates {OFF, SLOW, FAST}; //Light states for flashy lights

/*-----( Global Variables )-----*/
static unsigned long buttonDown[3] = {0}; //Button debounce counters
static lightStates currentLightState = OFF;   //Current state of flashy lights
static lightStates requestedLightState = OFF; //requested state of flashy lights
static long timer25 = 0;     //25 millisecond timer
static long timer250 = 0;    //250 millisecond timer
static bool lightsOn = false; //Toggler for lights flashing

/*-----( Global Constants )-----*/
static const int lightDelay = 300;  //delay when transmitting message to flashy light module
static const int length = 15; // the number of notes
static const char notes[] = "ccggaagffeeddc "; // a space represents a rest
static const int beats[] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 4 };
static const int tempo = 150;

/*-----( Class Objects )-----*/
RCSwitch mySwitch = RCSwitch();

/*-----( Functions )-----*/
void playTone(int tone, int duration) 
{
  for (long i = 0; i < duration * 1000L; i += tone * 2) 
  {
    digitalWrite(SPEAKER_OUT, HIGH);
    delayMicroseconds(tone);
    digitalWrite(SPEAKER_OUT, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) 
{
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C' };
  int tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956 };

  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++)
  {
    if (names[i] == note)
    {
      playTone(tones[i], duration);
    }
  }
}

void updateLights()
{
  if(currentLightState == OFF && requestedLightState != OFF)
  {
    mySwitch.send(5592076, 24); //btn c, power on and off
    delay(lightDelay);
    mySwitch.send(5592256, 24);//btn a, flashing mode //flashing
    delay(lightDelay);
    mySwitch.send(5592256, 24);//btn a, flashing mode //slow alternate flash, good

    if(requestedLightState == FAST)
    {
      delay(lightDelay);
      mySwitch.send(5592256, 24);//btn a, flashing mode //fast alternate flash, good
      Serial.println("Lights FAST");
    }
    else
    {
      Serial.println("Lights SLOW");
    }
  }

  if(currentLightState == SLOW && requestedLightState == FAST)
  {
    Serial.println("Lights FAST");
    mySwitch.send(5592256, 24);//btn a, flashing mode //fast alternate flash, good
  }

  if(currentLightState == FAST && requestedLightState == SLOW)
  {
    Serial.println("Lights OFF");
    mySwitch.send(5592076, 24); //btn c, power on and off
    delay(lightDelay);
    currentLightState = OFF;
    return;
  }

  if(currentLightState != OFF && requestedLightState == OFF)
  {
    Serial.println("Lights OFF");
    mySwitch.send(5592076, 24); //btn c, power on and off
  }

  currentLightState = requestedLightState;
}

void buttonPress()
{
  if(digitalRead(BTN1_IN) == LOW)
  {
    buttonDown[0]++;
  }
  else
  {
    if(buttonDown[0] >= PRESS_COUNT)
    {
      requestedLightState = OFF;
      Serial.println("Request OFF Lights");
    }
    buttonDown[0] = 0;
  }

  if(digitalRead(BTN2_IN) == LOW)
  {
    buttonDown[1]++;
  }
   else
  {
    if(buttonDown[1] >= PRESS_COUNT)
    {
      requestedLightState = SLOW;
      Serial.println("Request SLOW Lights");
    }
    buttonDown[1] = 0;
  }
  

  if(digitalRead(BTN3_IN) == LOW)
  {
    buttonDown[2]++;
  }
   else
  {
    if(buttonDown[2] >= PRESS_COUNT)
    {
      requestedLightState = FAST;
      Serial.println("Request FAST Lights");
    }
    buttonDown[2] = 0;
  }
}

// Function used to read the remote messages, !disabled!
void sniff()
{
  if (mySwitch.available()) 
  {
    Serial.print("Received ");
    Serial.print( mySwitch.getReceivedValue() );
    Serial.print(" / ");
    Serial.print( mySwitch.getReceivedBitlength() );
    Serial.print(" delay: ");
    Serial.print(mySwitch.getReceivedDelay());
    Serial.print(" raw: ");
    Serial.print(mySwitch.getReceivedValue(), BIN);
    Serial.print(" Protocol: ");
    Serial.println( mySwitch.getReceivedProtocol() );

    mySwitch.resetAvailable();
  }
}

void loop() 
{
  if(millis() - timer25 > 25)
  {
    buttonPress();
    updateLights();
    timer25 = millis();
  }

  if(millis() - timer250 > 250)
  {
    timer250 = millis();
    lightsOn = !lightsOn;
    lightsOn ? digitalWrite(LED_OUT, HIGH) : digitalWrite(LED_OUT, LOW);
    
    if(currentLightState != OFF)
    {
      lightsOn ? digitalWrite(RLY1_OUT, HIGH) : digitalWrite(RLY1_OUT, LOW);
      lightsOn ? digitalWrite(RLY2_OUT, HIGH) : digitalWrite(RLY2_OUT, LOW);
      lightsOn ? digitalWrite(SIDELED_OUT, HIGH) : digitalWrite(SIDELED_OUT, LOW);
    }
    else
    {
      digitalWrite(RLY2_OUT, LOW);
      digitalWrite(RLY1_OUT, HIGH);
      digitalWrite(SIDELED_OUT, LOW);
    }
  }

  if(digitalRead(HORN_IN) == LOW)
  {
    pinMode(SPEAKER_OUT, OUTPUT);
    for (int i = 0; i < length; i++) 
    {
      if (notes[i] == ' ') 
      {
        delay(beats[i] * tempo); // rest
      } 
      else 
      {
        playNote(notes[i], beats[i] * tempo);
      }
  
      // pause between notes
      delay(tempo / 2); 
    }
    pinMode(SPEAKER_OUT, INPUT);
  }
}

void setup() 
{
  Serial.begin(9600);
  //mySwitch.enableReceive(0);  // Receiver on interrupt 0 => that is pin #2
  
  // Transmitter is connected to Arduino Pin #10    
  mySwitch.enableTransmit(FLASHYLIGHTS_OUT); 
  
  // Optional set pulse length. 
  mySwitch.setPulseLength(365);
  mySwitch.setProtocol(1); 
  mySwitch.setRepeatTransmit(1); 

  pinMode(BTN1_IN, INPUT_PULLUP);
  pinMode(BTN2_IN, INPUT_PULLUP); 
  pinMode(BTN3_IN, INPUT_PULLUP);
  pinMode(HORN_IN, INPUT_PULLUP);

  pinMode(RLY1_OUT, OUTPUT);
  pinMode(RLY2_OUT, OUTPUT);
  pinMode(LED_OUT, OUTPUT);
  pinMode(SPEAKER_OUT, INPUT);
  pinMode(SIDELED_OUT, OUTPUT);
  
  digitalWrite(RLY1_OUT, HIGH);
  digitalWrite(RLY2_OUT, LOW);
  digitalWrite(SIDELED_OUT, LOW);
}
