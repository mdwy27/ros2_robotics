#include <Arduino.h>
#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <rmw_microros/rmw_microros.h>
#include <rosidl_runtime_c/string_functions.h>
#include <geometry_msgs/msg/twist.h>
#include <nav_msgs/msg/odometry.h>

//wiring scheme, connects ESP32-S3 (microcontroller) and AT8236 (motordriver)
//the esp32 I am using is from Adafruit, the AT8236 board is from Yahboom
//edit these pin numbers as needed to accomodate your setup
const int L_IN1 = 6, L_IN2 = 7;   // left motor/wheel -> AT8236 (swapped because originally motor A spun wheel backwards)
const int R_IN1 = 5, R_IN2 = 4;   // right motor/wheel -> AT8236

//encoder wiring: each motor's hall encoder has two signals (A and B) -> ESP32 GPIO
//checked with multimeter first: encoder signals peak at ~3.3V, safe for ESP32 pins
const int R_ENC_A = 10, R_ENC_B = 11;   // E1A, E1B (motor A, right wheel)
const int L_ENC_A = 12, L_ENC_B = 13;   // E2A, E2B (motor B, left wheel)

//robot measurements, all in meters (ROS standard units)
const float WHEEL_SEP = 0.12;     // m, measure center-to-center of wheels
const float WHEEL_RADIUS = 0.033;      // m, measured ~66 mm wheel diameter
const float TICKS_PER_REV = 1783.0;    // measured: turned each wheel 10 full turns by hand, ~17830 ticks (4x quadrature decoding)
const float M_PER_TICK = 2.0 * PI * WHEEL_RADIUS / TICKS_PER_REV;   // distance traveled per encoder tick (~0.116 mm)

const float MAX_SPEED = 0.5;      // m/s treated as full PWM (rough, tune later)
const unsigned long TIMEOUT_MS = 500; // stop if no command for this long
const int MIN_PWM = 120;   // figured this out from troubleshooting: kept raising MIN_PWM until bot was able to turn R/L just slightly
//the threshold MIN_PWM appeared to be ~100, anything below this and the bot couldn't turn, motors board beeped indicating error
const unsigned long ODOM_PERIOD_MS = 50;   // publish odometry every 50 ms (20 Hz)

//declaring nodes and subscriptions to shared topics
rcl_subscription_t sub;
geometry_msgs__msg__Twist cmd_msg;   // incoming /cmd_vel message (renamed from msg to avoid confusion with odom_msg)
rcl_publisher_t odom_pub;            // publisher for /odom
nav_msgs__msg__Odometry odom_msg;    // outgoing odometry message
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
unsigned long lastCmd = 0;
unsigned long lastOdom = 0;

//encoder tick counters, updated by interrupts every time an encoder signal changes
//volatile tells the compiler these can change at any moment (from the interrupts)
volatile long rightTicks = 0, leftTicks = 0;
volatile uint8_t rightState = 0, leftState = 0;

//quadrature lookup table: compares old vs new A/B signal pattern to decide
//if the wheel moved one step forward (+1), backward (-1), or not at all (0)
const int8_t QDEC[16] = { 0, +1, -1,  0,
                         -1,  0,  0, +1,
                         +1,  0,  0, -1,
                          0, -1, +1,  0 };

//interrupt functions: run automatically whenever an encoder signal changes
//IRAM_ATTR keeps them in fast memory so they run quickly
void IRAM_ATTR rightISR() {
  uint8_t s = (digitalRead(R_ENC_A) << 1) | digitalRead(R_ENC_B);
  rightTicks += QDEC[(rightState << 2) | s];
  rightState = s;
}
void IRAM_ATTR leftISR() {
  uint8_t s = (digitalRead(L_ENC_A) << 1) | digitalRead(L_ENC_B);
  leftTicks += QDEC[(leftState << 2) | s];
  leftState = s;
}

//robot's estimated position: x, y in meters, theta (heading) in radians
//starts at 0,0,0 = wherever the robot is when it powers on (the "odom" frame origin)
float x = 0.0, y = 0.0, theta = 0.0;
long prevLeft = 0, prevRight = 0;

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

//odometry: turns encoder ticks into the robot's estimated position and publishes it on /odom
//this is the reverse of cmdVelCb: there, robot motion -> wheel speeds; here, wheel motion -> robot motion
void updateOdometry() {
  //copy tick counts with interrupts paused, so they can't change halfway through reading
  noInterrupts();
  long r = rightTicks;
  long l = -leftTicks;   // left encoder counts negative going forward (mounted mirrored), so flip its sign
  interrupts();

  float dt = ODOM_PERIOD_MS / 1000.0;

  //how far each wheel moved since the last update (m)
  float dR = (r - prevRight) * M_PER_TICK;
  float dL = (l - prevLeft)  * M_PER_TICK;
  prevRight = r;
  prevLeft = l;

  //how the robot moved: forward distance = average of wheels; turning = difference between wheels
  float ds = (dR + dL) / 2.0;
  float dth = (dR - dL) / WHEEL_SEP;

  //add this small motion onto the running position estimate (dead reckoning)
  //uses the heading halfway through the step for better accuracy on curves
  x += ds * cos(theta + dth / 2.0);
  y += ds * sin(theta + dth / 2.0);
  theta += dth;

  //timestamp the message using the clock synced with the Pi (see setup)
  int64_t ns = rmw_uros_epoch_nanos();
  odom_msg.header.stamp.sec = (int32_t)(ns / 1000000000LL);
  odom_msg.header.stamp.nanosec = (uint32_t)(ns % 1000000000LL);

  //position and heading (ROS stores heading as a quaternion; for rotation about z only, z = sin(theta/2), w = cos(theta/2))
  odom_msg.pose.pose.position.x = x;
  odom_msg.pose.pose.position.y = y;
  odom_msg.pose.pose.position.z = 0.0;
  odom_msg.pose.pose.orientation.x = 0.0;
  odom_msg.pose.pose.orientation.y = 0.0;
  odom_msg.pose.pose.orientation.z = sin(theta / 2.0);
  odom_msg.pose.pose.orientation.w = cos(theta / 2.0);

  //current speed, in the robot's own frame
  odom_msg.twist.twist.linear.x = ds / dt;
  odom_msg.twist.twist.angular.z = dth / dt;

  rcl_publish(&odom_pub, &odom_msg, NULL);
}

//runs when esp32 is powered on
//gets motors, nodes, subscriptions up and running; connects esp32 to raspberry pi
void setup() {
  pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  stopMotors();

  //encoder pins as inputs, record their starting state, and attach interrupts
  //so every change on either signal (A or B) gets counted
  pinMode(R_ENC_A, INPUT_PULLUP); pinMode(R_ENC_B, INPUT_PULLUP);
  pinMode(L_ENC_A, INPUT_PULLUP); pinMode(L_ENC_B, INPUT_PULLUP);
  rightState = (digitalRead(R_ENC_A) << 1) | digitalRead(R_ENC_B);
  leftState  = (digitalRead(L_ENC_A) << 1) | digitalRead(L_ENC_B);
  attachInterrupt(digitalPinToInterrupt(R_ENC_A), rightISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(R_ENC_B), rightISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(L_ENC_A), leftISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(L_ENC_B), leftISR, CHANGE);

  Serial.begin(115200);
  set_microros_serial_transports(Serial);
  delay(2000);

  allocator = rcl_get_default_allocator();
  rclc_support_init(&support, 0, NULL, &allocator);
  rclc_node_init_default(&node, "gigi_base", "", &support);
  rclc_subscription_init_default(&sub, &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "cmd_vel");

  //publisher: same node now also sends odometry out on /odom
  rclc_publisher_init_default(&odom_pub, &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(nav_msgs, msg, Odometry), "odom");

  //set the odometry message's fixed fields once: which frames it relates (matches the simulation)
  nav_msgs__msg__Odometry__init(&odom_msg);
  rosidl_runtime_c__String__assign(&odom_msg.header.frame_id, "odom");
  rosidl_runtime_c__String__assign(&odom_msg.child_frame_id, "base_footprint");

  //esp32 doesn't know the real time on its own; ask the agent on the pi so timestamps match the pi's clock
  rmw_uros_sync_session(1000);

  rclc_executor_init(&executor, &support.context, 1, &allocator);
  rclc_executor_add_subscription(&executor, &sub, &cmd_msg, &cmdVelCb, ON_NEW_DATA);
}

void loop() {
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
  if (millis() - lastCmd > TIMEOUT_MS) stopMotors();   // stops motors if no command has been received for 5 seconds; safety feature

  //compute and publish odometry on a fixed schedule (every ODOM_PERIOD_MS)
  if (millis() - lastOdom >= ODOM_PERIOD_MS) {
    lastOdom = millis();
    updateOdometry();
  }
}