//t = current time (in any unit measure, but same unit as duration)
//b = starting value to interpolate
//c = the total change in value of b that needs to occur
//d = total time it should take to complete (duration)

static float 
EaseLinear(float t, float b, float c, float d) { 
	return c*t/d + b; 
}

static float 
EaseSineIn(float t, float b, float c, float d) { 
	return (-c*cosf(t/d*(PI32/2.0f)) + c + b); 
}

static float 
EaseSineOut(float t, float b, float c, float d) { 
	return (c*sinf(t/d*(PI32/2.0f)) + b); 
} 

static float 
EaseSineInOut(float t, float b, float c, float d) {
	return (-c/2.0f*(cosf(PI32*t/d) - 1.0f) + b);
}
