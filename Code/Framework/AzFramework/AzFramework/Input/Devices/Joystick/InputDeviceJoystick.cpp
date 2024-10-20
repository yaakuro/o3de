/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Devices/Joystick/InputDeviceJoystick.h>
#include <AzFramework/Input/Utils/AdjustAnalogInputForDeadZone.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AzFramework
{
    bool InputDeviceJoystick::IsJoystickDevice(const InputDeviceId& inputDeviceId)
    {
        return (inputDeviceId.GetNameCrc32() == IdForIndex0.GetNameCrc32());
    }

    void InputDeviceJoystick::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<InputDeviceJoystick>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Constant("name", BehaviorConstant(IdForIndex0.GetName()))

                ->Constant(Button::Button_0.GetName(), BehaviorConstant(Button::Button_0.GetName()))
                ->Constant(Button::Button_1.GetName(), BehaviorConstant(Button::Button_1.GetName()))
                ->Constant(Button::Button_2.GetName(), BehaviorConstant(Button::Button_2.GetName()))
                ->Constant(Button::Button_3.GetName(), BehaviorConstant(Button::Button_3.GetName()))
                ->Constant(Button::Button_4.GetName(), BehaviorConstant(Button::Button_4.GetName()))
                ->Constant(Button::Button_5.GetName(), BehaviorConstant(Button::Button_5.GetName()))
                ->Constant(Button::Button_6.GetName(), BehaviorConstant(Button::Button_6.GetName()))
                ->Constant(Button::Button_7.GetName(), BehaviorConstant(Button::Button_7.GetName()))
                ->Constant(Button::Button_8.GetName(), BehaviorConstant(Button::Button_8.GetName()))
                ->Constant(Button::Button_9.GetName(), BehaviorConstant(Button::Button_9.GetName()))
                ->Constant(Button::Button_10.GetName(), BehaviorConstant(Button::Button_10.GetName()))
                ->Constant(Button::Button_11.GetName(), BehaviorConstant(Button::Button_11.GetName()))
                ->Constant(Button::Button_12.GetName(), BehaviorConstant(Button::Button_12.GetName()))
                ->Constant(Button::Button_13.GetName(), BehaviorConstant(Button::Button_13.GetName()))
                ->Constant(Button::Button_14.GetName(), BehaviorConstant(Button::Button_14.GetName()))
                ->Constant(Button::Button_15.GetName(), BehaviorConstant(Button::Button_15.GetName()))
                ->Constant(Button::Button_16.GetName(), BehaviorConstant(Button::Button_16.GetName()))
                ->Constant(Button::Button_17.GetName(), BehaviorConstant(Button::Button_17.GetName()))
                ->Constant(Button::Button_18.GetName(), BehaviorConstant(Button::Button_18.GetName()))
                ->Constant(Button::Button_19.GetName(), BehaviorConstant(Button::Button_19.GetName()))
                ->Constant(Button::Button_20.GetName(), BehaviorConstant(Button::Button_20.GetName()))
                ->Constant(Button::Button_21.GetName(), BehaviorConstant(Button::Button_21.GetName()))
                ->Constant(Button::Button_22.GetName(), BehaviorConstant(Button::Button_22.GetName()))
                ->Constant(Button::Button_23.GetName(), BehaviorConstant(Button::Button_23.GetName()))
                ->Constant(Button::Button_24.GetName(), BehaviorConstant(Button::Button_24.GetName()))
                ->Constant(Button::Button_25.GetName(), BehaviorConstant(Button::Button_25.GetName()))
                ->Constant(Button::Button_26.GetName(), BehaviorConstant(Button::Button_26.GetName()))
                ->Constant(Button::Button_27.GetName(), BehaviorConstant(Button::Button_27.GetName()))
                ->Constant(Button::Button_28.GetName(), BehaviorConstant(Button::Button_28.GetName()))
                ->Constant(Button::Button_29.GetName(), BehaviorConstant(Button::Button_29.GetName()))
                ->Constant(Button::Button_30.GetName(), BehaviorConstant(Button::Button_30.GetName()))
                ->Constant(Button::Button_31.GetName(), BehaviorConstant(Button::Button_31.GetName()))
                ->Constant(Button::Button_32.GetName(), BehaviorConstant(Button::Button_32.GetName()))
                ->Constant(Button::Button_33.GetName(), BehaviorConstant(Button::Button_33.GetName()))
                ->Constant(Button::Button_34.GetName(), BehaviorConstant(Button::Button_34.GetName()))
                ->Constant(Button::Button_35.GetName(), BehaviorConstant(Button::Button_35.GetName()))

                ->Constant(Axis1D::JA_0.GetName(), BehaviorConstant(Axis1D::JA_0.GetName()))
                ->Constant(Axis1D::JA_1.GetName(), BehaviorConstant(Axis1D::JA_1.GetName()))
                ->Constant(Axis1D::JA_2.GetName(), BehaviorConstant(Axis1D::JA_2.GetName()))
                ->Constant(Axis1D::JA_3.GetName(), BehaviorConstant(Axis1D::JA_3.GetName()))
                ->Constant(Axis1D::JA_4.GetName(), BehaviorConstant(Axis1D::JA_4.GetName()))
                ->Constant(Axis1D::JA_5.GetName(), BehaviorConstant(Axis1D::JA_5.GetName()))
                ->Constant(Axis1D::JA_6.GetName(), BehaviorConstant(Axis1D::JA_6.GetName()))
                ->Constant(Axis1D::JA_7.GetName(), BehaviorConstant(Axis1D::JA_7.GetName()))
                ->Constant(Axis1D::JA_8.GetName(), BehaviorConstant(Axis1D::JA_8.GetName()))
                ->Constant(Axis1D::JA_9.GetName(), BehaviorConstant(Axis1D::JA_9.GetName()))
            ;
        }

    }

    InputDeviceJoystick::~InputDeviceJoystick()
    {
        InputHapticFeedbackRequestBus::Handler::BusDisconnect(GetInputDeviceId());

        m_pimpl.reset();

        for (const auto& channelById : m_axis1DChannelsById)
        {
            delete channelById.second;
        }

        for (const auto& channelById : m_buttonChannelsById)
        {
            delete channelById.second;
        }
    }

    void InputDeviceJoystick::InitializeChannels()
    {
        for (const InputChannelId& channelId : Button::All)
        {
            auto* channel = aznew InputChannelDigital(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_buttonChannelsById[channelId] = channel;
        }

        for (const InputChannelId& channelId : Axis1D::All)
        {
            auto* channel = aznew InputChannelAxis1D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_axis1DChannelsById[channelId] = channel;
        }
    }

    InputDeviceJoystick::InputDeviceJoystick(AZ::u32 index)
        : InputDeviceJoystick(InputDeviceId(Name, index))
    {
        InitializeChannels();
    }

    InputDeviceJoystick::InputDeviceJoystick(const InputDeviceId& inputDeviceId, ImplementationFactory* implementationFactory)
        : InputDevice(inputDeviceId)
        , m_axis1DChannelsById()
        , m_buttonChannelsById()
    {
        m_pimpl = (implementationFactory != nullptr) ? implementationFactory->Create(*this) : nullptr;

        InitializeChannels();

        InputHapticFeedbackRequestBus::Handler::BusConnect(GetInputDeviceId());
    }

    LocalUserId InputDeviceJoystick::GetAssignedLocalUserId() const
    {
        return m_pimpl ? m_pimpl->GetAssignedLocalUserId() : InputDevice::GetAssignedLocalUserId();
    }
    void InputDeviceJoystick::PromptLocalUserSignIn() const
    {
        if (m_pimpl)
        {
            m_pimpl->PromptLocalUserSignIn();
        }
    }

    const InputDeviceRequests::InputChannelByIdMap& InputDeviceJoystick::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    bool InputDeviceJoystick::IsSupported() const
    {
        return m_pimpl != nullptr;
    }

    bool InputDeviceJoystick::IsConnected() const
    {
        return m_pimpl && m_pimpl->IsConnected();
    }

    void InputDeviceJoystick::TickInputDevice()
    {
        if(m_pimpl)
        {
            m_pimpl->TickInputDevice();
        }
    }

    void InputDeviceJoystick::SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized)
    {
        if(m_pimpl)
        {
            m_pimpl->SetVibration(leftMotorSpeedNormalized, rightMotorSpeedNormalized);
        }
    }

    void InputDeviceJoystick::GetPhysicalKeyOrButtonText(const InputChannelId& inputChannelId, AZStd::string& o_keyOrButtonText) const
    {
        if (m_pimpl && m_pimpl->GetPhysicalKeyOrButtonText(inputChannelId, o_keyOrButtonText))
        {
            return;
        }

        if (inputChannelId == Button::Button_0) { o_keyOrButtonText = "BUTTON_00"; }
        else if (inputChannelId == Button::Button_1) { o_keyOrButtonText = "BUTTON_01"; }
        else if (inputChannelId == Button::Button_2) { o_keyOrButtonText = "BUTTON_02"; }
        else if (inputChannelId == Button::Button_3) { o_keyOrButtonText = "BUTTON_03"; }
        else if (inputChannelId == Button::Button_4) { o_keyOrButtonText = "BUTTON_04"; }
        else if (inputChannelId == Button::Button_5) { o_keyOrButtonText = "BUTTON_05"; }
        else if (inputChannelId == Button::Button_6) { o_keyOrButtonText = "BUTTON_06"; }
        else if (inputChannelId == Button::Button_7) { o_keyOrButtonText = "BUTTON_07"; }
        else if (inputChannelId == Button::Button_8) { o_keyOrButtonText = "BUTTON_08"; }
        else if (inputChannelId == Button::Button_9) { o_keyOrButtonText = "BUTTON_09"; }
        else if (inputChannelId == Button::Button_10) { o_keyOrButtonText = "BUTTON_10"; }
        else if (inputChannelId == Button::Button_11) { o_keyOrButtonText = "BUTTON_11"; }
        else if (inputChannelId == Button::Button_12) { o_keyOrButtonText = "BUTTON_12"; }
        else if (inputChannelId == Button::Button_13) { o_keyOrButtonText = "BUTTON_13"; }
        else if (inputChannelId == Button::Button_14) { o_keyOrButtonText = "BUTTON_14"; }
        else if (inputChannelId == Button::Button_15) { o_keyOrButtonText = "BUTTON_15"; }
        else if (inputChannelId == Button::Button_16) { o_keyOrButtonText = "BUTTON_16"; }
        else if (inputChannelId == Button::Button_17) { o_keyOrButtonText = "BUTTON_17"; }
        else if (inputChannelId == Button::Button_18) { o_keyOrButtonText = "BUTTON_18"; }
        else if (inputChannelId == Button::Button_19) { o_keyOrButtonText = "BUTTON_19"; }
        else if (inputChannelId == Button::Button_20) { o_keyOrButtonText = "BUTTON_20"; }
        else if (inputChannelId == Button::Button_21) { o_keyOrButtonText = "BUTTON_21"; }
        else if (inputChannelId == Button::Button_22) { o_keyOrButtonText = "BUTTON_22"; }
        else if (inputChannelId == Button::Button_23) { o_keyOrButtonText = "BUTTON_23"; }
        else if (inputChannelId == Button::Button_24) { o_keyOrButtonText = "BUTTON_24"; }
        else if (inputChannelId == Button::Button_25) { o_keyOrButtonText = "BUTTON_25"; }
        else if (inputChannelId == Button::Button_26) { o_keyOrButtonText = "BUTTON_26"; }
        else if (inputChannelId == Button::Button_27) { o_keyOrButtonText = "BUTTON_27"; }
        else if (inputChannelId == Button::Button_28) { o_keyOrButtonText = "BUTTON_28"; }
        else if (inputChannelId == Button::Button_29) { o_keyOrButtonText = "BUTTON_29"; }
        else if (inputChannelId == Button::Button_30) { o_keyOrButtonText = "BUTTON_30"; }
        else if (inputChannelId == Button::Button_31) { o_keyOrButtonText = "BUTTON_31"; }
        else if (inputChannelId == Button::Button_32) { o_keyOrButtonText = "BUTTON_32"; }
        else if (inputChannelId == Button::Button_33) { o_keyOrButtonText = "BUTTON_33"; }
        else if (inputChannelId == Button::Button_34) { o_keyOrButtonText = "BUTTON_34"; }
        else if (inputChannelId == Button::Button_35) { o_keyOrButtonText = "BUTTON_35"; }
    }

    InputDeviceJoystick::Implementation::Implementation(InputDeviceJoystick& inputDevice)
        : m_inputDevice(inputDevice)
    {
    }

    InputDeviceJoystick::Implementation::~Implementation()
    = default;

    LocalUserId InputDeviceJoystick::Implementation::GetAssignedLocalUserId() const
    {
        return GetInputDeviceIndex();
    }

    AZ::u32 InputDeviceJoystick::Implementation::GetInputDeviceIndex() const
    {
        return m_inputDevice.GetInputDeviceId().GetIndex();
    }

    void InputDeviceJoystick::Implementation::ResetInputChannelStates() const
    {
        m_inputDevice.ResetInputChannelStates();
    }

    void InputDeviceJoystick::Implementation::SetVibration(float leftMotorSpeedNormalized, float rightMotorSpeedNormalized)
    {
        AZ_UNUSED(leftMotorSpeedNormalized)
        AZ_UNUSED(rightMotorSpeedNormalized)
    }

    void InputDeviceJoystick::Implementation::SetLightBarColor(const AZ::Color& color)
    {
        AZ_UNUSED(color)
    }

    void InputDeviceJoystick::Implementation::RawJoystickState::Reset()
    {

    }

    InputDeviceJoystick::Implementation::RawJoystickState::RawJoystickState()
    {
    }

    void InputDeviceJoystick::Implementation::ProcessRawJoystickState(const RawJoystickState& rawGamepadState)
    {

    }

    void InputDeviceJoystick::Implementation::BroadcastInputDeviceConnectedEvent() const
    {
        m_inputDevice.BroadcastInputDeviceConnectedEvent();
    }

    void InputDeviceJoystick::Implementation::BroadcastInputDeviceDisconnectedEvent() const
    {
        m_inputDevice.BroadcastInputDeviceDisconnectedEvent();
    }
}