// JDTools - Patch conversion utility for Roland JD-800 / JD-990
// 2022 - 2024 by Johannes Schultz
// License: BSD 3-clause

#include "JD-800.hpp"
#include "JD-990.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <iostream>

static void ConvertToneControl(const uint8_t source, const uint8_t dest, uint8_t depth, uint8_t &aTouchBend800, Tone800 &t800)
{
	if (source == 0 && dest == 4)
	{
		// Mod Wheel to Pitch via LFO 1
		if (depth < 50)
		{
			std::cerr << "LOSSY CONVERSION! Mod Wheel to LFO1 mod matrix routing with negative modulation!" << std::endl;
			depth = 100 - depth;
		}
		t800.wg.leverSens = 50 + (depth - 50);
	}
	else if (source == 0 && dest == 5)
	{
		// Mod wheel to Pitch via LFO 2
		if (depth < 50)
		{
			std::cerr << "LOSSY CONVERSION! Mod Wheel to LFO2 mod matrix routing with negative modulation!" << std::endl;
			depth = 100 - depth;
		}
		t800.wg.leverSens = 50 - (depth - 50);
	}
	else if (source == 1 && dest == 4)
	{
		// Aftertouch to Pitch via LFO 1
		if (depth < 50)
		{
			std::cerr << "LOSSY CONVERSION! Aftertouch to LFO1 mod matrix routing with negative modulation!" << std::endl;
			depth = 100 - depth;
		}
		t800.wg.aTouchModSens = 50 + (depth - 50);
	}
	else if (source == 1 && dest == 5)
	{
		// Aftertouch to Pitch via LFO 2
		if (depth < 50)
		{
			std::cerr << "LOSSY CONVERSION! Aftertouch to LFO2 mod matrix routing with negative modulation!" << std::endl;
			depth = 100 - depth;
		}
		t800.wg.aTouchModSens = 50 - (depth - 50);
	}
	else if (source == 1 && dest == 0 && depth != 50)
	{
		// Aftertouch to Pitch
		t800.wg.aTouchBend = 1;
		if (depth == -36 + 50)
			aTouchBend800 = 0;
		else if (depth == -24 + 50)
			aTouchBend800 = 1;
		else if (depth >= -12 + 50 && depth <= 12 + 50)
			aTouchBend800 = depth - (-12 + 50) + 2;
		else
			std::cerr << "LOSSY CONVERSION! Aftertouch to pitch bend modulation has incompatible value: " << int(depth) << std::endl;
	}
	else if (source == 1 && dest == 1)
	{
		// Aftertouch to TVF
		t800.tvf.aTouchSens = depth;
	}
	else if (source == 1 && dest == 3)
	{
		// Aftertouch to TVA
		t800.tva.aTouchSens = depth;
	}
	else if (depth != 50)
	{
		std::cerr << "LOSSY CONVERSION! Unknown mod matrix routing: source = " << int(source) << ", dest = " << int(dest) << std::endl;
	}
}

static void ConvertTone990To800(const uint8_t toneControlSource1, const uint8_t toneControlSource2, const Tone990 &t990, uint8_t &aTouchBend800, Tone800 &t800, const bool isSetupConversion)
{
	t800.common.velocityCurve = t990.common.velocityCurve;
	t800.common.holdControl = t990.common.holdControl;

	static constexpr uint8_t LfoWaveform990to800[] = { 0, 0 | 0x80, 1, 2, 2 | 0x80, 3, 4, 4 | 0x80 };

	t800.lfo1.rate = t990.lfo1.rate;
	t800.lfo1.delay = t990.lfo1.delay;
	t800.lfo1.fade = t990.lfo1.fade;
	t800.lfo1.waveform = SafeTable(LfoWaveform990to800, t990.lfo1.waveform);
	t800.lfo1.offset = t990.lfo1.offset;
	t800.lfo1.keyTrigger = t990.lfo1.keyTrigger;
	if (t800.lfo1.waveform & 0x80)
	{
		t800.lfo1.waveform &= 0x7F;
		std::cerr << "LOSSY CONVERSION! JD-990 tone LFO1 has unsupported LFO waveform: " << int(t990.lfo1.waveform) << std::endl;
	}

	t800.lfo2.rate = t990.lfo2.rate;
	t800.lfo2.delay = t990.lfo2.delay;
	t800.lfo2.fade = t990.lfo2.fade;
	t800.lfo2.waveform = SafeTable(LfoWaveform990to800, t990.lfo2.waveform);
	t800.lfo2.offset = t990.lfo2.offset;
	t800.lfo2.keyTrigger = t990.lfo2.keyTrigger;
	if (t800.lfo2.waveform & 0x80)
	{
		t800.lfo2.waveform &= 0x7F;
		std::cerr << "LOSSY CONVERSION! JD-990 tone LFO2 has unsupported LFO waveform: " << int(t990.lfo2.waveform) << std::endl;
	}

	t800.wg.waveSource = t990.wg.waveSource;
	t800.wg.waveformMSB = t990.wg.waveformMSB;
	t800.wg.waveformLSB = t990.wg.waveformLSB;
	t800.wg.pitchCoarse = t990.wg.pitchCoarse;
	t800.wg.pitchFine = t990.wg.pitchFine;
	t800.wg.pitchRandom = t990.wg.pitchRandom;
	t800.wg.keyFollow = t990.wg.keyFollow;
	t800.wg.benderSwitch = t990.wg.benderSwitch;
	t800.wg.aTouchBend = 0;  // Will be populated by tone control conversion
	t800.wg.lfo1Sens = t990.lfo1.depthPitch;
	t800.wg.lfo2Sens = t990.lfo2.depthPitch;
	t800.wg.leverSens = 0;      // Will be populated by tone control conversion
	t800.wg.aTouchModSens = 0;  // Will be populated by tone control conversion
	if (t990.wg.waveSource == 0 && t990.wg.waveformMSB > 1)
		t800.wg.waveformMSB = 0;  // This makes sense neither with the JD-880 nor the JD-990 but was found in TECHNOJD.MID (conversion error?) - silently fix it
	if (t990.wg.waveSource == 0 && (t800.wg.waveformMSB > 0 || t800.wg.waveformLSB > 107))
	{
		const int waveform = (t990.wg.waveformMSB << 7) | t990.wg.waveformLSB;
		std::cerr << "LOSSY CONVERSION! JD-990 tone uses unsupported internal waveform: " << waveform << std::endl;
		if (waveform >= 108 && waveform <= 194)
		{
			// Most of these will of course not be close to the original.
			// The +DC variations should be "easy" to translate as there is no ring modulation on the JD-800,
			// so there should be no practical difference in sound (except for distortion effect maybe?)
			// For ease of cross-referencing, the indices in the table correspond to the 1-based waveform numbers found in the UI and manual
			static constexpr uint8_t WaveformMap[] =
			{
				71, 72, 72, 72, 72, 19, 40, 40, 40, 58, 58, 58, 58, 38, 38, 38,
				39, 36, 36, 70, 70, 36, 36, 36, 36, 92, 96, 96, 96, 96, 96, 94,
				97, 20, 42, 43, 44, 45, 66, 66, 47, 47, 45, 1, 1, 107, 61, 104,
				91, 91, 91, 84, 84, 84, 84, 84, 86, 86, 86, 86, 98, 98, 98, 86,
				86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 1, 4, 5, 6, 7, 8,
				9, 11, 12, 107, 107, 107, 107,
			};
			t800.wg.waveformLSB = WaveformMap[waveform - 108] - 1u;
			t800.wg.waveformMSB = 0;
		}
	}
	if (t990.wg.fxmColor != 0 || t990.wg.fxmDepth != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 tone has FXM enabled!" << std::endl;
	if (t990.wg.syncSlaveSwitch != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 tone has sync slave switch enabled!" << std::endl;
	if (t990.wg.toneDelayTime != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 tone has tone delay enabled!" << std::endl;
	if (t990.wg.envDepth != 24 && (t990.pitchEnv.level0 != 50 || t990.pitchEnv.level1 != 50 || t990.pitchEnv.sustainLevel != 50 || t990.pitchEnv.level3 != 50))
		std::cerr << "LOSSY CONVERSION! JD-990 tone has pitch envelope depth level != 24: " << int(t990.wg.envDepth) << std::endl;

	t800.pitchEnv.velo = t990.pitchEnv.velo;
	t800.pitchEnv.timeVelo = t990.pitchEnv.timeVelo;
	t800.pitchEnv.timeKF = t990.pitchEnv.timeKF;
	t800.pitchEnv.level0 = t990.pitchEnv.level0;
	t800.pitchEnv.time1 = t990.pitchEnv.time1;
	t800.pitchEnv.level1 = t990.pitchEnv.level1;
	t800.pitchEnv.time2 = t990.pitchEnv.time2;
	t800.pitchEnv.time3 = t990.pitchEnv.time3;
	t800.pitchEnv.level2 = t990.pitchEnv.level3;
	if (t990.pitchEnv.sustainLevel != 50)
		std::cerr << "LOSSY CONVERSION! JD-990 tone has pitch envelope sustain level != 50: " << int(t990.pitchEnv.sustainLevel) << std::endl;

	t800.tvf.filterMode = t990.tvf.filterMode;
	t800.tvf.cutoffFreq = t990.tvf.cutoffFreq;
	t800.tvf.resonance = t990.tvf.resonance;
	t800.tvf.keyFollow = t990.tvf.keyFollow;
	t800.tvf.aTouchSens = 0;  // Will be populated by tone control conversion
	if (t990.lfo2.depthTVF != 50)
	{
		t800.tvf.lfoSelect = 1;
		t800.tvf.lfoDepth = t990.lfo2.depthTVF;
		if (t990.lfo1.depthTVF != 50)
			std::cerr << "LOSSY CONVERSION! JD-990 tone has both LFOs controlling TVF!" << std::endl;
	}
	else
	{
		t800.tvf.lfoSelect = 0;
		t800.tvf.lfoDepth = t990.lfo1.depthTVF;
	}
	t800.tvf.envDepth = t990.tvf.envDepth;

	t800.tvfEnv.velo = t990.tvfEnv.velo;
	t800.tvfEnv.timeVelo = t990.tvfEnv.timeVelo;
	t800.tvfEnv.timeKF = t990.tvfEnv.timeKF;
	t800.tvfEnv.time1 = t990.tvfEnv.time1;
	t800.tvfEnv.level1 = t990.tvfEnv.level1;
	t800.tvfEnv.time2 = t990.tvfEnv.time2;
	t800.tvfEnv.level2 = t990.tvfEnv.level2;
	t800.tvfEnv.time3 = t990.tvfEnv.time3;
	t800.tvfEnv.sustainLevel = t990.tvfEnv.sustainLevel;
	t800.tvfEnv.time4 = t990.tvfEnv.time4;
	t800.tvfEnv.level4 = t990.tvfEnv.level4;

	t800.tva.biasDirection = t990.tva.biasDirection;
	t800.tva.biasPoint = t990.tva.biasPoint;
	t800.tva.biasLevel = t990.tva.biasLevel;
	t800.tva.level = t990.tva.level;
	t800.tva.aTouchSens = 0;  // Will be populated by tone control conversion
	if (t990.lfo2.depthTVA != 50)
	{
		t800.tva.lfoSelect = 1;
		t800.tva.lfoDepth = t990.lfo2.depthTVA;
		if (t990.lfo1.depthTVA != 50)
			std::cerr << "LOSSY CONVERSION! JD-990 tone has both LFOs controlling TVA!" << std::endl;
	}
	else
	{
		t800.tva.lfoSelect = 0;
		t800.tva.lfoDepth = t990.lfo1.depthTVA;
	}
	if (t990.tva.pan != 50 && !isSetupConversion)
	{
		std::cerr << "LOSSY CONVERSION! JD-990 tone has pan position != 50: " << int(t990.tva.pan) << std::endl;
	}
	if (t990.tva.panKeyFollow != 7)
	{
		std::cerr << "LOSSY CONVERSION! JD-990 tone uses pan key follow: " << int(t990.tva.panKeyFollow) << std::endl;
	}

	t800.tvaEnv.velo = t990.tvaEnv.velo;
	t800.tvaEnv.timeVelo = t990.tvaEnv.timeVelo;
	t800.tvaEnv.timeKF = t990.tvaEnv.timeKF;
	t800.tvaEnv.time1 = t990.tvaEnv.time1;
	t800.tvaEnv.level1 = t990.tvaEnv.level1;
	t800.tvaEnv.time2 = t990.tvaEnv.time2;
	t800.tvaEnv.level2 = t990.tvaEnv.level2;
	t800.tvaEnv.time3 = t990.tvaEnv.time3;
	t800.tvaEnv.sustainLevel = t990.tvaEnv.sustainLevel;
	t800.tvaEnv.time4 = t990.tvaEnv.time4;

	if (toneControlSource1 > 1)
	{
		std::cerr << "LOSSY CONVERSION! JD-990 patch uses tone control source 1 other than mod wheel or aftertouch: " << int(toneControlSource1) << std::endl;
	}
	if (toneControlSource2 > 1)
	{
		std::cerr << "LOSSY CONVERSION! JD-990 patch uses tone control source 2 other than mod wheel or aftertouch: " << int(toneControlSource1) << std::endl;
	}

	ConvertToneControl(toneControlSource1, t990.cs1.destination1, t990.cs1.depth1, aTouchBend800, t800);
	ConvertToneControl(toneControlSource1, t990.cs1.destination2, t990.cs1.depth2, aTouchBend800, t800);
	ConvertToneControl(toneControlSource1, t990.cs1.destination3, t990.cs1.depth3, aTouchBend800, t800);
	ConvertToneControl(toneControlSource1, t990.cs1.destination4, t990.cs1.depth4, aTouchBend800, t800);
	ConvertToneControl(toneControlSource2, t990.cs2.destination1, t990.cs2.depth1, aTouchBend800, t800);
	ConvertToneControl(toneControlSource2, t990.cs2.destination2, t990.cs2.depth2, aTouchBend800, t800);
	ConvertToneControl(toneControlSource2, t990.cs2.destination3, t990.cs2.depth3, aTouchBend800, t800);
	ConvertToneControl(toneControlSource2, t990.cs2.destination4, t990.cs2.depth4, aTouchBend800, t800);
}

static void FixupStructure990To800(const uint8_t structureType, Tone800 &tone1, Tone800 &tone2)
{
	if (structureType == 1)
	{
		// Shared filters, TVA of first tone is ignored
		tone1.tva = tone2.tva;
		tone1.tvaEnv = tone2.tvaEnv;
	}
	else if (structureType > 1)
	{
		// When using ring modulation, ignore pitch envelope of second tone.
		// This is really just a cheap cleanup to avoid weird pitches that were meant to go into the ring modulator.
		tone2.pitchEnv.level0 = 50;
		tone2.pitchEnv.level1 = 50;
		tone2.pitchEnv.level2 = 50;
	}
}

void ConvertPatch990To800(const Patch990 &p990, Patch800 &p800)
{
	if (p990.structureType.structureAB != 0 && (p990.common.activeTone & (1 | 2)) != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch tones AB have unsupported structure type: " << int(p990.structureType.structureAB) << std::endl;
	if (p990.structureType.structureCD != 0 && (p990.common.activeTone & (4 | 8)) != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch tones CD have unsupported structure type: " << int(p990.structureType.structureCD) << std::endl;

	if (p990.velocity.velocityRange1 != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch velocity range 1 is enabled: " << int(p990.velocity.velocityRange1) << std::endl;
	if (p990.velocity.velocityRange2 != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch velocity range 2 is enabled: " << int(p990.velocity.velocityRange2) << std::endl;
	if (p990.velocity.velocityRange3 != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch velocity range 3 is enabled: " << int(p990.velocity.velocityRange3) << std::endl;
	if (p990.velocity.velocityRange4 != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch velocity range 4 is enabled: " << int(p990.velocity.velocityRange4) << std::endl;

	p800.common.name = p990.common.name;
	p800.common.patchLevel = p990.common.patchLevel;
	p800.common.keyRangeLowA = p990.keyRanges.keyRangeLowA;
	p800.common.keyRangeHighA = p990.keyRanges.keyRangeHighA;
	p800.common.keyRangeLowB = p990.keyRanges.keyRangeLowB;
	p800.common.keyRangeHighB = p990.keyRanges.keyRangeHighB;
	p800.common.keyRangeLowC = p990.keyRanges.keyRangeLowC;
	p800.common.keyRangeHighC = p990.keyRanges.keyRangeHighC;
	p800.common.keyRangeLowD = p990.keyRanges.keyRangeLowD;
	p800.common.keyRangeHighD = p990.keyRanges.keyRangeHighD;
	p800.common.benderRangeDown = p990.common.bendRangeDown;
	p800.common.benderRangeUp = p990.common.bendRangeUp;
	p800.common.aTouchBend = 14;  // Will be populated by tone conversion
	p800.common.soloSW = p990.keyEffects.soloSW;
	p800.common.soloLegato = p990.keyEffects.soloLegato;
	p800.common.portamentoSW = p990.keyEffects.portamentoSW;
	p800.common.portamentoMode = p990.keyEffects.portamentoMode;
	p800.common.portamentoTime = p990.keyEffects.portamentoTime;
	p800.common.layerTone = p990.common.layerTone;
	p800.common.activeTone = p990.common.activeTone;

	if (p990.common.patchPan != 50)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has pan != 50: " << int(p990.common.patchPan) << std::endl;
	if (p990.common.analogFeel != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has analog feel != 0: " << int(p990.common.analogFeel) << std::endl;
	if (p990.common.voicePriority != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has voice priority != 0: " << int(p990.common.voicePriority) << std::endl;
	if (p990.keyEffects.portamentoType != 1 && p990.keyEffects.portamentoSW != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has portamento type != 1: " << int(p990.keyEffects.portamentoType) << std::endl;
	if (p990.keyEffects.soloSyncMaster != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has solo sync master != 0: " << int(p990.keyEffects.soloSyncMaster) << std::endl;
	if (p990.octaveSwitch != 1)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has octave switch != 1: " << int(p990.octaveSwitch) << std::endl;

	p800.eq.lowFreq = p990.eq.lowFreq;
	p800.eq.lowGain = p990.eq.lowGain;
	p800.eq.midFreq = p990.eq.midFreq;
	p800.eq.midQ = p990.eq.midQ;
	p800.eq.midGain = p990.eq.midGain;
	p800.eq.highFreq = p990.eq.highFreq;
	p800.eq.highGain = p990.eq.highGain;

	p800.midiTx.keyMode = 0;
	p800.midiTx.splitPoint = 36;
	p800.midiTx.lowerChannel = 1;
	p800.midiTx.upperChannel = 0;
	p800.midiTx.lowerProgramChange = 0;
	p800.midiTx.upperProgramChange = 0;
	p800.midiTx.holdMode = 2;
	p800.midiTx.dummy = 0;

	p800.effect.groupAsequence = p990.effect.groupAsequence;
	p800.effect.groupBsequence = p990.effect.groupBsequence;
	p800.effect.groupAblockSwitch1 = p990.effect.groupAblockSwitch1;
	p800.effect.groupAblockSwitch2 = p990.effect.groupAblockSwitch2;
	p800.effect.groupAblockSwitch3 = p990.effect.groupAblockSwitch3;
	p800.effect.groupAblockSwitch4 = p990.effect.groupAblockSwitch4;
	p800.effect.groupBblockSwitch1 = p990.effect.groupBblockSwitch1;
	p800.effect.groupBblockSwitch2 = p990.effect.groupBblockSwitch2;
	p800.effect.groupBblockSwitch3 = p990.effect.groupBblockSwitch3;
	p800.effect.effectsBalanceGroupB = p990.effect.effectsBalanceGroupB;

	p800.effect.distortionType = p990.effect.distortionType;
	p800.effect.distortionDrive = p990.effect.distortionDrive;
	p800.effect.distortionLevel = p990.effect.distortionLevel;

	p800.effect.phaserManual = p990.effect.phaserManual;
	p800.effect.phaserRate = p990.effect.phaserRate;
	p800.effect.phaserDepth = p990.effect.phaserDepth;
	p800.effect.phaserResonance = p990.effect.phaserResonance;
	p800.effect.phaserMix = p990.effect.phaserMix;

	p800.effect.spectrumBand1 = p990.effect.spectrumBand1;
	p800.effect.spectrumBand2 = p990.effect.spectrumBand2;
	p800.effect.spectrumBand3 = p990.effect.spectrumBand3;
	p800.effect.spectrumBand4 = p990.effect.spectrumBand4;
	p800.effect.spectrumBand5 = p990.effect.spectrumBand5;
	p800.effect.spectrumBand6 = p990.effect.spectrumBand6;
	p800.effect.spectrumBandwidth = p990.effect.spectrumBandwidth;

	p800.effect.enhancerSens = p990.effect.enhancerSens;
	p800.effect.enhancerMix = p990.effect.enhancerMix;

	p800.effect.delayCenterTap = std::min(p990.effect.delayCenterTapLSB, uint8_t(0x7D));
	p800.effect.delayCenterLevel = p990.effect.delayCenterLevel;
	p800.effect.delayLeftTap = std::min(p990.effect.delayLeftTapLSB, uint8_t(0x7D));
	p800.effect.delayLeftLevel = p990.effect.delayLeftLevel;
	p800.effect.delayRightTap = std::min(p990.effect.delayRightTapLSB, uint8_t(0x7D));
	p800.effect.delayRightLevel = p990.effect.delayRightLevel;
	p800.effect.delayFeedback = p990.effect.delayFeedback;
	if (p990.effect.delayCenterTapMSB != 0 || p990.effect.delayCenterTapLSB > 0x7D)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has unsupported delay center tap: " << int(p990.effect.delayCenterTapMSB) << "/" << int(p990.effect.delayCenterTapLSB) << std::endl;
	if (p990.effect.delayLeftTapMSB != 0 || p990.effect.delayLeftTapLSB > 0x7D)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has unsupported delay left tap: " << int(p990.effect.delayLeftTapMSB) << "/" << int(p990.effect.delayLeftTapLSB) << std::endl;
	if (p990.effect.delayRightTapMSB != 0 || p990.effect.delayRightTapLSB > 0x7D)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has unsupported delay right tap: " << int(p990.effect.delayRightTapMSB) << "/" << int(p990.effect.delayRightTapLSB) << std::endl;
	if (p990.effect.delayMode != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 patch has delay effect mode != 0: " << int(p990.effect.delayMode) << std::endl;

	p800.effect.chorusRate = p990.effect.chorusRate;
	p800.effect.chorusDepth = p990.effect.chorusDepth;
	p800.effect.chorusDelayTime = p990.effect.chorusDelayTime;
	p800.effect.chorusFeedback = p990.effect.chorusFeedback;
	p800.effect.chorusLevel = p990.effect.chorusLevel;

	p800.effect.reverbType = p990.effect.reverbType;
	p800.effect.reverbPreDelay = p990.effect.reverbPreDelay;
	p800.effect.reverbEarlyRefLevel = p990.effect.reverbEarlyRefLevel;
	p800.effect.reverbHFDamp = p990.effect.reverbHFDamp;
	p800.effect.reverbTime = p990.effect.reverbTime;
	p800.effect.reverbLevel = p990.effect.reverbLevel;
	p800.effect.dummy = 0;

	ConvertTone990To800(p990.common.toneControlSource1, p990.common.toneControlSource2, p990.toneA, p800.common.aTouchBend, p800.toneA, false);
	ConvertTone990To800(p990.common.toneControlSource1, p990.common.toneControlSource2, p990.toneB, p800.common.aTouchBend, p800.toneB, false);
	ConvertTone990To800(p990.common.toneControlSource1, p990.common.toneControlSource2, p990.toneC, p800.common.aTouchBend, p800.toneC, false);
	ConvertTone990To800(p990.common.toneControlSource1, p990.common.toneControlSource2, p990.toneD, p800.common.aTouchBend, p800.toneD, false);

	FixupStructure990To800(p990.structureType.structureAB, p800.toneA, p800.toneB);
	FixupStructure990To800(p990.structureType.structureCD, p800.toneC, p800.toneD);
}

void ConvertSetup990To800(const SpecialSetup990 &s990, SpecialSetup800 &s800)
{
	std::cerr << "(Setup name and effect settings cannot be converted)" << std::endl;

	s800.eq.lowFreq = s990.eq.lowFreq;
	s800.eq.lowGain = s990.eq.lowGain;
	s800.eq.midFreq = s990.eq.midFreq;
	s800.eq.midQ = s990.eq.midQ;
	s800.eq.midGain = s990.eq.midGain;
	s800.eq.highFreq = s990.eq.highFreq;
	s800.eq.highGain = s990.eq.highGain;

	s800.common.benderRangeDown = s990.common.benderRangeDown;
	s800.common.benderRangeUp = s990.common.benderRangeUp;
	s800.common.aTouchBendSens = 14;  // Will be populated by tone conversion

	if (s990.common.level != 80)
		std::cerr << "LOSSY CONVERSION! JD-990 setup has level != 80: " << int(s990.common.level) << std::endl;
	if (s990.common.pan != 50)
		std::cerr << "LOSSY CONVERSION! JD-990 setup has pan != 50: " << int(s990.common.pan) << std::endl;
	if (s990.common.analogFeel != 0)
		std::cerr << "LOSSY CONVERSION! JD-990 setup has analog feel != 0: " << int(s990.common.analogFeel) << std::endl;

	for (size_t i = 0; i < s990.keys.size(); i++)
	{
		const auto &k990 = s990.keys[i];
		auto &k800 = s800.keys[i];
		k800.name = k990.name;
		k800.muteGroup = k990.muteGroup;
		if (k990.muteGroup > 8)
		{
			std::cerr << "LOSSY CONVERSION! JD-990 setup key " << i << " has unsupported mute group: " << int(k990.muteGroup) << std::endl;
			k800.muteGroup = 0;
		}
		k800.envMode = k990.envMode;
		k800.pan = (k990.tone.tva.pan * 3u + 2u) / 5u;
		k800.effectMode = k990.effectMode;
		if (k990.effectMode > 3)
		{
			std::cerr << "LOSSY CONVERSION! JD-990 setup key " << i << " has unsupported effect mode: " << int(k990.effectMode) << std::endl;
			k800.effectMode = 0;
		}
		k800.effectLevel = k990.effectLevel;
		k800.dummy = 0;
		
		ConvertTone990To800(s990.common.toneControlSource1, s990.common.toneControlSource2, k990.tone, s800.common.aTouchBendSens, k800.tone, true);
	}
}
