using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NoiseGate
{
	class Sma
	{
		private double[] queue;
		private int sampleCount;

		private int head;
		private double sum;
		private double dbDecayPerSample;
		

		public Sma(int sampleCount)
		{
			this.sampleCount = sampleCount;
			this.queue = new double[sampleCount];
			for (int i = 0; i < sampleCount; i++)
				queue[i] = 0.0;

			head = 0;
			sum = 0.0;
			dbDecayPerSample = 0.0;
		}

		public double GetDbDecayPerSample()
		{
			return dbDecayPerSample;
		}

		public double Update(double sample)
		{
			var takeAway = queue[head];
			queue[head] = sample;
			head++;
			if (head >= sampleCount)
				head = 0;

			sum -= takeAway;
			sum += sample;
			
			var sampleDb = Utils.Gain2Db(sample);
			var takeAwayDb = Utils.Gain2Db(takeAway);

			if (sampleDb < -150)
				sampleDb = -150;
			if (takeAwayDb < -150)
				takeAwayDb = -150;

			dbDecayPerSample = (sampleDb - takeAwayDb) / sampleCount;

			return sum / sampleCount;
		}
	}

	class Ema
	{
		private double alpha;
		private double value;
		
		public Ema(double alpha)
		{
			this.alpha = alpha;
		}

		public double Update(double sample)
		{
			value = sample * alpha + value * (1 - alpha);
			return value;
		}
	}

	class EmaLatch
	{
		private double alpha;
		private double latch;
		private double value;
		private double currentValue;

		public EmaLatch(double alpha, double latch)
		{
			this.alpha = alpha;
			this.latch = latch;
		}

		public double Update(bool input)
		{
			var sample = input ? 1.0 : -1.0;
			value = sample * alpha + value * (1 - alpha);

			if (value > latch)
				currentValue = 1.0;
			if (value < -latch)
				currentValue = -1.0;

			return currentValue;
		}
	}
}
