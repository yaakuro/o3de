/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Trace.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/Input/Devices/Joystick/InputDeviceJoystick.h>
#include <AzFramework/Input/LibUDevWrapper.h>

namespace JoystickLinuxPrivate
{
     class InternalState;
}

namespace AzFramework
{
    class InputDeviceJoystickLinux
        : public InputDeviceJoystick::Implementation
        , public LinuxJoystickNotificationBus::Handler
    {
    public:
        friend struct LinuxDeviceJoystickImplFactory;

        AZ_CLASS_ALLOCATOR(InputDeviceJoystickLinux, AZ::SystemAllocator);

        explicit InputDeviceJoystickLinux(InputDeviceJoystick& inputDevice);
        ~InputDeviceJoystickLinux() override;

        static InputDeviceJoystick::Implementation* Create(InputDeviceJoystick& inputDevice);

    private:

        bool IsConnected() const override;
        void SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized) override;
        bool GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                        AZStd::string& o_keyOrButtonText) const override;
        void TickInputDevice() override;
        bool ConnectDevice(const AZStd::string& path);
        void DisconnectDevice();
        void OnConnected(const AZStd::string& path) override;
        void OnDisconnected(const AZStd::string& path) override;

        RawJoystickState m_rawJoystickState;
        bool m_isConnected{};
        AZStd::unique_ptr<JoystickLinuxPrivate::InternalState> m_internalState;
    };

}
