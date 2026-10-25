#include <Arduino.h>
#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>

//wiring scheme, connects ESP32-S3 (microcontroller) and AT8236 (motordriver)
//the esp32 I am using is from Adafruit, the AT8236 board is from Yahboom
//edit these pin numbers as needed to accomodate your setup
const int L_IN1 = 6, L_IN2 = 7;   // left motor/wheel -> AT8236 (swapped because originally motor A spun wheel backwards)
const int R_IN1 = 5, R_IN2 = 4;   // right motor/wheel -> AT8236
const float WHEEL_SEP = 0.12;     // m, measure center-to-center of wheels
const float MAX_SPEED = 0.5;      // m/s treated as full PWM (rough, tune later)
const unsigned long TIMEOUT_MS = 500; // stop if no command for this long
const int MIN_PWM = 120;   // figured this out from troubleshooting: kept raising MIN_PWM until bot was able to turn R/L just slightly
//the threshold MIN_PWM appeared to be ~100, anything below this and the bot couldn't turn, motors board beeped indicating error

//declaring nodes and subscriptions to shared topics
rcl_subscription_t sub;
geometry_msgs__msg__Twist msg;
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
unsigned long lastCmd = 0;

//declaring and defining the function that will set speed of both motors; turns the commanded speed (m/s) into a PWM value
//originally, motors weren't getting large enough signal to actually turn wheels, there was a loud beeping noise
//so, now a MIN_PWM is being implemented to ensure the wheels get a strong enough signal
void setMotor(int in1, int in2, float v) {
  int pwm = 0;
  if (fabs(v) > 0.001) {
    // maps (0, MAX_SPEED] onto [MIN_PWM, 255] instead of [0, 255] to ensure signal is sufficient to turn wheels
    pwm = MIN_PWM + (int)(fabs(v) / MAX_SPEED * (255 - MIN_PWM));
  }
  pwm = constrain(pwm, 0, 255);
  if (v >= 0) { analogWrite(in1, pwm); analogWrite(in2, 0); }
  else        { analogWrite(in1, 0);   analogWrite(in2, pwm); }
}

//declaring and defining the function that will stop motors (useful if cmd_vel stops coming through, prevents robot from continuing on forever)
void stopMotors() { setMotor(L_IN1, L_IN2, 0); setMotor(R_IN1, R_IN2, 0); }

//this function will be used to turn commands sent over cmd_vel into velocity form (m/s)
void cmdVelCb(const void *m) {
  const auto *t = (const geometry_msgs__msg__Twist *)m;
  float v = t->linear.x, w = t->angular.z;
  setMotor(L_IN1, L_IN2, v - w * WHEEL_SEP / 2);   // diff-drive kinematics
  setMotor(R_IN1, R_IN2, v + w * WHEEL_SEP / 2);
  lastCmd = millis();
}

//runs when esp32 is powered on
//gets motors, nodes, subscriptions up and running; connects esp32 to raspberry pi
void setup() {
  pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  stopMotors();

  Serial.begin(115200);
  set_microros_serial_transports(Serial);
  delay(2000);

  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "gigi_base", "", &support);
  rclc_subscription_init_default(&sub, &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "cmd_vel");
  rclc_executor_init(&executor, &support.context, 1, &allocator);
  rclc_executor_add_subscription(&executor, &sub, &msg, &cmdVelCb, ON_NEW_DATA);
}

void loop() {
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
  if (millis() - lastCmd > TIMEOUT_MS) stopMotors();   // stops motors if no command has been received for 5 seconds; safety feature
}