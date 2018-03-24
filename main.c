#include <stdio.h>

#include "ev3.h"
#include "ev3_light.h"
#include "ev3_dc.h"

#define Sleep( msec ) usleep(( msec ) * 1000 )

int main( void )
{
  char s[ 256 ], name_port[ 16 ];
  int i;
  uint8_t sn, sn_port;
  uint8_t port = OUTPUT_B;

  sn_port = ev3_search_port( port, EXT_PORT__NONE_ );
  set_port_mode_inx( sn_port, OUTPUT_DC_MOTOR );
  if ( get_port_mode( sn_port, s, sizeof( s ))) {
      printf( "%s: %s\n", name_port, s );
  }
  Sleep( 200 );
  ev3_dc_init();


if ( ev3_search_dc_plugged_in( port, EXT_PORT__NONE_, &sn, 0 )) {
        printf( "DC motor is found, run for 5 sec...\n" );
        set_dc_ramp_up_sp( sn, 2000 );
        set_dc_duty_cycle_sp( sn, 100 );
        set_dc_stop_action_inx( sn, DC_COAST );
        set_dc_command_inx( sn, DC_RUN_FOREVER );
        if ( get_dc_state( sn, s, sizeof( s ))) {
            printf( "state: %s\n", s );
        }
        Sleep( 5000 );
        set_dc_command_inx( sn, DC_STOP );
        if ( get_dc_state( sn, s, sizeof( s ))) {
            printf( "state: %s\n", s );
        }
    } else {
        printf( "DC motor is NOT found\n" );
    }

 Sleep( 200 );
    printf( "Reset mode of the EV3 output port...\n" );
    set_port_mode_inx( sn_port, OUTPUT_AUTO );

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
