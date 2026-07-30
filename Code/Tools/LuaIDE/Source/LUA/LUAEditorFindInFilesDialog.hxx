/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_FINDINFILESDIALOG_H
#define LUAEDITOR_FINDINFILESDIALOG_H
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QDialog>

namespace Ui
{
    class LUAEditorFindInFilesDialog;
}

namespace LUAEditor
{
    class LUAEditorFindWidget;

    //////////////////////////////////////////////////////////////////////////
    // Dialog for finding text across files when no document is currently open.
    // Hosts a LUAEditorFindWidget so the user can configure search options,
    // pick a directory, and trigger the search from a standalone container.
    class LUAEditorFindInFilesDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(LUAEditorFindInFilesDialog, AZ::SystemAllocator);
        explicit LUAEditorFindInFilesDialog(class LUAEditorMainWindow* mainWindow, QWidget* parent = nullptr);
        ~LUAEditorFindInFilesDialog() override;

    private slots:
        void OnSearchClicked();
        void OnCancelClicked();

    private:
        Ui::LUAEditorFindInFilesDialog* m_gui = nullptr;
        LUAEditorFindWidget* m_findWidget = nullptr;
        class LUAEditorMainWindow* m_mainWindow = nullptr;
    };

} // namespace LUAEditor

#endif // LUAEDITOR_FINDINFILESDIALOG_H
