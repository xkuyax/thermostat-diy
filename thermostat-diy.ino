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
#define MILLIS_CLOSE_TIMEOUT 10000
#define MOTOR_ENABLE_OFF 0
#define MOTOR_ENABLE_ON 255
#define MOTOR_ENABLE_CLOSE 200

uint8 encoderCount1 = 0;
uint8 encoderCount2 = 0;
unsigned long lastEncoderPulse = 0;
unsigned long lastEncoderDiff = 0;
uint8 state = 0;
unsigned long lastStateSwitch = 0;

void setState(int newState)
{
    lastStateSwitch = millis();
    state = newState;
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
    motorControl(1, MOTOR_ENABLE_ON);
}

void closeValve()
{
    setState(STATE_CLOSING);
    digitalWrite(LED_BUILTIN, HIGH);
    motorControl(2, MOTOR_ENABLE_CLOSE);
}

void motorControl(int mode, int power)
{
    lastEncoderPulse = micros();
    analogWrite(PIN_MOTOR_ENABLE, power);
    //   digitalWrite(PIN_MOTOR_ENABLE, 1);
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
    delay(100);
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
        bool disableMotors = false;
        uint32 delay = 0;
        if (state == STATE_CLOSING)
        {
            delay = MILLIS_CLOSE_TIMEOUT;
        }
        else
        {
            delay = MILLIS_STATE_TIMEOUT;
        }
        if (lastEncoderDiff > MICROS_ENCODER_TIMEOUT)
        {
            setState(STATE_STALL);
            Serial.print("State stall! ");
            Serial.println(lastEncoderDiff);
            disableMotors = true;
        }
        auto diff = millis() - lastStateSwitch;
        if (diff > delay)
        {
            setState(STATE_TIMEOUT);
            Serial.println("State timeout!");
            Serial.println(diff);
            Serial.println(delay);
            disableMotors = true;
        }
        if (disableMotors)
        {
            motorControl(0, MOTOR_ENABLE_OFF);
        }
    }
}