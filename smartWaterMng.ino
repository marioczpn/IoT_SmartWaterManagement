#define BLYNK_PRINT Serial


#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <SerialRelay.h>

const int NumModules = 1;    // maximum of 10
const int PauseTime = 1000;  // [ms]
const int lvlSensorPin = 3;

int dam_liquid_level = 1000;
int dam_liquid_percentage;
int dam_top_level = 100;
int dam_bottom_level = 0;

int tank_liquid_level = 0;
int tank_liquid_percentage;
int tank_top_level = 100;//Maximum water level
int tank_bottom_level = 0;//Minimum water level

//water flow meter
byte sensorInterrupt = 0;  // 0 = digital pin 2
byte sensorPin       = 2;

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;

SerialRelay relays(4, 5, NumModules); // (data, clock, number of modules)
// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "PjMYdW3-5JAy1B_y5qJ3LmiR7AFIKnn3";

#define W5100_CS  10
//#define SDCARD_CS 4

void setup()
{
  // Debug console
  Serial.begin(9600);
  pinMode(lvlSensorPin, INPUT);
  relays.Info(&Serial, BIN);

  //Water flow meter
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

  Blynk.virtualWrite(V0, dam_liquid_level);
  Blynk.virtualWrite(V2, totalMilliLitres );
  Blynk.virtualWrite(V1, dam_liquid_percentage);

  Blynk.virtualWrite(V3, tank_liquid_percentage);

  Blynk.begin(auth);


  // You can also specify server:
  //Blynk.begin(auth, "blynk-cloud.com", 80);
  //Blynk.begin(auth, IPAddress(192,168,1,100), 8080);
}

void loop()
{

  flowMeter();


  int state = digitalRead(lvlSensorPin);
  //Serial.print("Sensor state: ");
  //Serial.println(state);
  switch (state)
  {
    case 0:
      Serial.println("emtpy");
      relays.SetRelay(1, SERIAL_RELAY_ON, 1);
      relays.SetRelay(1, SERIAL_RELAY_OFF, 1);

      break;
    case 1:
      Serial.println("full");
      Blynk.notify("Caixa CHEIA!!");
      relays.SetRelay(1, SERIAL_RELAY_OFF, 1);
      relays.SetRelay(1, SERIAL_RELAY_ON, 1);
      break;
  }





  delay(100);
  Blynk.run();
  // You can inject your own code or combine it with other sketches.
  // Check other examples on how to communicate with Blynk. Remember
  // to avoid delay() function!
}


void flowMeter() {
  if ((millis() - oldTime) > 1000)   // Only process counters once per second
  {
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;

    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

    unsigned int frac;

    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Blynk.virtualWrite(V6, flowRate);
    Serial.print(".");             // Print the decimal point
    // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
    frac = (flowRate - int(flowRate)) * 10;
    Serial.print(frac, DEC) ;      // Print the fractional part of the variable
    Serial.print("L/min");
    // Print the number of litres flowed in this second
    Serial.print("  Current Liquid Flowing: ");             // Output separator
    Serial.print(flowMilliLitres);
    Blynk.virtualWrite(V4, flowMilliLitres);
    Serial.print("mL/Sec");

    // Print the cumulative total of litres flowed since starting
    Serial.print("  Output Liquid Quantity: ");             // Output separator
    Serial.print(totalMilliLitres);
    Blynk.virtualWrite(V5, totalMilliLitres);
    Serial.println("mL");
    dam_liquid_level -= flowMilliLitres;
    Blynk.virtualWrite(V0, dam_liquid_level);
    Blynk.virtualWrite(V2, totalMilliLitres);
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
}

void damLiquidLevel() {
  dam_liquid_level--;
  dam_liquid_percentage = ((dam_liquid_level - dam_bottom_level) / dam_top_level) / 100; //Percentage of water in the container
  Serial.println(dam_liquid_level);//This will print the liquid level in the monitor
  Serial.println(dam_liquid_percentage);//This will print the percentage of liquid in the monitor
  Blynk.virtualWrite(V0, dam_liquid_level);
  Blynk.virtualWrite(V1, dam_liquid_percentage);
}

void tankLiquidLevel() {
  tank_liquid_level++;
  tank_liquid_percentage = ((tank_liquid_level - tank_bottom_level) / tank_top_level) * 100; //Percentage of water in the container
  Serial.println(tank_liquid_level);//This will print the liquid level in the monitor
  Serial.println(tank_liquid_percentage);//This will print the percentage of liquid in the monitor
  Blynk.virtualWrite(V2, tank_liquid_level);
  Blynk.virtualWrite(V3, tank_liquid_percentage);
}


void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
