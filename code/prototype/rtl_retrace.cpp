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
 * @file rtl_retrace.cpp
 *
 * Retrace flight path
 *
 * @author Nicholas Martinez <nicholasammartinez@gmail.com>
 */

#include "rtl_retrace.h"

#include "navigator.h"

#include <lib/geo/geo.h>
#include <px4_platform_common/log.h>

#include <cmath>

RtlRetrace::RtlRetrace(Navigator *navigator) :
	NavigatorMode(navigator, vehicle_status_s::NAVIGATION_STATE_AUTO_RTL),
	ModuleParams(navigator)
{
}

bool RtlRetrace::parameters_update()
{
	if (_parameter_update_sub.updated()) {
		parameter_update_s param_update;
		_parameter_update_sub.copy(&param_update);
		updateParams();
		return true;
	}

	return false;
}

bool RtlRetrace::parameters_valid() const
{
	return PX4_ISFINITE(_param_rtl_rtr_dist.get()) && _param_rtl_rtr_dist.get() > 0.f
	       && PX4_ISFINITE(_param_rtl_rtr_rate.get()) && _param_rtl_rtr_rate.get() > 0.f
	       && PX4_ISFINITE(_param_rtl_rtr_xy_acc.get()) && _param_rtl_rtr_xy_acc.get() > 0.f
	       && PX4_ISFINITE(_param_rtl_rtr_z_acc.get()) && _param_rtl_rtr_z_acc.get() > 0.f
	       && PX4_ISFINITE(_param_rtl_rtr_tout.get()) && _param_rtl_rtr_tout.get() > 0.f;
}

void RtlRetrace::clear_path()
{
	_path_start = 0;
	_path_count = 0;
	_target_index = -1;
	_last_record_time = 0;
	_target_start_time = 0;
	_path_valid = true;
	_reference_initialized = false;
	_failed = false;
	_complete = false;
}

void RtlRetrace::invalidate_path()
{
	if (_path_valid) {
		_path_valid = false;
		PX4_INFO("RtlRetrace path invalidated");
	}
}

matrix::Vector3f RtlRetrace::current_position() const
{
	const auto &position = *_navigator->get_local_position();
	return matrix::Vector3f(position.x, position.y, position.z);
}

const matrix::Vector3f &RtlRetrace::path_point(size_t index) const
{
	return _path[(_path_start + index) % MAX_PATH_POINTS];
}

void RtlRetrace::append_position()
{
	if (_path_count < MAX_PATH_POINTS) {
		_path[(_path_start + _path_count) % MAX_PATH_POINTS] = current_position();
		++_path_count;

	} else {
		_path[_path_start] = current_position();
		_path_start = (_path_start + 1) % MAX_PATH_POINTS;
	}

	_last_record_time = hrt_absolute_time();
}

bool RtlRetrace::local_position_valid() const
{
	const auto &position = *_navigator->get_local_position();
	const hrt_abstime now = hrt_absolute_time();

	return position.xy_valid && position.z_valid
	       && position.xy_global && position.z_global
	       && PX4_ISFINITE(position.x) && PX4_ISFINITE(position.y) && PX4_ISFINITE(position.z)
	       && PX4_ISFINITE(position.ref_lat) && PX4_ISFINITE(position.ref_lon)
	       && PX4_ISFINITE(position.ref_alt)
	       && position.timestamp != 0 && position.timestamp <= now
	       && now - position.timestamp <= LOCAL_POSITION_TIMEOUT_US;
}

bool RtlRetrace::position_reference_changed() const
{
	const auto &position = *_navigator->get_local_position();

	return position.xy_reset_counter != _xy_reset_counter
	       || position.z_reset_counter != _z_reset_counter
	       || position.ref_timestamp != _position_reference_timestamp;
}

bool RtlRetrace::can_activate() const
{
	const auto &status = *_navigator->get_vstatus();

	return _param_rtl_rtr_en.get() != 0 && parameters_valid()
	       && status.arming_state == vehicle_status_s::ARMING_STATE_ARMED
	       && status.vehicle_type == vehicle_status_s::VEHICLE_TYPE_ROTARY_WING
	       && !status.is_vtol
	       && _path_valid && _reference_initialized && _path_count >= 2
	       && !_failed && !_complete
	       && local_position_valid() && !position_reference_changed();
}

void RtlRetrace::on_inactive()
{
	parameters_update();

	const auto &status = *_navigator->get_vstatus();

	if (status.arming_state != vehicle_status_s::ARMING_STATE_ARMED) {
		_was_armed = false;
		return;
	}

	if (!_was_armed) {
		clear_path();
		_was_armed = true;
	}

	if (!_path_valid) {
		return;
	}

	// Regular RTL has started, so this recorded route cannot be reused.
	if (status.nav_state == vehicle_status_s::NAVIGATION_STATE_AUTO_RTL) {
		invalidate_path();
		return;
	}

	if (_param_rtl_rtr_en.get() == 0 || !parameters_valid()) {
		return;
	}

	if (!local_position_valid()) {
		if (_path_count > 0) {
			invalidate_path();
		}

		return;
	}

	if (_reference_initialized && position_reference_changed()) {
		invalidate_path();
		return;
	}

	if (!_reference_initialized) {
		const auto &position = *_navigator->get_local_position();
		_xy_reset_counter = position.xy_reset_counter;
		_z_reset_counter = position.z_reset_counter;
		_position_reference_timestamp = position.ref_timestamp;
		_reference_initialized = true;
		append_position();
		return;
	}

	const hrt_abstime record_interval_us =
		static_cast<hrt_abstime>(1000000.f / _param_rtl_rtr_rate.get());

	if (hrt_elapsed_time(&_last_record_time) >= record_interval_us
	    && (current_position() - path_point(_path_count - 1)).norm() >= _param_rtl_rtr_dist.get()) {
		append_position();
	}
}

void RtlRetrace::on_activation()
{
	parameters_update();

	_failed = false;
	_complete = false;

	if (!can_activate()) {
		handle_failure("route, parameters, or position is not usable");
		return;
	}

	// Capture the current position as the start of the return traversal.
	append_position();
	_target_index = static_cast<int32_t>(_path_count) - 1;
	_target_start_time = hrt_absolute_time();
	set_current_target();
}

void RtlRetrace::set_current_target()
{
	const auto &position = *_navigator->get_local_position();
	const matrix::Vector3f &target = path_point(static_cast<size_t>(_target_index));
	MapProjection projection(position.ref_lat, position.ref_lon, position.ref_timestamp);

	_navigator->reset_triplets();

	auto &current = _navigator->get_position_setpoint_triplet()->current;
	projection.reproject(target(0), target(1), current.lat, current.lon);
	current.alt = position.ref_alt - target(2);
	current.type = position_setpoint_s::SETPOINT_TYPE_POSITION;
	current.yaw = NAN;
	current.acceptance_radius = _param_rtl_rtr_xy_acc.get();
	current.timestamp = hrt_absolute_time();
	current.valid = true;

	_navigator->set_position_setpoint_triplet_updated();
}

void RtlRetrace::on_active()
{
	const bool params_changed = parameters_update();

	if (_failed || _complete) {
		return;
	}

	if (_param_rtl_rtr_en.get() == 0 || !parameters_valid()) {
		handle_failure("retrace disabled or parameters invalid");
		return;
	}

	if (params_changed && _target_index >= 0) {
		set_current_target();
	}

	if (!local_position_valid()) {
		handle_failure("local position is invalid or stale");
		return;
	}

	if (position_reference_changed()) {
		handle_failure("local coordinate frame changed");
		return;
	}

	if (_target_index < 0 || static_cast<size_t>(_target_index) >= _path_count) {
		handle_failure("invalid target index");
		return;
	}

	const matrix::Vector3f error =
		current_position() - path_point(static_cast<size_t>(_target_index));
	const float horizontal_error = matrix::Vector2f(error(0), error(1)).norm();
	const float vertical_error = fabsf(error(2));
	const hrt_abstime target_timeout_us =
		static_cast<hrt_abstime>(_param_rtl_rtr_tout.get() * 1000000.f);

	if (horizontal_error <= _param_rtl_rtr_xy_acc.get()
	    && vertical_error <= _param_rtl_rtr_z_acc.get()) {
		if (_target_index > 0) {
			--_target_index;
			_target_start_time = hrt_absolute_time();
			set_current_target();

		} else {
			_complete = true;
			_target_start_time = 0;
			PX4_INFO("RtlRetrace reached the oldest route point");
		}

	} else if (hrt_elapsed_time(&_target_start_time) >= target_timeout_us) {
		handle_failure("target timed out");
	}
}

void RtlRetrace::handle_failure(const char *reason)
{
	if (!_failed) {
		_failed = true;
		_complete = false;
		_path_valid = false;
		PX4_WARN("RtlRetrace failed (%s). use regular RTL", reason);
	}
}

void RtlRetrace::on_inactivation()
{
	_target_index = -1;
	_target_start_time = 0;
	_complete = false;
	invalidate_path();

	if (_navigator->get_vstatus()->arming_state != vehicle_status_s::ARMING_STATE_ARMED) {
		_was_armed = false;
	}
}
