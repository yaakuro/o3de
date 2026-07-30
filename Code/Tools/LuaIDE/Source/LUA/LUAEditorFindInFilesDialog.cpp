/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUAEditorFindInFilesDialog.hxx"
#include <Source/LUA/ui_LUAEditorFindInFilesDialog.h>
#include "LUAEditorFindWidget.h"
#include "LUAEditorMainWindow.hxx"

#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>

namespace LUAEditor
{
    LUAEditorFindInFilesDialog::LUAEditorFindInFilesDialog(LUAEditorMainWindow* mainWindow, QWidget* parent)
        : QDialog(parent)
        , m_mainWindow(mainWindow)
    {
        m_gui = azcreate(Ui::LUAEditorFindInFilesDialog, ());
        m_gui->setupUi(this);
        this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

        m_findWidget = static_cast<LUAEditorFindWidget*>(m_gui->m_findInFilesWidget);
        m_findWidget->SetShowFind(true);
        m_findWidget->SetFindInFilesMode();

        QPushButton* searchButton = m_gui->buttonBox->button(QDialogButtonBox::StandardButton::Ok);
        if (searchButton)
        {
            searchButton->setText(tr("Search"));
        }

        connect(searchButton, &QPushButton::clicked, this, &LUAEditorFindInFilesDialog::OnSearchClicked);
        connect(m_gui->buttonBox, &QDialogButtonBox::rejected, this, &LUAEditorFindInFilesDialog::OnCancelClicked);
    }

    LUAEditorFindInFilesDialog::~LUAEditorFindInFilesDialog()
    {
        azdestroy(m_gui);
    }

    void LUAEditorFindInFilesDialog::OnSearchClicked()
    {
        if (m_findWidget && m_mainWindow)
        {
            m_findWidget->SetSearchScope(2); // Directory scope — prompts for directory selection via QFileDialog

            int resultIndex = m_findWidget->GetCurrentResultIndex();
            auto* resultsWidget = m_mainWindow->GetFindResultsWidget(resultIndex);
            if (resultsWidget)
            {
                resultsWidget->Clear();
                m_mainWindow->ResetSearchClicks();

                bool hasMatches = m_findWidget->SearchInDirectory(
                    this,
                    resultsWidget,
                    m_findWidget->GetSearchText()
                );

                if (hasMatches)
                {
                    auto* findTabWidget = m_mainWindow->GetFindTabWidget();
                    if (findTabWidget)
                    {
                        findTabWidget->show();
                        findTabWidget->raise();
                        findTabWidget->activateWindow();
                        findTabWidget->setFocus();
                        m_mainWindow->OnOpenFindView(resultIndex);
                    }
                }
            }

            accept();
        }
    }

    void LUAEditorFindInFilesDialog::OnCancelClicked()
    {
        reject();
    }

} // namespace LUAEditor
