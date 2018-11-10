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
#include <math.h>

#include <sndfile.hh>

int main(int argc, char *argv[])
{
  printf("TapeWorm: A Memotech wav tapefile cleaner and optimizer by Lars Muldjord 2018\n");
  std::string srcName;
  std::string dstName = "output.wav"; // Name will always be changed, but set just in case
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

  std::vector<short> data;

  // Read
  {
    short buffer;
    unsigned long dots = 0;
    unsigned long dotMod = srcFile.frames() / 10;
    printf("Reading from '%s'", srcName.c_str()) ;
    while(srcFile.read(&buffer, 1) != 0) {
      data.push_back(buffer);
      if(dots % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
      dots++;
    }
    printf(" Done!\n");
  }

  std::vector<short> dataClean;
  // Do stuff
  {
    printf("Cleaning and optimizing");
    // Set bit length limits based on samplerate. This might need reworking
    /*
    unsigned short minLen = srcFile.samplerate() / 7350;
    unsigned short maxLen = srcFile.samplerate() / 1297;
    unsigned short bitZer = srcFile.samplerate() / 4410;
    unsigned short bitOne = srcFile.samplerate() / 2205;
    unsigned short bitSig = srcFile.samplerate() / 1470;
    unsigned short bitZerOpt = srcFile.samplerate() / 5512;
    unsigned short bitOneOpt = srcFile.samplerate() / 2756;
    unsigned short bitSigOpt = srcFile.samplerate() / 1836;
    */
    unsigned short minLen = 3;
    unsigned short maxLen = 17;
    unsigned short bitZer = 5;
    unsigned short bitOne = 10;
    unsigned short bitSig = 15;
    unsigned short bitZerOpt = 4;
    unsigned short bitOneOpt = 8;
    unsigned short bitSigOpt = 12;
    unsigned long dotMod = data.size() / 10;
    // Set threshold that needs to be exceeded before doing traceback to bitstart
    short sampleMax = 0;
    for(int a = 0; a < data.size(); ++a) {
      if(abs(data.at(a)) > sampleMax)
	sampleMax = abs(data.at(a));
    }
    short thres = sampleMax / 4;

    // Figure out the necessary zero correction delta from the first few samples
    short zeroCorr = 0;
    for(int a = 0; a < 25; ++a) {
      zeroCorr += abs(data.at(a));
    }
    zeroCorr /= 25;
    printf("zeroCorr: %d\n", zeroCorr);

    // Set zero threshold used to determine when we are back at 0 after a signal
    short zeroThres = (sampleMax / 5) + zeroCorr;

    // Sample iteration
    for(int a = 0; a < data.size(); ++a) {
      // Seek until threshold is exceeded
      if(abs(data.at(a) + zeroCorr) > thres) {
	long int smpSum = 0;
	// Seek-back to crossing
	while(a - 1 >= 0 && abs(data.at(a - 1) + zeroCorr) > zeroThres) {
	  a--;
	  dataClean.pop_back();
	}
	short bitLength = 1;
	while(a + bitLength < data.size() && abs(data.at(a + bitLength) + zeroCorr) > zeroThres) {
	  smpSum += data.at(a + bitLength) + zeroCorr;
	  bitLength++;
	}
	if(bitLength <= minLen || bitLength >= maxLen) {
	  for(int b = 0; b < bitLength; ++b) {
	    dataClean.push_back(0);
	  }
	} else {
	  printf("Bitlength=%d!!!\n", bitLength);
	  for(int b = 0; b < round((double)bitLength / (double)bitZer) * bitZerOpt; ++b) {
	    if(smpSum >= 0) {
	      dataClean.push_back(SHRT_MAX);
	    } else {
	      dataClean.push_back(SHRT_MIN);
	    }
	  }
	}
	a += bitLength;
      } else {
	dataClean.push_back(0);
      }
	      
      if(a % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
    }
    printf(" Done!\n");
  }
  
  // Write
  {
    unsigned long dotMod = dataClean.size() / 10;
    printf("Saving to '%s'", dstName.c_str()) ;
    SndfileHandle dstFile(dstName.c_str(), SFM_WRITE, srcFile.format(), srcFile.channels(), srcFile.samplerate());
    for(int a = 0; a < dataClean.size(); ++a) {
      if(a % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
      dstFile.write(&dataClean.at(a), 1);
    }
    printf(" Done!\n");
  }

  return 0;
}
