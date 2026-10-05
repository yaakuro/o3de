/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <QWidget>
#include <QMenu>
#include <QRadioButton>
#include <QVBoxLayout>
#endif

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#if !defined(Q_MOC_RUN)
#include <AzCore/UserSettings/UserSettings.h>
#endif

namespace Ui {
    class LUAEditorFindWidget;
}

namespace LUAEditor {
    class FindResults;

    // Persisted find/replace options for the Lua IDE.
    // Registered with the serialize context from Context::Reflect so the values are saved and loaded.
    class LUAFindReplaceSettings
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(LUAFindReplaceSettings, "{93331EE4-42C5-11F0-906D-BB605C163CFB}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(LUAFindReplaceSettings, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext *reflection);

        bool GetCaseSensitive() const { return m_caseSensitive; }
        void SetCaseSensitive(bool caseSensitive) { m_caseSensitive = caseSensitive; }
        bool GetWrap() const { return m_wrap; }
        void SetWrap(bool wrap) { m_wrap = wrap; }
        bool GetRegularExpressions() const { return m_regularExpressions; }
        void SetRegularExpressions(bool regularExpressions) { m_regularExpressions = regularExpressions; }
        bool GetWholeWords() const { return m_wholeWords; }
        void SetWholeWords(bool wholeWords) { m_wholeWords = wholeWords; }
        unsigned int GetResultIndex() const { return m_resultIndex; }
        void SetResultIndex(unsigned int resultIndex) { m_resultIndex = resultIndex; }

    private:
        bool m_wrap = true;
        bool m_caseSensitive = false;
        bool m_regularExpressions = false;
        bool m_wholeWords = false;
        unsigned int m_resultIndex = 0;
    };

    class FindResultsWidget
    : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(FindResultsWidget, AZ::SystemAllocator);
        FindResultsWidget(QWidget *parent = nullptr);
        ~FindResultsWidget() override;

        int GetCurrentResultIndex() const;
        void SetCurrentResultIndex(int index);

    private:
        QRadioButton* m_result1Radio = nullptr;
        QRadioButton* m_result2Radio = nullptr;
        QRadioButton* m_result3Radio = nullptr;
        QRadioButton* m_result4Radio = nullptr;
        QButtonGroup* m_radioGroup = nullptr;
        QVBoxLayout* m_layout = nullptr;
    };

    class LUAEditorFindWidget : public QWidget
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(LUAEditorFindWidget, AZ::SystemAllocator);

        enum class FindType
        {
            Find,
            FindAndReplace,
        };

        explicit LUAEditorFindWidget(QWidget *parent = nullptr, class LUAViewWidget* viewWidget = nullptr);
        ~LUAEditorFindWidget() override;

        void SetCurrentView(class LUAViewWidget* viewWidget);
        void SetMainWindow(class LUAEditorMainWindow* viewWidget);

        void SyncFindNext();
        void SyncFindPrevious();
        void SetFocus(bool state) const;

        void SetSearchText(const QString& text);
        QString GetSearchText() const;

        void DisableNavigationButtons();

        void SetShowFind(bool state);
        void SetShowFindAndReplace(bool state);
        int GetCurrentResultIndex();

        void OnFindAllClicked();
        void OnReplaceAllClicked();
        void SetSearchScope(int scope);
        bool SearchInDirectory(class QWidget* parentWidget, class FindResults* resultsWidget, const QString& searchText);
        void CreateContextMenu();
        void SetFindInFilesMode() const;

    private slots:

        void OnFindNextClicked();
        void OnFindPreviousClicked();
        void OnCaseSensitiveToggled(bool InToggled);
        void OnCaseWholeWordsToggled(bool InToggled);
        void OnFindTextReturnPressed();
        void OnReplacedClicked();
        void OnSearchWhereChanged(int index);
        void FindInDirectory();

    private:
        void FindInOpenDocs();
        void FindInCurrentDoc();
        // void FindInAllAssets(); // Placeholder for when asset database is used
        Ui::LUAEditorFindWidget *ui = nullptr;
        LUAViewWidget* m_viewWidget = nullptr;
        LUAEditorMainWindow* pLUAEditorMainWindow = nullptr;
        QMenu* m_menu = nullptr;
        bool m_wrap = true;
        int m_resultIndex = 0;
        int m_searchScope = 0; // 0: Current File, 1: All Open Files, 2: Directory
        QString m_searchDirectory;
        FindResultsWidget* m_findResultsWidget = nullptr;
        bool m_replaceAllRunning = false;

        struct ResultEntry
        {
            QString m_lineText;
            int m_lineNumber;
            AZStd::vector<AZStd::pair<int, int>> m_matches; //position and length within line
        };

        struct ResultDocument
        {
            AZStd::string m_assetId;
            AZStd::vector<ResultEntry> m_entries;
        };
        QHash<QString, ResultDocument> m_resultList;

        QAction* m_currentFileAction = nullptr;
        QAction* m_openFilesAction = nullptr;
        QAction* m_directoryAction  = nullptr;
    };

}
