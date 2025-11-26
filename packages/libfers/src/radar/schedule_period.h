// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

#pragma once

#include "core/config.h"

namespace radar
{
	/**
	 * @struct SchedulePeriod
	 * @brief Represents a time period during which the transmitter is active.
	 */
	struct SchedulePeriod
	{
		RealType start{};
		RealType end{};
	};
}
