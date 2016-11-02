#pragma once

#include "AudioLib/Utils.h"

namespace NoiseInvader
{
	class Sma
	{
	private:
		double* queue;
		int sampleCount;

		int head;
		double sum;
		double dbDecayPerSample;

	public:

		Sma(int sampleCount)
		{
			this->sampleCount = sampleCount;
			this->queue = new double[sampleCount];
			for (int i = 0; i < sampleCount; i++)
				queue[i] = 0.0;

			head = 0;
			sum = 0.0;
			dbDecayPerSample = 0.0;
		}

		~Sma()
		{
			delete queue;
		}

		double GetDbDecayPerSample()
		{
			return dbDecayPerSample;
		}

		double Update(double sample)
		{
			auto takeAway = queue[head];
			queue[head] = sample;
			head++;
			if (head >= sampleCount)
				head = 0;

			sum -= takeAway;
			sum += sample;

			auto sampleDb = AudioLib::Utils::Gain2DB(sample);
			auto takeAwayDb = AudioLib::Utils::Gain2DB(takeAway);

			if (sampleDb < -150)
				sampleDb = -150;
			if (takeAwayDb < -150)
				takeAwayDb = -150;

			dbDecayPerSample = (sampleDb - takeAwayDb) / sampleCount;

			return sum / sampleCount;
		}
	};

	class Ema
	{
	private:
		double alpha;
		double value;

	public:

		Ema(double alpha)
		{
			this->alpha = alpha;
		}

		double Update(double sample)
		{
			value = sample * alpha + value * (1 - alpha);
			return value;
		}
	};

	class EmaLatch
	{
	private:
		double alpha;
		double latch;
		double value;
		double currentValue;

	public:

		EmaLatch(double alpha, double latch)
		{
			this->alpha = alpha;
			this->latch = latch;
		}

		double Update(bool input)
		{
			auto sample = input ? 1.0 : -1.0;
			value = sample * alpha + value * (1 - alpha);

			if (value > latch)
				currentValue = 1.0;
			if (value < -latch)
				currentValue = -1.0;

			return currentValue;
		}
	};
}
