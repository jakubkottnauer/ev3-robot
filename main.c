#include <stdio.h>
#include <string.h>

#include "ev3.h"
#include "ev3_light.h"
#include "ev3_tacho.h"
#include "ev3_port.h"
#include "ev3_sensor.h"

const char const *color[] = { "?", "BLACK", "BLUE", "GREEN", "YELLOW", "RED", "WHITE", "BROWN" };

#define COLOR_COUNT  (( int )( sizeof( color ) / sizeof( color[ 0 ])))
#define Sleep( msec ) usleep(( msec ) * 1000 )

int getMaxSpeed(sn) {
  int max_speed;
  get_tacho_max_speed( sn, &max_speed );
  return max_speed;
}


static bool _check_pressed( uint8_t sn )
{
    int val;
    if ( sn == SENSOR__NONE_ ) {
        return ( ev3_read_keys(( uint8_t *) &val ) && ( val & EV3_KEY_UP ));
    }
    return ( get_sensor_value( 0, sn, &val ) && ( val != 0 ));
}

void tachoMotor(void) {
  int i;
  uint8_t sn;
  FLAGS_T state;
  char s[ 256 ];

  printf( "*** ( EV3 ) Hello! ***\n" );
  printf( "Found tacho motors:\n" );
  for ( i = 0; i < DESC_LIMIT; i++ ) {
    if ( ev3_tacho[ i ].type_inx != TACHO_TYPE__NONE_ ) {
      printf( "  type = %s\n", ev3_tacho_type( ev3_tacho[ i ].type_inx ));
      printf( "  port = %s\n", ev3_tacho_port_name( i, s ));
    }
  }
  if ( ev3_search_tacho( LEGO_EV3_L_MOTOR, &sn, 0 )) {
    
    int max_speed = getMaxSpeed(sn);

    set_tacho_stop_action_inx( sn, TACHO_COAST );
    set_tacho_speed_sp( sn, max_speed * 2 / 3 );
    set_tacho_time_sp( sn, 5000 );
    set_tacho_ramp_up_sp( sn, 2000 );
    set_tacho_ramp_down_sp( sn, 2000 );
    set_tacho_command_inx( sn, TACHO_RUN_TIMED );

    /* Wait tacho stop */
    Sleep( 100 );
    do {
        get_tacho_state_flags( sn, &state );
    } while ( state );
    printf( "run to relative position...\n" );
    set_tacho_speed_sp( sn, max_speed / 2 );
    set_tacho_ramp_up_sp( sn, 0 );
    set_tacho_ramp_down_sp( sn, 0 );
    set_tacho_position_sp( sn, 90 );
    for ( i = 0; i < 8; i++ ) {
      set_tacho_command_inx( sn, TACHO_RUN_TO_REL_POS );
      Sleep( 500 );
    }
  } else {
      printf( "LEGO_EV3_M_MOTOR is NOT found\n" );
  }
}

void led(void) {
  switch ( get_light( LIT_LEFT )) {

  case LIT_GREEN:
    set_light( LIT_LEFT, LIT_RED );
    break;

  case LIT_RED:
    set_light( LIT_LEFT, LIT_AMBER );
    break;

  default:
    set_light( LIT_LEFT, LIT_GREEN );
    break;
  }
}

int sensors(void) {
  char s[ 256 ];
  int val;
  uint32_t n, i, ii;
  uint8_t sn_touch, sn_color, sn_ir;

  printf( "Waiting the EV3 brick online...\n" );
  if ( ev3_init() < 1 ) return ( 1 );
  printf( "*** ( EV3 ) Hello! ***\n" );
  ev3_sensor_init();
  printf( "Found sensors:\n" );
  for ( i = 0; i < DESC_LIMIT; i++ ) {
      if ( ev3_sensor[ i ].type_inx != SENSOR_TYPE__NONE_ ) {
          printf( "  type = %s\n", ev3_sensor_type( ev3_sensor[ i ].type_inx ));
          printf( "  port = %s\n", ev3_sensor_port_name( i, s ));
          if ( get_sensor_mode( i, s, sizeof( s ))) {
              printf( "  mode = %s\n", s );
          }
          if ( get_sensor_num_values( i, &n )) {
              for ( ii = 0; ii < n; ii++ ) {
                  if ( get_sensor_value( ii, i, &val )) {
                      printf( "  value%d = %d\n", ii, val );
                  }
              }
          }
      }
  }
  if ( ev3_search_sensor( LEGO_EV3_IR, &sn_ir, 0 )) {
      printf( "IR sensor is found\n" );
  } else {
      printf( "IR sensor is NOT found\n" );
  }
  if ( ev3_search_sensor( LEGO_EV3_TOUCH, &sn_touch, 0 )) {
      printf( "TOUCH sensor is found, press BUTTON for EXIT...\n" );
  } else {
      printf( "TOUCH sensor is NOT found, press UP on the EV3 brick for EXIT...\n" );
  }
  if ( ev3_search_sensor( LEGO_EV3_COLOR, &sn_color, 0 )) {
      printf( "COLOR sensor is found, reading COLOR...\n" );
      set_sensor_mode( sn_color, "COL-COLOR" );
      for ( ; ; ) {
          if ( !get_sensor_value( 0, sn_color, &val ) || ( val < 0 ) || ( val >= COLOR_COUNT )) {
              val = 0;
          }
          printf( "\r(%s)", color[ val ]);
          fflush( stdout );
          if ( _check_pressed( sn_touch )) break;
          Sleep( 200 );
          printf( "\r        " );
          fflush( stdout );
          if ( _check_pressed( sn_touch )) break;
          Sleep( 200 );
      }
  } else {
      printf( "COLOR sensor is NOT found\n" );
      while ( !_check_pressed( sn_touch )) Sleep( 100 );
  }
}

static int state = 0;

void followPath(void) {
  uint8_t sn_color;
    int val;


if ( ev3_search_sensor( LEGO_EV3_COLOR, &sn_color, 0 )) {
      printf( "COLOR sensor is found, reading COLOR...\n" );
      set_sensor_mode( sn_color, "COL-AMBIENT");
      for ( ; ; ) {
          //if ( !get_sensor_value( 0, sn_color, &val ) || ( val < 0 ) || ( val >= COLOR_COUNT )) {
          //    val = 0;
          //}
          get_sensor_value( 0, sn_color, &val )
          printf("barva %d", val);
          fflush( stdout );
          Sleep( 200 );
      }
  }
}


int main(void)
{
  if ( ev3_init() < 1 ) return ( 1 );
  ev3_port_init();
  ev3_tacho_init();
  ev3_sensor_init();

  //led();
  //tachoMotor();
  //sensors();


  while(true) {
    switch(state) {
      case 0: {
        followPath();
      }
    }
  }
  


  
  
  ev3_uninit();

  return 0;
}
