#include <Wire.h>
#include <Narcoleptic.h>
typedef void (*DirectionHandler)(float);
void AddHandler(int ordinal, void(*handler)(float));

/*
 * Setup the compass module up as such
 * SCL -> A5
 * SDA -> A4
 * VCC -> 3.3V
 * GND -> GND
*/
 
/*
 *Wrap debug code in a if(DEBUG) {  statements } 
 *so it can be optimized out to save space on the Arduino
 *
 *TIP: if your debug statements contain only calls to debug-functions
 *don't put the wrapper in the function as there is a low chance it will optimize out.
 *
 *If possible use inline function decorator to reduce size, and provide hints to avr-g++.
 *Usually functions can be forced to be inlined if they contain few function calls and few local variables
 *or only use their parameters to wrap other function calls. 
 *    Ex. turnOffPin(NORTH_PIN) just wraps digitalWrite(NORTH_PIN,LOW)
 */
const boolean DEBUG = true;

const int NORTH_PIN = 9;
const int EAST_PIN = 10;
const int SOUTH_PIN = 11;
const int WEST_PIN = 12;

const int NORTH = 0;
const int NORTH_EAST  = 1;
const int EAST = 2;
const int SOUTH_EAST = 3;
const int SOUTH = 4;
const int SOUTH_WEST = 5;
const int WEST = 6;
const int NORTH_WEST = 7;

const int HMC6352Address = 0x42;
// Shift the device's documented slave address (0x42) 1 bit right
// This compensates for how the TWI library only wants the
// 7 most significant bits (with the high bit padded with 0)
// This results in 0x21 as the address to pass to TWI
const int SLAVE_ADDRESS = HMC6352Address >> 1;

DirectionHandler handlers[8];

void setup()
{  
  Serial.begin(9600);    
  Wire.begin();    
  setupPins();
  setupHandlers();
}

void setupPins()
{
  pinMode(NORTH_PIN,OUTPUT);
  pinMode(EAST_PIN,OUTPUT);
  pinMode(SOUTH_PIN,OUTPUT);
  pinMode(WEST_PIN,OUTPUT);    
}

void setupHandlers()
{
  AddHandler(NORTH, onNorth);
  AddHandler(NORTH_EAST, onNorthEast);
  AddHandler(EAST, onEast);
  AddHandler(SOUTH_EAST,onSouthEast);
  AddHandler(SOUTH, onSouth);
  AddHandler(SOUTH_WEST, onSouthWest);
  AddHandler(WEST, onWest);
  AddHandler(NORTH_WEST, onNorthWest);  
}

void loop()
{
  if(DEBUG)
  {
     Serial.println("loop->");
  }
  longDelay(16000);
  float degreeValue = readCompassModule();
  int ordinalValue = convertToOrdinal(degreeValue);

  pulseHandlers(4, ordinalValue,degreeValue, 500);
}

void longDelay(long milliseconds)
{
   if(DEBUG)
   {
     Serial.print("longDelay-> milliseconds: "); 
     Serial.println(milliseconds);
   }
  
   while(milliseconds > 0)
   {
      if(milliseconds > 8000)
      {
         milliseconds -= 8000;
         Narcoleptic.delay(8000);
      }
      else
      {
        Narcoleptic.delay(milliseconds);
        break;
      }
   }
}

void turnOffPins()
{
  if(DEBUG)
  {
     Serial.println("turnOffPins->");    
  }
  
  turnOffPin(NORTH_PIN);   
  turnOffPin(SOUTH_PIN);
  turnOffPin(EAST_PIN);  
  turnOffPin(WEST_PIN);   
}

void pulseHandlers(const int amount, const int ordinalValue, const float degreeValue, const int runInterval)
{
   if(DEBUG)
   {
     Serial.print("pulseHandlers -> amount: "); 
     Serial.print(amount);
     Serial.print("ordinalValue: "); 
     Serial.print(ordinalValue);
     Serial.print("degreeValue: "); 
     Serial.print(degreeValue);
     Serial.print("runInterval: "); 
     Serial.println(runInterval);
   }
   for(int i = 0; i < amount;i++)
   {
    CallHandler(ordinalValue,degreeValue);
    
    delay(runInterval);
    
    turnOffPins();
    
    delay(100);
   }
}

void AddHandler(int ordinal,void (*handler)(float) )
{
  handlers[ordinal] = handler;
} 

void CallHandler(const int ordinal, const float degreeValue)
{
    DirectionHandler handle = handlers[ordinal];    
    if(handle == NULL) return;    
    
    if(DEBUG)
    {
     Serial.print("CallHandler-> "); 
     Serial.print("ordinal: "); 
     Serial.print(ordinal);
     Serial.print("degreeValue: "); 
     Serial.println(degreeValue);     
    }
    
    handle(degreeValue);
}

float readCompassModule()
{
  if(DEBUG)
  {
     Serial.print("readCompassModule-> "); 
  }
  byte headingData[2];
  int i = 0;

  // Send a "A" command to the HMC6352
  // This requests the current heading data
  Wire.beginTransmission(SLAVE_ADDRESS);
  // The "Get Data" command
  Wire.write("A");              
  Wire.endTransmission();
  // The HMC6352 needs at least a 70us (microsecond) delay
  // after this command.  Using 10ms just makes it safe 
  delay(10);    
  // Read the 2 heading bytes, MSB first
  // The resulting 16bit word is the compass heading in 10th's of a degree
  // For example: a heading of 1345 would be 134.5 degrees
  // Request the 2 byte heading (MSB comes first)
  Wire.requestFrom(SLAVE_ADDRESS, 2);        

  while(Wire.available() && i < 2)
  { 
    headingData[i] = Wire.read();
    i++;
  }

  // Put the MSB and LSB together
  // and divide by 10 to get the degree value
  return (headingData[0]*256 + headingData[1]) / 10;     
}

int convertToOrdinal(float degreeValue)
{
   /*
   * Each Ordinal Direction has a range of 45degrees
   * Value      |   Range
   * North      | NNW to NNE
   * North-East | NNE to ENE
   * EAST       | ENE to ESE
   * South-East | ESE to SSE 
   * South      | SSE to SSW
   * South-West | SSW to WSW
   * West       | WSW to WNW
   * North-West | WNW to NNW 
   * 
   *             360/0
   *        337.5     22.5                    
   *      315      N      45
   *   292.5	            67.5          
   *  270      W        E      90  
   *    247.5 	            112.5         
   *      225      S      135
   *        202.5     157.5
   *              180      
   *  
   */
  
  if (degreeValue <= 22.5 )
  {
    return NORTH;
  } 
  else if  (degreeValue <= 67.5)
  {
    return NORTH_EAST;
  }
  else if  (degreeValue <= 112.5)
  {
    return EAST;
  } 
  else if  (degreeValue <= 157.5)
  {
    return SOUTH_EAST;
  } 
  else if (degreeValue <= 202.5)
  {
    return SOUTH;
  } 
  else if (degreeValue <= 247.5)
  {
    return SOUTH_WEST;
  } 
  else if (degreeValue <= 292.5)
  {
    return WEST;
  } 
  else if (degreeValue <= 337.5) //NORTH WEST
  {
    return NORTH_WEST;
  } 
  else
  {
    return NORTH;
  }  
}

void turnOffPin(int pin)
{
  if(DEBUG)
  {
     Serial.print("OFF PIN: "); 
     Serial.println(pin);
  }
  
  digitalWrite(pin, LOW);  
}

void turnOnPin(int pin)
{
  if(DEBUG)
  {
     Serial.print("ON Pin: "); 
     Serial.println(pin);
  }
  
  digitalWrite(pin, HIGH);  
}

void onNorth(float degreeValue)
{
  if(DEBUG)
  {
     Serial.print("onNorth: "); 
     Serial.println(degreeValue);
  }
  
  turnOffPin(EAST_PIN);
  turnOffPin(SOUTH_PIN);
  turnOffPin(WEST_PIN);

  turnOnPin(NORTH_PIN);
}


void onNorthEast(float degreeValue)
{
  if(DEBUG)
  {
     Serial.print("onNorthEast: "); 
     Serial.println(degreeValue);
  }
  turnOffPin(NORTH_PIN);   
  turnOffPin(EAST_PIN);

  turnOnPin(NORTH_PIN);
  turnOnPin(EAST_PIN);
}

void onEast(float degreeValue)
{
  if(DEBUG)
  {
     Serial.print("onEast: "); 
     Serial.println(degreeValue);
  }
  turnOffPin(NORTH_PIN);   
  turnOffPin(SOUTH_PIN);
  turnOffPin(WEST_PIN);

  turnOnPin(EAST_PIN);
}

void onSouthEast(float degreeValue)
{
  if(DEBUG)
  {
     Serial.print("onSouthEast: "); 
     Serial.println(degreeValue);
  }
  turnOffPin(NORTH_PIN);   
  turnOffPin(WEST_PIN);

  turnOnPin(EAST_PIN);
  turnOnPin(SOUTH_PIN);    
}

void onSouth(float degreeValue)
{
  if(DEBUG)
  {
     Serial.print("onSouth: "); 
     Serial.println(degreeValue);
  }
  
  turnOffPin(NORTH_PIN);   
  turnOffPin(EAST_PIN);
  turnOffPin(WEST_PIN);

  turnOnPin(SOUTH_PIN);  
}

void onSouthWest(float degreeValue)
{
  if(DEBUG)
  {
     Serial.print("onSouthWest: "); 
     Serial.println(degreeValue);
  }
  
  turnOffPin(NORTH_PIN);   
  turnOffPin(EAST_PIN);  

  turnOnPin(SOUTH_PIN);
  turnOnPin(WEST_PIN);   
}

void onWest(float degreeValue)
{
  if(DEBUG)
  {
     Serial.print("onWest: "); 
     Serial.println(degreeValue);
  }
  
  turnOffPin(NORTH_PIN);   
  turnOffPin(SOUTH_PIN);
  turnOffPin(EAST_PIN);  
  
  turnOnPin(WEST_PIN); 
}

void onNorthWest(float degreeValue)
{
  if(DEBUG)
  {
     Serial.print("onNorthWest: "); 
     Serial.println(degreeValue);
  }
  turnOffPin(SOUTH_PIN);
  turnOffPin(EAST_PIN);
  
  turnOnPin(WEST_PIN);  
  turnOnPin(NORTH_PIN);   
}
