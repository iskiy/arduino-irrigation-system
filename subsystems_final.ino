#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);

class Subsystem
{
  private:
    bool _isWorking = false;
    unsigned long  _stopwatch;
    unsigned long _valveDowntime;
    unsigned long  _valveWorkingDelay;
    float _sensorValue;
    int _sensorPin;
    int _valvePin;

    int _triggerThreshold;
    byte _persistentMemoryAddress;

    void resetStopwatch() {
      _stopwatch = 0;
    };
    void closeValve();
  public:
    Subsystem(const int sensorPin, const int valvePin, const int persistentMemoryAddress,
              unsigned long valveDowntime = 240000, unsigned long valveWorkingDelay = 5000, int triggerThreshold = 95):
      _sensorPin(sensorPin), _valvePin(valvePin), _persistentMemoryAddress(persistentMemoryAddress),
      _triggerThreshold(triggerThreshold), _valveDowntime(valveDowntime), _valveWorkingDelay(valveWorkingDelay) {};

    ~Subsystem() {};
    const byte& persistentMemoryAddress() const {
      return _persistentMemoryAddress;
    }
    int& triggerThreshold() {
      return _triggerThreshold;
    }
    const float& sensorValue() const {
      return _sensorValue;
    };
    void irrigateGroundIfTooDry();
    void updateSensorValue();
    void updateStopwatch(unsigned int systemDelay) {
      _stopwatch += systemDelay;
    };
    void updateTriggerThreshold(unsigned int moistureStep);
    void checkValveActivation();
};

void Subsystem::checkValveActivation()
{
  if (!_isWorking && _stopwatch >= _valveDowntime) {
    resetStopwatch();
    irrigateGroundIfTooDry();
  } else if (_isWorking && _stopwatch >= _valveWorkingDelay) {
    resetStopwatch();
    closeValve();
  }
};

void Subsystem::updateSensorValue()
{
  _sensorValue = analogRead(_sensorPin);
  _sensorValue = map(_sensorValue, 0, 1023, 100, 0);
}

void Subsystem::irrigateGroundIfTooDry()
{
  if (sensorValue() < triggerThreshold())  {
    Serial.println("Valve " + String(_valvePin) + " is irrigating");
    digitalWrite(_valvePin, HIGH);
    _isWorking = true;
  }
}

void Subsystem::closeValve()
{
  digitalWrite(_valvePin, LOW);
  _isWorking = false;
}

void Subsystem::updateTriggerThreshold(unsigned int moistureStep) {
  _triggerThreshold >= 100 ? _triggerThreshold = 0 : _triggerThreshold += moistureStep;
}


// Subsystems values declaration

// Subsystem values 1
const int sensorPin1          = A0;
const int valvePin1           = 5;
int triggerThreshold1         = 95;
byte persistentMemoryAddress1 = 0;

// Subsystem values 2
const int sensorPin2          = A1;
const int valvePin2           = 6;
int triggerThreshold2         = 95;
byte persistentMemoryAddress2 = 1;

// Subsystem values 3
const int sensorPin3          = A2;
const int valvePin3           = 7;
int triggerThreshold3         = 95;
byte persistentMemoryAddress3 = 2;

// Subsystem values 4
const int sensorPin4          = A3;
const int valvePin4           = 8;
int triggerThreshold4         = 95;
byte persistentMemoryAddress4 = 3;

const unsigned long valveDowntime     = 5000; // the time (in milliseconds) during which the valves don`t work
const unsigned long valveWorkingDelay = 2000; // the time (in milliseconds) during which the valves operate after turning on

Subsystem sub1(sensorPin1, valvePin1, persistentMemoryAddress1, valveDowntime, valveWorkingDelay, triggerThreshold1);
Subsystem sub2(sensorPin2,valvePin2, persistentMemoryAddress2,valveDowntime, valveWorkingDelay, triggerThreshold2);
Subsystem sub3(sensorPin3, valvePin3, persistentMemoryAddress3, valveDowntime, valveWorkingDelay, triggerThreshold3);
Subsystem sub4(sensorPin4,valvePin4, persistentMemoryAddress4,valveDowntime, valveWorkingDelay, triggerThreshold4);

Subsystem subs [] = {sub1,sub2,sub3,sub4};
const size_t systemSize = sizeof(subs) / sizeof(subs[0]);

unsigned int systemDelay = 150;

const byte workingState          = 0;
const byte chooseSensorState     = 1;
const byte moistureSettingsState = 2;


// Buttons pins
const byte settingsBtnPin     = 4;
const byte valueCyclingBtnPin = 2;
const byte chooseBtnPin       = 3;

byte settingsBtnState;
byte valueCyclingBtnState;
byte chooseBtnState;

byte currentState = workingState; // in the initial state the system works in the working state

byte chosenSubsystem = 0; // index of the selected subsystem (required during subsystems settings)


void setup() {
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);

  pinMode(settingsBtnPin, INPUT_PULLUP);
  pinMode(valueCyclingBtnPin, INPUT_PULLUP);
  pinMode(chooseBtnPin, INPUT_PULLUP);

  pinMode(sensorPin1, INPUT);
  pinMode(valvePin1, OUTPUT);

  pinMode(sensorPin2, INPUT);
  pinMode(valvePin2, OUTPUT);

  pinMode(sensorPin3, INPUT);
  pinMode(valvePin3, OUTPUT);

  pinMode(sensorPin4, INPUT);
  pinMode(valvePin4, OUTPUT);
}

void loop() {
  readButtonsStates();

  switch (currentState)
  {
    case workingState:
      handleWorkingState();
      break;
    case chooseSensorState:
      handleChoosingSensorState();
      break;
    case moistureSettingsState:
      handleMoistureSettingsState();
      break;
    default:
      currentState = workingState;
      break;
  }

  updateStopwatches();
  delay(systemDelay);
}

void updateStopwatches() {
  for (size_t i = 0; i < systemSize; ++i)
  {
    subs[i].updateStopwatch(systemDelay);
  }
}

void readButtonsStates() {
  settingsBtnState = !digitalRead(settingsBtnPin);
  valueCyclingBtnState = !digitalRead(valueCyclingBtnPin);
  chooseBtnState = !digitalRead(chooseBtnPin);
}

void handleWorkingState() {
  showSensorValue();
  for (size_t i = 0; i < systemSize; ++i)
  {
    subs[i].triggerThreshold() = EEPROM.read(subs[i].persistentMemoryAddress());
    subs[i].updateSensorValue();
    chosenSubsystem = i;
    if (settingsBtnState == HIGH) {
      chosenSubsystem = 0;
      currentState = chooseSensorState;
      waitWhilePressed(settingsBtnPin);
    } else {
      subs[i].checkValveActivation();
    }
  }
}


void handleChoosingSensorState() {
  showChosenSensor();
  if (chooseBtnState == HIGH) {
    currentState = moistureSettingsState;
    waitWhilePressed(chooseBtnPin);
  } else if (settingsBtnState == HIGH) {
    currentState = workingState;
    waitWhilePressed(settingsBtnPin);
  } else if (valueCyclingBtnState == HIGH) {
    chosenSubsystem = (chosenSubsystem + 1) % systemSize;
  }
}

void handleMoistureSettingsState() {
  if (chooseBtnState == HIGH) {
    EEPROM.write(subs[chosenSubsystem].persistentMemoryAddress(), subs[chosenSubsystem].triggerThreshold());
    currentState = workingState;
    waitWhilePressed(chooseBtnPin);
  } else if (valueCyclingBtnState == HIGH) {
    subs[chosenSubsystem].updateTriggerThreshold(5);
  } else if (settingsBtnState == HIGH) {
    currentState = chooseSensorState;
    waitWhilePressed(settingsBtnPin);
  }

  showSensorSettings();
}

void waitWhilePressed(const byte buttonPin)
{
  int pressClock = 0;
  while(!digitalRead(buttonPin) && pressClock <= 500){
      pressClock += 50;
      delay(50);
  }
}

void showChosenSensor()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Chosen sensor: " + String(chosenSubsystem + 1));
}

void showSensorSettings()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sensor " + String(chosenSubsystem + 1));
  lcd.setCursor(1, 1);
  lcd.print("Threshold: " + String(subs[chosenSubsystem].triggerThreshold()));
}

void showSensorValue()
{
  lcd.clear();
  for (int i = 0; i < systemSize; ++i)
  {
    lcd.setCursor(0, i);
    lcd.print("Sensor " + String(i + 1) + ": " + subs[i].sensorValue());
  }
}
