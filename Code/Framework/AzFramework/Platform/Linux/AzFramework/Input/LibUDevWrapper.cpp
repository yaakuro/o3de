#include <AzCore/Debug/Trace.h>
#include <AzFramework/Input/LibUDevWrapper.h>

namespace AzFramework
{
    UDevSingletonClass* UDevSingletonClass::instance = nullptr;

    UDevSingletonClass::~UDevSingletonClass() {
    }

    const AZStd::string UDevSingletonClass::GetJoystickFilePath(AZ::u32 index) const
    {
        const auto PathItr = m_udevDeviceProperties.find(index);
        if (PathItr != m_udevDeviceProperties.end())
        {
            return PathItr->second;
        }

        return AZStd::string();
    }

    const AZStd::string UDevSingletonClass::GetGamepadFilePath(AZ::u32 index) const
    {
        const auto PathItr = m_udevGamepadProperties.find(index);
        if (PathItr != m_udevGamepadProperties.end())
        {
            return PathItr->second;
        }

        return AZStd::string();
    }

    AZ::u32 UDevSingletonClass::GetJoystickFileIndex(const AZStd::string &path) const
    {
        for (const auto& entry : m_udevDeviceProperties)
        {
            if(entry.second == path) {
                return entry.first;
            }
        }

        return 0 - 1;
    }

    AZ::u32 UDevSingletonClass::GetGamepadFileIndex(const AZStd::string &path) const
    {
        for (const auto& entry : m_udevGamepadProperties)
        {
            if(entry.second == path) {
                return entry.first;
            }
        }

        return 0 - 1;
    }

    UDevSingletonClass::UDevResults UDevSingletonClass::Init()
    {
        if(m_initialized)
        {
            return UDevResults::Success;
        }

        InputSystemNotificationBus::Handler::BusConnect();

        if (InitUDev() == UDevResults::Failed)
        {
            return UDevResults::Failed;
        }

        m_initialized = true;

        for(int i = MaxJoystickDevices - 1; i >= 0; --i)
        {
            m_freeDeviceIndices.push(i);
        }

        for(int i = MaxGamepadDevices - 1; i >= 0; --i)
        {
            m_freeGamepadIndices.push(i);
        }

        FindAllConnectedDevices();

        return UDevResults::Success;
    }

    UDevSingletonClass::UDevResults UDevSingletonClass::InitUDev()
    {
        m_libudevHandle = AZ::DynamicModuleHandle::Create("libudev.so");
        if (!m_libudevHandle->Load())
        {
            AZ_Info("Input", "libudev.so not found - gamepad support disabled. Install libuvdev2 to enable.\n");
            m_libudevHandle.reset();
            return UDevResults::Failed;
        }

        m_udev_new = m_libudevHandle->GetFunction<functionType_udev_new>("udev_new");
        m_udev_monitor_new_from_netlink =
            m_libudevHandle->GetFunction<functionType_udev_monitor_new_from_netlink>("udev_monitor_new_from_netlink");
        m_udev_monitor_filter_add_match_subsystem_devtype =
            m_libudevHandle->GetFunction<functionType_udev_monitor_filter_add_match_subsystem_devtype>("udev_monitor_filter_add_match_subsystem_devtype");
        m_udev_monitor_enable_receiving = m_libudevHandle->GetFunction<functionType_udev_monitor_enable_receiving>("udev_monitor_enable_receiving");
        m_udev_monitor_unref = m_libudevHandle->GetFunction<functionType_udev_monitor_unref>("udev_monitor_unref");
        m_udev_unref = m_libudevHandle->GetFunction<functionType_udev_unref>("udev_unref");
        m_udev_monitor_receive_device = m_libudevHandle->GetFunction<functionType_udev_monitor_receive_device>("udev_monitor_receive_device");
        m_udev_device_get_subsystem = m_libudevHandle->GetFunction<functionType_udev_device_get_subsystem>("udev_device_get_subsystem");
        m_udev_device_get_devnode = m_libudevHandle->GetFunction<functionType_udev_device_get_devnode>("udev_device_get_devnode");
        m_udev_device_get_action = m_libudevHandle->GetFunction<functionType_udev_device_get_action>("udev_device_get_action");
        m_udev_device_unref = m_libudevHandle->GetFunction<functionType_udev_device_unref>("udev_device_unref");
        m_udev_enumerate_unref = m_libudevHandle->GetFunction<funntionType_udev_enumerate_unref>("udev_enumerate_unref");
        m_udev_list_entry_get_next = m_libudevHandle->GetFunction<functionType_udev_list_entry_get_next>("udev_list_entry_get_next");
        m_udev_enumerate_new = m_libudevHandle->GetFunction<functionType_udev_enumerate_new>("udev_enumerate_new");
        m_udev_enumerate_add_match_subsystem = m_libudevHandle->GetFunction<functionType_udev_enumerate_add_match_subsystem>("udev_enumerate_add_match_subsystem");
        m_udev_enumerate_scan_devices = m_libudevHandle->GetFunction<functionType_udev_enumerate_scan_devices>("udev_enumerate_scan_devices");
        m_udev_enumerate_get_list_entry = m_libudevHandle->GetFunction<functionType_udev_enumerate_get_list_entry>("udev_enumerate_get_list_entry");
        m_udev_list_entry_get_name = m_libudevHandle->GetFunction<functionType_udev_list_entry_get_name>("udev_list_entry_get_name");
        m_udev_device_new_from_syspath = m_libudevHandle->GetFunction<functionType_udev_device_new_from_syspath>("udev_device_new_from_syspath");
        m_udev_device_get_property_value = m_libudevHandle->GetFunction<functionType_udev_device_get_property_value>("udev_device_get_property_value");

        if ((m_udev_new) &&
            ((m_udev_unref)))
        {
            AZ_Trace("Input", "libudev.so loaded and all symbols found.\n")
        }
        else
        {
            AZ_Warning("Input", false, "libudev.so loaded but missing symbols.\n");
            m_libudevHandle.reset();
        }

        m_udev = m_udev_new();
        if(!m_udev)
        {
            m_libudevHandle.reset();

            return UDevResults::Failed;

        }
        m_udevMonitor = m_udev_monitor_new_from_netlink(m_udev, "udev");
        if(!m_udevMonitor)
        {
            m_udev_monitor_unref(m_udevMonitor);
            m_libudevHandle.reset();

            return UDevResults::Failed;
        }

        m_udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor, "input", nullptr);
        m_udev_monitor_enable_receiving(m_udevMonitor);

        return UDevResults::Success;
    }

    void UDevSingletonClass::ShutdownUDev()
    {
        if(m_udevMonitor)
        {
            m_udev_monitor_unref(m_udevMonitor);
        }

        if(m_udev)
        {
            m_udev_unref(m_udev);
        }

        if (m_libudevHandle)
        {
            m_libudevHandle.reset();
        }

        m_udev = nullptr;
        m_udevMonitor = nullptr;
    }

    void UDevSingletonClass::Shutdown()
    {
        InputSystemNotificationBus::Handler::BusDisconnect();

        ShutdownUDev();

        m_initialized = false;
    }

    void UDevSingletonClass::HandleNewDevice(const char* const devnode, const AZStd::string eventNameStr)
    {
        const int fileDescriptor = open(eventNameStr.c_str(), O_RDONLY | O_NONBLOCK);
        if (fileDescriptor != -1)
        {
            libevdev* m_evdevDevice = nullptr;
            if (LibEVDevWrapper::GetInstance()->m_libevdev_new_from_fd(fileDescriptor, &m_evdevDevice) == 0)
            {
                auto deviceType = LibEVDevWrapper::GetInstance()->GetDeviceType(m_evdevDevice);
                close(fileDescriptor);

                switch (deviceType)
                {
                case LibEVDevWrapper::DeviceType::Gamepad:
                    {
                        // A new device got added. Let's check if we have this device already assigned somehow.
                        bool found = false;
                        for (const auto& Itr : m_udevGamepadProperties)
                        {
                            if (Itr.second == eventNameStr)
                            {
                                found = true;
                            }
                        }
                        if (!found)
                        {
                            const auto index = m_freeGamepadIndices.top();
                            m_udevGamepadProperties[index] = devnode;
                            m_freeGamepadIndices.pop();
                        }
                        LinuxGamepadNotificationBus::Broadcast(&LinuxGamepadNotificationBus::Events::OnConnected, eventNameStr.c_str());
                    }
                    break;
                case LibEVDevWrapper::DeviceType::Joystick:
                    {
                        // A new device got added. Let's check if we have this device already assigned somehow.
                        bool found = false;
                        for (const auto& Itr : m_udevDeviceProperties)
                        {
                            if (Itr.second == eventNameStr)
                            {
                                found = true;
                            }
                        }
                        if (!found)
                        {
                            const auto index = m_freeDeviceIndices.top();
                            m_udevDeviceProperties[index] = devnode;
                            m_freeDeviceIndices.pop();
                        }
                        LinuxJoystickNotificationBus::Broadcast(&LinuxJoystickNotificationBus::Events::OnConnected, eventNameStr.c_str());
                    }
                    break;
                default:
                    break;
                }

                LibEVDevWrapper::GetInstance()->m_libevdev_free(m_evdevDevice);
            }
        }
    }
        void UDevSingletonClass::HandleRemoveDevice(const char* const devnode, const AZStd::string eventNameStr)
    {
        const int fileDescriptor = open(eventNameStr.c_str(), O_RDONLY | O_NONBLOCK);
        if (fileDescriptor != -1)
        {
            libevdev* m_evdevDevice = nullptr;
            if (LibEVDevWrapper::GetInstance()->m_libevdev_new_from_fd(fileDescriptor, &m_evdevDevice) == 0)
            {
                auto deviceType = LibEVDevWrapper::GetInstance()->GetDeviceType(m_evdevDevice);
                close(fileDescriptor);

                switch (deviceType)
                {
                case LibEVDevWrapper::DeviceType::Gamepad:
                    {
                        for(const auto& Itr : m_udevGamepadProperties)
                        {
                            if(Itr.second == eventNameStr)
                            {
                                m_freeGamepadIndices.push(Itr.first);
                            }
                        }

                        LinuxGamepadNotificationBus::Broadcast(&LinuxGamepadNotificationBus::Events::OnDisconnected, eventNameStr.c_str());
                    }
                    break;
                case LibEVDevWrapper::DeviceType::Joystick:
                    {
                        for(const auto& Itr : m_udevDeviceProperties)
                        {
                            if(Itr.second == eventNameStr)
                            {
                                m_freeDeviceIndices.push(Itr.first);
                            }
                        }

                        LinuxJoystickNotificationBus::Broadcast(&LinuxJoystickNotificationBus::Events::OnDisconnected, eventNameStr.c_str());
                    }
                    break;
                default:
                    break;
                }

                LibEVDevWrapper::GetInstance()->m_libevdev_free(m_evdevDevice);
            }
        }
    }
    void UDevSingletonClass::OnPreInputUpdate()
    {
        if(!m_initialized)
        {
            return;
        }

        const auto device = m_udev_monitor_receive_device(m_udevMonitor);
        if (!device)
        {
            return;
        }

        const auto subsystem = m_udev_device_get_subsystem(device);
        if (subsystem && strcmp(subsystem, "input") != 0)
        {
            return;
        }

        const auto devnode = m_udev_device_get_devnode(device);
        if (!devnode)
        {
            return;
        }

        if(const auto actionName = m_udev_device_get_action(device))
        {
            const AZStd::string eventNameStr(devnode);
            if (eventNameStr.contains("event"))
            {
                const AZStd::string actionNameStr = actionName;
                if (actionNameStr == "add")
                {
                    HandleNewDevice(devnode, eventNameStr);
                }
                else if (actionNameStr == "remove")
                {
                    HandleRemoveDevice(devnode, eventNameStr);
                }
            }
        }
    }

    void UDevSingletonClass::FindAllConnectedDevices()
    {
        const auto udev = m_udev_new();
        if (!udev)
        {
            AZ_Warning("GamepadLinux", false, "Failed to create udev instance.");
            return;
        }

        const auto enumerate = m_udev_enumerate_new(udev);
        if (!enumerate)
        {
            AZ_Warning("GamepadLinux", false, "Failed to create udev enumerate instance.");
            m_udev_unref(udev);
            return;
        }

        // Only focus on input devices.
        m_udev_enumerate_add_match_subsystem(enumerate, "input");

        // Enumerate all devices connected so far.
        m_udev_enumerate_scan_devices(enumerate);

        const auto devices = m_udev_enumerate_get_list_entry(enumerate);
        if(!devices)
        {
            m_udev_enumerate_unref(enumerate);
            m_udev_unref(udev);
            return;
        }

        udev_list_entry* entry = nullptr;
        for (entry = devices; entry; entry = m_udev_list_entry_get_next(entry))
        {
            const auto path = m_udev_list_entry_get_name(entry);
            if (!path)
            {
               continue;
            }

            if (const auto device = m_udev_device_new_from_syspath(udev, path))
            {
                if (const auto devnode = m_udev_device_get_devnode(device))
                {
                    int fd = open(devnode, O_RDONLY | O_NONBLOCK);
                    if (fd >= 0)
                    {
                        libevdev* dev = nullptr;
                        int rc = LibEVDevWrapper::GetInstance()->m_libevdev_new_from_fd(fd, &dev);
                        if (rc == 0)
                        {
                            if (LibEVDevWrapper::GetInstance()->IsJoystickDevice(dev))
                            {
                                AZ::u16 Index = m_freeDeviceIndices.top();
                                m_udevDeviceProperties[Index] = devnode;
                                m_freeDeviceIndices.pop();

                                if (GetDeviceInfo(device, devnode))
                                {
                                }
                            }
                            else if (LibEVDevWrapper::GetInstance()->IsGamepadDevice(dev))
                            {
                                AZ::u16 Index = m_freeGamepadIndices.top();
                                m_udevGamepadProperties[Index] = devnode;
                                m_freeGamepadIndices.pop();
                            }
                            LibEVDevWrapper::GetInstance()->m_libevdev_free(dev);
                        }
                        close(fd);
                    }
                }
                m_udev_device_unref(device);
            }
        }

        m_udev_enumerate_unref(enumerate);
        m_udev_unref(udev);
    }

    bool UDevSingletonClass::GetDeviceInfo(udev_device* device, const AZStd::string& eventName)
    {
        bool result = false;
        const auto fd = open(eventName.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd >= 0)
        {
            libevdev* dev = nullptr;
            const auto rc = LibEVDevWrapper::GetInstance()->m_libevdev_new_from_fd(fd, &dev);
            if (rc >= 0)
            {
                if (const auto str = m_udev_device_get_property_value(device, "ID_MODEL"))
                {
                    const AZStd::string modelName = str;
                }
                const auto bustype = LibEVDevWrapper::GetInstance()->m_libevdev_get_id_bustype(dev);
                const auto vendor = LibEVDevWrapper::GetInstance()->m_libevdev_get_id_vendor(dev);
                const auto product = LibEVDevWrapper::GetInstance()->m_libevdev_get_id_product(dev);

                AZ_Info("Input", "Device: Bustype: %02x, Vendor: %02x, Product: %02x\n", bustype, vendor, product);
            }
            LibEVDevWrapper::GetInstance()->m_libevdev_free(dev);
            close(fd);
        }

        return result;
    }
}