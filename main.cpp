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
#include <algorithm>
#include <map>

#include <sndfile.hh>

#define BUFFER 1024
#define INITDIV 4
#define ZERODIV 20

int getZero(std::vector<short> &data, int idx, int span)
{
  int segMax = data.at(idx);
  int segMin = data.at(idx);
  for(int a = 0; a < span; ++a) {
    if(idx + a >= data.size())
      break;
    int sample = data.at(idx + a);
    if(sample > segMax)
      segMax = sample;
    if(sample < segMin)
      segMin = sample;
  }
  int zero = (segMax + segMin) / 2;
  return zero;
}

void pushInit(std::vector<short> &data, unsigned short &bitLengthOpt)
{
  for(int b = 0; b < ceil(bitLengthOpt / 2.0); ++b) {
    data.push_back(SHRT_MIN);
  }
  for(int b = 0; b < bitLengthOpt + ceil(bitLengthOpt / 2.0); ++b) {
    data.push_back(SHRT_MAX);
  }
}

void pushStop(std::vector<short> &data, unsigned short &bitLengthOpt)
{
  for(int b = 0; b < bitLengthOpt * 2; ++b) {
    data.push_back(SHRT_MIN);
  }
}

void pushZero(std::vector<short> &data, unsigned short &bitLengthOpt)
{
  for(int b = 0; b < ceil(bitLengthOpt / 2.0); ++b) {
    data.push_back(SHRT_MIN);
  }
  for(int b = 0; b < floor(bitLengthOpt / 2.0); ++b) {
    data.push_back(SHRT_MAX);
  }
}

void pushOne(std::vector<short> &data, unsigned short &bitLengthOpt)
{
  for(int b = 0; b < bitLengthOpt; ++b) {
    data.push_back(SHRT_MIN);
  }
  for(int b = 0; b < bitLengthOpt; ++b) {
    data.push_back(SHRT_MAX);
  }
}

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
  printf("Input: '%s'\n", srcName.c_str());
  SndfileHandle srcFile(srcName.c_str());
  printf("  Samplerate : %d\n", srcFile.samplerate()) ;
  printf("  Channels   : %d\n", srcFile.channels()) ;
  printf("  Frames     : %ld\n", srcFile.frames()) ;
  //printf("  Format     : %#08x\n", srcFile.format());
  if(srcFile.samplerate() < 22050) {
    printf("Error: File has samplerate of %d, tapeworm expects 22050 Hz or more, exiting...\n", srcFile.samplerate());
    return 1;
  }
  if(srcFile.channels() != 1) {
    printf("Error: File contains %d channels, tapeworm expects 1, exiting...\n", srcFile.channels());
    return 1;
  }

  std::vector<short> data;
  int segLen = srcFile.samplerate() / 1102; // 20 for a 22050 kHz file

  // Read
  {
    short buffer[BUFFER];
    unsigned long dots = 0;
    unsigned long dotMod = srcFile.frames() / BUFFER / 10;
    printf("Reading data");
    sf_count_t read = 0;
    while(read = srcFile.read(buffer, BUFFER)) {
      for(int a = 0; a < read; ++a) {
	data.push_back(buffer[a]);
      }
      if(dots % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
      dots++;
    }
    printf(" Done!\n");
  }

  // Zero adjust all data
  {
    printf("Zero adjusting data");
    unsigned long dots = 0;
    unsigned long dotMod = data.size() / 10;
    for(int a = 0; a < data.size(); ++a) {
      int zero = getZero(data, a, segLen);
      data.at(a) = data.at(a) - zero;
      if(dots % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
      dots++;
    }
    printf(" Done!\n");
  }

  // Write zero adjusted
  {
    printf("Saving to 'zero_adjusted.wav'");
    SndfileHandle dstFile("zero_adjusted.wav", SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_U8, srcFile.channels(), srcFile.samplerate());
    unsigned long dotMod = data.size() / BUFFER / 10;
    short buffer[BUFFER] = {0};
    dstFile.write(buffer, BUFFER); // Write a couple of silent buffers. Some players don't play
    dstFile.write(buffer, BUFFER); // the first part, so a bit of silence is important
    for(int a = 0; a < data.size(); a = a + BUFFER) {
      sf_count_t write = 0;
      for(int b = 0; b < BUFFER; ++b) {
	if(a + b >= data.size())
	  break;
	buffer[b] = data.at(a + b);
	write = b + 1;
      }
      dstFile.write(buffer, write);
      if(a % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
    }
    printf(" Done!\n");
  }

  // Set maximum and minimum sample value
  int maxVal = 0;
  int minVal = 0;
  for(int a = 0; a < data.size(); ++a) {
    int sample = data.at(a);
    if(sample > maxVal)
      maxVal = sample;
    if(sample < minVal)
      minVal = sample;
  }

  typedef std::pair<short, long int> pair;
  std::vector<pair> segSizes;
  unsigned short bitLength = 0;
  // Analyze
  {
    printf("Analyzing");
    std::map<short, long int> segSizeMap;
    unsigned long dotMod = data.size() / 10;
    for(int a = 0; a < data.size(); ++a) {
      // Detect signal drop that indicate bit begin
      if(data.at(a) < minVal / INITDIV) {
	// Backtrack to bit beginning
	while(a - 1 >= 0 && data.at(a - 1) < minVal / ZERODIV) {
	  a--;
	}
	int curLength = 1;
	// Drop phase
	while(a + curLength < data.size() && data.at(a + curLength) < maxVal / INITDIV) {
	  curLength++;
	}
	// Rise phase
	while(a + curLength < data.size() && data.at(a + curLength) > maxVal / ZERODIV) {
	  curLength++;
	}
	segSizeMap[curLength] += 1;
	a += curLength - 1;
      }
      if(a % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
    }
    std::copy(segSizeMap.begin(),
	      segSizeMap.end(),
	      std::back_inserter<std::vector<pair>>(segSizes));
    std::sort(segSizes.begin(), segSizes.end(),
	      [](const pair& segLength, const pair& segCount) {
                if (segLength.second != segCount.second)
		  return segLength.second > segCount.second;
                return segLength.first > segCount.first;
	      });
    
    // print the vector
    printf(" Done!\n");
    std::vector<short> relevant;
    for(int a = 0; a < segSizes.size() - 1; ++a) {
      if(segSizes.at(a).second > srcFile.frames() / 137) {
	relevant.push_back(segSizes.at(a).first);
	printf("segLength=%d, segCount=%ld\n", segSizes.at(a).first, segSizes.at(a).second);
      }
    }
    std::sort(relevant.begin(), relevant.end());
    int segCount = 0;
    for(int a = 0; a < 3; ++a) {
      bitLength += relevant.at(a);
      segCount++;
      if(a + 1 < relevant.size() && abs(relevant.at(a + 1) - relevant.at(a)) >= 2) {
	break;
      }
    }
    bitLength = round((double)bitLength / (double)segCount);
  }

  std::vector<short> dataClean;
  // Clean and optimize
  {
    // Set bit length limits based on samplerate. This might need reworking
    unsigned short bitLengthOpt = srcFile.samplerate() / 2450;
    unsigned short minLen = bitLength / 4;
    unsigned short maxLen = bitLength * 3 + bitLength / 4;
    unsigned long dotMod = data.size() / 10;
    printf("  Detected zero bitlength = %d (appr. %d baud)\n", bitLength, srcFile.samplerate() / bitLength);
    printf("  Optimal zero bitlength = %d\n", bitLengthOpt);
    printf("  minLen=%d\n", minLen);
    printf("  maxLen=%d\n", maxLen);
    printf("Cleaning and optimizing");
    bool inData = false;
    // Sample iteration
    short allowedNullData = 0;
    for(int a = 0; a < data.size(); ++a) {
      // Detect signal drop that indicate bit begin
      if(data.at(a) < minVal / INITDIV) {
	// Backtrack to bit beginning
	while(a - 1 >= 0 && data.at(a - 1) < minVal / ZERODIV) {
	  a--;
	}
	long int loLen = 0;
	long int hiLen = 0;
	long int totLen = 0;
	// Drop phase
	while(a + totLen < data.size() && data.at(a + totLen) <= 0) {
	  loLen++;
	  totLen++;
	}
	// Rise phase
	while(a + totLen < data.size() && data.at(a + totLen) > maxVal / ZERODIV) {
	  hiLen++;
	  totLen++;
	}
	if(totLen <= minLen || totLen >= maxLen) {
	  /*
	  if(inData) {
	    printf("STOPBIT!!!\n");
	    // Push stop bit
	    pushStop(dataClean, bitLengthOpt);
	  }
	  */
	  inData = false;
	  for(int b = 0; b < totLen; ++b) {
	    dataClean.push_back(0);
	  }
	} else {
	  allowedNullData = bitLength;
	  inData = true;
	  if(abs(loLen - bitLength / 2) < abs(loLen - bitLength)) {
	    if(abs(hiLen - bitLength / 2) < abs(hiLen - bitLength)) {
	      // Push 0 bit
	      pushZero(dataClean, bitLengthOpt);
	    } else {
	      // Push init bit
	      pushInit(dataClean, bitLengthOpt);
	    }
	  } else {
	    // Push 1 bit
	    pushOne(dataClean, bitLengthOpt);
	  }
	}
	a += totLen - 1;
      } else {
	if(inData) {
	  if(!--allowedNullData) {
	    printf("STOPBIT!!!\n");
	    // Push stop bit
	    pushStop(dataClean, bitLengthOpt);
	    inData = false;
	  }
	} else {
	  dataClean.push_back(0);
	}
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
    printf("Saving to '%s'", dstName.c_str());
    SndfileHandle dstFile(dstName.c_str(), SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_U8, srcFile.channels(), srcFile.samplerate());
    unsigned long dotMod = dataClean.size() / BUFFER / 10;
    short buffer[BUFFER] = {0};
    dstFile.write(buffer, BUFFER); // Write a couple of silent buffers. Some players don't play
    dstFile.write(buffer, BUFFER); // the first part, so a bit of silence is important
    for(int a = 0; a < dataClean.size(); a = a + BUFFER) {
      sf_count_t write = 0;
      for(int b = 0; b < BUFFER; ++b) {
	if(a + b >= dataClean.size())
	  break;
	buffer[b] = dataClean.at(a + b);
	write = b + 1;
      }
      dstFile.write(buffer, write);
      if(a % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
    }
    printf(" Done!\n");
  }

  return 0;
}

/* TODO
- Analyze all data and group segment lengths in a map to determine what is zero, one and pause.
- Maybe even segment wav into groups of BUFFER samples and determine zero, one and pause for each in case we have a wobbly tape.
- Do running zero adjustments by continuously finding max and min samples and subtracting them from each other.
 */
