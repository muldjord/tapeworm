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
#include <numeric>

#include <sndfile.hh>

#define BUFFER 1024
#define INITDIV 4
#define ZERODIV 20
#define ZEROSPAN 100

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

double stdDevFromAbs(std::vector<short> &data,
		     long unsigned int idx,
		     long unsigned int span,
		     short maxVal = SHRT_MAX,
		     bool population = false)
{
  std::vector<short> vec;
  for(int a = 0; a < span; ++a) {
    if(abs(data.at(idx + a)) <= maxVal) {
      vec.push_back(abs(data.at(idx + a)));
    }
  }
  if(vec.size() < span / 2) {
    printf("Warning: Low quality standard deviation calculation.\n");
  }
  
  double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
  double mean = sum / vec.size();
  printf("mean from absolutes=%f\n", mean);
  std::vector<double> diff(vec.size());
  std::transform(vec.begin(), vec.end(), diff.begin(), [mean](double x) { return x - mean; });
  double sqSum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
  double variance = sqSum / (vec.size() - (int)population);
  double stdDev = std::sqrt(variance);

  return stdDev;
}

double meanFromAbs(std::vector<short> &data,
		     long unsigned int idx,
		     long unsigned int span,
		     short maxVal = SHRT_MAX)
{
  std::vector<short> vec;
  for(int a = 0; a < span; ++a) {
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

void writeAll(std::vector<short> &data, SndfileHandle &dstFile, int zeroPadding = 0)
{
  unsigned long dotMod = ceil(data.size() / 10.0);
  short buffer[BUFFER] = {0};
  short silence[zeroPadding] = {0};
  dstFile.write(silence, zeroPadding); // Initial zero padding
  for(int a = 0; a < data.size(); a += BUFFER) {
    sf_count_t write;
    for(write = 0; write < BUFFER; ++write) {
      if(a + write >= data.size())
	break;
      buffer[write] = data.at(a + write);
      if((a + write) % dotMod == 0) {
	printf(".");
	fflush(stdout);
      }
    }
    dstFile.write(buffer, write);
  }
  dstFile.write(silence, zeroPadding); // Trailing zero padding
  printf(" Done!\n");
}

void zeroAdjust(std::vector<short> &data, SndfileHandle &srcFile)
{
  int segLen = srcFile.samplerate() / 1102; // 20 for a 22050 Hz file
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
  zeroAdjust(data, srcFile);

  // Write zero adjusted
  {
    printf("Writing to 'zero_adjusted.wav'");
    SndfileHandle dstFile("zero_adjusted.wav", SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_U8, srcFile.channels(), srcFile.samplerate());
    writeAll(data, dstFile);
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
  printf("maxVal=%d\n", maxVal);
  printf("minVal=%d\n", minVal);
  int initThres = stdDevFromAbs(data, 0, data.size());
  int zeroThres = (meanFromAbs(data, 0, ZEROSPAN, initThres / 2) +
		   meanFromAbs(data, data.size() - ZEROSPAN, ZEROSPAN, initThres / 2));
  printf("Detected thresholds:\n");
  printf("  Signal trigger=%d\n", initThres);
  printf("  Zero level=%d\n", zeroThres);
  
  typedef std::pair<short, long int> pair;
  std::vector<pair> segSizes;
  double bitLength = 0.0;
  // Analyze
  {
    printf("Analyzing");
    short neededBits = 512;
    short dotMod = neededBits / 10;
    std::vector<short> bitLengths;
    short noSignal = 0;
    for(int a = 0; a < data.size(); ++a) {
      // Detect signal rise that indicate bit begin
      if(data.at(a) > initThres) {
	short curLength = 0;
	while(data.at(a + curLength) >= 0 &&
	      a + curLength < data.size()) {
	  curLength++;
	}
	while(data.at(a + curLength) <= initThres &&
	      a + curLength < data.size()) {
	  curLength++;
	}
	bitLengths.push_back(curLength);
	if(bitLengths.size() == neededBits) {
	  break;
	}
	a += curLength - 1;
	if(bitLengths.size() % dotMod) {
	  printf(".");
	  fflush(stdout);
	}
      } else {
	noSignal++;
      }
      if(noSignal == 20) {
	bitLengths.clear();
	noSignal = 0;
      }
    }
    for(int a = bitLengths.size() / 2; a < bitLengths.size(); ++a) {
      bitLength += bitLengths.at(a);
    }
    bitLength /= bitLengths.size() / 2;
    printf(" Done!\n");
  }

  std::vector<short> dataClean;
  // Clean and optimize
  {
    // Set bit length limits based on samplerate. This might need reworking
    unsigned short bitLengthOpt = srcFile.samplerate() / 2450;
    unsigned short minLen = bitLength / 4;
    unsigned short maxLen = bitLength * 3 + bitLength / 4;
    unsigned long dotMod = data.size() / 10;
    printf("  Detected zero bitlength = %f (appr. %f baud)\n", bitLength, srcFile.samplerate() / bitLength);
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
	    //printf("STOPBIT!!!\n");
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
    printf("Writing to '%s'", dstName.c_str());
    SndfileHandle dstFile(dstName.c_str(), SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_U8, srcFile.channels(), srcFile.samplerate());
    writeAll(dataClean, dstFile, srcFile.samplerate());
  }
  return 0;
}

/* TODO
NEW:
- Lav std dev på al data. Frasorter al data der ligger over/under std dev'en. Al tilbageværende data tages der gennemsnit af. Det er min silence threshold. Det kan være jeg kun behøver kigge på data > 0. Ellers kan jeg lave abs på al data før udregningerne.
- Segmenter data i klumper af 1024. Tag max og min for hvert segment ved at finde det højeste og laveste tal. Lav vector med hver enkelt segments max og min. Tag gennemsnittet af dem og brug dem init threshold, men nok divideret med 2. Det kan være det er nok med kun at finde max, da jeg jo har zero adjusted. Så er nedenstående også nemmere, da jeg kan nøjes med en if abs(sample) > max / 2 til både negativ og positiv signalretning.
- Jeg behøver ikke lave zero cross detection. Jeg skal bare lave init threshold detection hele tiden. Med en abs på. Det vil give mig distancen mellem to pulser.
- Det gør ikke noget at jeg har det "grimme" rise grundet zero adjustment. Jeg skal bare sørge for at den er for lang, så bliver den discarded og lavet til nuller i final signal.

- Der er en udfordring i signaler i Mission Alphatron, hvor enkelte bits ikke er clean, så enten neg eller pos pulsen er for kort.
- Ovenstående kan fikses ved at lave en state machine. pause state (bare stilhed), wait signal state (en masse 0'er), init state (en enkelt lang puls i enten neg eller pos retning), og data state (bits). stop state (der kommer igen stilhed, der skal indsættes et stop).


OLD:
- Analyze all data and group segment lengths in a map to determine what is zero, one and pause.
- Maybe even segment wav into groups of BUFFER samples and determine zero, one and pause for each in case we have a wobbly tape.
- Do running zero adjustments by continuously finding max and min samples and subtracting them from each other.
 */
