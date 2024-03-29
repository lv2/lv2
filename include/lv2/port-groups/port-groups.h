// Copyright 2012-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_PORT_GROUPS_PORT_GROUPS_H
#define LV2_PORT_GROUPS_PORT_GROUPS_H

/**
   @defgroup port-groups Port Groups
   @ingroup lv2

   Multi-channel groups of LV2 ports.

   See <http://lv2plug.in/ns/ext/port-groups> for details.

   @{
*/

// clang-format off

#define LV2_PORT_GROUPS_URI    "http://lv2plug.in/ns/ext/port-groups"  ///< http://lv2plug.in/ns/ext/port-groups
#define LV2_PORT_GROUPS_PREFIX LV2_PORT_GROUPS_URI "#"                 ///< http://lv2plug.in/ns/ext/port-groups#

#define LV2_PORT_GROUPS__DiscreteGroup          LV2_PORT_GROUPS_PREFIX "DiscreteGroup"           ///< http://lv2plug.in/ns/ext/port-groups#DiscreteGroup
#define LV2_PORT_GROUPS__Element                LV2_PORT_GROUPS_PREFIX "Element"                 ///< http://lv2plug.in/ns/ext/port-groups#Element
#define LV2_PORT_GROUPS__FivePointOneGroup      LV2_PORT_GROUPS_PREFIX "FivePointOneGroup"       ///< http://lv2plug.in/ns/ext/port-groups#FivePointOneGroup
#define LV2_PORT_GROUPS__FivePointZeroGroup     LV2_PORT_GROUPS_PREFIX "FivePointZeroGroup"      ///< http://lv2plug.in/ns/ext/port-groups#FivePointZeroGroup
#define LV2_PORT_GROUPS__FourPointZeroGroup     LV2_PORT_GROUPS_PREFIX "FourPointZeroGroup"      ///< http://lv2plug.in/ns/ext/port-groups#FourPointZeroGroup
#define LV2_PORT_GROUPS__Group                  LV2_PORT_GROUPS_PREFIX "Group"                   ///< http://lv2plug.in/ns/ext/port-groups#Group
#define LV2_PORT_GROUPS__InputGroup             LV2_PORT_GROUPS_PREFIX "InputGroup"              ///< http://lv2plug.in/ns/ext/port-groups#InputGroup
#define LV2_PORT_GROUPS__MidSideGroup           LV2_PORT_GROUPS_PREFIX "MidSideGroup"            ///< http://lv2plug.in/ns/ext/port-groups#MidSideGroup
#define LV2_PORT_GROUPS__MonoGroup              LV2_PORT_GROUPS_PREFIX "MonoGroup"               ///< http://lv2plug.in/ns/ext/port-groups#MonoGroup
#define LV2_PORT_GROUPS__OutputGroup            LV2_PORT_GROUPS_PREFIX "OutputGroup"             ///< http://lv2plug.in/ns/ext/port-groups#OutputGroup
#define LV2_PORT_GROUPS__SevenPointOneGroup     LV2_PORT_GROUPS_PREFIX "SevenPointOneGroup"      ///< http://lv2plug.in/ns/ext/port-groups#SevenPointOneGroup
#define LV2_PORT_GROUPS__SevenPointOneWideGroup LV2_PORT_GROUPS_PREFIX "SevenPointOneWideGroup"  ///< http://lv2plug.in/ns/ext/port-groups#SevenPointOneWideGroup
#define LV2_PORT_GROUPS__SixPointOneGroup       LV2_PORT_GROUPS_PREFIX "SixPointOneGroup"        ///< http://lv2plug.in/ns/ext/port-groups#SixPointOneGroup
#define LV2_PORT_GROUPS__StereoGroup            LV2_PORT_GROUPS_PREFIX "StereoGroup"             ///< http://lv2plug.in/ns/ext/port-groups#StereoGroup
#define LV2_PORT_GROUPS__ThreePointZeroGroup    LV2_PORT_GROUPS_PREFIX "ThreePointZeroGroup"     ///< http://lv2plug.in/ns/ext/port-groups#ThreePointZeroGroup
#define LV2_PORT_GROUPS__center                 LV2_PORT_GROUPS_PREFIX "center"                  ///< http://lv2plug.in/ns/ext/port-groups#center
#define LV2_PORT_GROUPS__centerLeft             LV2_PORT_GROUPS_PREFIX "centerLeft"              ///< http://lv2plug.in/ns/ext/port-groups#centerLeft
#define LV2_PORT_GROUPS__centerRight            LV2_PORT_GROUPS_PREFIX "centerRight"             ///< http://lv2plug.in/ns/ext/port-groups#centerRight
#define LV2_PORT_GROUPS__element                LV2_PORT_GROUPS_PREFIX "element"                 ///< http://lv2plug.in/ns/ext/port-groups#element
#define LV2_PORT_GROUPS__group                  LV2_PORT_GROUPS_PREFIX "group"                   ///< http://lv2plug.in/ns/ext/port-groups#group
#define LV2_PORT_GROUPS__left                   LV2_PORT_GROUPS_PREFIX "left"                    ///< http://lv2plug.in/ns/ext/port-groups#left
#define LV2_PORT_GROUPS__lowFrequencyEffects    LV2_PORT_GROUPS_PREFIX "lowFrequencyEffects"     ///< http://lv2plug.in/ns/ext/port-groups#lowFrequencyEffects
#define LV2_PORT_GROUPS__mainInput              LV2_PORT_GROUPS_PREFIX "mainInput"               ///< http://lv2plug.in/ns/ext/port-groups#mainInput
#define LV2_PORT_GROUPS__mainOutput             LV2_PORT_GROUPS_PREFIX "mainOutput"              ///< http://lv2plug.in/ns/ext/port-groups#mainOutput
#define LV2_PORT_GROUPS__rearCenter             LV2_PORT_GROUPS_PREFIX "rearCenter"              ///< http://lv2plug.in/ns/ext/port-groups#rearCenter
#define LV2_PORT_GROUPS__rearLeft               LV2_PORT_GROUPS_PREFIX "rearLeft"                ///< http://lv2plug.in/ns/ext/port-groups#rearLeft
#define LV2_PORT_GROUPS__rearRight              LV2_PORT_GROUPS_PREFIX "rearRight"               ///< http://lv2plug.in/ns/ext/port-groups#rearRight
#define LV2_PORT_GROUPS__right                  LV2_PORT_GROUPS_PREFIX "right"                   ///< http://lv2plug.in/ns/ext/port-groups#right
#define LV2_PORT_GROUPS__side                   LV2_PORT_GROUPS_PREFIX "side"                    ///< http://lv2plug.in/ns/ext/port-groups#side
#define LV2_PORT_GROUPS__sideChainOf            LV2_PORT_GROUPS_PREFIX "sideChainOf"             ///< http://lv2plug.in/ns/ext/port-groups#sideChainOf
#define LV2_PORT_GROUPS__sideLeft               LV2_PORT_GROUPS_PREFIX "sideLeft"                ///< http://lv2plug.in/ns/ext/port-groups#sideLeft
#define LV2_PORT_GROUPS__sideRight              LV2_PORT_GROUPS_PREFIX "sideRight"               ///< http://lv2plug.in/ns/ext/port-groups#sideRight
#define LV2_PORT_GROUPS__source                 LV2_PORT_GROUPS_PREFIX "source"                  ///< http://lv2plug.in/ns/ext/port-groups#source
#define LV2_PORT_GROUPS__subGroupOf             LV2_PORT_GROUPS_PREFIX "subGroupOf"              ///< http://lv2plug.in/ns/ext/port-groups#subGroupOf

// clang-format on

/**
   @}
*/

#endif // LV2_PORT_GROUPS_PORT_GROUPS_H
