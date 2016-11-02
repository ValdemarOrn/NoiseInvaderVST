using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NoiseGate
{
	class Sma
	{
		private Queue<double> queue;
		private double sum;
		private double dbDecayPerSample;

		public double DbDecayPerSample { get { return dbDecayPerSample; } }

		public Sma(int samples)
		{
			this.queue = new Queue<double>();
			for (int i = 0; i < samples; i++)
			{
				queue.Enqueue(0.0);
			}
		}

		public double Update(double sample)
		{
			var takeAway = queue.Dequeue();
			queue.Enqueue(sample);
			sum -= takeAway;
			sum += sample;
			
			var sampleDb = Utils.Gain2Db(sample);
			var takeAwayDb = Utils.Gain2Db(takeAway);

			if (sampleDb < -150)
				sampleDb = -150;
			if (takeAwayDb < -150)
				takeAwayDb = -150;

			dbDecayPerSample = (sampleDb - takeAwayDb) / queue.Count;

			return sum / queue.Count;
		}
	}

	class Ema
	{
		private double value;
		private double alpha;

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
		private double value;
		private double alpha;
		private double latch;
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
