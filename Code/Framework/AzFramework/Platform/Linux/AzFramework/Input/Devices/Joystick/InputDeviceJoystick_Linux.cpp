/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include <linux/input.h>
#include <linux/input-event-codes.h>

#include <AzCore/IO/SystemFile.h>
#include <AzFramework/Input/LibEVDevWrapper.h>
#include <AzFramework/Input/LibUDevWrapper.h>
#include <AzFramework/Input/Utils/AdjustAnalogInputForDeadZone.h>
#include <AzFramework/Input/Devices/Joystick/InputDeviceJoystick.h>
#include <AzFramework/Input/Devices/Joystick/InputDeviceJoystick_Linux.h>

#include <AzCore/Debug/Trace.h>

#include <utility>

#include <utility>


namespace JoystickLinuxPrivate
{
    class InternalState
    {
        public:
            int m_fileDescriptor = -1;
            libevdev *m_evdevDevice = nullptr;

            bool OpenDevice(const AZStd::string& devicePath)
            {
                m_fileDescriptor = open(devicePath.c_str(), O_RDONLY|O_NONBLOCK);
                if (m_fileDescriptor  == -1)
                {
                    return true;
                }

                if (AzFramework::LibEVDevWrapper::GetInstance()->m_libevdev_new_from_fd(m_fileDescriptor, &m_evdevDevice) != 0)
                {
                    CloseDevice();
                    return true;
                }

                // Check if the device is a gamepad else ignore.
                if(!AzFramework::LibEVDevWrapper::GetInstance()->IsJoystickDevice(m_evdevDevice))
                {
                    CloseDevice();
                    return true;
                }

                return false;
            }

            void CloseDevice()
            {
                if (m_evdevDevice)
                {
                    AzFramework::LibEVDevWrapper::GetInstance()->m_libevdev_free(m_evdevDevice);
                    m_evdevDevice = nullptr;
                }
                if (m_fileDescriptor != -1)
                {
                    close(m_fileDescriptor);
                    m_fileDescriptor = -1;
                }
            }
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    using namespace JoystickLinuxPrivate;

    InputDeviceJoystick::Implementation* InputDeviceJoystickLinux::Create(InputDeviceJoystick& inputDevice)
    {
        return aznew InputDeviceJoystickLinux(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceJoystickLinux::InputDeviceJoystickLinux(InputDeviceJoystick& inputDevice)
        : InputDeviceJoystick::Implementation(inputDevice)
        , m_isConnected(false)
    {
        m_internalState.reset(new JoystickLinuxPrivate::InternalState());

        const auto Index = GetInputDeviceIndex();
        const auto path = UDevSingletonClass::getInstance()->GetJoystickFilePath(Index);
        if(!path.empty())
        {
            ConnectDevice(path);
        }

        LinuxJoystickNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceJoystickLinux::~InputDeviceJoystickLinux()
    {
        LinuxJoystickNotificationBus::Handler::BusDisconnect();
        m_internalState.reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceJoystickLinux::IsConnected() const
    {
        return m_isConnected;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceJoystickLinux::SetVibration([[maybe_unused]]float leftMotorSpeedNormalized,
                                               [[maybe_unused]]float rightMotorSpeedNormalized)
    {
        if (m_isConnected)
        {
            // (TODO) This is for future implementors.  Better to have gamepad support without force feedback.
            // than no gamepad support at all.  evdev does have the capability of doing force feedback.
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceJoystickLinux::GetPhysicalKeyOrButtonText(const InputChannelId& /*inputChannelId*/,
                                                               AZStd::string& /*o_keyOrButtonText*/) const
    {
        return false;
    }

    bool InputDeviceJoystickLinux::ConnectDevice(const AZStd::string& path)
    {
        if(m_internalState->OpenDevice(path))
        {
            return true;
        }

        AZ_Info("Input", "Detected Joystick at index %u, with name: \"%s\"\n", GetInputDeviceIndex(),
                LibEVDevWrapper::GetInstance()->m_libevdev_get_name(m_internalState->m_evdevDevice));

        // Iterate over all supported buttons.
        AZ::u32 mapIndex = 0;
        for (AZ::u32 i = BTN_JOYSTICK; i < KEY_MAX; ++i)
        {
            if (LibEVDevWrapper::GetInstance()->m_libevdev_has_event_code(m_internalState->m_evdevDevice, EV_KEY, i))
            {
                if(mapIndex < AzFramework::InputDeviceJoystick::Button::All.size())
                {
                    m_rawJoystickState.m_digitalButtonIdsByBitMask[i] = &AzFramework::InputDeviceJoystick::Button::All[mapIndex++];
                }
                else
                {
                    AZ_Warning("Input", false, "Too many buttons detected on joystick %u", GetInputDeviceIndex());
                }
            }
        }

        // Iterate over all supported axes.
        mapIndex = 0;
        for (int i = 0; i < ABS_MAX; ++i)
        {
            if (LibEVDevWrapper::GetInstance()->m_libevdev_has_event_code(m_internalState->m_evdevDevice, EV_ABS, i))
            {
                if (const auto absInfo = LibEVDevWrapper::GetInstance()->m_libevdev_get_abs_info(m_internalState->m_evdevDevice, i))
                {
                    if(mapIndex < AzFramework::InputDeviceJoystick::Axis1D::All.size())
                    {
                        // Register platform axis index.
                        m_rawJoystickState.m_analogMask[i] = &AzFramework::InputDeviceJoystick::Axis1D::All[mapIndex++];

                        // Register axis info.
                        m_rawJoystickState.m_axisInfo[i] =
                        {
                            static_cast<float>(absInfo->minimum),
                            static_cast<float>(absInfo->maximum - absInfo->minimum),
                            0.0f
                        };
                    }
                }
            }
        }

        m_isConnected = true;
        ResetInputChannelStates();
        BroadcastInputDeviceConnectedEvent();
        return false;
    }

    void InputDeviceJoystickLinux::DisconnectDevice()
    {
        m_isConnected = false;
        m_rawJoystickState.Reset();
        m_internalState->CloseDevice();
        ResetInputChannelStates();
        BroadcastInputDeviceDisconnectedEvent();
    }

    void InputDeviceJoystickLinux::OnConnected(const AZStd::string& path)
    {
        const auto index = UDevSingletonClass::getInstance()->GetJoystickFileIndex(path);
        if(GetInputDeviceIndex() != index)
        {
            return;
        }

        ConnectDevice(path);

        AZ_Info("Input", "Joystick %s connected", path.c_str());
    }

    void InputDeviceJoystickLinux::OnDisconnected(const AZStd::string& path)
    {
        const auto index = UDevSingletonClass::getInstance()->GetJoystickFileIndex(path);
        if(GetInputDeviceIndex() != index)
        {
            return;
        }

        DisconnectDevice();

        AZ_Info("Input", "Joystick %s disconnected", path.c_str());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceJoystickLinux::TickInputDevice()
    {
        if (!m_isConnected)
        {
            return;
        }

        input_event ev {};

        int libevdevResult = 0;
        auto& buttonChannelsByIdMap = m_inputDevice.GetButtonChannelsByIdMap();
        auto& axisChannelsByIdMap = m_inputDevice.GetAxis1DChannelsByIdMap();

        const auto nextEventFn = AzFramework::LibEVDevWrapper::GetInstance()->m_libevdev_next_event;
        while ((libevdevResult = nextEventFn(m_internalState->m_evdevDevice, O3DEWRAPPER_LIBEVDEV_READ_FLAG_NORMAL, &ev)) == O3DEWRAPPER_LIBEVDEV_READ_STATUS_SUCCESS)
        {
            switch (ev.type)
            {
                case EV_KEY:
                {
                    auto buttonChannelId = m_rawJoystickState.m_digitalButtonIdsByBitMask.find(ev.code);
                    if(buttonChannelId != m_rawJoystickState.m_digitalButtonIdsByBitMask.end())
                    {
                        buttonChannelsByIdMap[*buttonChannelId->second]->ProcessRawInputEvent(ev.value != 0);
                    }
                }break;
                case EV_ABS:
                {
                    auto axisChannelId = m_rawJoystickState.m_analogMask.find(ev.code);
                    if(axisChannelId != m_rawJoystickState.m_analogMask.end())
                    {
                        auto& axisInfo = m_rawJoystickState.m_axisInfo[ev.code];
                        const float invScaleFactor = AZ::IsClose(axisInfo.m_scaleFactor, 0.0f) ? 2.0f : 2.0f / axisInfo.m_scaleFactor;
                        const float axisValue = (static_cast<float>(ev.value) - axisInfo.m_minValue) * invScaleFactor - 1.0f;
                        const float value = AdjustForDeadZoneAndNormalizeAnalogInput(axisValue, axisInfo.m_deadZoneEpsilon, 1.0f );

                        axisChannelsByIdMap[*axisChannelId->second]->ProcessRawInputEvent(value);
                    }

                }break;
                default: // We ignore other events.
                    break;
            }
        }

        if (libevdevResult == -ENODEV)
        {
            DisconnectDevice();
        }
    }

} // namespace AzFramework
