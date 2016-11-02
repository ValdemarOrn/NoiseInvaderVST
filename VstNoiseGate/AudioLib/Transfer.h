#pragma once

#include <vector>
using namespace std;

// UNTESTED BUT SHOULD WORK

namespace AudioLib
{
	class Transfer
	{
	private:
		vector<double> b;
		vector<double> a;

		double bufIn[64];
		double bufOut[64];

		double Gain;

		// buffer size
		int modulo = 64;
		int index = 0;

	public:

		Transfer()
		{
			SetB(vector<double> { 1.0 });
			SetA(vector<double> { 1.0 });
		}

		int GetOrder()
		{
			return (b.size() > a.size()) ? b.size() - 1 : a.size() - 1;
		}

		void SetB(vector<double> b)
		{
			this->b = b;
		}

		void SetA(vector<double> a)
		{
			this->a = a;
			if (a[0] == 0.0)
				Gain = 0.0;
			else
				Gain = 1 / a[0];
		}

		double Process(double input)
		{
			index = (index + 1) % modulo;

			bufIn[index] = input;
			bufOut[index] = 0;

			for (int j = 0; j < b.size(); j++)
				bufOut[index] += (b[j] * bufIn[((index - j) + modulo) % modulo]);

			for (int j = 1; j < a.size(); j++)
				bufOut[index] -= (a[j] * bufOut[((index - j) + modulo) % modulo]);

			bufOut[index] = bufOut[index] * Gain;

			auto output = bufOut[index];
			return output;
		}
	};
}
