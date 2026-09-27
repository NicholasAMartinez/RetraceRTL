/****************************************************************************
 *
 *   Copyright (c) 2013-2024 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
/**
 * @file rtl_retrace.h
 *
 * Retrace flight path
 *
 * @author Nicholas Martinez <nicholasammartinez@gmail.com>
 */

#pragma once

#include <px4_platform_common/module_params.h>
#include <uORB/SubscriptionInterval.hpp>
#include <uORB/topics/parameter_update.h>

#include "navigator_mode.h"

#include <drivers/drv_hrt.h>
#include <matrix/matrix/math.hpp>

#include <cstddef>
#include <cstdint>

using namespace time_literals;

class Navigator;

class RtlRetrace : public NavigatorMode, public ModuleParams
{
public:
	explicit RtlRetrace(Navigator *navigator);
	~RtlRetrace() = default;

	void initialize() override {}

	/**
	 * This function is called while the mode is inactive
	 */
	void on_inactive() override;

	/**
	 * This function is called one time when mode becomes active, pos_sp_triplet must be initialized here
	 */
	void on_activation() override;

	/**
	 * This function is called one time when mode becomes inactive
	 */
	void on_inactivation() override;

	/**
	 * This function is called while the mode is active
	 */
	void on_active() override;

	bool can_activate() const;
	bool should_fallback_to_rtl() const { return _failed || _complete; }

private:
	static constexpr size_t MAX_PATH_POINTS{1024};
	static constexpr hrt_abstime LOCAL_POSITION_TIMEOUT_US{500000};
	static constexpr hrt_abstime PARAMETER_UPDATE_INTERVAL{1000000};

	bool parameters_update();
	bool parameters_valid() const;

	void clear_path();
	void invalidate_path();
	void append_position();
	void set_current_target();
	void handle_failure(const char *reason);

	bool local_position_valid() const;
	bool position_reference_changed() const;
	matrix::Vector3f current_position() const;
	const matrix::Vector3f &path_point(size_t index) const;


	// Bread crumb path
	matrix::Vector3f _path[MAX_PATH_POINTS] {};
	size_t _path_start{0};
	size_t _path_count{0};
	int32_t _target_index{-1};

	hrt_abstime _last_record_time{0};
	hrt_abstime _target_start_time{0};
	uint64_t _position_reference_timestamp{0};
	uint8_t _xy_reset_counter{0};
	uint8_t _z_reset_counter{0};

	bool _path_valid{false};
	bool _reference_initialized{false};
	bool _was_armed{false};
	bool _failed{false};
	bool _complete{false};

	DEFINE_PARAMETERS(
		(ParamInt<px4::params::RTL_RTR_EN>) _param_rtl_rtr_en,
		(ParamFloat<px4::params::RTL_RTR_DIST>) _param_rtl_rtr_dist,
		(ParamFloat<px4::params::RTL_RTR_RATE>) _param_rtl_rtr_rate,
		(ParamFloat<px4::params::RTL_RTR_XY_ACC>) _param_rtl_rtr_xy_acc,
		(ParamFloat<px4::params::RTL_RTR_Z_ACC>) _param_rtl_rtr_z_acc,
		(ParamFloat<px4::params::RTL_RTR_TOUT>) _param_rtl_rtr_tout
	)

	uORB::SubscriptionInterval _parameter_update_sub{ORB_ID(parameter_update), 1_s};
};
