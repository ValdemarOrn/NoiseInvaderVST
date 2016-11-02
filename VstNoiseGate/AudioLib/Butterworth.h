#pragma once

#include "Transfer.h"
#include "MathDefs.h"
#include <cmath>

// TODO: Port Bilinear code to C++
/*
namespace AudioLib
{
	class Butterworth : Transfer
	{
	public:
		
		double Fs;
		double CutoffHz;
		int Order;

		Butterworth(double fs)
		{ 
			this->Fs = fs;
		}

		void Update()
		{
			auto T = 1.0 / Fs;
			auto warpedWc = 2 / T * std::tan(CutoffHz * 2 * M_PI * T / 2);

			// don't go over nyquist, with 10 hz safety buffer
			if (warpedWc >= Fs / 2 * 2 * M_PI)
				warpedWc = (Fs - 10) / 2 * 2 * M_PI;

			double[] sb = { 1 };
			double[] sa = null;
			Func<double, double> ScaleFreq = (power) = > Math.Pow(1.0 / warpedWc, power);

			var order = (int)Math.Round(Parameters[P_ORDER]);

			if (order == 1)
			{
				sa = new [] { 1.0, ScaleFreq(1) };
			}
			else if (order == 2)
			{
				sa = new[] { 1.0, 1.4142 * ScaleFreq(1), 1 * ScaleFreq(2) };
			}
			else if (order == 3)
			{
				sa = new[] { 1, 2 * ScaleFreq(1), 2 * ScaleFreq(2), 1 * ScaleFreq(3) };
			}
			else if (order == 4)
			{
				sa = new[] { 1, 2.613 * ScaleFreq(1), 3.414 * ScaleFreq(2), 2.613 * ScaleFreq(3), 1 * ScaleFreq(4) };
			}
			else if (order == 5)
			{
				sa = new[] { 1, 3.236 * ScaleFreq(1), 5.236 * ScaleFreq(2), 5.236 * ScaleFreq(3), 3.236 * ScaleFreq(4), 1 * ScaleFreq(5) };
			}
			else if (order == 6)
			{
				sa = new[] { 1, 3.864 * ScaleFreq(1), 7.464 * ScaleFreq(2), 9.142 * ScaleFreq(3), 7.464 * ScaleFreq(4), 3.864 * ScaleFreq(5), 1 * ScaleFreq(6) };
			}
			else
			{
				throw new Exception("Orders higher than 6 are not supported");
			}

			double[] zb, za;

			Bilinear.Transform(sb, sa, out zb, out za, this.Fs);

			this.B = zb;
			this.A = za;
		}
	};
}*/