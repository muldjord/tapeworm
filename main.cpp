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
#include <vector>
#include <climits>

#include <sndfile.hh>

int main(int argc, char *argv[])
{
  printf("TapeWorm: A Memotech wav tapefile cleaner by Lars Muldjord, 2018\n");
  std::string srcName;
  std::string dstName = "output.wav";
  if(argc == 2) {
    srcName = argv[1];
    dstName = "cleaned_" + srcName;
  } else if(argc == 3) {
    srcName = argv[1];
    dstName = argv[2];
  } else if(argc != 2) {
    printf("Usage: tapeworm SOURCE [DESTINATION]\n");
    return 0;
  }
  printf("Input: %s\n", srcName.c_str());
  SndfileHandle srcFile(srcName.c_str());
  printf("  Samplerate : %d\n", srcFile.samplerate()) ;
  printf("  Channels   : %d\n", srcFile.channels()) ;
  printf("  Format     : %#08x\n", srcFile.format());
  if(srcFile.samplerate() != 22050 && srcFile.samplerate() != 44100) {
    printf("Error: File has samplerate of %d, tapeworm expects 22050 Hz or 44100 Hz, exiting...\n", srcFile.samplerate());
    return 1;
  }
  if(srcFile.channels() != 1) {
    printf("Error: File contains %d channels, tapeworm expects 1, exiting...\n", srcFile.channels());
    return 1;
  }

  short *buffer;
  std::vector<short> *data = new std::vector<short>;

  // Read
  while(srcFile.read(buffer, 1) != 0) {
    data->push_back(*buffer);
  }

  std::vector<short> *dataClean = new std::vector<short>;
  // Do stuff
  {
    unsigned long dots = 0;
    unsigned long dotMod = data->size() / 10;
    short thres = SHRT_MAX / 10;
    printf("Cleaning and optimizing");
    for(int a = 0; a < data->size(); ++a) {
      if(data->at(a) > thres) {
	dataClean->push_back(SHRT_MAX);
      } else if(data->at(a) < -thres) {
	dataClean->push_back(SHRT_MIN);
      } else {
	dataClean->push_back(data->at(a));
      }
	      
      if(a % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
    }
  }
  
  printf(" Done!\n");

  // Write
  {
    unsigned long dots = 0;
    unsigned long dotMod = dataClean->size() / 10;
    printf("Saving to '%s'", dstName.c_str()) ;
    SndfileHandle dstFile(dstName.c_str(), SFM_WRITE, srcFile.format(), srcFile.channels(), srcFile.samplerate());
    for(int a = 0; a < dataClean->size(); ++a) {
      if(a % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
      dstFile.write(&dataClean->at(a), 1);
    }
    
    printf(" Done!\n");
  }

  delete data;
  delete dataClean;

  return 0;
}


