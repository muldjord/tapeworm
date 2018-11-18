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
 *  tapeworm is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  Foobar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with tapeworm. If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <vector>
#include <climits>
#include <math.h>
#include <algorithm>
#include <numeric>

#include <sndfile.hh>

#define BUFFER 1024

#define ST_SLEEP 42
#define ST_TRIGGER 43
#define ST_WAIT 44
#define ST_DATA 45

#define PH_RISE 666
#define PH_FALL 667
#define PH_NONE 668

//#define DEBUG

void showProgress(const long dots, const long modulus)
{
  if(dots % modulus == 0) {
    printf(".");
    fflush(stdout);
  }
}

double getBitlength(const std::vector<short> &data,
		    const short noSignalMax,
		    const int &initThres)
{
  printf("Analyzing");
  double bitLength = 0.0;
  short neededBits = 256;
  short dotMod = neededBits / 10;
  std::vector<short> bitLengths;
  long noSignal = 0;
  for(long a = 0; a < data.size(); ++a) {
    // Detect signal begin
    if(data.at(a) > initThres) {
      short curLength = 0;
      while(data.at(a + curLength) >= 0 &&
	    a + curLength < data.size()) {
	curLength++;
      }
      while(data.at(a + curLength) < initThres &&
	    a + curLength < data.size()) {
	curLength++;
      }
      bitLengths.push_back(curLength);
      if(bitLengths.size() == neededBits) {
	break;
      }
      a += curLength - 1;
      showProgress(bitLengths.size(), dotMod);
    } else {
      noSignal++;
    }
    if(bitLengths.size() && noSignal >= noSignalMax) {
      bitLengths.clear();
      noSignal = 0;
    }
  }
  for(int a = bitLengths.size() / 2; a < bitLengths.size(); ++a) {
    bitLength += bitLengths.at(a);
  }
  bitLength /= bitLengths.size() / 2;
  printf(" Done!\n");

  return bitLength;
}

short getDirection(const short &currentValue)
{
  if(currentValue >= 0) {
    return PH_RISE;
  } else {
    return PH_FALL;
  }
}

long getPulseLength(const std::vector<short> &data, long &idx, const short &direction)
{
  long startIdx = idx;
  if(direction == PH_RISE) {
    while(idx < startIdx + 50 && idx < data.size() && data.at(idx) >= 0) {
      idx++;
    }
  } else if(direction == PH_FALL) {
    while(idx < startIdx + 50 && idx < data.size() && data.at(idx) < 0) {
      idx++;
    }
  }
  long pulseLength = idx - startIdx + 1;

  return pulseLength;
}

int getZero(const std::vector<short> &data, const long idx, const long span)
{
  int segMax = data.at(idx);
  int segMin = data.at(idx);
  for(long a = 0; a < span; ++a) {
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

void pushShort(std::vector<short> &data,
	      const unsigned short &pulseLengthOptimal,
	      const short direction)
{
  if(direction == PH_RISE) {
    int b = ceil(pulseLengthOptimal / 2.0);
    while(b--) {
      data.push_back(SHRT_MAX);
    }
  } else if(direction == PH_FALL) {
    int b = floor(pulseLengthOptimal / 2.0);
    while(b--) {
      data.push_back(SHRT_MIN);
    }
  }
}

void pushMedium(std::vector<short> &data,
		const unsigned short &pulseLengthOptimal,
		const short direction)
{
  if(direction == PH_RISE) {
    int b = floor(pulseLengthOptimal);
    while(b--) {
      data.push_back(SHRT_MAX);
    }
  } else if(direction == PH_FALL) {
    int b = ceil(pulseLengthOptimal);
    while(b--) {
      data.push_back(SHRT_MIN);
    }
  }
}

void pushLong(std::vector<short> &data,
	      const unsigned short &pulseLengthOptimal,
	      const short direction)
{
  if(direction == PH_RISE) {
    int b = ceil(pulseLengthOptimal + pulseLengthOptimal / 2.0);
    while(b--) {
      data.push_back(SHRT_MAX);
    }
  } else if(direction == PH_FALL) {
    int b = floor(pulseLengthOptimal + pulseLengthOptimal / 2.0);
    while(b--) {
      data.push_back(SHRT_MIN);
    }
  }
}

void pushInit(std::vector<short> &data, const unsigned short &pulseLengthOptimal)
{
#ifdef DEBUG
  printf("Pushing INIT\n");
#endif
  for(int b = 0; b < ceil(pulseLengthOptimal / 2.0); ++b) {
    data.push_back(SHRT_MIN);
  }
  for(int b = 0; b < pulseLengthOptimal + ceil(pulseLengthOptimal / 2.0); ++b) {
    data.push_back(SHRT_MAX);
  }
}

void pushStop(std::vector<short> &data, const unsigned short &pulseLengthOptimal)
{
#ifdef DEBUG
  printf("Pushing STOP\n");
#endif
  for(int b = 0; b < pulseLengthOptimal * 4; ++b) {
    data.push_back(SHRT_MIN);
  }
}

void pushZero(std::vector<short> &data, const unsigned short &pulseLengthOptimal, int count = 1)
{
  while(count--) {
    for(int b = 0; b < ceil(pulseLengthOptimal / 2.0); ++b) {
      data.push_back(SHRT_MIN);
    }
    for(int b = 0; b < floor(pulseLengthOptimal / 2.0); ++b) {
      data.push_back(SHRT_MAX);
    }
  }
}

void pushOne(std::vector<short> &data, const unsigned short &pulseLengthOptimal, int count = 1)
{
  while(count--) {
    for(int b = 0; b < pulseLengthOptimal; ++b) {
      data.push_back(SHRT_MIN);
    }
    for(int b = 0; b < pulseLengthOptimal; ++b) {
      data.push_back(SHRT_MAX);
    }
  }
}

double stdDevFromAbs(const std::vector<short> &data,
		     const long idx,
		     const long span,
		     const short maxVal = SHRT_MAX,
		     const bool population = false)
{
  std::vector<short> vec;
  for(long a = 0; a < span; ++a) {
    if(abs(data.at(idx + a)) <= maxVal) {
      vec.push_back(abs(data.at(idx + a)));
    }
  }
  if(vec.size() < span / 2) {
    printf("Warning: Low quality standard deviation calculation.\n");
  }
  
  double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
  double mean = sum / vec.size();
  std::vector<double> diff(vec.size());
  std::transform(vec.begin(), vec.end(), diff.begin(), [mean](double x) { return x - mean; });
  double sqSum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
  double variance = sqSum / (vec.size() - (int)population);
  double stdDev = std::sqrt(variance);

  return stdDev;
}

double meanFromAbs(const std::vector<short> &data,
		   const long idx,
		   const long span,
		   const short maxVal = SHRT_MAX)
{
  std::vector<short> vec;
  for(long a = 0; a < span; ++a) {
    if(abs(data.at(idx + a)) <= maxVal) {
      vec.push_back(abs(data.at(idx + a)));
    }
  }
  if(vec.size() < span / 2) {
    printf("Warning: Low quality mean calculation.\n");
  }
  
  double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
  double mean = sum / vec.size();

  return mean;
}

void readAll(std::vector<short> &data, SndfileHandle &srcFile)
{
  short buffer[BUFFER] = {0};
  long dots = 0;
  long dotMod = srcFile.frames() / BUFFER / 10;
  printf("Reading data");
  sf_count_t read = 0;
  while(read = srcFile.read(buffer, BUFFER)) {
    for(int a = 0; a < read; ++a) {
      data.push_back(buffer[a]);
    }
    showProgress(dots, dotMod);
    dots++;
  }
  printf(" Done!\n");
}

void writeAll(const std::vector<short> &data, SndfileHandle &dstFile, const int zeroPadding = 0)
{
  long dotMod = ceil(data.size() / 10.0);
  short buffer[BUFFER] = {0};
  short silence[zeroPadding] = {0};
  dstFile.write(silence, zeroPadding); // Initial zero padding
  for(long a = 0; a < data.size(); a += BUFFER) {
    sf_count_t write;
    for(write = 0; write < BUFFER; ++write) {
      if(a + write >= data.size())
	break;
      buffer[write] = data.at(a + write);
      showProgress(a + write, dotMod);
    }
    dstFile.write(buffer, write);
  }
  dstFile.write(silence, zeroPadding); // Trailing zero padding
  printf(" Done!\n");
}

void zeroAdjust(std::vector<short> &data, int segmentLength)
{
  printf("Zero adjusting data");
  long dots = 0;
  long dotMod = data.size() / 10;
  for(long a = 0; a < data.size(); ++a) {
    int zero = getZero(data, a, segmentLength);
    data.at(a) = data.at(a) - zero;
    showProgress(dots, dotMod);
    dots++;
  }
  printf(" Done!\n");
}

int main(int argc, char *argv[])
{
  printf("TapeWorm: A Memotech wav tapefile cleaner and optimizer by Lars Muldjord 2018\n");
  std::string srcName = "input.wav"; // Name will always be changed, but set just in case
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

  // Read
  readAll(data, srcFile);

  // Zero adjust all data
  zeroAdjust(data, srcFile.samplerate() / 1102);

#ifdef DEBUG
  // Write zero adjusted
  {
    printf("Writing to 'zero_adjusted.wav'");
    SndfileHandle dstFile("zero_adjusted.wav", SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_U8, srcFile.channels(), srcFile.samplerate());
    writeAll(data, dstFile);
  }
#endif

  int initThres = stdDevFromAbs(data, 0, data.size());
#ifdef DEBUG
  printf("Calculated values:\n");
  printf("  Signal trigger=%d\n", initThres);
#endif
  
  // Analyze to get optimal bitlength (2 short pulses)
  double bitLength = getBitlength(data, srcFile.samplerate() / 1102, initThres);
  
  std::vector<short> dataClean;
  // Clean and optimize
  {
    // Set bit length limits based on samplerate. This might need reworking
    double flutter = bitLength / 4.0;
    double shortPulse = bitLength / 2.0;
    double mediumPulse = bitLength;
    double longPulse = mediumPulse + shortPulse;
#ifdef DEBUG
    printf("Flutter     : %f samples\n", flutter);
    printf("Short pulse : %f samples\n", shortPulse);
    printf("Medium Pulse: %f samples\n", mediumPulse);
    printf("Long Pulse  : %f samples\n", longPulse);
#endif
    short unsigned pulseLengthOptimal = srcFile.samplerate() / 2450;
    long dotMod = data.size() / 10;
    printf("  Zero bit length = %f (%d baud)\n", bitLength, (int)(srcFile.samplerate() / bitLength));
    printf("Cleaning and optimizing");
    short state = ST_SLEEP;
    short direction = PH_NONE;
    std::string pulseBuffer = "";
    std::string bitBuffer = "";
    bool onlyShorts = true;
    // Process all samples and push detected bits to dataClean vector
    for(long a = 0; a < data.size(); ++a) {
      if(state == ST_SLEEP) {
	if(abs(data.at(a)) > initThres) {
	  state = ST_TRIGGER;
	}
	dataClean.push_back(0);
      }
      if(state == ST_TRIGGER) {
	long b = a;
	// Seek to next zero crossing
	if(direction == PH_RISE) {
	  while(a < data.size() && data.at(a) >= 0) {
	    a++;
	  }
	} else if(direction == PH_FALL) {
	  while(a < data.size() && data.at(a) < 0) {
	    a++;
	  }
	}
	while(b++ <= a) {
	  dataClean.push_back(0);
	}
	state = ST_WAIT;
      }
      if(state == ST_WAIT) {
	direction = getDirection(data.at(a));
	long pulseLength = getPulseLength(data, a, direction);
	if(pulseLength <= shortPulse + flutter &&
	   pulseLength >= shortPulse - flutter) {
	  pulseBuffer.append("s");
	  pushShort(dataClean, pulseLengthOptimal, direction);
	} else {
	  if(pulseBuffer.size() >= 64) {
#ifdef DEBUG
	    printf("Accepting wait signal with a length of %ld small pulses!\n", pulseBuffer.size());
#endif
	    a = a - pulseLength;
	    state = ST_DATA;
	  } else {
	    while(pulseLength--) {
	      dataClean.push_back(0);
	    }
	    state = ST_SLEEP;
	  }
	  pulseBuffer = "";
	}
      } else if(state == ST_DATA) {
	direction = getDirection(data.at(a));
	long pulseLength = getPulseLength(data, a, direction);
	if(pulseLength <= shortPulse + flutter) {
	  pushShort(dataClean, pulseLengthOptimal, direction);
	  pulseBuffer.append("s");
	} else if(pulseLength <= mediumPulse + flutter) {
	  pushMedium(dataClean, pulseLengthOptimal, direction);
	  pulseBuffer.append("m");
	} else if(pulseLength <= longPulse + flutter) {
	  pushLong(dataClean, pulseLengthOptimal, direction);
	  pulseBuffer.append("l");
	} else {
#ifdef DEBUG
	  printf("Data ended or malformed bit at %ld, pushing stopsignal and resetting\n", a);
	  printf("Constructed bits for this segment:\n%s\n", bitBuffer.c_str());
#endif
	  pushLong(dataClean, pulseLengthOptimal, direction);
	  state = ST_SLEEP;
	  pulseBuffer = "";
	  bitBuffer = "";
	}
	if(pulseBuffer.size() == 2) {
	  if(pulseBuffer == "ss") {
	    bitBuffer.append("0");
	  } else if(pulseBuffer == "sm") {
	    bitBuffer.append("0");
	  } else if(pulseBuffer == "ms") {
	    bitBuffer.append("0");
	  } else if(pulseBuffer == "mm") {
	    bitBuffer.append("1");
	  } else {
#ifdef DEBUG
	    printf("Detected bad bit '%s' ending at %ld\n", pulseBuffer.c_str(), a);
#endif
	  }
	  pulseBuffer = "";
	}
	if(pulseBuffer.size() == 1 && pulseBuffer != "s") {
	  onlyShorts = false;
	}
	if(bitBuffer.size() >= 64 && onlyShorts) {
#ifdef DEBUG
	  printf("Where back in a waiting signal, resetting to ST_WAIT\n");
	  printf("Constructed bits for this segment:\n%s\n", bitBuffer.c_str());
#endif
	  state = ST_WAIT;
	  onlyShorts = true;
	  pulseBuffer = "";
	  bitBuffer = "";
	}
      }
      showProgress(a, dotMod);
    }
    printf(" Done!\n");
  }

  // Write
  {
    printf("Writing to '%s'", dstName.c_str());
    SndfileHandle dstFile(dstName.c_str(), SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_U8, srcFile.channels(), srcFile.samplerate());
    writeAll(dataClean, dstFile, srcFile.samplerate());
  }
  return 0;
}

/* TODO
- REMEMBER ZERO DETECT SO IT'LL WORK WITH PERFECT ZERO FILES!!!
- Maybe segment wav into groups of BUFFER samples and determine zero bitlength for each in case we have a wobbly tape.
- Make zero adjustment use moving push/pop vector for BIG optimization.
 */
