#pragma once

class IntFloatPair
{
public:
	long Int;
	float Float;

	IntFloatPair()
	{
		Int = 0;
		Float = 0.0f;
	}

	IntFloatPair(int i, float f)
	{
		Int = i;
		Float = f;
	}
};

class PeakDetector
{
private:
	
	// array of peaks and the time indices
	IntFloatPair* peakStorage;

	// Samplerate
	float fs;

	// The decay of the output value, when no peak is active to keep it level
	float decay;

	// The maximum number of samples we see in the hold period specified
	int windowSize;
	
	// The previous input value. If new input value < prevInputValue, then that was a peak
	float prevInputValue;

	// iterator that increments by one for every sample processed.
	long timeIndex;

	int peakReadIndex;
	int peakWriteIndex;

	// the output value of the detector
	float currentValue;

public:
	PeakDetector(double fs, float decay = 0.995f, float peakHoldMillis = 10.0f)
	{
		this->fs = fs;

		// currently, I just use the defaults, they work very well for a guitar signal.
		this->decay = decay;
		windowSize = (int)(peakHoldMillis / 1000.0f * fs);
		
		peakStorage = new IntFloatPair[windowSize];

		prevInputValue = 0.0f;
		timeIndex = 0;
		peakReadIndex = 0;
		peakWriteIndex = 0;

		currentValue = 0.0f;
	}

	~PeakDetector()
	{
		delete[] peakStorage;
	}

	inline float ProcessPeaks(float val)
	{
		if (val < prevInputValue) // we just saw a peak, store it
		{
			peakStorage[peakWriteIndex] = IntFloatPair(timeIndex, prevInputValue);
			peakWriteIndex = (peakWriteIndex + 1) % windowSize;
		}
		prevInputValue = val;

		// find largest peak within our look-back period
		IntFloatPair maxPeak;
		bool foundPeak = false;
		int readIdx = peakReadIndex;
		int minTimeIndex = timeIndex - windowSize;
		while (readIdx != peakWriteIndex)
		{
			auto p = peakStorage[readIdx];
			if (p.Int < minTimeIndex)
			{
				// this is old data, move read header forward
				peakReadIndex = (peakReadIndex + 1) % windowSize;
			}
			else
			{
				if (!foundPeak || p.Float > maxPeak.Float)
				{
					maxPeak = p;
					foundPeak = true;
				}
			}

			readIdx = (readIdx + 1) % windowSize;
		}

		// if no peak has occurred in the time period we are looking back at, fall back to a decaying signal
		auto fallbackValue = currentValue * decay;
		if (fallbackValue < val)
			fallbackValue = val;

		if (foundPeak && maxPeak.Float > fallbackValue)
			currentValue = maxPeak.Float;
		else
			currentValue = fallbackValue;

		timeIndex++;
		return currentValue;
	}
};
