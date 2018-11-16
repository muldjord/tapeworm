# tapeworm
A Memotech wav tapefile cleaner and optimizer by Lars Muldjord 2018. Worms through original from-tape Memotech wav files in order to try and clean and optimize the data for better loading on real Memotech hardware.

## Features
* Automatic baud rate detection based on pulses from the waiting tone.
* Segmented zero adjusting before analysis to ensure optimal pulse length detection.
* Built as a state machine, being somewhat aware of what it should be expecting next in the sample stream.
* Detection of short, medium and long pulses (not really sure what the long ones are for, but they exist).
* Separate vector output data meaning that I "build" the output from optimal pulses instead of just squaring the input directly.
* Should work for 22050 Hz files and up (But only tested thoroughly with Claus' 22050 Hz wav's). Currently requires mono files.

## Planned features
* Converting wav's back to binary

## Compiling
### Linux
The 'libsndfile' development package is required for tapeworm to compile. So install it using your distributions package manager prior to compilation. Otherwise it'll complain about a missing libsndfile header.

When you have that, compile it with:
```
$ make
```
This will give you the executable "tapeworm", ready for use.

## Usage
```
$ ./tapeworm INFILE [OUTFILE]
```
OUTFILE is optional. If none is provided it will export as "cleaned_INFILE".

## Bugs / feature requests
Can be added in a well-described topic to "Issues".
