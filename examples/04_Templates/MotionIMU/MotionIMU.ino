// Motion / IMU — a 3D orientation cube + accel/gyro + compass, all with NO hardware. The cube widget
// rotates live with the bound pitch/roll/yaw; dash.sensor("mpu6050", …) expands the accelerometer +
// gyroscope readouts; a compass gauge shows heading. RisalFakeIMU drives everything as a slowly
// tumbling board. Swap the fake for a real MPU6050/9250/BNO055 later — the dashboard is unchanged.
// Served over a plain access point — connect to "RisalDash-IMU" and open http://192.168.4.1/
#include <RisalUI.h>
#include <RisalFake.h>

RisalUI dash("Motion IMU");

RisalFakeIMU imu;
float ax = 0, ay = 0, az = 1, gx = 0, gy = 0, gz = 0;
float pitch = 0, roll = 0, yaw = 0, heading = 0, steps = 0;

void setup() {
  dash.timezone(180);

  dash.layout("Orientation", RICON_MOTION);
  dash.cube("Attitude", &pitch, &roll, &yaw).size(RSIZE_L);  // CSS 3D cube, pure client-side
  dash.gauge("Heading", &heading, 0, 360, "deg");
  dash.metric("Steps", &steps);

  dash.layout("Accel / Gyro", RICON_GAUGE);
  dash.sensor("mpu6050", &ax, &ay, &az, &gx, &gy, &gz).chart();  // 6 axes, each with a trend

  dash.beginAP("RisalDash-IMU", "12345678");
}

uint32_t last = 0;
void loop() {
  dash.update();
  if (millis() - last > 60) {   // ~16 Hz for lively motion
    last = millis();
    imu.update();
    ax = imu.accelX(); ay = imu.accelY(); az = imu.accelZ();
    gx = imu.gyroX(); gy = imu.gyroY(); gz = imu.gyroZ();
    pitch = imu.pitch(); roll = imu.roll(); yaw = imu.yaw();
    heading = imu.heading(); steps = imu.steps();
  }
}
