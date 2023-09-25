#define PIN_MOTOR_PHASE_1 D5
#define PIN_MOTOR_PHASE_2 D6
#define PIN_MOTOR_ENCODER_1 D1
#define PIN_MOTOR_ENCODER_2 D2
#define PIN_MOTOR_ENABLE D7
#define STATE_IDLE 0
#define STATE_OPENING 1
#define STATE_CLOSING 2
#define STATE_STALL 3
#define STATE_TIMEOUT 4
#define MICROS_ENCODER_TIMEOUT 5000
#define MILLIS_STATE_TIMEOUT 5000

uint8 encoderCount1 = 0;
uint8 encoderCount2 = 0;
unsigned long lastEncoderPulse = 0;
unsigned long lastEncoderDiff = 0;
uint8 state = 0;
unsigned long lastStateSwitch = 0;

void setState(int newState)
{
    state = newState;
    lastStateSwitch = millis();
}

void isrShared()
{
    auto now = micros();
    lastEncoderDiff = now - lastEncoderPulse;
    lastEncoderPulse = now;
}

void ICACHE_RAM_ATTR isrEncoder1()
{
    encoderCount1++;
    isrShared();
}

void ICACHE_RAM_ATTR isrEncoder2()
{
    encoderCount2++;
    isrShared();
}

void openValve()
{
    setState(STATE_OPENING);
    digitalWrite(LED_BUILTIN, HIGH);
    motorControl(1, 255);
}

void closeValve()
{
    setState(STATE_CLOSING);
    digitalWrite(LED_BUILTIN, HIGH);
    motorControl(2, 127);
    delay(100);
    int i = 0;
    unsigned long diff = 0;
    while ((diff < 20 || diff > 400000) && i < 6000)
    {
        diff = millis() - lastEncoderPulse;
        // Serial.println(diff);
        delay(1);
        i++;
    }
    if (i >= 5000)
    {
        Serial.println("timeout");
    }
    else
    {
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("stall detected");
    }
    motorControl(0, 0);
}

void motorControl(int mode, int power)
{
    lastEncoderPulse = micros();
    if (mode == 0)
    {
        digitalWrite(PIN_MOTOR_PHASE_1, 0);
        digitalWrite(PIN_MOTOR_PHASE_2, 0);
    }
    if (mode == 1)
    {
        digitalWrite(PIN_MOTOR_PHASE_1, 1);
        digitalWrite(PIN_MOTOR_PHASE_2, 0);
    }
    if (mode == 2)
    {
        digitalWrite(PIN_MOTOR_PHASE_1, 0);
        digitalWrite(PIN_MOTOR_PHASE_2, 1);
    }
    analogWrite(PIN_MOTOR_ENABLE, power);
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_MOTOR_PHASE_1, OUTPUT);
    pinMode(PIN_MOTOR_PHASE_2, OUTPUT);
    pinMode(PIN_MOTOR_ENCODER_1, INPUT_PULLUP);
    pinMode(PIN_MOTOR_ENCODER_1, INPUT_PULLUP);
    pinMode(PIN_MOTOR_ENABLE, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_MOTOR_ENCODER_1), isrEncoder1, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_MOTOR_ENCODER_2), isrEncoder2, RISING);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.begin(115200);
}

void loop()
{
    if (Serial.available() > 0)
    {
        auto read = Serial.readString();
        read.trim();
        if (read == "O")
        {
            Serial.println("Opening...!");
            openValve();
        }
        else if (read == "C")
        {
            Serial.println("Closing...!");
            closeValve();
        }
        else
        {
            Serial.print("Unknown command: ");
            Serial.println(read);
        }
    }
    if (state == STATE_CLOSING || state == STATE_OPENING)
    {
        if (lastEncoderDiff < MICROS_ENCODER_TIMEOUT)
        {
            setState(STATE_STALL);
            Serial.println("State stall!");
        }
        if (millis() - lastStateSwitch > MILLIS_STATE_TIMEOUT)
        {
            setState(STATE_TIMEOUT);
            Serial.println("State timoeut!");
        }
    }
}