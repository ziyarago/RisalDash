#include <RisalUI.h>

RisalUI dash("JoystickCube");

float pitch = 0.0f;
float roll  = 0.0f;
float yaw   = 0.0f;

//Joystick output: -1..1
float pitchSpeed = 0.0f;
float rollSpeed = 0.0f;
float yawSpeed = 0.0f;

int pitchSlider = 0;
int rollSlider  = 0;
int yawSlider   = 0;

int sensitivity   = 30;
uint32_t lastUpdate = 0;

void setup() {  
  // Serial.begin(115200); //debug

  // 3D cube visualizes pitch/roll/yaw
  dash.cube("Cube", &pitch, &roll, &yaw).size(RSIZE_M);

  // Joystick controls pitch/yaw/roll speed
  dash.joystick("Joy Pitch&Yaw", [](float x, float y) { yawSpeed  = x; pitchSpeed = y;
    // Serial.print("yaw="); Serial.print(x); Serial.print("pitch="); Serial.println(y); //debug
    }).variant("xy").threshold(0.1f).size(RSIZE_S);
  dash.joystick("Joy Roll", [](float x, float y) { rollSpeed  = x;
    // Serial.print("roll="); Serial.println(x); //debug
    }).variant("x").threshold(0.1f).size(RSIZE_S);  

  // Independent controls for all three axes
  dash.slider("Pitch", &pitchSlider, -180, 180, [](int v) { pitch = (float)v; }).size(RSIZE_S);
  dash.slider("Roll", &rollSlider, -180, 180, [](int v) { roll = (float)v; }).size(RSIZE_S);
  dash.slider("Yaw", &yawSlider, -180, 180, [](int v) { yaw = (float)v; }).size(RSIZE_S);

  // Joystick sensivity
  dash.number("Sensivity", &sensitivity, 1, 100, 1, [](int v) { (void)v; }).size(RSIZE_S);
  
  dash.button("Button", "Reset", []() { pitch = 0.0f; roll = 0.0f; yaw = 0.0f; pitchSlider = 0; rollSlider = 0; yawSlider = 0; }).size(RSIZE_S);

  dash.apName("RisalDash-Joystick");
  dash.begin();
}

void loop() {
  uint32_t now = millis();
  float dt = (now - lastUpdate) * 0.001f;
  lastUpdate = now;

  pitch += pitchSpeed * sensitivity * dt;
  roll  += rollSpeed  * sensitivity * dt;
  yaw   += yawSpeed   * sensitivity * dt;

  // Keep angles in a convenient range.
  if (pitch > 180.0f) pitch -= 360.0f;
  if (pitch < -180.0f) pitch += 360.0f;

  if (roll > 180.0f) roll -= 360.0f;
  if (roll < -180.0f) roll += 360.0f;

  if (yaw > 180.0f) yaw -= 360.0f;
  if (yaw < -180.0f) yaw += 360.0f;

  pitchSlider = (int)pitch;
  rollSlider = (int)roll;
  yawSlider = (int)yaw;

  dash.update();
}
