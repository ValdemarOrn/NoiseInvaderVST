#pragma once

class HumEliminator
{
	float fs;

public:

	// 0 == off, 1 == 50hz, 2 == 60hz
	int Mode;

	HumEliminator(float fs)
	{
		this->fs = fs;
	}
	
	inline void Process(float* input, float* output, int len)
	{

	}
};

