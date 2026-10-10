/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/XcbConnectionManager.h>
#include <AzFramework/XcbEventHandler.h>
#include <AzFramework/XcbInterface.h>

#include <xcb/xfixes.h>
#include <xcb/xinput.h>

// The maximum number of raw input axis this mouse device supports.
constexpr uint32_t MAX_XI_RAW_AXIS = 2;

// The sensitivity of the wheel.
constexpr float MAX_XI_WHEEL_SENSITIVITY = 140.0f;

namespace AzFramework
{
    class XcbInputDeviceMouse
        : public InputDeviceMouse::Implementation
        , public XcbEventHandlerBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(XcbInputDeviceMouse, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        XcbInputDeviceMouse(InputDeviceMouse& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        virtual ~XcbInputDeviceMouse();

        static XcbInputDeviceMouse::Implementation* Create(InputDeviceMouse& inputDevice);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorState
        void SetSystemCursorState(SystemCursorState systemCursorState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorState
        SystemCursorState GetSystemCursorState() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorPositionNormalized
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorPositionNormalized
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::TickInputDevice
        void TickInputDevice() override;

        //! Handle X11 events.
        void HandleXcbEvent(xcb_generic_event_t* event) override;

        //! Initialize XFixes extension. Used for barriers.
        static bool InitializeXFixes();

        //! Initialize XInput extension. Used for raw input during confinement and showing/hiding the cursor.
        static bool InitializeXInput();

        //! Create barriers.
        void CreateBarriers(xcb_window_t window, bool create);

        //! Maps a system cursor state to whether the cursor should be confined and whether it should be visible.
        void SystemCursorStateToLogic(SystemCursorState systemCursorState, bool& confined, bool& cursorShown);

        //! Requests the cursor to be shown or hidden. XFixes counts hide/shows per screen (and per
        //! client), so the window only selects the screen. Returns true if the request was applied
        //! (or the cursor was already in the requested state).
        bool ShowCursor(xcb_window_t window, bool show);

        //! Returns the root window of the screen the given window belongs to, which can be used as
        //! a stable target for XFixes hide/show requests. Falls back to the default screen root if
        //! the window is unknown or no longer exists.
        xcb_window_t GetScreenRootForWindow(xcb_window_t window) const;

        //! Get the normalized cursor position. The coordinates returned are relative to the specified window.
        AZ::Vector2 GetSystemCursorPositionNormalizedInternal(xcb_window_t window) const;

        //! Set the normalized cursor position. The normalized position will be relative to the specified window.
        void SetSystemCursorPositionNormalizedInternal(xcb_window_t window, AZ::Vector2 positionNormalized);

        //! Handle button press/release events.
        void HandleButtonPressEvents(uint32_t detail, bool pressed);

        //! Applies the current system cursor state: while the cursor should be hidden it is hidden on
        //! the cursor constraint window and on the currently focused window, and it is shown again on
        //! every other window it was hidden on; (un)confines the cursor as needed.
        void ReconcileCursorState();

        //! Will handle all raw input events.
        void HandleRawInputEvents(const xcb_ge_generic_event_t* event);

        //! Convert XInput fp1616 to float.
        inline float fp1616ToFloat(xcb_input_fp1616_t value) const
        {
            return static_cast<float>((value >> 16) + (value & 0xffff) / 0xffff);
        }

        //! Convert XInput fp3232 to float.
        inline float fp3232ToFloat(xcb_input_fp3232_t value) const
        {
            return static_cast<float>(value.integral) + static_cast<float>(value.frac / (float)(1ull << 32));
        }

        const InputChannelId* InputChannelFromMouseEvent(xcb_button_t button, bool& isWheel, float& direction) const
        {
            isWheel = false;
            direction = 1.0f;
            switch (button)
            {
            case XCB_BUTTON_INDEX_1:
                return &InputDeviceMouse::Button::Left;
            case XCB_BUTTON_INDEX_2:
                return &InputDeviceMouse::Button::Middle;
            case XCB_BUTTON_INDEX_3:
                return &InputDeviceMouse::Button::Right;
            case XCB_BUTTON_INDEX_4:
                isWheel = true;
                direction = 1.0f;
                break;
            case XCB_BUTTON_INDEX_5:
                isWheel = true;
                direction = -1.0f;
                break;
            default:
                break;
            }

            return nullptr;
        }

        // Barriers work only with positive values. We clamp here to zero.
        inline int16_t Clamp(int16_t value) const
        {
            return value < 0 ? 0 : value;
        }

    private:
        //! The current system cursor state
        SystemCursorState m_systemCursorState;

        //! The cursor position before it got hidden.
        AZ::Vector2 m_cursorHiddenPosition;

        AZ::Vector2 m_systemCursorPositionNormalized;

        static xcb_connection_t* s_xcbConnection;
        static xcb_screen_t* s_xcbScreen;

        //! Will be true if the xfixes extension could be initialized.
        static bool m_xfixesInitialized;

        //! Will be true if the xinput2 extension could be initialized.
        static bool m_xInputInitialized;

        //! The current window that has focus
        xcb_window_t m_focusWindow;

        //! Will be true if the cursor is not hidden on any window, else false.
        bool m_cursorShown;

        //! The set of screen root windows that currently hold an outstanding XFixes hide request.
        //! XFixes counts hide/show requests per screen (and per client); the window argument only
        //! selects the screen. Because a hide is not released when a view/overlay window is
        //! destroyed, every hide/show is issued on the screen's root window (which is valid for as
        //! long as the connection lives) and this set tracks which screens currently have an
        //! outstanding hide, so the cursor can never remain hidden for the lifetime of the process.
        AZStd::unordered_set<xcb_window_t> m_hiddenCursorScreens;

        struct XFixesBarrierProperty
        {
            xcb_xfixes_barrier_t id;
            uint32_t direction;
            int16_t x0, y0, x1, y1;
        };

        //! Array that holds barrier information used to confine the cursor.
        std::vector<XFixesBarrierProperty> m_activeBarriers;
    };
} // namespace AzFramework
