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
  int segLen = srcFile.samplerate() / 1102; // 20 for a 22050 kHz file

  // Read
  {
    short buffer;
    unsigned long dots = 0;
    unsigned long dotMod = srcFile.frames() / 10;
    printf("Reading from '%s'", srcName.c_str());
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

  // Zero adjust all data
  for(int a = 0; a < data.size(); ++a) {
    int zero = getZero(data, a, segLen);
    data.at(a) = data.at(a) - zero;
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

  typedef std::pair<short, short> pair;
  std::vector<pair> segSizes;
  // Analyze
  {
    printf("Analyzing");
    std::map<short, short> segSizeMap;
    unsigned long dotMod = data.size() / 10;
    for(int a = 0; a < data.size(); ++a) {
      if(data.at(a) > maxVal / 4) {
	//printf("Found rise at sample: %d\n", a);
	while(a - 1 >= 0 && data.at(a - 1) > maxVal / 20) {
	  a--;
	  //printf("Backtracked to %d\n", a);
	}
	short curLength = 0;
	while(a + curLength < data.size() && data.at(a + curLength) > maxVal / 20) {
	  curLength++;
	}
	segSizeMap[curLength] += 1;
	a += curLength;
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
    for (auto const &pair: segSizes) {
      printf("SegLength: %d, SegCount: %d\n", pair.first, pair.second);
    }
  }

  std::vector<short> dataClean;
  // Clean and optimize
  {
    printf("Cleaning and optimizing");
    // Set bit length limits based on samplerate. This might need reworking
    unsigned short bitLength = segSizes.at(0).first;
    unsigned short bitLengthOpt = srcFile.samplerate() / 4410;
    unsigned short minLen = bitLength / 2;
    unsigned short maxLen = (bitLength * 3) + (bitLength / 2);
    unsigned long dotMod = data.size() / 10;
    printf("bitLength=%d\n", bitLength);
    printf("bitLengthOpt=%d\n", bitLengthOpt);
    printf("minLen=%d\n", minLen);
    printf("maxLen=%d\n", maxLen);
    // Sample iteration
    for(int a = 0; a < data.size(); ++a) {
      // Seek until threshold is exceeded
      if(data.at(a) > maxVal / 4) {
	while(a - 1 >= 0 && data.at(a - 1) > maxVal / 20) {
	  a--;
	  dataClean.pop_back();
	}
	short curLength = 0;
	while(a + curLength < data.size() && data.at(a + curLength) > maxVal / 20) {
	  curLength++;
	}
	if(curLength <= minLen || curLength >= maxLen) {
	  for(int b = 0; b < curLength; ++b) {
	    dataClean.push_back(0);
	  }
	} else {
	  //printf("Bitlength=%d!!!\n", curLength);
	  for(int b = 0; b < round((double)curLength / (double)bitLength) * bitLengthOpt; ++b) {
	    dataClean.push_back(SHRT_MAX / 4 * 3);
	  }
	}
	a += curLength - 1;
      } else if(data.at(a) < minVal / 4) {
	while(a - 1 >= 0 && data.at(a - 1) < minVal / 20) {
	  a--;
	  dataClean.pop_back();
	}
	short curLength = 0;
	while(a + curLength < data.size() && data.at(a + curLength) < minVal / 20) {
	  curLength++;
	}
	if(curLength <= minLen || curLength >= maxLen) {
	  for(int b = 0; b < curLength; ++b) {
	    dataClean.push_back(0);
	  }
	} else {
	  //printf("Bitlength=%d!!!\n", curLength);
	  for(int b = 0; b < round((double)curLength / (double)bitLength) * bitLengthOpt; ++b) {
	    dataClean.push_back(SHRT_MIN / 4 * 3);
	  }
	}
	a += curLength - 1;
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

/* TODO
- Analyze all data and group segment lengths in a map to determine what is zero, one and pause.
- Maybe even segment wav into groups of 1024 samples and determine zero, one and pause for each in case we have a wobbly tape.
- Do running zero adjustments by continuously finding max and min samples and subtracting them from each other.
 */
