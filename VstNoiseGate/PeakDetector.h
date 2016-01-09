#pragma once

class IntFloatPair
{
public:
	int Int;
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
	
	IntFloatPair* peakStorage;

	float fs;
	float decay;
	int PEAK_COUNT;
	
	float prevValue;
	int timeIndex;
	int peakReadIndex;
	int peakWriteIndex;

	float currentValue;

public:
	PeakDetector(double fs, float decay = 0.995f, float peakHoldMillis = 10.0f)
	{
		this->fs = fs;
		this->decay = decay;
		PEAK_COUNT = (int)(peakHoldMillis / 1000.0f * fs);

		peakStorage = new IntFloatPair[PEAK_COUNT];

		prevValue = 0.0f;
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
		if (val < prevValue) // we just saw a peak, store it
		{
			peakStorage[peakWriteIndex] = IntFloatPair(timeIndex, prevValue);
			peakWriteIndex = (peakWriteIndex + 1) % PEAK_COUNT;
		}

		// find peak
		IntFloatPair maxPeak;
		bool foundPeak = false;
		int readIdx = peakReadIndex;
		int minTimeIndex = timeIndex - PEAK_COUNT;
		while (readIdx != peakWriteIndex)
		{
			auto p = peakStorage[readIdx];
			if (p.Int < minTimeIndex)
			{
				// this is old data, move read header
				peakReadIndex = (peakReadIndex + 1) % PEAK_COUNT;
			}
			else
			{
				if (!foundPeak || p.Float > maxPeak.Float)
				{
					maxPeak = p;
					foundPeak = true;
				}
			}

			readIdx = (readIdx + 1) % PEAK_COUNT;
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

		prevValue = val;
		timeIndex++;

		return currentValue;
	}
};
