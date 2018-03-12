// Copyright (c) 2010-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

/**
 * @file battery.h
 */

#ifndef BATTERY_H_
#define BATTERY_H_

#include <nyx/common/nyx_error.h>
#include <nyx/common/nyx_battery_common.h>

// These functions are implemented in device/battery.c or emulator/fake_battery.c

// called by batterylib.c
nyx_error_t battery_init(void);
nyx_error_t battery_deinit(void);
nyx_battery_ctia_t *get_battery_ctia_params(void);

// called by battery_read_status() in batterylib.c
int battery_percent(void);
int battery_temperature(void);
int battery_voltage(void);
int battery_current(void);
int battery_avg_current(void);
double battery_full40(void);
double battery_rawcoulomb(void);
double battery_coulomb(void);
double battery_age(void);
bool battery_is_present(void);

// not currently supported by device/battery.c or emulator/fake_battery.c (stub implementations)
bool battery_authenticate(void);
void battery_set_wakeup_percent(int);

void battery_set_fakemode(bool);
nyx_error_t battery_get_fakemode(bool *);

// not currently called by batterylib.c
// bool battery_is_authenticated(const char *pair_challenge, const char *pair_response);

#endif /* BATTERY_H_ */
