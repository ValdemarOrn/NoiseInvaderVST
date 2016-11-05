#pragma once

#include "AudioLib/Utils.h"
#include "AudioLib/Biquad.h"
#include "Indicators.h"
#include "AudioLib/OnePoleFilters.h"

namespace NoiseInvader
{
	class EnvelopeFollower
	{
	private:
		const double InputFilterHpCutoff = 100.0;
		const double InputFilterCutoff = 2000.0;
		const double EmaFc = 200.0;
		const double SmaPeriodSeconds = 0.01; // 10ms
		const double TimeoutPeriodSeconds = 0.01; // 10ms
		const double HoldSmootherFc = 200.0;

		double Fs;
		double ReleaseMs;

		AudioLib::Hp1 hpFilter;
		AudioLib::Biquad* inputFilter;
		Sma* sma;
		Ema* ema;
		EmaLatch* movementLatch;

		int triggerCounterTimeoutSamples;
		double slowDecay;
		double fastDecay;
		double holdAlpha;

		double hold;
		int lastTriggerCounter;
		double h1, h2, h3, h4;
		double holdFiltered;

	public:

		EnvelopeFollower(double fs, double releaseMs)
		{
			Fs = fs;
			double ts = 1.0 / fs;

			hpFilter.SetFc(InputFilterHpCutoff / (fs * 0.5));

			inputFilter = new AudioLib::Biquad(AudioLib::Biquad::FilterType::LowPass, fs);
			inputFilter->Frequency = InputFilterCutoff;
			inputFilter->SetQ(1.0f);
			inputFilter->Update();
			
			auto emaAlpha = AudioLib::Utils::ComputeLpAlpha(EmaFc, ts);

			double slowDbDecayPerSample = -60 / (3000 / 1000.0 * fs);
			slowDecay = AudioLib::Utils::DB2gain(slowDbDecayPerSample);

			SetRelease(releaseMs);

			sma = new Sma((int)(fs * SmaPeriodSeconds));
			ema = new Ema(emaAlpha);
			movementLatch = new EmaLatch(0.005, 0.2); // frequency dependent, but not really that critical...

			triggerCounterTimeoutSamples = (int)(fs * TimeoutPeriodSeconds);

			holdAlpha = AudioLib::Utils::ComputeLpAlpha(HoldSmootherFc, ts);
		}

		~EnvelopeFollower()
		{
			delete inputFilter;
			delete sma;
			delete ema;
			delete movementLatch;
		}

		void SetRelease(double releaseMs)
		{
			ReleaseMs = releaseMs;
			double dbDecayPerSample = -60 / (ReleaseMs / 1000.0 * Fs);
			fastDecay = AudioLib::Utils::DB2gain(dbDecayPerSample);
		}

		double GetOutput()
		{
			return holdFiltered;
		}

		void ProcessEnvelope(double val)
		{
			double combinedFiltered;
			double decay;

			// 1. Rectify the input signal
			val = std::abs(val);

			// 2. Band pass filter to ~  100hz - 2Khz
			val = hpFilter.Process(val);
			auto lpValue = inputFilter->Process(val);
			//hpSignal = hipassAlpha * lpSignal + (1 - hipassAlpha) * hpSignal

			// rectify the lpValue again, because the resonance in the filter can cause a tiny bit of ringing and cause the values to go negative again
			lpValue = std::abs(lpValue);

			auto mainInput = lpValue;

			// 3. Compute the EMA and SMA of the band-filtered signal. Also compute the per-sample dB decay baed on the SMA
			auto emaValue = ema->Update(mainInput);
			auto smaValue = sma->Update(mainInput);

			// 4. use a latching low-pass classifier to determine if signal strength is generally increasing or decreasing.
			// This removes spike from the signal where the SMA may move in the opposite direction for a short period
			auto movementValue = movementLatch->Update(sma->GetDbDecayPerSample() > 0);

			// 5. If the movement is going up, prefer the faster moving EMA signal if it's above the SMA
			// If the movement is going down, prefer the faster moving EMA signal if it's below the SMA
			// otherwise, use SMA
			if (movementValue > 0) // going up
				combinedFiltered = emaValue > smaValue ? emaValue : smaValue;
			else // going down
				combinedFiltered = emaValue < smaValue ? emaValue : smaValue;

			// 6. use a hold mechanism to store the peak
			if (combinedFiltered > hold)
			{
				hold = combinedFiltered;
				lastTriggerCounter = 0;
			}

			// 7. Choosing the decay speed
			// Under normal conditions, use the decay from the SMA, scaled by a fudge factor to make it slightly faster decaying.
			// The reason for this is so that we gently bump into the peaks of the signal once in a while.
			// If the hold mechanism hasn't been triggered for a specific timeout, then the current hold value is too high, and we need to rapidly decay downwards.
			// Use the fastDecay (based on the user- specified release value) as a slew limited value
			if (lastTriggerCounter > triggerCounterTimeoutSamples)
				decay = fastDecay;
			else
				decay = AudioLib::Utils::DB2gain(sma->GetDbDecayPerSample() * 1.2); // 1.2 is fudge factor to make the follower decay slightly faster than actual signal, so we gently bump into the peaks

			// 7.5 Limit the decay speed in the general range of slowDecay...fastDecay, the slow decay is currently a fixed 3 seconds to -60dB value
			if (decay > slowDecay)
				decay = slowDecay;
			if (decay < fastDecay)
				decay = fastDecay;

			hold = hold * decay;

			// 8. Filter the resulting hold signal to retrieve a smooth envelope.
			// Currently using 4x 1 pole lowpass, should replace with a proper 4th order butterworth
			h1 = holdAlpha * hold + (1 - holdAlpha) * h1;
			h2 = holdAlpha * h1 + (1 - holdAlpha) * h2;
			h3 = holdAlpha * h2 + (1 - holdAlpha) * h3;
			h4 = holdAlpha * h3 + (1 - holdAlpha) * h4;

			holdFiltered = h4;
			lastTriggerCounter++;
		}

	};
}
