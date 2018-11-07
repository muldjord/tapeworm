/***************************************************************************
 *            main.cpp
 *
 *  Wed Nov 7 12:00:00 CEST 2018
 *  Copyright 2018 Lars Muldjord
 *  muldjordlars@gmail.com
 ****************************************************************************/
/*
 *  This file is part of tapeworm.
 *
 *  tapeworm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  tapeworm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with tapeworm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#include <iostream>
#include <cstdio>
#include <cstring>

#include <sndfile.hh>

#define BUFFER_LEN 1024

static void
create_file (const char * fname, int format)
{	static short buffer [BUFFER_LEN] ;

	SndfileHandle file ;
	int channels = 1 ;
	int srate = 44100 ;

	printf ("Creating file named '%s'\n", fname) ;

	file = SndfileHandle (fname, SFM_WRITE, format, channels, srate) ;

	memset (buffer, 0, sizeof (buffer)) ;

	file.write (buffer, BUFFER_LEN) ;

	puts ("") ;
	/*
	**	The SndfileHandle object will automatically close the file and
	**	release all allocated memory when the object goes out of scope.
	**	This is the Resource Acquisition Is Initailization idom.
	**	See : http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization
	*/
} /* create_file */

static void
read_file (const char * fname)
{	short buffer[BUFFER_LEN] ;

	SndfileHandle file ;

	file = SndfileHandle(fname) ;

	printf ("Opened file '%s'\n", fname) ;
	printf ("    Sample rate : %d\n", file.samplerate ()) ;
	printf ("    Channels    : %d\n", file.channels ()) ;

	while(file.read(buffer, BUFFER_LEN) != 0) {
	}

	for(int a = 0; a < sizeof(buffer); ++a) {
	  short prevBuf = (buffer[a - 1] >= 0?1:-1);
	  short currBuf = (buffer[a] >= 0?1:-1);
	  if(a > 0 && prevBuf != currBuf) {
	    printf("\n");
	  }
	  printf("O");
	}

	/* RAII takes care of destroying SndfileHandle object. */
} /* read_file */

int main(int argc, char *argv[])
{
  if(argc != 2) {
    printf("Usage: tapeworm SOURCE [DESTINATION]\n");
    return 0;
  }
  const char * wavname = argv[1];
  printf("Input: %s\n", wavname);
  SndfileHandle wavfile(wavname);
  printf ("  Samplerate : %d\n", wavfile.samplerate()) ;
  printf ("  Channels   : %d\n", wavfile.channels()) ;
  if(wavfile.samplerate() != 22050 && wavfile.samplerate() != 44100) {
    printf("Error: File has samplerate of %d, tapeworm expects 22050 Hz or 44100 Hz, exiting...\n", wavfile.samplerate());
    return 1;
  }
  if(wavfile.channels() != 1) {
    printf("Error: File contains %d channels, tapeworm expects 1, exiting...\n", wavfile.channels());
    return 1;
  }

  short buffer[BUFFER_LEN] ;

  // Read
  while(file.read(buffer, BUFFER_LEN) != 0) {
  }

  // Do stuff
  	for(int a = 0; a < sizeof(buffer); ++a) {
	  short prevBuf = (buffer[a - 1] >= 0?1:-1);
	  short currBuf = (buffer[a] >= 0?1:-1);
	  if(a > 0 && prevBuf != currBuf) {
	    printf("\n");
	  }
	  printf("O");
	}


  // Write
  SndfileHandle file ;
  int channels = 1 ;
  int srate = 44100 ;
  
  printf ("Creating file named '%s'\n", fname) ;
  
  file = SndfileHandle (fname, SFM_WRITE, format, channels, srate) ;
  
  memset (buffer, 0, sizeof (buffer)) ;
  
  file.write (buffer, BUFFER_LEN) ;
}


