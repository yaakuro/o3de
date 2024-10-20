/*
* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Buses/Requests/InputHapticFeedbackRequestBus.h>
#include <AzFramework/Input/Buses/Requests/InputLightBarRequestBus.h>
#include <AzFramework/Input/Channels/InputChannelAnalog.h>
#include <AzFramework/Input/Channels/InputChannelAxis1D.h>
#include <AzFramework/Input/Channels/InputChannelDigital.h>
#include <AzFramework/Input/Devices/InputDevice.h>

namespace AzFramework
{
    class InputDeviceJoystick
        : public InputDevice
        , public InputHapticFeedbackRequestBus::Handler
    {
    public:
        static constexpr inline const char* Name{"joystick"};

        static constexpr inline InputDeviceId IdForIndex0{Name, 0};
        static constexpr inline InputDeviceId IdForIndex1{Name, 1};
        static constexpr inline InputDeviceId IdForIndex2{Name, 2};
        static constexpr inline InputDeviceId IdForIndex3{Name, 3};

        static bool IsJoystickDevice(const InputDeviceId& inputDeviceId);

        struct Button
        {
            static constexpr inline InputChannelId Button_0 {"joystick_button_00"};
            static constexpr inline InputChannelId Button_1 {"joystick_button_01"};
            static constexpr inline InputChannelId Button_2 {"joystick_button_02"};
            static constexpr inline InputChannelId Button_3 {"joystick_button_03"};
            static constexpr inline InputChannelId Button_4 {"joystick_button_04"};
            static constexpr inline InputChannelId Button_5 {"joystick_button_05"};
            static constexpr inline InputChannelId Button_6 {"joystick_button_06"};
            static constexpr inline InputChannelId Button_7 {"joystick_button_07"};
            static constexpr inline InputChannelId Button_8 {"joystick_button_08"};
            static constexpr inline InputChannelId Button_9 {"joystick_button_09"};
            static constexpr inline InputChannelId Button_10{"joystick_button_10"};
            static constexpr inline InputChannelId Button_11{"joystick_button_11"};
            static constexpr inline InputChannelId Button_12{"joystick_button_12"};
            static constexpr inline InputChannelId Button_13{"joystick_button_13"};
            static constexpr inline InputChannelId Button_14{"joystick_button_14"};
            static constexpr inline InputChannelId Button_15{"joystick_button_15"};
            static constexpr inline InputChannelId Button_16{"joystick_button_16"};
            static constexpr inline InputChannelId Button_17{"joystick_button_17"};
            static constexpr inline InputChannelId Button_18{"joystick_button_18"};
            static constexpr inline InputChannelId Button_19{"joystick_button_19"};
            static constexpr inline InputChannelId Button_20{"joystick_button_20"};
            static constexpr inline InputChannelId Button_21{"joystick_button_21"};
            static constexpr inline InputChannelId Button_22{"joystick_button_22"};
            static constexpr inline InputChannelId Button_23{"joystick_button_23"};
            static constexpr inline InputChannelId Button_24{"joystick_button_24"};
            static constexpr inline InputChannelId Button_25{"joystick_button_25"};
            static constexpr inline InputChannelId Button_26{"joystick_button_26"};
            static constexpr inline InputChannelId Button_27{"joystick_button_27"};
            static constexpr inline InputChannelId Button_28{"joystick_button_28"};
            static constexpr inline InputChannelId Button_29{"joystick_button_29"};
            static constexpr inline InputChannelId Button_30{"joystick_button_30"};
            static constexpr inline InputChannelId Button_31{"joystick_button_31"};
            static constexpr inline InputChannelId Button_32{"joystick_button_32"};
            static constexpr inline InputChannelId Button_33{"joystick_button_33"};
            static constexpr inline InputChannelId Button_34{"joystick_button_34"};
            static constexpr inline InputChannelId Button_35{"joystick_button_35"};

            static constexpr inline AZStd::array All
            {
                Button_0,
                Button_1,
                Button_2,
                Button_3,
                Button_4,
                Button_5,
                Button_6,
                Button_7,
                Button_8,
                Button_9,
                Button_10,
                Button_11,
                Button_12,
                Button_13,
                Button_14,
                Button_15,
                Button_16,
                Button_17,
                Button_18,
                Button_19,
                Button_20,
                Button_21,
                Button_22,
                Button_23,
                Button_24,
                Button_25,
                Button_26,
                Button_27,
                Button_28,
                Button_29,
                Button_30,
                Button_31,
                Button_32,
                Button_33,
                Button_34,
                Button_35
            };
        };

        struct Axis1D
        {
            static constexpr inline InputChannelId JA_0{"joystick_axis_0"};
            static constexpr inline InputChannelId JA_1{"joystick_axis_1"};
            static constexpr inline InputChannelId JA_2{"joystick_axis_2"};
            static constexpr inline InputChannelId JA_3{"joystick_axis_3"};
            static constexpr inline InputChannelId JA_4{"joystick_axis_4"};
            static constexpr inline InputChannelId JA_5{"joystick_axis_5"};
            static constexpr inline InputChannelId JA_6{"joystick_axis_6"};
            static constexpr inline InputChannelId JA_7{"joystick_axis_7"};
            static constexpr inline InputChannelId JA_8{"joystick_axis_8"};
            static constexpr inline InputChannelId JA_9{"joystick_axis_9"};
            static constexpr inline InputChannelId JA_10{"joystick_axis_10"};
            static constexpr inline InputChannelId JA_11{"joystick_axis_11"};
            static constexpr inline InputChannelId JA_12{"joystick_axis_12"};
            static constexpr inline InputChannelId JA_13{"joystick_axis_13"};
            static constexpr inline InputChannelId JA_14{"joystick_axis_14"};
            static constexpr inline InputChannelId JA_15{"joystick_axis_15"};
            static constexpr inline InputChannelId JA_16{"joystick_axis_16"};
            static constexpr inline InputChannelId JA_17{"joystick_axis_17"};
            static constexpr inline InputChannelId JA_18{"joystick_axis_18"};

            //!< All game-pad thumb-stick 1D axis input channel ids
            static constexpr inline AZStd::array All
            {
                JA_0,
                JA_1,
                JA_2,
                JA_3,
                JA_4,
                JA_5,
                JA_6,
                JA_7,
                JA_8,
                JA_9,
                JA_10,
                JA_11,
                JA_12,
                JA_13,
                JA_14,
                JA_15,
                JA_16,
                JA_17,
                JA_18
            };
        };

        class Implementation;

        class ImplementationFactory
        {
        public:
            AZ_TYPE_INFO(ImplementationFactory, "{FB3F2476-9111-11EF-B849-43B2078210DF}");
            virtual ~ImplementationFactory() = default;
            virtual AZStd::unique_ptr<Implementation> Create(InputDeviceJoystick& InputDeviceJoystick) = 0;
            virtual AZ::u32 GetMaxSupportedJoysticks() const = 0;
        };

        AZ_CLASS_ALLOCATOR(InputDeviceJoystick, AZ::SystemAllocator);
        AZ_RTTI(InputDeviceJoystick, "{31F816E8-8F2E-11EF-9DD5-C77A683BDF18}", InputDevice);

        static void Reflect(AZ::ReflectContext* context);

        ~InputDeviceJoystick() override;
        void InitializeChannels();
        explicit InputDeviceJoystick(AZ::u32 index);
        explicit InputDeviceJoystick(const InputDeviceId& inputDeviceId,
                                    ImplementationFactory* implementationFactory = AZ::Interface<ImplementationFactory>::Get());

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputDeviceJoystick);

         ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::GetAssignedLocalUserId
        LocalUserId GetAssignedLocalUserId() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::PromptLocalUserSignIn
        void PromptLocalUserSignIn() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::GetInputChannelsById
        const InputChannelByIdMap& GetInputChannelsById() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::IsSupported
        bool IsSupported() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDevice::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::GetPhysicalKeyOrButtonText
        void GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId,
                                        AZStd::string& o_keyOrButtonText) const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceRequests::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputHapticFeedbackRequests::SetVibration
        void SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized) override;

        class Implementation
        {
        public:
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator);
            AZ_DISABLE_COPY_MOVE(Implementation);

            static Implementation* Create(InputDeviceJoystick& inputDevice);
            explicit Implementation(InputDeviceJoystick& inputDevice);

            virtual ~Implementation();

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Access to the input device's currently assigned local user id
            //! \return Id of the local user currently assigned to the input device
            virtual LocalUserId GetAssignedLocalUserId() const;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Prompt a local user sign-in request from this input device
            virtual void PromptLocalUserSignIn() const {}

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Query the connected state of the input device
            //! \return True if the input device is currently connected, false otherwise
            virtual bool IsConnected() const = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Set the current vibration (force-feedback) speed of the joystick motors
            //! \param[in] leftMotorSpeedNormalized Speed of the left (large/low frequency) motor
            //! \param[in] rightMotorSpeedNormalized Speed of the right (small/high frequency) motor
            virtual void SetVibration(float leftMotorSpeedNormalized,
                                      float rightMotorSpeedNormalized) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Set the current light bar color of the joystick (if one exists)
            //! \param[in] color The color to set the joystick's light bar
            virtual void SetLightBarColor(const AZ::Color& color);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Reset the light bar color of the joystick (if one exists) to it's default
            virtual void ResetLightBarColor() {}

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Get the text displayed on the physical key/button associated with an input channel.
            //! \param[in] inputChannelId The input channel id whose key or button text to return
            //! \param[out] o_keyOrButtonText The text displayed on the physical key/button if found
            //! \return True if o_keyOrButtonText was set, false otherwise
            virtual bool GetPhysicalKeyOrButtonText(const InputChannelId& /*inputChannelId*/,
                                                    AZStd::string& /*o_keyOrButtonText*/) const { return false; }

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Tick/update the input device to broadcast all input events since the last frame
            virtual void TickInputDevice() = 0;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Broadcast an event when the input device connects to the system
            void BroadcastInputDeviceConnectedEvent() const;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Broadcast an event when the input device disconnects from the system
            void BroadcastInputDeviceDisconnectedEvent() const;

        protected:

            struct RawJoystickState
            {
                struct AxisInfo
                {
                    float m_minValue;
                    float m_scaleFactor;
                    float m_deadZoneEpsilon;
                };

                using DigitalButtonIdByBitMaskMap = AZStd::unordered_map<AZ::u32, const InputChannelId*>;
                using ButtonMap = AZStd::unordered_map<InputChannelId, AZ::u32>;

                using AnalogIdByMaskMap = AZStd::unordered_map<AZ::u32, const InputChannelId*>;
                using AnalogMap = AZStd::unordered_map<AZ::u32, AxisInfo>;

                ////////////////////////////////////////////////////////////////////////////////////
                //! Constructor
                //! \param[in] digitalButtonIdsByBitMask A map of digital button ids by bitmask
                //! \param[in] analogMask A map of analog axis ids.
                RawJoystickState();

                ////////////////////////////////////////////////////////////////////////////////////
                //! Reset the raw gamepad state
                void Reset();

                AnalogMap m_axisInfo;
                DigitalButtonIdByBitMaskMap m_digitalButtonIdsByBitMask;
                AnalogIdByMaskMap m_analogMask;
            };

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Process a joystick state that has been obtained since the last call to this function.
            //! This function is not thread safe, and so should only be called from the main thread.
            //! \param[in] rawJoystickState The raw joystick state
            void ProcessRawJoystickState(const RawJoystickState& rawJoystickState);

            ////////////////////////////////////////////////////////////////////////////////////////
            //! Reset the state of all this input device's associated input channels
            void ResetInputChannelStates() const;

            ////////////////////////////////////////////////////////////////////////////////////////
            //! \ref AzFramework::InputDeviceId::GetIndex
            AZ::u32 GetInputDeviceIndex() const;

            InputDeviceJoystick& m_inputDevice;
        };

        void SetImplementation(AZStd::unique_ptr<Implementation> impl) { m_pimpl = AZStd::move(impl); }
        using ButtonChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelDigital*>;
        using Axis1DChannelByIdMap = AZStd::unordered_map<InputChannelId, InputChannelAxis1D*>;

        ButtonChannelByIdMap& GetButtonChannelsByIdMap() { return m_buttonChannelsById; }
        Axis1DChannelByIdMap& GetAxis1DChannelsByIdMap() { return m_axis1DChannelsById; }
    private:

        AZStd::unique_ptr<Implementation> m_pimpl;
        InputChannelByIdMap m_allChannelsById;
        ButtonChannelByIdMap m_buttonChannelsById;
        Axis1DChannelByIdMap m_axis1DChannelsById;
    };
}