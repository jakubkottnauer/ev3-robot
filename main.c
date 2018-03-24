#include <stdio.h>

#include "ev3.h"
#include "ev3_light.h"
#include "ev3_tacho.h"
#include "ev3_port.h"

#define Sleep( msec ) usleep(( msec ) * 1000 )

int main( void )
{
    int i;
    uint8_t sn;
    FLAGS_T state;
    char s[ 256 ];
  if ( ev3_init() == -1 ) return ( 1 );

    while ( ev3_tacho_init() < 1 ) Sleep( 1000 );
    printf( "*** ( EV3 ) Hello! ***\n" );
    printf( "Found tacho motors:\n" );
    for ( i = 0; i < DESC_LIMIT; i++ ) {
        if ( ev3_tacho[ i ].type_inx != TACHO_TYPE__NONE_ ) {
            printf( "  type = %s\n", ev3_tacho_type( ev3_tacho[ i ].type_inx ));
            printf( "  port = %s\n", ev3_tacho_port_name( i, s ));
        }
    }
    if ( ev3_search_tacho( LEGO_EV3_L_MOTOR, &sn, 0 )) {
        int max_speed;
        printf( "LEGO_EV3_M_MOTOR is found, run for 5 sec...\n" );
        get_tacho_max_speed( sn, &max_speed );
        printf("  max_speed = %d\n", max_speed );
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
    ev3_uninit();

  printf( "Hello, LEGO World!\n" );

  if ( ev3_init() < 1 ) return ( 1 );
  ev3_port_init();
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
  ev3_uninit();

  return 0;
}
