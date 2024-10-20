/*
* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>
#include <AzFramework/Input/Devices/Joystick/InputDeviceJoystick.h>
#include <AzFramework/Input/Buses/Notifications/InputSystemNotificationBus.h>

namespace AzFramework
{
    class LinuxGamepadNotificationInterface : public AZ::EBusTraits
    {
    public:
        virtual ~LinuxGamepadNotificationInterface() = default;

        virtual void OnConnected(const AZStd::string& path) = 0;
        virtual void OnDisconnected(const AZStd::string& path) = 0;
    };

    using LinuxGamepadNotificationBus = AZ::EBus<LinuxGamepadNotificationInterface>;

    class LinuxJoystickNotificationInterface : public AZ::EBusTraits
    {
    public:
        virtual ~LinuxJoystickNotificationInterface() = default;

        virtual void OnConnected(const AZStd::string& path) = 0;
        virtual void OnDisconnected(const AZStd::string& path) = 0;
    };

    using LinuxJoystickNotificationBus = AZ::EBus<LinuxJoystickNotificationInterface>;

    class UDevSingletonClass
        : public InputSystemNotificationBus::Handler
    {
    private:
        static UDevSingletonClass* instance;

        UDevSingletonClass() {}
        ~UDevSingletonClass() override;

    public:
        enum class UDevResults
        {
            Success, Failed
        };

        static const int MaxJoystickDevices = 4;
        static const int MaxGamepadDevices = 4;

        static UDevSingletonClass* getInstance()
        {
            if (!instance)
            {
                instance = new UDevSingletonClass();
            }
            return instance;
        }

        [[nodiscard]] static AZ::u32 GetMaxDevicesSupported()
        {
            return MaxJoystickDevices;
        }

        [[nodiscard]] static AZ::u32 GetMaxGamepadsSupported()
        {
            return MaxGamepadDevices;
        }

        // Returns the device path of a specified index.
        const AZStd::string GetJoystickFilePath(AZ::u32 index) const;
        const AZStd::string GetGamepadFilePath(AZ::u32 index) const;

        // Returns index of a device specifying its path.
        AZ::u32 GetJoystickFileIndex(const AZStd::string& path) const;
        AZ::u32 GetGamepadFileIndex(const AZStd::string& path) const;

        // Initialize system.
        UDevResults Init();

        // Shutdown system.
        void Shutdown();
        void HandleNewDevice(const char* devnode, AZStd::string eventNameStr);
        void HandleRemoveDevice(const char* devnode, AZStd::string eventNameStr);

        // Initialize udev related instances.
        UDevResults InitUDev();

        // Shutdown udev related instances.
        void ShutdownUDev();

        // Will be updated by the InputSystem in every pre-tick.
        void OnPreInputUpdate() override;

        // Finds all connected joystick devices.
        void FindAllConnectedDevices();

        // Returns all info from a supported device. This will be used in future releases.
        bool GetDeviceInfo(struct udev_device* device, const AZStd::string& eventName);

        // Function type declarations.
        using functionType_udev_new =  struct udev*(*)();
        using functionType_udev_monitor_new_from_netlink = struct udev_monitor *(*)(struct udev *udev, const char *name);
        using functionType_udev_monitor_filter_add_match_subsystem_devtype = int (*)(struct udev_monitor *udev_monitor,
                                                    const char *subsystem, const char *devtype);
        using functionType_udev_monitor_enable_receiving = int (*)(struct udev_monitor *udev_monitor);
        using functionType_udev_monitor_unref = void(*)(struct udev_monitor *udev_monitor);
        using functionType_udev_unref = void(*)(udev*);
        using functionType_udev_monitor_receive_device = struct udev_device *(*)(struct udev_monitor *udev_monitor);
        using functionType_udev_device_get_subsystem = const char *(*)(struct udev_device *udev_device);
        using functionType_udev_device_get_devnode = const char *(*)(struct udev_device *udev_device);
        using functionType_udev_device_get_action = const char *(*)(struct udev_device *udev_device);
        using functionType_udev_device_unref = struct udev_device *(*)(struct udev_device *udev_device);
        using funntionType_udev_enumerate_unref = struct udev_enumerate *(*)(struct udev_enumerate *udev_enumerate);
        using functionType_udev_list_entry_get_next = struct udev_list_entry * (*)(struct udev_list_entry *list_entry);
        using functionType_udev_enumerate_new = struct udev_enumerate * (*)(struct udev *udev);
        using functionType_udev_enumerate_add_match_subsystem = int (*)(struct udev_enumerate *udev_enumerate, const char *subsystem);
        using functionType_udev_enumerate_scan_devices = int (*)(struct udev_enumerate *udev_enumerate);
        using functionType_udev_enumerate_get_list_entry = struct udev_list_entry * (*)(struct udev_enumerate *udev_enumerate);
        using functionType_udev_list_entry_get_name = const char * (*)(struct udev_list_entry *list_entry);
        using functionType_udev_device_new_from_syspath = struct udev_device * (*)(struct udev *udev, const char *syspath);
        using functionType_udev_device_get_property_value = const char * (*)(struct udev_device *udev_device, const char *key);

        // Function type definitions for the udev library.
        functionType_udev_new m_udev_new = nullptr;
        functionType_udev_monitor_new_from_netlink m_udev_monitor_new_from_netlink = nullptr;
        functionType_udev_monitor_filter_add_match_subsystem_devtype m_udev_monitor_filter_add_match_subsystem_devtype = nullptr;
        functionType_udev_monitor_enable_receiving m_udev_monitor_enable_receiving = nullptr;
        functionType_udev_monitor_unref m_udev_monitor_unref = nullptr;
        functionType_udev_unref m_udev_unref = nullptr;
        functionType_udev_monitor_receive_device m_udev_monitor_receive_device = nullptr;
        functionType_udev_device_get_subsystem m_udev_device_get_subsystem = nullptr;
        functionType_udev_device_get_devnode m_udev_device_get_devnode = nullptr;
        functionType_udev_device_get_action m_udev_device_get_action = nullptr;
        functionType_udev_device_unref m_udev_device_unref = nullptr;
        funntionType_udev_enumerate_unref m_udev_enumerate_unref = nullptr;
        functionType_udev_list_entry_get_next m_udev_list_entry_get_next = nullptr;
        functionType_udev_enumerate_new m_udev_enumerate_new = nullptr;
        functionType_udev_enumerate_add_match_subsystem m_udev_enumerate_add_match_subsystem = nullptr;
        functionType_udev_enumerate_scan_devices m_udev_enumerate_scan_devices = nullptr;
        functionType_udev_enumerate_get_list_entry m_udev_enumerate_get_list_entry = nullptr;
        functionType_udev_list_entry_get_name m_udev_list_entry_get_name = nullptr;
        functionType_udev_device_new_from_syspath m_udev_device_new_from_syspath = nullptr;
        functionType_udev_device_get_property_value m_udev_device_get_property_value = nullptr;

        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_libudevHandle;
        bool m_initialized = false;
        struct udev* m_udev = nullptr;
        struct udev_monitor* m_udevMonitor = nullptr;
        AZStd::map<AZ::u32, AZStd::string> m_udevDeviceProperties;
        AZStd::stack<AZ::u32> m_freeDeviceIndices;

        AZStd::map<AZ::u32, AZStd::string> m_udevGamepadProperties;
        AZStd::stack<AZ::u32> m_freeGamepadIndices;
    };
}