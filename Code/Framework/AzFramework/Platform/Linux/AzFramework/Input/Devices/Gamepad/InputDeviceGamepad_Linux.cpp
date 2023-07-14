/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>

#include <linux/input.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <dirent.h>
#include <cstdio>
#include <utility>
#include <libudev.h>

namespace AZStd
{
    bool getline(std::istream& input, AZStd::string& line, char delim = '\n')
    {
        line.clear();

        char c;
        while (input.get(c))
        {
            if (c == delim)
            {
                return true;
            }
            else
            {
                line.push_back(c);
            }
        }

        return !line.empty();
    }
}

namespace
{
    enum class GamepadDeviceResults
    {
        Success,
        Failed
    };

    enum class UDevResults
    {
        Success,
        Failed
    };

    /**
     * \brief Handles Linux Gamepad (Joystick) devices.
     */
    class GamepadDevice
    {
    public:

        struct Info
        {
            AZStd::string deviceName;
            AZStd::string jsName;
            AZStd::string eventName;
        };

        GamepadDevice(size_t index, AZStd::string jsNodeName, AZStd::string eventNodeName)
            : m_jsNodeName(AZStd::move(jsNodeName))
            , m_eventNodeName(AZStd::move(eventNodeName))
            , index(index)
        {

        }

        explicit GamepadDevice(size_t index)
            : index(index)
        {

        }

        ~GamepadDevice()
        {
            if(IsTracking())
            {
                StopTracking();
            }
        }

        [[nodiscard]] const AZStd::string& GetName() const { return m_name; }
        [[nodiscard]] const AZStd::string& GetJsNodeName() const { return m_jsNodeName; }
        [[nodiscard]] const AZStd::string& GetEventNodeName() const { return m_eventNodeName; }
        void SetName(const AZStd::string& NewName) { m_name = NewName;}
        void SetJsNodeName(const AZStd::string& NewJs) { m_jsNodeName = NewJs;}
        void SetEventNodeName(const AZStd::string& NewEvent) { m_eventNodeName = NewEvent; }
        [[nodiscard]] size_t GetIndex() const { return index; }
        [[nodiscard]] size_t GetNumberOfButton() const{ return m_numberOfButtons; }
        [[nodiscard]] size_t GetNumberOfAxis() const { return m_numberOfAxis; }
        [[nodiscard]] bool IsTracking() const { return m_fd != -1; }

        // Initialize the system. This mainly will get once all currently connected gamepad devices.
        [[nodiscard]] GamepadDeviceResults Init()
        {
            return RetrieveDeviceInformation();
        }

        // Connect to the device and get also its information.
        [[nodiscard]] GamepadDeviceResults StartTracking()
        {
            // Open the device.in non-blocking mode.
            m_fd = open(m_jsNodeName.c_str(), O_RDONLY | O_NONBLOCK);
            if (m_fd <= 0)
            {
                AZ_Warning("GamepadLinux", false, "Failed to open device: %s: %s", m_jsNodeName.c_str(), strerror(errno));
                return GamepadDeviceResults::Failed;
            }

            return GamepadDeviceResults::Success;
        }

        void StopTracking()
        {
            if(m_fd != -1)
            {
                close(m_fd);
                m_fd = -1;
            }
        }

        GamepadDeviceResults RetrieveDeviceInformation()
        {
            // Open the device.in non-blocking mode.
            m_fd = open(m_jsNodeName.c_str(), O_RDONLY);
            if (m_fd <= 0)
            {
                AZ_Warning("GamepadLinux", false, "Failed to open device: %s: %s", m_jsNodeName.c_str(), strerror(errno));
                return GamepadDeviceResults::Failed;
            }

            // Retrieve device information.
            char TmpName[NAME_MAX] = { 0 };
            if ((ioctl(m_fd, JSIOCGNAME(NAME_MAX), TmpName) < 0) ||
                (ioctl(m_fd, JSIOCGAXES, &m_numberOfAxis) < 0) ||
                (ioctl(m_fd, JSIOCGBUTTONS, &m_numberOfButtons)) < 0)
            {
                close(m_fd);
                m_fd = -1;
                AZ_Warning("GamepadLinux", false, "Failed to acquire gamepad information.");
                return GamepadDeviceResults::Failed;
            }

            m_name = TmpName;
            close(m_fd);
            m_fd = -1;

            return GamepadDeviceResults::Success;
        }

        [[nodiscard]] bool CheckIsNode(const AZStd::string& nodeName) const
        {
            return m_jsNodeName == nodeName;
        }

        bool GetNextEvent(struct js_event& event) const
        {
            ssize_t size = read(m_fd, &event, sizeof(struct js_event));
            if (-1 == size)
            {
                return false;
            }

            return true;
        }

    private:

        AZStd::string m_name;
        AZStd::string m_jsNodeName;
        AZStd::string m_eventNodeName;
        size_t m_numberOfAxis = 0;
        size_t m_numberOfButtons = 0;
        int m_fd = -1;
        size_t index = 0;
    };

    /**
     * \brief Simple delegate class to handle joystick connect/disconnect events.
     */
     template<typename ParamType>
     class UDevDelegateOneParam
     {
     public:
         template<typename T, typename Func>
         void Connect(T* instance, Func function)
         {
             delegates.emplace_back([instance, function](ParamType Device) { (instance->*function)(Device); });
         }

        void Broadcast(ParamType Device)
        {
            for (const auto& delegate : delegates)
            {
                delegate(Device);
            }
        }

    private:
        AZStd::vector<AZStd::function<void(ParamType)>> delegates;
    };

    /**
     * \brief Helper singleton class to handle udev events.
     */
    class UDevSingletonClass {
    private:
        static UDevSingletonClass* instance;

        UDevSingletonClass() {}
        ~UDevSingletonClass()
        {
            Shutdown();
        }
    public:
        static const int MaxDevices = 4;

        static UDevSingletonClass* getInstance()
        {
            if (!instance)
            {
                instance = new UDevSingletonClass();
            }
            return instance;
        }

        [[nodiscard]] static int GetMaxDevicesSupported()
        {
            return MaxDevices;
        }

        [[nodiscard]] UDevResults Init()
        {
            if(m_Initialized)
            {
                return UDevResults::Success;
            }

            m_Udev = udev_new();
            if (!m_Udev)
            {
                AZ_Warning("GamepadLinux", false, "Failed to create udev instance.");
                return UDevResults::Failed;
            }

            // Allocate monitor and set filter for joystick events
            m_Monitor = udev_monitor_new_from_netlink(m_Udev, "udev");
            if(!m_Monitor)
            {
                AZ_Warning("GamepadLinux", false, "Failed to create udev monitor.");
                return UDevResults::Failed;
            }

            // Set filter for input devices.
            udev_monitor_filter_add_match_subsystem_devtype(m_Monitor, "input", nullptr);

            // Enable receiving events.
            if(udev_monitor_enable_receiving(m_Monitor) < 0)
            {
                AZ_Warning("GamepadLinux", false, "Failed to enable receiving events.");
                return UDevResults::Failed;
            }

            m_Initialized = true;

            // Acquire all connected joysticks to the system.
            FindAllJoystickDevices();

            return UDevResults::Success;
        }

        void Shutdown()
        {
            if(m_Monitor)
            {
                udev_monitor_unref(m_Monitor);
                m_Monitor = nullptr;
            }
            if(m_Udev)
            {
                udev_unref(m_Udev);
                m_Udev = nullptr;
            }

            m_Initialized = false;
            for(auto& ControllerDevice : m_AllControllerDevices)
            {
                azdestroy(ControllerDevice);
            }

            m_AllControllerDevices.clear();
            m_AllJoystickDevices.clear();
        }

        [[nodiscard]] GamepadDevice* FindControllerDeviceByNodeName(const AZStd::string& nodeName) const
        {
            for (int i = 0; i < GetMaxDevicesSupported(); ++i)
            {
                if (m_AllControllerDevices[i]->CheckIsNode(nodeName))
                {
                    return m_AllControllerDevices[i];
                }
            }
            return nullptr;
        }

        [[nodiscard]] GamepadDevice* FindControllerDeviceByNonRunningState() const
        {
            for (int i = 0; i < GetMaxDevicesSupported(); ++i)
            {
               if(!m_AllControllerDevices[i]->IsTracking())
               {
                   return m_AllControllerDevices[i];
               }
            }
            return nullptr;
        }

        [[nodiscard]] GamepadDevice* FindControllerFreeDevice() const
        {
            for (int i = 0; i < GetMaxDevicesSupported(); ++i)
            {
                if (!m_AllControllerDevices[i]->IsTracking())
                {
                    return m_AllControllerDevices[i];
                }
            }
            return nullptr;
        }

        void Tick()
        {
            // Do not tick if we could not get a monitor.
            if(!m_Initialized)
            {
                return;
            }

            struct udev_device* device = udev_monitor_receive_device(m_Monitor);
            if (device)
            {
                if (strcmp(udev_device_get_subsystem(device), "input") == 0)
                {
                    if (udev_device_get_action(device))
                    {
                        const char* nodeNamePtr = udev_device_get_devnode(device);

                        // Make sure we are only checking for joystick events.
                        const char* devName = udev_device_get_property_value(device, "DEVNAME");
                        if (devName && strstr(devName, "js") != nullptr)
                        {
                            AZStd::string nodeName(nodeNamePtr);

                            if (strcmp(udev_device_get_action(device), "add") == 0)
                            {
                                if(GamepadDevice* FoundDevice = FindControllerDeviceByNodeName(nodeName))
                                {
                                    if(FoundDevice->RetrieveDeviceInformation() == GamepadDeviceResults::Success)
                                    {
                                        OnConnectedToSystemDelegate.Broadcast(*FoundDevice);
                                    }
                                    else
                                    {
                                        AZ_Warning("GamepadLinux", false, "Failed to retrieve device information for %s",
                                                   FoundDevice->GetName().c_str());
                                    }
                                }
                                else if(GamepadDevice* NextNoneRunningDevice = FindControllerDeviceByNonRunningState())
                                {
                                    // The name of the device will be populated in the Start method of the controller
                                    NextNoneRunningDevice->SetJsNodeName(devName);

                                    OnConnectedToSystemDelegate.Broadcast(*NextNoneRunningDevice);
                                }
                            }
                            else if (strcmp(udev_device_get_action(device), "remove") == 0)
                            {
                                if(const GamepadDevice* FoundDevice = FindControllerDeviceByNodeName(nodeName))
                                {
                                    OnDisconnectedFromSystemDelegate.Broadcast(*FoundDevice);
                                }
                            }
                        }
                    }
                }

                udev_device_unref(device);
            }
        }

        /* We could use udev to get gamepad devices, but we need also event assignments later
         * for force feedback handling. Meaning, we will use /proc/bus/input/devices to get the
         * event assignments.
         */
        void FindAllJoystickDevices()
        {
            std::ifstream inputFile("/proc/bus/input/devices");
            AZStd::string line;

            GamepadDevice::Info currentDevice;
            bool isGamepadBlock = false;

            m_AllJoystickDevices.clear();

            while (AZStd::getline(inputFile, line))
            {
                if (line.empty())
                {
                    // Check if the current block is not empty and has a joystick device assigned
                    if (!currentDevice.deviceName.empty() && isGamepadBlock)
                    {
                        m_AllJoystickDevices.push_back(currentDevice);
                    }
                    currentDevice = GamepadDevice::Info();
                    isGamepadBlock = false;
                }
                else
                {
                    // Check if the line contains "Handlers" and "js"
                    if (line.find("Handlers=") != AZStd::string::npos && line.find("js") != AZStd::string::npos)
                    {
                        isGamepadBlock = true;

                        // Extract the "js" part from the line.
                        size_t startPos = line.find("js");
                        size_t endPos = line.find_first_of(" \t", startPos);
                        AZStd::string jsName = line.substr(startPos, endPos - startPos);
                        currentDevice.jsName = "/dev/input/" + jsName;

                        // Extract the "event" part from the line.
                        startPos = line.find("event");
                        endPos = line.find_first_of(" \t", startPos);
                        AZStd::string eventName = line.substr(startPos, endPos - startPos);
                        currentDevice.eventName = "/dev/input/event/" + eventName;
                    }

                    // Extract the device name
                    if (line.find("N: Name=") != AZStd::string::npos)
                    {
                        size_t startPos = line.find_first_of('"') + 1;
                        size_t endPos = line.find_last_of('"');
                        currentDevice.deviceName = line.substr(startPos, endPos - startPos);
                    }
                }
            }

            // Check if there's a non-empty block with a joystick device at the end of the file
            if (!currentDevice.deviceName.empty() && isGamepadBlock)
            {
                m_AllJoystickDevices.push_back(currentDevice);
            }

            inputFile.close();
        }

        GamepadDevice* CreateController()
        {
            if(m_currentIndex >= GetMaxDevicesSupported())
            {
                return nullptr;
            }

            GamepadDevice* NewController;

            // We maybe collected already some connected joystick devices. Use the information to create a new device.
            if (m_currentIndex < m_AllJoystickDevices.size())
            {
                NewController = aznew GamepadDevice(m_currentIndex,
                                                    m_AllJoystickDevices[m_currentIndex].jsName,
                                                    m_AllJoystickDevices[m_currentIndex].eventName);

                // Initialize and start the controller which allows to track the gamepad.
                if( (NewController->Init() == GamepadDeviceResults::Success) &&
                    (NewController->StartTracking() == GamepadDeviceResults::Success))
                {
                    m_AllControllerDevices.push_back(NewController);
                    m_currentIndex++;
                }
                else
                {
                    azdestroy(NewController);
                    return nullptr;
                }
            }
            // Otherwise, create just an empty device. We will populate on demand.
            else
            {
                NewController = aznew GamepadDevice(m_currentIndex);

                m_AllControllerDevices.push_back(NewController);
                m_currentIndex++;
            }

            return NewController;
        }

    public:

        // Delegate used to inform if a Gamepad device gets connected to a computer.
        UDevDelegateOneParam<const GamepadDevice&> OnConnectedToSystemDelegate;

        // Delegate used to inform if a Gamepad device gets disconnected from a computer.
        UDevDelegateOneParam<const GamepadDevice&> OnDisconnectedFromSystemDelegate;

    private:

        struct udev* m_Udev = nullptr;
        struct udev_monitor* m_Monitor = nullptr;

        AZStd::vector<GamepadDevice::Info> m_AllJoystickDevices;
        AZStd::vector<GamepadDevice*> m_AllControllerDevices;

        bool m_Initialized = false;
        size_t m_currentIndex = 0;
    };

    UDevSingletonClass* UDevSingletonClass::instance = nullptr;

    AZStd::unordered_map<AZ::u32, const AzFramework::InputChannelId*> GetDigitalButtonIdByBitMaskMap()
    {
        AZStd::unordered_map<AZ::u32, const AzFramework::InputChannelId*> map =
        {
            { (1 << 10), &AzFramework::InputDeviceGamepad::Button::DU },
            { (1 << 11), &AzFramework::InputDeviceGamepad::Button::DD },
            { (1 << 12), &AzFramework::InputDeviceGamepad::Button::DL },
            { (1 << 13), &AzFramework::InputDeviceGamepad::Button::DR },

            { (1 <<  7), &AzFramework::InputDeviceGamepad::Button::Start },
            { (1 <<  6), &AzFramework::InputDeviceGamepad::Button::Select },

            { (1 <<  4), &AzFramework::InputDeviceGamepad::Button::L1 },
            { (1 <<  5), &AzFramework::InputDeviceGamepad::Button::R1 },
            { (1 <<  9), &AzFramework::InputDeviceGamepad::Button::L3 },
            { (1 << 10), &AzFramework::InputDeviceGamepad::Button::R3 },

            { (1 << 0), &AzFramework::InputDeviceGamepad::Button::A },
            { (1 << 1), &AzFramework::InputDeviceGamepad::Button::B },
            { (1 << 2), &AzFramework::InputDeviceGamepad::Button::X },
            { (1 << 3), &AzFramework::InputDeviceGamepad::Button::Y }
        };

        return map;
    }

    const float AnalogTriggerMaxValue = 65535.0f;
    const float AnalogTriggerDeadZone = 0.0f;
    const float ThumbStickMaxValue = 32767.0f;
    const float ThumbStickLeftDeadZone = 0.0f;
    const float ThumbStickRightDeadZone = 0.0f;
}


namespace AzFramework
{
    class InputDeviceGamepadLinux
        : public InputDeviceGamepad::Implementation
    {
    public:
        AZ_CLASS_ALLOCATOR(InputDeviceGamepadLinux, AZ::SystemAllocator);

        explicit InputDeviceGamepadLinux(InputDeviceGamepad& inputDevice, GamepadDevice* controllerDevice);
        ~InputDeviceGamepadLinux() override;

    private:

        [[nodiscard]] bool IsConnected() const override;
        void SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized) override;
        bool GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                        AZStd::string& o_keyOrButtonText) const override;

        void TickInputDevice() override;

    public:

        void OnGamepadConnected(const GamepadDevice& Device);
        void OnGamepadDisconnected(const GamepadDevice& Device);

    private:
        RawGamepadState m_rawGamepadState;
        GamepadDevice* m_Device = nullptr;
    };

    InputDeviceGamepad::Implementation* InputDeviceGamepad::Implementation::Create(InputDeviceGamepad& inputDevice)
    {
        if(UDevSingletonClass::getInstance()->Init() == UDevResults::Success)
        {
            if(GamepadDevice* ControllerDevice = UDevSingletonClass::getInstance()->CreateController())
            {
                auto* DeviceGamePad = aznew InputDeviceGamepadLinux(inputDevice, ControllerDevice);

                // Connect to delegates that will inform the InputDeviceGamepad about connected/disconnected events.
                UDevSingletonClass::getInstance()->OnConnectedToSystemDelegate.Connect(DeviceGamePad,
                                                                                       &InputDeviceGamepadLinux::OnGamepadConnected);
                UDevSingletonClass::getInstance()->OnDisconnectedFromSystemDelegate.Connect(DeviceGamePad,
                                                                                            &InputDeviceGamepadLinux::OnGamepadDisconnected);

                return DeviceGamePad;
            }
        }

        return nullptr;
    }

    AZ::u32 InputDeviceGamepad::GetMaxSupportedGamepads()
    {
        return UDevSingletonClass::GetMaxDevicesSupported();
    }

    InputDeviceGamepadLinux::InputDeviceGamepadLinux(InputDeviceGamepad& inputDevice,
                                                     GamepadDevice* controllerDevice)
        : Implementation(inputDevice)
        , m_Device(controllerDevice)
        , m_rawGamepadState(GetDigitalButtonIdByBitMaskMap())
    {
        m_rawGamepadState.m_triggerMaximumValue = AnalogTriggerMaxValue;
        m_rawGamepadState.m_triggerDeadZoneValue = AnalogTriggerDeadZone;
        m_rawGamepadState.m_thumbStickMaximumValue = ThumbStickMaxValue;
        m_rawGamepadState.m_thumbStickLeftDeadZone = ThumbStickLeftDeadZone;
        m_rawGamepadState.m_thumbStickRightDeadZone = ThumbStickRightDeadZone;
    }

    InputDeviceGamepadLinux::~InputDeviceGamepadLinux()
    {
        if(m_Device && m_Device->IsTracking())
        {
            m_Device->StopTracking();
        }
    }

    bool InputDeviceGamepadLinux::IsConnected() const
    {
        return m_Device && m_Device->IsTracking();
    }

    void InputDeviceGamepadLinux::SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized)
    {

    }

    bool InputDeviceGamepadLinux::GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                                            AZStd::string& o_keyOrButtonText) const
    {
        return false;
    }

    void InputDeviceGamepadLinux::OnGamepadConnected(const GamepadDevice& Device)
    {
        if(!m_Device)
        {
            return;
        }

        if(Device.GetIndex() != m_Device->GetIndex())
        {
            return;
        }

        if(m_Device->StartTracking() != GamepadDeviceResults::Success)
        {
            AZ_Warning("GamepadLinux", false, "Failed to start tracking device: %s: %s.\n", m_Device->GetName().c_str(),
                                                                                       m_Device->GetJsNodeName().c_str());
            return;
        }

        m_rawGamepadState.Reset();
        BroadcastInputDeviceConnectedEvent();

        AZ_Printf("GamepadLinux", "Successfully started tracking the newly connected device: %s: %s.\n", m_Device->GetName().c_str(),
                                                                                                    m_Device->GetJsNodeName().c_str());
    }

    void InputDeviceGamepadLinux::OnGamepadDisconnected(const GamepadDevice& Device)
    {
        if(!m_Device)
        {
            return;
        }

        if(Device.GetIndex() != m_Device->GetIndex())
        {
            return;
        }

        if(m_Device)
        {
            m_Device->StopTracking();
        }

        m_rawGamepadState.Reset();
        BroadcastInputDeviceDisconnectedEvent();

        AZ_Printf("GamepadLinux", "Gamepad got disconnected from the system: %s: %s.",   m_Device->GetName().c_str(),
                                                                                    m_Device->GetJsNodeName().c_str());
    }

    void InputDeviceGamepadLinux::TickInputDevice()
    {
        // This tick member function updates the connected/disconnected states for gamepad devices.
        UDevSingletonClass::getInstance()->Tick();

        if (!m_Device)
        {
            return;
        }

        if(!m_Device->IsTracking())
        {
            return;
        }

        struct js_event event = { 0 };
        while(m_Device->GetNextEvent(event))
        {
            switch (event.type)
            {
                case JS_EVENT_INIT:
                {
                    ResetInputChannelStates();
                    m_rawGamepadState.Reset();
                }break;
                case JS_EVENT_AXIS:
                {
                    switch (event.number)
                    {
                        case 0:
                            m_rawGamepadState.m_thumbStickLeftXState = static_cast<float>(event.value);
                            break;
                        case 1:
                            m_rawGamepadState.m_thumbStickLeftYState = static_cast<float>(event.value);
                            break;
                        case 3:
                            m_rawGamepadState.m_thumbStickRightXState = static_cast<float>(event.value);
                            break;
                        case 4:
                            m_rawGamepadState.m_thumbStickRightYState = static_cast<float>(event.value);
                            break;
                        case 2:
                        {
                            m_rawGamepadState.m_triggerButtonLState = static_cast<float>(event.value) + 32767.0f;
                        } break;
                        case 5:
                        {
                            m_rawGamepadState.m_triggerButtonRState = static_cast<float>(event.value) + 32767.0f;
                        } break;
                        // On Linux, D-Pad movements are axis movements. We need to convert them into button states.
                        case 6:
                        {
                            if (event.value <= -32767)
                            {
                                m_rawGamepadState.m_digitalButtonStates |= (1 << 12);
                            }
                            else if (event.value >= 32767)
                            {
                                m_rawGamepadState.m_digitalButtonStates |= (1 << 13);
                            }
                            else
                            {
                                m_rawGamepadState.m_digitalButtonStates &= ~(1 << 12);
                                m_rawGamepadState.m_digitalButtonStates &= ~(1 << 13);
                            }
                        } break;
                        // On Linux, D-Pad movements are axis movements. We need to convert them into button states.
                        case 7:
                        {
                            if (event.value <= -32767)
                            {
                                m_rawGamepadState.m_digitalButtonStates |= (1 << 10);
                            }
                            else if (event.value >= 32767)
                            {
                                m_rawGamepadState.m_digitalButtonStates |= (1 << 11);
                            }
                            else
                            {
                                m_rawGamepadState.m_digitalButtonStates &= ~(1 << 10);
                                m_rawGamepadState.m_digitalButtonStates &= ~(1 << 11);
                            }
                        } break;
                    }
                }break;
                case JS_EVENT_BUTTON:
                {
                    if (event.value)
                    {
                        m_rawGamepadState.m_digitalButtonStates |= (1 << event.number);
                    }
                    else
                    {
                        m_rawGamepadState.m_digitalButtonStates &= ~(1 << event.number);
                    }
                } break;
            }
        }

        ProcessRawGamepadState(m_rawGamepadState);
    }
}