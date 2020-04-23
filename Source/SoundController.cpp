/*
Copyright 2017-2020 Wilfried Rabouin

This file is part of CppGB.

CppGB is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CppGB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CppGB.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL.h>

#include "Error.h"
#include "Memory.h"
#include "SoundController.h"

constexpr int SAMPLING_FREQUENCY = 48000;
constexpr f32 SAMPLING_PERIOD = 1.0f / SAMPLING_FREQUENCY;

std::array<u8, 8> getRectangleWaveform(u8 dutyCycle)
{
	switch (dutyCycle)
	{
	case 0: return { 1,0,0,0,0,0,0,0 };
	case 1: return { 1,1,0,0,0,0,0,0 };
	case 2: return { 1,1,1,1,0,0,0,0 };
	case 3: return { 1,1,1,1,1,1,0,0 };
	}
	return {};
}

std::vector<u8> getEnvelope(u8 envelopeRegister)
{
	u8 defaultEnvelopeValue = envelopeRegister >> 4;
	std::vector<u8> envelope(defaultEnvelopeValue + 1, defaultEnvelopeValue);

	if (envelopeRegister & 0x07)
	{
		if (envelopeRegister & 0x08)
		{
			u8 envelopeValue = 0;

			for (u8& envelopeStep : envelope)
			{
				envelopeStep = envelopeValue;
				++envelopeValue;
			}
		}
		else
		{
			u8 envelopeValue = defaultEnvelopeValue;

			for (u8& envelopeStep : envelope)
			{
				envelopeStep = envelopeValue;
				--envelopeValue;
			}
		}
	}

	return envelope;
}

void audioCallback(void* userData, u8* stream, int streamLength)
{
	SoundController& soundController = *(SoundController*)userData;
	soundController.generateSamples(stream, streamLength);
}

SoundController::SoundController(Memory& memory) : m_memory(memory)
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO))
		throwError("Failed to init audio: ", SDL_GetError());
		
	SDL_AudioSpec desiredParameters{};
	desiredParameters.freq = SAMPLING_FREQUENCY;
	desiredParameters.format = AUDIO_U8;
	desiredParameters.channels = 1;
	desiredParameters.samples = 512;
	desiredParameters.callback = audioCallback;
	desiredParameters.userdata = this;

	if (SDL_OpenAudio(&desiredParameters, nullptr))
		throwError("Failed to open the audio device: ", SDL_GetError());

	SDL_PauseAudio(0);
}

SoundController::~SoundController()
{
	SDL_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SoundController::generateSamples(u8* stream, int streamLength)
{
	std::fill_n(stream, streamLength, (u8)0);

	if ((m_memory.NR52 & 0x80) == 0)
		return;

	m_levelDivisor_so1 = 8 - (m_memory.NR50 & 0x07);
	m_levelDivisor_so2 = 8 - ((m_memory.NR50 >> 4) & 0x07);

	if (((m_memory.NR12 >> 4) || (m_memory.NR12 & 0x08)) && (m_memory.NR52 & 0x01))
		generateSamples_channel1(stream, streamLength);

	if (((m_memory.NR22 >> 4) || (m_memory.NR22 & 0x08)) && (m_memory.NR52 & 0x02))
		generateSamples_channel2(stream, streamLength);

	if ((m_memory.NR30 & 0x80) && (m_memory.NR52 & 0x04))
		generateSamples_channel3(stream, streamLength);

	if (((m_memory.NR42 >> 4) || (m_memory.NR42 & 0x08)) && (m_memory.NR52 & 0x08))
		generateSamples_channel4(stream, streamLength);
}

void SoundController::writeToNR13(u8 value)
{
	m_memory.NR13 = value;

	m_channel1.timeOffset += m_channel1.sampleCounter * SAMPLING_PERIOD;
	m_channel1.waveStepCountOffset += m_channel1.sampleCounter * m_channel1.waveStepsPerSample;
	m_channel1.sampleCounter = 0;

	u16 x = ((m_memory.NR14 & 0x07) << 8) | m_memory.NR13;
	f32 waveStepFrequency = 1'048'576.0f / (2048 - x);
	m_channel1.waveStepsPerSample = waveStepFrequency / SAMPLING_FREQUENCY;
}

void SoundController::writeToNR14(u8 value)
{
	m_memory.NR14 = value;

	if (m_memory.NR14 & 0x80) // restart ?
	{
		m_channel1.sweepShiftCounter = 0;
		m_channel1.xShadowRegister = ((m_memory.NR14 & 0x07) << 8) | m_memory.NR13;
		m_channel1.timeOffset = 0;
		m_channel1.waveStepCountOffset = 0;
		m_memory.NR52 |= 0x01;
	}
	else
	{
		m_channel1.timeOffset += m_channel1.sampleCounter * SAMPLING_PERIOD;
		m_channel1.waveStepCountOffset += m_channel1.sampleCounter * m_channel1.waveStepsPerSample;
	}

	m_channel1.sampleCounter = 0;

	u16 x = ((m_memory.NR14 & 0x07) << 8) | m_memory.NR13;
	f32 waveStepFrequency = 1'048'576.0f / (2048 - x);
	m_channel1.waveStepsPerSample = waveStepFrequency / SAMPLING_FREQUENCY;
}

void SoundController::writeToNR23(u8 value)
{
	m_memory.NR23 = value;

	m_channel2.timeOffset += m_channel2.sampleCounter * SAMPLING_PERIOD;
	m_channel2.waveStepCountOffset += m_channel2.sampleCounter * m_channel2.waveStepsPerSample;
	m_channel2.sampleCounter = 0;

	u16 x = ((m_memory.NR24 & 0x07) << 8) | m_memory.NR23;
	f32 waveStepFrequency = 1'048'576.0f / (2048 - x);
	m_channel2.waveStepsPerSample = waveStepFrequency / SAMPLING_FREQUENCY;
}

void SoundController::writeToNR24(u8 value)
{
	m_memory.NR24 = value;

	if (m_memory.NR24 & 0x80) // restart ?
	{
		m_channel2.timeOffset = 0;
		m_channel2.waveStepCountOffset = 0;
		m_memory.NR52 |= 0x02;
	}
	else
	{
		m_channel2.timeOffset += m_channel2.sampleCounter * SAMPLING_PERIOD;
		m_channel2.waveStepCountOffset += m_channel2.sampleCounter * m_channel2.waveStepsPerSample;
	}

	m_channel2.sampleCounter = 0;

	u16 x = ((m_memory.NR24 & 0x07) << 8) | m_memory.NR23;
	f32 waveStepFrequency = 1'048'576.0f / (2048 - x);
	m_channel2.waveStepsPerSample = waveStepFrequency / SAMPLING_FREQUENCY;
}

void SoundController::writeToNR33(u8 value)
{
	m_memory.NR33 = value;

	m_channel3.timeOffset += m_channel3.sampleCounter * SAMPLING_PERIOD;
	m_channel3.waveStepCountOffset += m_channel3.sampleCounter * m_channel3.waveStepsPerSample;
	m_channel3.sampleCounter = 0;
	
	u16 x = ((m_memory.NR34 & 0x07) << 8) | m_memory.NR33;
	f32 waveStepFrequency = 2'097'152.0f / (2048 - x);
	m_channel3.waveStepsPerSample = waveStepFrequency / SAMPLING_FREQUENCY;
}

void SoundController::writeToNR34(u8 value)
{
	m_memory.NR34 = value;

	if ((m_memory.NR30 & 0x80) && (m_memory.NR34 & 0x80)) // restart ?
	{
		m_channel3.timeOffset = 0;
		m_channel3.waveStepCountOffset = 0;
		m_memory.NR52 |= 0x04;
	}
	else
	{
		m_channel3.timeOffset += m_channel3.sampleCounter * SAMPLING_PERIOD;
		m_channel3.waveStepCountOffset += m_channel3.sampleCounter * m_channel3.waveStepsPerSample;
	}

	m_channel3.sampleCounter = 0;

	u16 x = ((m_memory.NR34 & 0x07) << 8) | m_memory.NR33;
	f32 waveStepFrequency = 2'097'152.0f / (2048 - x);
	m_channel3.waveStepsPerSample = waveStepFrequency / SAMPLING_FREQUENCY;
}

void SoundController::writeToNR44(u8 value)
{
	m_memory.NR44 = value;

	if (m_memory.NR44 & 0x80) // restart ?
	{
		m_channel4.lfsr = 0x7FFF;
		m_memory.NR52 |= 0x08;
		m_channel4.sampleCounter = 0;
	}
}

void SoundController::generateSamples_channel1(u8* stream, int streamLength)
{
	std::vector<u8> envelope = getEnvelope(m_memory.NR12);
	f32 envelopeStepFrequency = 64.0f / (m_memory.NR12 & 0x07);

	std::array<u8, 8> waveform = getRectangleWaveform(m_memory.NR11 >> 6);
	f32 soundLength = (64 - (m_memory.NR11 & 0x3F)) / 256.0f;

	u8 sweepShiftCount = m_memory.NR10 & 0x07;
	f32 sweepTime = ((m_memory.NR10 & 0x70) >> 4) / 128.0f;

	for (u16 streamSampleCounter = 0; streamSampleCounter < streamLength; ++streamSampleCounter, ++m_channel1.sampleCounter)
	{
		f32 time = m_channel1.sampleCounter * SAMPLING_PERIOD + m_channel1.timeOffset;

		if ((m_memory.NR14 & 0x40) && (time >= soundLength))
		{
			m_memory.NR52 &= 0xFE;
			return;
		}

		if (sweepTime)
		{
			u32 t = (u32)(time / sweepTime);
			
			if (t > sweepShiftCount)
				t = sweepShiftCount;

			if (t != m_channel1.sweepShiftCounter) // sweep ?
			{
				m_channel1.sweepShiftCounter = (u8)t;

				u16 x = m_channel1.xShadowRegister;

				if (m_memory.NR10 & 0x08)
					x -= (x >> sweepShiftCount);
				else
					x += (x >> sweepShiftCount);

				if (x > 2047) // overflow ?
				{
					m_memory.NR52 &= 0xFE;
					return;
				}

				m_channel1.xShadowRegister = x;
				m_memory.NR13 = x & 0xFF;
				m_memory.NR14 = (m_memory.NR14 & 0xF8) | (x >> 8);
				f32 waveStepFrequency = 1'048'576.0f / (2048 - x);

				if (m_memory.NR10 & 0x08)
					x -= (x >> sweepShiftCount);
				else
					x += (x >> sweepShiftCount);

				if (x > 2047) // overflow ?
				{
					m_memory.NR52 &= 0xFE;
					return;
				}

				m_channel1.timeOffset += m_channel1.sampleCounter * SAMPLING_PERIOD;
				m_channel1.waveStepCountOffset += m_channel1.sampleCounter * m_channel1.waveStepsPerSample;
				m_channel1.sampleCounter = 0;
				m_channel1.waveStepsPerSample = waveStepFrequency / SAMPLING_FREQUENCY;
			}
		}

		u32 waveStepCount = (u32)(m_channel1.sampleCounter * m_channel1.waveStepsPerSample + m_channel1.waveStepCountOffset);
		u8 waveStepNumber = waveStepCount % 8;

		if (waveform[waveStepNumber])
		{
			u32 envelopeStepCount = (u32)(envelopeStepFrequency * time);
			u8 step = (envelopeStepCount < envelope.size()) ? envelope[envelopeStepCount] : envelope.back();

			if (m_memory.NR51 & 0x01)
				stream[streamSampleCounter] += step / m_levelDivisor_so1;

			if (m_memory.NR51 & 0x10)
				stream[streamSampleCounter] += step / m_levelDivisor_so2;
		}
	}
}

void SoundController::generateSamples_channel2(u8* stream, int streamLength)
{
	std::vector<u8> envelope = getEnvelope(m_memory.NR22);
	f32 envelopeStepFrequency = 64.0f / (m_memory.NR22 & 0x07);

	std::array<u8, 8> waveform = getRectangleWaveform(m_memory.NR21 >> 6);
	f32 soundLength = (64 - (m_memory.NR21 & 0x3F)) / 256.0f;

	for (u16 streamSampleCounter = 0; streamSampleCounter < streamLength; ++streamSampleCounter, ++m_channel2.sampleCounter)
	{
		f32 time = m_channel2.sampleCounter * SAMPLING_PERIOD + m_channel2.timeOffset;
		
		if ((m_memory.NR24 & 0x40) && (time >= soundLength))
		{
			m_memory.NR52 &= 0xFD;
			return;
		}

		u32 waveStepCount = (u32)(m_channel2.sampleCounter * m_channel2.waveStepsPerSample + m_channel2.waveStepCountOffset);
		u8 waveStepNumber = waveStepCount % 8;

		if (waveform[waveStepNumber])
		{
			u32 envelopeStepCount = (u32)(envelopeStepFrequency * time);
			u8 step = (envelopeStepCount < envelope.size()) ? envelope[envelopeStepCount] : envelope.back();

			if (m_memory.NR51 & 0x02)
				stream[streamSampleCounter] += step / m_levelDivisor_so1;

			if (m_memory.NR51 & 0x20)
				stream[streamSampleCounter] += step / m_levelDivisor_so2;
		}
	}
}

void SoundController::generateSamples_channel3(u8* stream, int streamLength)
{
	u8 levelShift = [this]() -> u8
	{
		switch (m_memory.NR32 & 0x60)
		{
		case 0x00: return 4;
		case 0x20: return 0;
		case 0x40: return 1;
		case 0x60: return 2;
		default: return 0;
		}
	}();

	f32 soundLength = (256 - m_memory.NR31) / 256.0f;

	for (u16 streamSampleCounter = 0; streamSampleCounter < streamLength; ++streamSampleCounter, ++m_channel3.sampleCounter)
	{
		if (m_memory.NR34 & 0x40)
		{
			f32 time = m_channel3.sampleCounter * SAMPLING_PERIOD + m_channel3.timeOffset;

			if (time >= soundLength)
			{
				m_memory.NR52 &= 0xFB;
				return;
			}
		}

		u32 stepCount = (u32)(m_channel3.sampleCounter * m_channel3.waveStepsPerSample + m_channel3.waveStepCountOffset);
		u8 stepNumber = stepCount % 32;
		u8 step = (stepNumber % 2) ? (m_memory.read(0xFF30 + stepNumber / 2) & 0x0F) : (m_memory.read(0xFF30 + stepNumber / 2) >> 4);
		step >>= levelShift;

		if (m_memory.NR51 & 0x04)
			stream[streamSampleCounter] += step / m_levelDivisor_so1;

		if (m_memory.NR51 & 0x40)
			stream[streamSampleCounter] += step / m_levelDivisor_so2;
	}
}

void SoundController::generateSamples_channel4(u8* stream, int streamLength)
{
	std::vector<u8> envelope = getEnvelope(m_memory.NR42);
	f32 envelopeStepFrequency = 64.0f / (m_memory.NR42 & 0x07);

	f32 soundLength = (64 - (m_memory.NR41 & 0x3F)) / 256.0f;

	u8 r = m_memory.NR43 & 0x07;
	u8 s = m_memory.NR43 >> 4;
	f32 stepFrequency = 524'288.0f / (r == 0 ? 0.5f : r) / (1 << (s + 1));
	f32 waveStepsPerSample = stepFrequency / SAMPLING_FREQUENCY;

	static f32 stepCounter = 0;

	for (u16 streamSampleCounter = 0; streamSampleCounter < streamLength; ++streamSampleCounter, ++m_channel4.sampleCounter, stepCounter += waveStepsPerSample)
	{
		f32 time = m_channel4.sampleCounter * SAMPLING_PERIOD;

		if ((m_memory.NR44 & 0x40) && (time >= soundLength))
		{
			m_memory.NR52 &= 0xF7;
			return;
		}

		while (stepCounter >= 1)
		{
			--stepCounter;

			u8 bit0 = m_channel4.lfsr & 1;
			m_channel4.lfsr >>= 1;
			u8 bit1 = m_channel4.lfsr & 1;

			u8 result = bit1 ^ bit0;
			m_channel4.lfsr |= result << 14;

			if (m_memory.NR43 & 0x08)
				m_channel4.lfsr = (result << 6) | (m_channel4.lfsr & 0xFFBF);
		}

		if (m_channel4.lfsr & 1)
		{
			u32 envelopeStepCount = (u32)(envelopeStepFrequency * time);
			u8 step = (envelopeStepCount < envelope.size()) ? envelope[envelopeStepCount] : envelope.back();

			if (m_memory.NR51 & 0x08)
				stream[streamSampleCounter] += step / m_levelDivisor_so1;

			if (m_memory.NR51 & 0x80)
				stream[streamSampleCounter] += step / m_levelDivisor_so2;
		}
	}
}
