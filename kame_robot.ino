
#include <Servo.h>
#include <math.h>

// =====================================================
// KAME QUADRUPED ROBOT
// SEQUENTIAL FOUR-LEG GAIT
// Board: Arduino Nano
// Servos: 8
//
// Sequence: FL -> BR -> FR -> BL -> REPEAT
// =====================================================

// ---------------- SERVO INDEX ----------------

enum {
  FL_HIP, FL_KNEE,
  FR_HIP, FR_KNEE,
  BL_HIP, BL_KNEE,
  BR_HIP, BR_KNEE
};

const uint8_t SERVO_PINS[8] = {
  2, 3, 4, 5, 6, 7, 8, 9
};

// ---------------- YOUR ORIGINAL CALIBRATION ----------------

int8_t TRIM[8] = {
  0, 33, 0, 13, -30, 13, 20, -75
};

int8_t DIR[8] = {
  1, -1, -1, 1, 1, -1, -1, 1
};

// ---------------- GAIT SETTINGS ----------------

// Movement amplitudes
float HIP_AMPLITUDE = 30.0;
float KNEE_LIFT = 35.0;

// Time for each movement stage
unsigned long STAGE_TIME = 2000;

// Servo smoothing
const float MAX_STEP_DEG = 2.5;

// Startup delay
const unsigned long START_DELAY_MS = 500;

// Update interval
const unsigned long UPDATE_INTERVAL = 20;

// ---------------- LEG ORDER ----------------

// 0 = FL
// 1 = FR
// 2 = BL
// 3 = BR

const uint8_t LEG_ORDER[4] = {
  0, 3, 1, 2
};

// Hip and knee indexes
const uint8_t hipIdx[4] = {
  FL_HIP, FR_HIP, BL_HIP, BR_HIP
};

const uint8_t kneeIdx[4] = {
  FL_KNEE, FR_KNEE, BL_KNEE, BR_KNEE
};

// ---------------- HIP DIRECTIONS ----------------

// +1 = RIGHT
// -1 = LEFT
//
// FL = RIGHT first
// BR = LEFT first
// FR = LEFT first
// BL = RIGHT first

const int8_t HIP_DIRECTION[4] = {
  1,   // FL: RIGHT then LEFT
  1,  // FR: LEFT then RIGHT
  -1,  // BL: LEFT then RIGHT
  1    // BR: RIGHT then LEFT
};
// ---------------- GLOBAL VARIABLES ----------------

Servo servos[8];

float cur[8];
float startAngle[8];
float targetAngle[8];

uint8_t orderPosition = 0;
uint8_t currentLeg = 0;
uint8_t currentStage = 0;

unsigned long stageStart = 0;
unsigned long lastUpdate = 0;
unsigned long bootTime = 0;

bool autoStarted = false;

// ---------------- SMOOTHING ----------------

float smooth(float x) {

  x = constrain(x, 0.0, 1.0);

  return x * x * (3.0 - 2.0 * x);
}

// ---------------- SERVO MOVEMENT ----------------

void setServoTarget(uint8_t index, float offset) {

  targetAngle[index] = constrain(
    90.0 + TRIM[index] + DIR[index] * offset,
    0.0,
    180.0
  );
}

void updateServos() {

  for (uint8_t i = 0; i < 8; i++) {

    if (cur[i] < targetAngle[i] - MAX_STEP_DEG) {

      cur[i] += MAX_STEP_DEG;

    }
    else if (cur[i] > targetAngle[i] + MAX_STEP_DEG) {

      cur[i] -= MAX_STEP_DEG;

    }
    else {

      cur[i] = targetAngle[i];

    }

    servos[i].write((int)cur[i]);
  }
}

// ---------------- INITIALIZE ALL LEGS ----------------

void resetTargets() {

  for (uint8_t i = 0; i < 8; i++) {

    setServoTarget(i, 0);

  }
}

// ---------------- START NEXT LEG ----------------

void startLeg(uint8_t leg) {
  currentLeg = leg;
  currentStage = 0;
  stageStart = millis();

  resetTargets();

  Serial.print("Starting leg: ");
  Serial.println(leg);
};

// ---------------- SET MOVEMENT STAGE ----------------

void setStage(uint8_t leg, uint8_t stage) {

  uint8_t hip = hipIdx[leg];
  uint8_t knee = kneeIdx[leg];

  int8_t direction = HIP_DIRECTION[leg];

  // Reset all targets first
  resetTargets();

  switch (stage) {

    // ---------------------------------
    // STAGE 0: KNEE UP
    // ---------------------------------

    case 0:

      setServoTarget(knee, KNEE_LIFT);

      break;

    // ---------------------------------
    // STAGE 1: HIP MOVES FIRST DIRECTION
    // ---------------------------------

    case 1:

      setServoTarget(knee, KNEE_LIFT);

      setServoTarget(
        hip,
        direction * HIP_AMPLITUDE
      );

      break;

    // ---------------------------------
    // STAGE 2: KNEE DOWN
    // ---------------------------------

    case 2:

      setServoTarget(knee, 0);

      setServoTarget(
        hip,
        direction * HIP_AMPLITUDE
      );

      break;

    // ---------------------------------
    // STAGE 3: HIP RETURNS
    // ---------------------------------

    case 3:

      setServoTarget(knee, 0);

      setServoTarget(
        hip,
        -direction * HIP_AMPLITUDE
      );

      break;
  }
}

// ---------------- SEQUENTIAL GAIT ----------------

void updateGait() {
  unsigned long now = millis();

  if (now - stageStart >= STAGE_TIME) {
    currentStage++;

    if (currentStage >= 4) {
      currentStage = 0;
      orderPosition = (orderPosition + 1) % 4;
      startLeg(LEG_ORDER[orderPosition]);
    }

    setStage(currentLeg, currentStage);
    stageStart = now;
  }
}
// ---------------- SETUP ----------------

void setup() {
  Serial.begin(9600);

  for (uint8_t i = 0; i < 8; i++) {
    servos[i].attach(SERVO_PINS[i]);

    cur[i] = 90 + TRIM[i];
    targetAngle[i] = cur[i];

    servos[i].write(cur[i]);
  }

  delay(1000);

  resetTargets();

  bootTime = millis();
  stageStart = millis();
  lastUpdate = millis();

  orderPosition = 0;

  startLeg(LEG_ORDER[orderPosition]);
  setStage(currentLeg, 0);

  Serial.println("KAME automatic gait started!");
}

// ---------------- MAIN LOOP ----------------

void loop() {

  unsigned long now = millis();

  // Wait before starting
  if (!autoStarted) {

    if (now - bootTime >= START_DELAY_MS) {

      autoStarted = true;

      Serial.println(F("Starting automatic walking."));

      startLeg(LEG_ORDER[0]);

      setStage(currentLeg, 0);

    }

    return;
  }

  // Update every 20 milliseconds
  if (now - lastUpdate < UPDATE_INTERVAL) {

    return;

  }

  lastUpdate = now;

  // Update gait sequence
  updateGait();

  // Update all servo positions
  updateServos();
}
