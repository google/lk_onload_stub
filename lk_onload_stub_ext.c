/*
 * Copyright 2023 Google LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/* lk_onload_stub_ext
 *
 * For regression tests only. Not intended to be used directly.
 *
 * Exports symbols so that test binary can build and link.
 * The test then uses LD_PRELOAD with ld_onload_stub to get the
 * "real" (stub) implementation.
 *
 * Therefore all functions here return with error.
 */

#include "lk_onload_stub_ext.h"
