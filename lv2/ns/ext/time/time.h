/*
  Copyright 2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file time.h C header for the LV2 Time extension
   <http://lv2plug.in/ns/ext/time>.
*/

#ifndef LV2_TIME_H
#define LV2_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Time states.
*/
typedef enum {
	LV2_TIME_STOPPED  = 0,  /**< Transport halted */
	LV2_TIME_ROLLING  = 1,  /**< Transport playing */
} LV2_Time_State;

/**
   Bits indicating properties of an LV2_Time_Position.
*/
typedef enum {
	LV2_TIME_HAS_BBT = 1  /**< Has Bar, Beat, Tick */
} LV2_Time_Flags;

/**
   Description of a position and/or tempo.

   This struct is used as the payload of an event to notify the plugin about
   time state, such as position and tempo.
*/
typedef struct {
	/**
	   @{
	   @name Mandatory Fields
	*/

	/**
	   Frame number on the timeline.
	*/
	uint64_t frame;

	/**
	   Bit field of LV2_Time_Flags values indicating which fields
	   of this struct are valid.
	*/
	uint32_t flags;

	/**
	   Transport state.
	*/
	LV2_Time_State state;

	/**
	   @}
	   @{
	   @name LV2_TIME_BBT fields
	   These fields are valid iff the LV2_TIME_BBT bit is set in @ref flags.
	*/

	/**
	   Current bar.
	   The first bar is number 0 (but should be represented in a UI as bar 1).
	*/
	int64_t bar;

	/**
	   Beat within the current bar.
	   The first beat is number 0.
	   Always <= @ref beats_per_bar.
	*/
	int32_t beat;

	/**
	   Tick within the current beat.
	   The first tick is number 0.
	   Always <= @ref ticks_per_beat.
	*/
	int32_t tick;

	/**
	   Number of beats per bar (top of time signature).
	*/
	int32_t beats_per_bar;

	/**
	   Type of note that counts as one beat (bottom of time signature).
	*/
	int32_t beat_type;

	/**
	   Number of ticks per beat.
	   Typically this is a large integer with many even divisors.
	*/
	int32_t ticks_per_beat;

	/**
	   Current tempo, in beats per minute.
	*/
	double beats_per_minute;

	/**
	   @}
	*/
} LV2_Time_Position;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_TIME_H */
