//
// emup.cc
//

#include <unistd.h>
#include <stdio.h>

#include <be/support/List.h>
#include <be/device/SerialPort.h>
#include <be/storage/File.h>

int main( int argc, char *argv[] )
{
	BSerialPort serial;
	BFile file;
	char buff[128];
	int read_len, write_len;
	
	if( argc != 3 )
	{
		printf( "usage: emup serialdev file\n" );
		return -1;
	}
	
	if( serial.Open( argv[1] ) <= 0 )
	{
		printf( "Can't open device \"%s\"\n", argv[1] );	
		return -2;
	}
	
	// set things for our serial emulator setttings
	serial.SetDataBits( B_DATA_BITS_8 );
	serial.SetStopBits( B_STOP_BITS_1 );
	serial.SetDataRate( B_115200_BPS );
	serial.SetFlowControl( 0 );
	
	if( file.SetTo( argv[2], B_READ_ONLY) != B_OK )
	{
		printf( "Can't open file \"%s\"\n", argv[2] );
		serial.Close();
		return -3;
	}
	
	strcpy( buff, "re\r" );
	serial.Write( buff, 3 );
	sleep(1);
 
	
	while( (read_len = file.Read( buff, 128 )) )
	{
		write_len = 0;
		
		while( write_len < read_len )
		{
			write_len += serial.Write( buff + write_len, read_len - write_len );
		}
	
	}
	
	file.Unset();
	serial.Close();
	
	return 0;	
}
	