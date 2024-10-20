/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzFramework/Input/LibEVDevWrapper.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>

namespace AzFramework
{
    LibEVDevWrapper* LibEVDevWrapper::instance = nullptr;

    LibEVDevWrapper::LibEVDevWrapper()
    {
        m_libevdevHandle = AZ::DynamicModuleHandle::Create("libevdev.so");
        if (!m_libevdevHandle->Load())
        {
            AZ_Info("Input", "libevdev.so not found - gamepad support disabled.  Install libevdev2 to enable.\n");
            m_libevdevHandle.reset();
            return;
        }

        m_libevdev_free = m_libevdevHandle->GetFunction<functionType_libevdev_free>("libevdev_free");
        m_libevdev_new_from_fd = m_libevdevHandle->GetFunction<functionType_libevdev_new_from_fd>("libevdev_new_from_fd");
        m_libevdev_has_event_code = m_libevdevHandle->GetFunction<functionType_libevdev_has_event_code>("libevdev_has_event_code");
        m_libevdev_get_name = m_libevdevHandle->GetFunction<functionType_libevdev_get_name>("libevdev_get_name");
        m_libevdev_event_code_get_name = m_libevdevHandle->GetFunction<functionType_libevdev_event_code_get_name>("libevdev_event_code_get_name");
        m_libevdev_get_abs_info = m_libevdevHandle->GetFunction<functionType_libevdev_get_abs_info>("libevdev_get_abs_info");
        m_libevdev_next_event = m_libevdevHandle->GetFunction<functionType_libevdev_next_event>("libevdev_next_event");
        m_libevdev_has_event_type = m_libevdevHandle->GetFunction<functionType_libevdev_has_event_type>("libevdev_has_event_type");
        m_libevdev_get_id_bustype = m_libevdevHandle->GetFunction<functionType_libevdev_get_id_bustype>("libevdev_get_id_bustype");
        m_libevdev_get_id_vendor = m_libevdevHandle->GetFunction<functionType_libevdev_get_id_vendor>("libevdev_get_id_vendor");
        m_libevdev_get_id_product = m_libevdevHandle->GetFunction<functionType_libevdev_get_id_product>("libevdev_get_id_product");

        if ((m_libevdev_free) &&
            (m_libevdev_new_from_fd) &&
            (m_libevdev_has_event_code) &&
            (m_libevdev_get_name) &&
            (m_libevdev_event_code_get_name) &&
            (m_libevdev_get_abs_info) &&
            (m_libevdev_next_event) &&
            (m_libevdev_has_event_type) &&
            (m_libevdev_get_id_bustype) &&
            (m_libevdev_get_id_vendor) &&
            (m_libevdev_get_id_product))
        {
            AZ_Trace("Input", "libevdev.so loaded and all symbols found.\n")
        }
        else
        {
            // this is not a normal circumstance, since libevdev.so should not be missing symbols
            // and its API is stable...
            AZ_Warning("Input", false, "libevdev.so loaded but missing symbols.\n");
            m_libevdevHandle.reset();
        }
    }

    LibEVDevWrapper::~LibEVDevWrapper()
    {
        if (m_libevdevHandle)
        {
            m_libevdevHandle.reset();
        }
    }

    LibEVDevWrapper::DeviceType LibEVDevWrapper::GetDeviceType(struct libevdev* dev) const
    {
        if (IsGamepadDevice(dev))
        {
            return DeviceType::Gamepad;
        }
        else if (IsJoystickDevice(dev))
        {
            return DeviceType::Joystick;
        }
        else
        {
            return DeviceType::Unknown;
        }
    }

    bool LibEVDevWrapper::IsGamepadDevice(struct libevdev* dev) const
    {
        if (m_libevdev_has_event_type(dev, EV_KEY) &&
            m_libevdev_has_event_type(dev, EV_ABS) &&
            m_libevdev_has_event_code(dev, EV_KEY, BTN_GAMEPAD))
        {
            return true;
        }

        return false;
    }

    bool LibEVDevWrapper::IsJoystickDevice(struct libevdev* dev) const
    {
        if (m_libevdev_has_event_type(dev, EV_KEY) &&
            m_libevdev_has_event_type(dev, EV_ABS) &&
            m_libevdev_has_event_code(dev, EV_KEY, BTN_JOYSTICK))
        {
            return true;
        }

        return false;
    }
} // end namespace AzFramework
