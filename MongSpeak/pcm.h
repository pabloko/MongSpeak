#pragma once

short mix_pcm_sample_short(short aa, short bb) {
	int a = aa, b = bb, m;
	a += 32768; b += 32768;
	if ((a < 32768) || (b < 32768))
		m = a * b / 32768;
	else
		m = 2 * (a + b) - (a * b) / 32768 - 65536;
	if (m >= 65536)
		m = 65535;
	m -= 32768;
	return (short)m;
}

float mix_pcm_sample_float(float aa, float bb) {
	float m = 0;
	aa += 0.5f;
	bb += 0.5f;
	if (aa < 0.5f && bb < 0.5f)
		m = 2 * (aa * bb);
	else
		m = 2 * (aa + bb) - ((aa * bb) / 0.5f) - 1.0f;
	m -= 0.5f;
	return m;
}

void vu_meter_float(float *buffer, float* volume, int count) {
	float K = 0, suml = 0, sumr = 0;
	for (int i = 0; i < count * OPUS_CHANNELS; i += OPUS_CHANNELS)
		suml += pow(buffer[i + 0], 2), sumr += pow(buffer[i + 1], 2);
	volume[0] = (sqrt(suml / count)), volume[1] = (sqrt(sumr / count));
}
