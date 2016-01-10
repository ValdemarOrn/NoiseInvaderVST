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

	// The maximum number of peaks we could see in the hold period specified
	int peakCount;
	
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
		this->decay = decay;
		peakCount = (int)(peakHoldMillis / 1000.0f * fs);

		peakStorage = new IntFloatPair[peakCount];

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
			peakWriteIndex = (peakWriteIndex + 1) % peakCount;
		}

		// find peak
		IntFloatPair maxPeak;
		bool foundPeak = false;
		int readIdx = peakReadIndex;
		int minTimeIndex = timeIndex - peakCount;
		while (readIdx != peakWriteIndex)
		{
			auto p = peakStorage[readIdx];
			if (p.Int < minTimeIndex)
			{
				// this is old data, move read header
				peakReadIndex = (peakReadIndex + 1) % peakCount;
			}
			else
			{
				if (!foundPeak || p.Float > maxPeak.Float)
				{
					maxPeak = p;
					foundPeak = true;
				}
			}

			readIdx = (readIdx + 1) % peakCount;
		}

		auto fallbackValue = currentValue * decay;
		if (fallbackValue < val)
			fallbackValue = val;

		if (foundPeak && maxPeak.Float > fallbackValue)
		{
			currentValue = maxPeak.Float;
		}
		else
		{
			currentValue = fallbackValue;
		}

		prevInputValue = val;
		timeIndex++;

		return currentValue;
	}
};
