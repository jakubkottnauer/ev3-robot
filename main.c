#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ev3.h"
#include "ev3_light.h"
#include "ev3_tacho.h"
#include "ev3_port.h"
#include "ev3_sensor.h"
#include "ev3_servo.h"
#include "time.h"

const char const *color[] = {"?", "BLACK", "BLUE", "GREEN", "YELLOW", "RED", "WHITE", "BROWN"};

#define COLOR_COUNT ((int)(sizeof(color) / sizeof(color[0])))
#define Sleep(msec) usleep((msec)*1000)

struct timespec tstart = {0, 0}, tend = {0, 0};

static float times[128] = {};
static int states[128] = {};
int updates = 0;
int dataLines = 0;

const float NORMAL_VELOCITY = 0.4;
const float TURBO_VELOCITY = 0.63;
const float TURN_VELOCITY = 0.2;
const int WHITE_THRESH = 65;
const int BLACK_THRESH = 25;

static int state = 0;
uint8_t sn_color;
uint8_t motor_left;
uint8_t motor_right;
uint8_t servo;
uint8_t sn_ir;

int getMaxSpeed(sn)
{
  int max_speed;
  get_tacho_max_speed(sn, &max_speed);
  return max_speed;
}

void moveIfObstacle()
{
  int val, i;
  get_sensor_value(0, sn_ir, &val);
  Sleep(20);
  if (val < 10)
  {
    for (i = 0; i < 5; i++)
    {
      reverse();
    }
  }
}

double getElapsedTime()
{
  clock_gettime(CLOCK_MONOTONIC, &tend);
  double seconds = ((double)tend.tv_sec + 1.0e-9 * tend.tv_nsec) -
                   ((double)tstart.tv_sec + 1.0e-9 * tstart.tv_nsec);
  printf("time %f\n", seconds);
  return seconds;
}

void runMotor(uint8_t sn, float mult, bool forward)
{
  int max_speed = getMaxSpeed(sn);
  FLAGS_T state;

  set_tacho_stop_action_inx(sn, TACHO_COAST);
  set_tacho_speed_sp(sn, max_speed * mult);
  set_tacho_time_sp(sn, 50);
  set_tacho_ramp_up_sp(sn, 20);
  set_tacho_ramp_down_sp(sn, 20);
  set_tacho_command_inx(sn, TACHO_RUN_TIMED);
  set_tacho_polarity_inx(sn, forward ? TACHO_NORMAL : TACHO_INVERSED);
}

void goForward(void)
{
  runMotor(motor_left, NORMAL_VELOCITY, true);
  runMotor(motor_right, NORMAL_VELOCITY, true);
}

void reverse(void)
{
  runMotor(motor_left, NORMAL_VELOCITY, false);
  runMotor(motor_right, NORMAL_VELOCITY, false);
}

void followPath(bool turbo)
{
  int val;

  // 100 white, 0 black
  set_sensor_mode(sn_color, "COL-REFLECT");
  get_sensor_value(0, sn_color, &val);

  if (val > WHITE_THRESH)
  {
    runMotor(motor_left, TURN_VELOCITY, true);
  }
  else if (val < BLACK_THRESH)
  {
    runMotor(motor_right, TURN_VELOCITY, true);
  }
  else
  {
    runMotor(motor_left, turbo ? TURBO_VELOCITY : NORMAL_VELOCITY, true);
    runMotor(motor_right, turbo ? TURBO_VELOCITY : NORMAL_VELOCITY, true);
  }
}

void turnRight(void)
{
  runMotor(motor_left, TURN_VELOCITY, true);
  runMotor(motor_right, TURN_VELOCITY, false);
}

void turnLeft(void)
{
  runMotor(motor_left, TURN_VELOCITY, false);
  runMotor(motor_right, TURN_VELOCITY, true);
}

void followPathUntilWhite(bool turbo)
{
  int val, i;
  int whiteCount = 0;
  // 100 white, 0 black
  while (true)
  {
    set_sensor_mode(sn_color, "COL-REFLECT");
    get_sensor_value(0, sn_color, &val);

    if (val > WHITE_THRESH)
    {
      runMotor(motor_left, TURN_VELOCITY, true);
    }
    else if (val < BLACK_THRESH)
    {
      runMotor(motor_right, TURN_VELOCITY, true);
      whiteCount = 0;
    }
    else
    {
      runMotor(motor_left, turbo ? TURBO_VELOCITY : NORMAL_VELOCITY, true);
      runMotor(motor_right, turbo ? TURBO_VELOCITY : NORMAL_VELOCITY, true);
      whiteCount++;
    }

    moveIfObstacle();

    if (whiteCount > 20)
    {
      set_sensor_mode(sn_color, "COL-COLOR");

      whiteCount = 0;
      Sleep(20);
      bool whiteDetected = true;
      for (i = 0; i < 15; i++)
      {
        turnLeft();
        get_sensor_value(0, sn_color, &val);
        whiteDetected = whiteDetected && strcmp(color[val], "WHITE") == 0;
        Sleep(50);
      }

      if (whiteDetected)
      {

        for (i = 0; i < 30; i++)
        {
          turnRight();
          get_sensor_value(0, sn_color, &val);
          whiteDetected = whiteDetected && strcmp(color[val], "WHITE") == 0;
          Sleep(50);
        }

        for (i = 0; i < 20; i++)
        {
          turnLeft();
        }

        Sleep(20);

        if (whiteDetected)
        {
          return;
        }
      }
      else
      {
        for (i = 0; i < 13; i++)
        {
          turnRight();
          Sleep(50);
        }
      }
    }

    Sleep(20);
  }
}

void turnRightUntilBlack(void)
{
  int val;
  set_sensor_mode(sn_color, "COL-COLOR");
  get_sensor_value(0, sn_color, &val);

  while (strcmp(color[val], "BLACK") != 0)
  {
    turnRight();
    Sleep(20);
    get_sensor_value(0, sn_color, &val);
  }

  //printf("exit %i\n", val);
}

void parseData()
{
  FILE *stream = fopen("data.txt", "r");
  char line[1024];
  while (fgets(line, 1024, stream))
  {
    char *tmp = strdup(line);
    char *pch;
    pch = strtok(tmp, ";");
    int c = 0;
    while (pch != NULL)
    {
      if (c == 0)
      {
        times[dataLines] = (float)atof(pch);
      }
      else if (c == 1)
      {
        states[dataLines] = (int)atoi(pch);
      }
      c++;
      pch = strtok(NULL, ";");
    }
    dataLines++;
    free(tmp);
  }
}

void measureLight(void)
{
  int val;
  set_sensor_mode(sn_color, "COL-COLOR");

  while (true)
  {
    get_sensor_value(0, sn_color, &val);
    printf("%s\n", color[val]);
    Sleep(100);
  }
}

void runServo(bool down)
{
  runMotor(servo, 1, down);
  runMotor(servo, 1, down);
  runMotor(servo, 1, down);
  runMotor(servo, 1, down);
  runMotor(servo, 1, down);
  runMotor(servo, 1, down);
  runMotor(servo, 1, down);
  runMotor(servo, 1, down);
  runMotor(servo, 1, down);
  Sleep(50);
}

void carSeeker()
{
  int i;
  int val;
  int min = 100;

  for (i = 0; i < 15; i++)
  {
    turnLeft();
  }

  for (i = 0; i < 30; i++)
  {
    turnRight();
    Sleep(20);
    get_sensor_value(0, sn_ir, &val);
    if (val < min)
    {
      min = val;
    }
  }

  min += 4;

  for (i = 0; i < 60; i++)
  {
    turnLeft();
    Sleep(20);
    get_sensor_value(0, sn_ir, &val);
    if (val < min)
    {
      break;
    }
  }

  runServo(false);
  turnRight();
  turnRight();
  Sleep(20);
  for (i = 0; i < 50; i++)
  {
    goForward();
    Sleep(20);
  }
  runServo(true);
  Sleep(20);

  for (i = 0; i < 50; i++)
  {
    reverse();
    Sleep(20);
  }
}

int main(void)
{
  parseData();
  clock_gettime(CLOCK_MONOTONIC, &tstart);
  int i = 0;
  uint8_t sn;
  FLAGS_T state;
  char s[256];

  if (ev3_init() < 1)
    return (1);
  ev3_port_init();
  ev3_tacho_init();
  ev3_sensor_init();

  ev3_search_sensor(LEGO_EV3_COLOR, &sn_color, 0);
  ev3_search_sensor(LEGO_EV3_IR, &sn_ir, 0);

  int LEFT_MOTOR = 2;
  int RIGHT_MOTOR = 0;
  int SERVO = 3;

  ev3_search_tacho_plugged_in(ev3_tacho[RIGHT_MOTOR].port, ev3_tacho[RIGHT_MOTOR].extport, &motor_right, 0);
  ev3_search_tacho_plugged_in(ev3_tacho[LEFT_MOTOR].port, ev3_tacho[LEFT_MOTOR].extport, &motor_left, 0);
  ev3_search_tacho_plugged_in(ev3_tacho[SERVO].port, ev3_tacho[SERVO].extport, &servo, 0);
  for (i = 0; i < DESC_LIMIT; i++)
  {
    if (ev3_tacho[i].type_inx != TACHO_TYPE__NONE_)
    {
      printf("  type = %s\n", ev3_tacho_type(ev3_tacho[i].type_inx));
      printf("  port = %s\n", ev3_tacho_port_name(i, s));
      printf("  i = %d\n", i);
      printf("  ext port = %d\n", ev3_tacho[i].extport);
    }
  }

  while (true)
  {
    switch (state)
    {
    case 0:
    {
      followPath(false);
      break;
    }
    case 1:
    {
      turnRight();
      break;
    }
    case 2:
    {
      turnLeft();
      break;
    }
    case 3:
    {
      reverse();
      break;
    }
    case 4:
    {
      followPath(true);
      break;
    }
    case 5:
    {
      goForward();
      break;
    }
    case 6:
    {
      break;
    }
    case 7:
    {
      turnRightUntilBlack();
      break;
    }
    case 8:
    {
      followPathUntilWhite(false);
      break;
    }
    case 9:
    {
      carSeeker();
      break;
    }
    case 88:
    {
      measureLight();
      break;
    }
    }

    Sleep(20);

    if (updates >= dataLines)
    {
      continue;
    }

    double elapsedTime = getElapsedTime();

    if (elapsedTime >= times[updates])
    {
      state = states[updates];
      updates++;
      clock_gettime(CLOCK_MONOTONIC, &tstart);
    }
  }

  ev3_uninit();

  return 0;
}
