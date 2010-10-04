/* LV2 OSC Messages Extension - Pretty printing methods
 * Copyright (C) 2007-2009 David Robillard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 * Helper functions for printing LV2 OSC messages as defined by the
 * LV2 OSC extension <http://lv2plug.in/ns/ext/osc>.
 */

#ifndef LV2_OSC_PRINT_H
#define LV2_OSC_PRINT_H

#include "ext/osc.lv2/osc.h"

#ifdef __cplusplus
extern "C" {
#endif

void
lv2_osc_argument_print(char type, const LV2_OSC_Argument* arg);

void
lv2_osc_message_print(const LV2_OSC_Event* msg);

#ifdef __cplusplus
}
#endif

#endif /* LV2_OSC_PRINT_H */
