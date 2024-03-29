// Copyright 2012-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_PARAMETERS_PARAMETERS_H
#define LV2_PARAMETERS_PARAMETERS_H

/**
   @defgroup parameters Parameters
   @ingroup lv2

   Common parameters for audio processing.

   See <http://lv2plug.in/ns/ext/parameters> for details.

   @{
*/

// clang-format off

#define LV2_PARAMETERS_URI    "http://lv2plug.in/ns/ext/parameters"  ///< http://lv2plug.in/ns/ext/parameters
#define LV2_PARAMETERS_PREFIX LV2_PARAMETERS_URI "#"                 ///< http://lv2plug.in/ns/ext/parameters#

#define LV2_PARAMETERS__CompressorControls LV2_PARAMETERS_PREFIX "CompressorControls"  ///< http://lv2plug.in/ns/ext/parameters#CompressorControls
#define LV2_PARAMETERS__ControlGroup       LV2_PARAMETERS_PREFIX "ControlGroup"        ///< http://lv2plug.in/ns/ext/parameters#ControlGroup
#define LV2_PARAMETERS__EnvelopeControls   LV2_PARAMETERS_PREFIX "EnvelopeControls"    ///< http://lv2plug.in/ns/ext/parameters#EnvelopeControls
#define LV2_PARAMETERS__FilterControls     LV2_PARAMETERS_PREFIX "FilterControls"      ///< http://lv2plug.in/ns/ext/parameters#FilterControls
#define LV2_PARAMETERS__OscillatorControls LV2_PARAMETERS_PREFIX "OscillatorControls"  ///< http://lv2plug.in/ns/ext/parameters#OscillatorControls
#define LV2_PARAMETERS__amplitude          LV2_PARAMETERS_PREFIX "amplitude"           ///< http://lv2plug.in/ns/ext/parameters#amplitude
#define LV2_PARAMETERS__attack             LV2_PARAMETERS_PREFIX "attack"              ///< http://lv2plug.in/ns/ext/parameters#attack
#define LV2_PARAMETERS__bypass             LV2_PARAMETERS_PREFIX "bypass"              ///< http://lv2plug.in/ns/ext/parameters#bypass
#define LV2_PARAMETERS__cutoffFrequency    LV2_PARAMETERS_PREFIX "cutoffFrequency"     ///< http://lv2plug.in/ns/ext/parameters#cutoffFrequency
#define LV2_PARAMETERS__decay              LV2_PARAMETERS_PREFIX "decay"               ///< http://lv2plug.in/ns/ext/parameters#decay
#define LV2_PARAMETERS__delay              LV2_PARAMETERS_PREFIX "delay"               ///< http://lv2plug.in/ns/ext/parameters#delay
#define LV2_PARAMETERS__dryLevel           LV2_PARAMETERS_PREFIX "dryLevel"            ///< http://lv2plug.in/ns/ext/parameters#dryLevel
#define LV2_PARAMETERS__frequency          LV2_PARAMETERS_PREFIX "frequency"           ///< http://lv2plug.in/ns/ext/parameters#frequency
#define LV2_PARAMETERS__gain               LV2_PARAMETERS_PREFIX "gain"                ///< http://lv2plug.in/ns/ext/parameters#gain
#define LV2_PARAMETERS__hold               LV2_PARAMETERS_PREFIX "hold"                ///< http://lv2plug.in/ns/ext/parameters#hold
#define LV2_PARAMETERS__pulseWidth         LV2_PARAMETERS_PREFIX "pulseWidth"          ///< http://lv2plug.in/ns/ext/parameters#pulseWidth
#define LV2_PARAMETERS__ratio              LV2_PARAMETERS_PREFIX "ratio"               ///< http://lv2plug.in/ns/ext/parameters#ratio
#define LV2_PARAMETERS__release            LV2_PARAMETERS_PREFIX "release"             ///< http://lv2plug.in/ns/ext/parameters#release
#define LV2_PARAMETERS__resonance          LV2_PARAMETERS_PREFIX "resonance"           ///< http://lv2plug.in/ns/ext/parameters#resonance
#define LV2_PARAMETERS__sampleRate         LV2_PARAMETERS_PREFIX "sampleRate"          ///< http://lv2plug.in/ns/ext/parameters#sampleRate
#define LV2_PARAMETERS__sustain            LV2_PARAMETERS_PREFIX "sustain"             ///< http://lv2plug.in/ns/ext/parameters#sustain
#define LV2_PARAMETERS__threshold          LV2_PARAMETERS_PREFIX "threshold"           ///< http://lv2plug.in/ns/ext/parameters#threshold
#define LV2_PARAMETERS__waveform           LV2_PARAMETERS_PREFIX "waveform"            ///< http://lv2plug.in/ns/ext/parameters#waveform
#define LV2_PARAMETERS__wetDryRatio        LV2_PARAMETERS_PREFIX "wetDryRatio"         ///< http://lv2plug.in/ns/ext/parameters#wetDryRatio
#define LV2_PARAMETERS__wetLevel           LV2_PARAMETERS_PREFIX "wetLevel"            ///< http://lv2plug.in/ns/ext/parameters#wetLevel

// clang-format on

/**
   @}
*/

#endif // LV2_PARAMETERS_PARAMETERS_H
