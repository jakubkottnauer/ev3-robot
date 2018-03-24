#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ev3.h"
#include "ev3_light.h"
#include "ev3_tacho.h"
#include "ev3_port.h"
#include "ev3_sensor.h"
#include "time.h"

const char const *color[] = {"?", "BLACK", "BLUE", "GREEN", "YELLOW", "RED", "WHITE", "BROWN"};

#define COLOR_COUNT ((int)(sizeof(color) / sizeof(color[0])))
#define Sleep(msec) usleep((msec)*1000)

struct timespec tstart = {0, 0}, tend = {0, 0};

static float times[128] = {};
static int states[128] = {};
int updates = 0;
int dataLines = 0;

const float NORMAL_VELOCITY = 0.35;
const float TURN_VELOCITY = 0.6;
const int WHITE_THRESH = 60;
const int BLACK_THRESH = 20;

static int state = 0;
uint8_t sn_color;
uint8_t motor_left;
uint8_t motor_right;
uint8_t servo;

int getMaxSpeed(sn)
{
  int max_speed;
  get_tacho_max_speed(sn, &max_speed);
  return max_speed;
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

void followPath(void)
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
    runMotor(motor_left, NORMAL_VELOCITY, true);
    runMotor(motor_right, NORMAL_VELOCITY, true);
  }
}

void followPathTurbo(void)
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
    runMotor(motor_left, TURN_VELOCITY, true);
    runMotor(motor_right, TURN_VELOCITY, true);
  }
}

void turnRight(void)
{
  runMotor(motor_left, NORMAL_VELOCITY, true);
  runMotor(motor_right, NORMAL_VELOCITY, false);
}

void turnLeft(void)
{
  runMotor(motor_left, NORMAL_VELOCITY, false);
  runMotor(motor_right, NORMAL_VELOCITY, true);
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

  ev3_search_tacho_plugged_in(ev3_tacho[0].port, ev3_tacho[0].extport, &motor_right, 0);
  ev3_search_tacho_plugged_in(ev3_tacho[1].port, ev3_tacho[1].extport, &motor_left, 0);
  /*for (i = 0; i < DESC_LIMIT; i++ ) {
    if (ev3_tacho[i].type_inx != TACHO_TYPE__NONE_) {
      printf( "  type = %s\n", ev3_tacho_type( ev3_tacho[ i ].type_inx ));
      printf( "  port = %s\n", ev3_tacho_port_name( i, s ));
      printf( "  i = %d\n", i);
      printf( "  ext port = %d\n", ev3_tacho[i].extport);

      if (ev3_tacho[i].port == 0) {
        ev3_search_tacho_plugged_in(ev3_tacho[i].port, ev3_tacho[i].extport, &motor_right, 0);
      } else if (ev3_tacho[i].port == 2) {
        
      }
    }
  }*/

  while (true)
  {
    switch (state)
    {
    case 0:
    {
      followPath();
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
    case 2:
    {
      reverse();
      break;
    }
    case 4:
    {
      followPathTurbo();
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
    }

    Sleep(20);
    clock_gettime(CLOCK_MONOTONIC, &tend);
    double seconds = ((double)tend.tv_sec + 1.0e-9 * tend.tv_nsec) -
                     ((double)tstart.tv_sec + 1.0e-9 * tstart.tv_nsec);
    printf("time %f\n", seconds);
    if (updates >= dataLines)
    {
      continue;
    }

    if (seconds >= times[updates])
    {
      state = states[updates];
      updates++;
      clock_gettime(CLOCK_MONOTONIC, &tstart);
    }
  }

  ev3_uninit();

  return 0;
}
