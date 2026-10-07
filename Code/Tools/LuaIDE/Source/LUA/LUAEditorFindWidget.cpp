/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUAEditorFindWidget.h"
#include "AzCore/UserSettings/UserSettingsComponent.h"
#include "LUAEditorView.hxx"
#include "LUAEditorFindResults.hxx"
#include "LUAEditorMainWindow.hxx"
#include "LUAEditorBlockState.h"
#include "Source/LUA/ui_LUAEditorFindWidget.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Utils/Utils.h>

#include <QListWidgetItem>
#include <QPushButton>
#include <QWidgetAction>
#include <QButtonGroup>
#include <QActionGroup>
#include <QIcon>
#include <QFileDialog>
#include <QDirIterator>
#include <QTextStream>
#include <QRegularExpression>
#include <QTimer>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QProgressDialog>

namespace LUAEditor {
    FindResultsWidget::FindResultsWidget(QWidget *parent)
        : QWidget(parent) {
        // Create radio buttons
        m_result1Radio = new QRadioButton(tr("Results in Find Window 1"), this);
        m_result2Radio = new QRadioButton(tr("Results in Find Window 2"), this);
        m_result3Radio = new QRadioButton(tr("Results in Find Window 3"), this);
        m_result4Radio = new QRadioButton(tr("Results in Find Window 4"), this);

        // Create button group to manage radio button behavior
        m_radioGroup = new QButtonGroup(this);
        m_radioGroup->addButton(m_result1Radio);
        m_radioGroup->addButton(m_result2Radio);
        m_radioGroup->addButton(m_result3Radio);
        m_radioGroup->addButton(m_result4Radio);

        // Report user changes so the selected results window can be persisted.
        const auto reportResultIndex = [this](int index) { emit ResultIndexChanged(index); };
        connect(m_result1Radio, &QRadioButton::clicked, this, [=] { reportResultIndex(0); });
        connect(m_result2Radio, &QRadioButton::clicked, this, [=] { reportResultIndex(1); });
        connect(m_result3Radio, &QRadioButton::clicked, this, [=] { reportResultIndex(2); });
        connect(m_result4Radio, &QRadioButton::clicked, this, [=] { reportResultIndex(3); });

        // Set initial state - first button checked
        m_result1Radio->setChecked(true);

        // Create layout and add radio buttons
        m_layout = new QVBoxLayout(this);
        m_layout->addWidget(m_result1Radio);
        m_layout->addWidget(m_result2Radio);
        m_layout->addWidget(m_result3Radio);
        m_layout->addWidget(m_result4Radio);
        m_layout->setSpacing(2);
        m_layout->setContentsMargins(4, 4, 4, 4);

        // Set the widget to be a context menu compatible widget
        setAttribute(Qt::WA_NoSystemBackground);
    }

    FindResultsWidget::~FindResultsWidget() {
    }

    int FindResultsWidget::GetCurrentResultIndex() const {
        if (m_result1Radio->isChecked()) {
            return 0;
        } else if (m_result2Radio->isChecked()) {
            return 1;
        } else if (m_result3Radio->isChecked()) {
            return 2;
        } else if (m_result4Radio->isChecked()) {
            return 3;
        }
        return 0; // default to first option
    }

    void FindResultsWidget::SetCurrentResultIndex(int index) {
        switch (index) {
            case 0:
                m_result1Radio->setChecked(true);
                break;
            case 1:
                m_result2Radio->setChecked(true);
                break;
            case 2:
                m_result3Radio->setChecked(true);
                break;
            case 3:
                m_result4Radio->setChecked(true);
                break;
            default:
                m_result1Radio->setChecked(true); // default to first option
                break;
        }
    }

    void LUAFindReplaceSettings::Reflect(AZ::ReflectContext *reflection) {
        AZ::SerializeContext *serializeContext = azrtti_cast<AZ::SerializeContext *>(reflection);
        if (serializeContext) {
            serializeContext->Class<LUAFindReplaceSettings, AZ::UserSettings>()
                    ->Version(1)
                    ->Field("m_wrap", &LUAFindReplaceSettings::m_wrap)
                    ->Field("m_caseSensitive", &LUAFindReplaceSettings::m_caseSensitive)
                    ->Field("m_regularExpressions", &LUAFindReplaceSettings::m_regularExpressions)
                    ->Field("m_wholeWords", &LUAFindReplaceSettings::m_wholeWords)
                    ->Field("m_resultIndex", &LUAFindReplaceSettings::m_resultIndex);

            AZ::EditContext *editContext = serializeContext->GetEditContext();
            if (editContext) {
                editContext->Class<LUAFindReplaceSettings>("Lua Find Settings",
                                                           "Persisted options for the Lua IDE find and replace bar.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")

                        ->DataElement(AZ::Edit::UIHandlers::Default, &LUAFindReplaceSettings::m_wrap, "Wrap", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LUAFindReplaceSettings::m_caseSensitive,
                                      "CaseSensitive", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LUAFindReplaceSettings::m_regularExpressions,
                                      "RegularExpressions", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LUAFindReplaceSettings::m_wholeWords,
                                      "WholeWords", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &LUAFindReplaceSettings::m_resultIndex,
                                      "ResultIndex", "");
            }
        }
    }


    void LUAEditorFindWidget::CreateContextMenu() {
        m_menu = new QMenu(this);
        auto *wrapFindingAction = new QAction(tr("Wrap finding"), this);
        wrapFindingAction->setCheckable(true);
        wrapFindingAction->setChecked(m_wrap);

        connect(wrapFindingAction, &QAction::toggled, [this](bool checked) {
            m_wrap = checked;
            AZStd::intrusive_ptr<LUAFindReplaceSettings> findReplaceSettings =
                    AZ::UserSettings::CreateFind<LUAFindReplaceSettings>(
                        AZ_CRC_CE("LUA Editor Find Settings"), AZ::UserSettings::CT_LOCAL);

            findReplaceSettings->SetWrap(checked);

            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequestBus::Events::Save);
        });
        m_menu->QWidget::addAction(wrapFindingAction);
        m_menu->addSeparator();

        auto *scopeGroup = new QActionGroup(this);
        m_currentFileAction = new QAction(tr("Search in Current File"), this);
        m_currentFileAction->setCheckable(true);
        m_currentFileAction->setChecked(true);
        m_currentFileAction->setData(0);
        m_menu->QWidget::addAction(m_currentFileAction);
        scopeGroup->addAction(m_currentFileAction);

        m_openFilesAction = new QAction(tr("Search in All Open Files"), this);
        m_openFilesAction->setCheckable(true);
        m_openFilesAction->setData(1);
        m_menu->QWidget::addAction(m_openFilesAction);
        scopeGroup->addAction(m_openFilesAction);

        m_directoryAction = new QAction(tr("Search in Files..."), this);
        m_directoryAction->setCheckable(true);
        m_directoryAction->setData(2);
        m_menu->QWidget::addAction(m_directoryAction);
        scopeGroup->addAction(m_directoryAction);

        connect(scopeGroup, &QActionGroup::triggered, this, [this](QAction *action) {
            SetSearchScope(action->data().toInt());
        });

        m_menu->addSeparator();

        // Create our new FindResultsWidget for context menu
        m_findResultsWidget = new FindResultsWidget(this);

        // Add the widget to the menu as a QWidgetAction
        auto *widgetAction = new QWidgetAction(this);
        widgetAction->setDefaultWidget(m_findResultsWidget);
        m_menu->QWidget::addAction(widgetAction);

        // Restore the persisted results window selection and persist future changes.
        m_findResultsWidget->SetCurrentResultIndex(m_resultIndex);
        connect(m_findResultsWidget, &FindResultsWidget::ResultIndexChanged, this, [this](int index) {
            m_resultIndex = index;
            AZStd::intrusive_ptr<LUAFindReplaceSettings> findReplaceSettings =
                    AZ::UserSettings::CreateFind<LUAFindReplaceSettings>(
                        AZ_CRC_CE("LUA Editor Find Settings"), AZ::UserSettings::CT_LOCAL);

            findReplaceSettings->SetResultIndex(static_cast<unsigned int>(index));

            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequestBus::Events::Save);
        });
    }

    void LUAEditorFindWidget::SetFindInFilesMode() const {
        m_currentFileAction->setVisible(false);
        m_openFilesAction->setVisible(false);
        m_directoryAction->setVisible(false);
        ui->pushButtonFindPrevious->setVisible(false);
        ui->pushButtonFindNext->setVisible(false);
        ui->pushButtonFindAll->setVisible(false);
    }


    LUAEditorFindWidget::LUAEditorFindWidget(QWidget *parent, class LUAViewWidget *viewWidget)
        : QWidget(parent)
          , ui(azcreate(Ui::LUAEditorFindWidget, ()))
          , m_viewWidget(viewWidget) {
        AZ_TracePrintf("LUAEditorFindWidget", "Constructor called with viewWidget = %p\n", viewWidget);
        ui->setupUi(this);

        const QString toggleButtonStyle = QStringLiteral(
            "QPushButton[checkable=\"true\"] {"
            "  border: 1px solid #555;"
            "  border-radius: 3px;"
            "  padding: 2px;"
            "  background: transparent;"
            "}"
            "QPushButton[checkable=\"true\"]:checked {"
            "  background: #4a90d9;"
            "  border: 1px solid #2a70c9;"
            "  color: white;"
            "}"
        );
        ui->pushButtonCaseSensitive->setStyleSheet(toggleButtonStyle);
        ui->pushButtonWholeWords->setStyleSheet(toggleButtonStyle);
        ui->pushButtonRegEx->setStyleSheet(toggleButtonStyle);

        const QString actionButtonStyle = QStringLiteral(
            "QPushButton {"
            "  border: 1px solid #555;"
            "  border-radius: 3px;"
            "  padding: 2px;"
            "  background: transparent;"
            "}"
            "QPushButton:pressed {"
            "  background: #4a90d9;"
            "  border: 1px solid #2a70c9;"
            "  color: white;"
            "}"
        );
        ui->pushButtonFindNext->setIcon(QIcon(":/general/find_next"));
        ui->pushButtonFindNext->setText(QString());
        ui->pushButtonFindNext->setStyleSheet(actionButtonStyle);
        ui->pushButtonFindPrevious->setIcon(QIcon(":/general/find_previous"));
        ui->pushButtonFindPrevious->setText(QString());
        ui->pushButtonFindPrevious->setStyleSheet(actionButtonStyle);
        ui->pushButtonFindAll->setIcon(QIcon(":/general/find_all"));
        ui->pushButtonFindAll->setText(QString());
        ui->pushButtonFindAll->setStyleSheet(actionButtonStyle);
        ui->pushButtonMenu->setIcon(QIcon(":/general/burger_menu"));
        ui->pushButtonMenu->setText(QString());
        ui->pushButtonMenu->setStyleSheet(actionButtonStyle);
        ui->pushButtonReplaceAll->setIcon(QIcon(":/general/replace_all"));
        ui->pushButtonReplaceAll->setText(QString());
        ui->pushButtonReplaceAll->setStyleSheet(actionButtonStyle);
        ui->pushButtonReplace->setIcon(QIcon(":/general/replace_with"));
        ui->pushButtonReplace->setText(QString());

        AZStd::intrusive_ptr<LUAFindReplaceSettings> findReplaceSettings =
                AZ::UserSettings::CreateFind<LUAFindReplaceSettings>(
                    AZ_CRC_CE("LUA Editor Find Settings"), AZ::UserSettings::CT_LOCAL);

        ui->pushButtonWholeWords->setChecked(findReplaceSettings->GetWholeWords());
        ui->pushButtonCaseSensitive->setChecked(findReplaceSettings->GetCaseSensitive());
        ui->pushButtonRegEx->setChecked(findReplaceSettings->GetRegularExpressions());

        m_wrap = findReplaceSettings->GetWrap();
        m_resultIndex = findReplaceSettings->GetResultIndex();

        ui->pushButtonCaseSensitive->setCheckable(true);
        ui->pushButtonWholeWords->setCheckable(true);

        connect(ui->pushButtonFindNext, &QPushButton::clicked, this, &LUAEditorFindWidget::OnFindNextClicked);
        connect(ui->pushButtonFindPrevious, &QPushButton::clicked, this, &LUAEditorFindWidget::OnFindPreviousClicked);
        connect(ui->pushButtonCaseSensitive, &QPushButton::toggled, this, &LUAEditorFindWidget::OnCaseSensitiveToggled);
        connect(ui->pushButtonWholeWords, &QPushButton::toggled, this, &LUAEditorFindWidget::OnCaseWholeWordsToggled);
        connect(ui->lineEditFindText, &QLineEdit::returnPressed, this, &LUAEditorFindWidget::OnFindTextReturnPressed);
        connect(ui->pushButtonReplace, &QPushButton::clicked, this, &LUAEditorFindWidget::OnReplacedClicked);
        connect(ui->pushButtonFindAll, &QPushButton::clicked, this, &LUAEditorFindWidget::OnFindAllClicked);
        connect(ui->pushButtonReplaceAll, &QPushButton::clicked, this, &LUAEditorFindWidget::OnReplaceAllClicked);

        // TODO This is a hack. The lineEdit widget does not get focus setting it manually. Single shot will fire after a short of time.
        QTimer::singleShot(0, ui->lineEditFindText, SLOT(setFocus()));

        // In case the user selected already a text inside the Editor, use that.
        if (m_viewWidget) {
            ui->lineEditFindText->setText(m_viewWidget->GetSelectedText());
            ui->lineEditFindText->selectAll();
        }

        CreateContextMenu();

        connect(ui->pushButtonMenu, &QPushButton::clicked, [this] {
            QPoint globalPos = ui->pushButtonMenu->mapToGlobal(QPoint(0, 0));
            m_menu->exec(globalPos);
        });
    }

    int LUAEditorFindWidget::GetCurrentResultIndex() {
        if (m_findResultsWidget) {
            return m_findResultsWidget->GetCurrentResultIndex();
        }
        return 0; // default to first option
    }

    LUAEditorFindWidget::~LUAEditorFindWidget() {
        azdestroy(ui);
    }

    void LUAEditorFindWidget::SetCurrentView(class LUAViewWidget *viewWidget) {
        m_viewWidget = viewWidget;
    }

    void LUAEditorFindWidget::SetMainWindow(class LUAEditorMainWindow *mainWindow) {
        pLUAEditorMainWindow = mainWindow;
    }

    void LUAEditorFindWidget::SetSearchText(const QString &text) {
        ui->lineEditFindText->setText(text);
        ui->lineEditFindText->selectAll();
    }

    QString LUAEditorFindWidget::GetSearchText() const {
        return ui->lineEditFindText->text();
    }

    void LUAEditorFindWidget::DisableNavigationButtons() {
        ui->pushButtonFindNext->setEnabled(false);
        ui->pushButtonFindPrevious->setEnabled(false);
    }

    void LUAEditorFindWidget::SyncFindNext() {
        OnFindNextClicked();
    }

    void LUAEditorFindWidget::SyncFindPrevious() {
        OnFindPreviousClicked();
    }

    void LUAEditorFindWidget::SetFocus(bool state) const {
        state ? ui->lineEditFindText->setFocus() : ui->lineEditFindText->clearFocus();
        if (m_viewWidget && state) {
            QString selectedText = m_viewWidget->GetSelectedText();
            if (!selectedText.isEmpty()) {
                ui->lineEditFindText->setText(selectedText);
            }
            ui->lineEditFindText->selectAll();
        }
    }

    void LUAEditorFindWidget::SetShowFind(bool state) {
        ui->pushButtonReplace->hide();
        ui->lineEditReplaceText->hide();
        ui->pushButtonReplaceAll->hide();

        if (m_viewWidget && state) {
            QString selectedText = m_viewWidget->GetSelectedText();
            if (!selectedText.isEmpty()) {
                ui->lineEditFindText->setText(selectedText);
            }
            ui->lineEditFindText->selectAll();
        }

        state ? QWidget::show() : QWidget::hide();
    }

    void LUAEditorFindWidget::SetShowFindAndReplace(bool state) {
        ui->pushButtonReplace->show();
        ui->lineEditReplaceText->show();
        ui->pushButtonReplaceAll->show();

        if (m_viewWidget && state) {
            QString selectedText = m_viewWidget->GetSelectedText();
            if (!selectedText.isEmpty()) {
                ui->lineEditFindText->setText(selectedText);
            }
            ui->lineEditFindText->selectAll();
        }

        if (state) {
            ui->lineEditReplaceText->setFocus();
            ui->lineEditReplaceText->selectAll();
        }

        state ? QWidget::show() : QWidget::hide();
    }

    void LUAEditorFindWidget::FocusSearchInput() {
        ui->lineEditFindText->setFocus();
        ui->lineEditFindText->selectAll();
    }

    void LUAEditorFindWidget::FocusReplaceInput() {
        ui->lineEditReplaceText->setFocus();
        ui->lineEditReplaceText->selectAll();
    }

    void LUAEditorFindWidget::OnFindNextClicked() {
        if (!m_viewWidget) {
            return;
        }

        const LUAViewWidget::FindOperation findOperation =
                m_viewWidget->FindFirst(ui->lineEditFindText->text(),
                                        ui->pushButtonRegEx->isChecked(),
                                        ui->pushButtonCaseSensitive->isChecked(),
                                        ui->pushButtonWholeWords->isChecked(),
                                        m_wrap,
                                        true);
    }

    void LUAEditorFindWidget::OnFindPreviousClicked() {
        if (!m_viewWidget) {
            return;
        }

        const LUAViewWidget::FindOperation findOperation =
                m_viewWidget->FindFirst(ui->lineEditFindText->text(),
                                        ui->pushButtonRegEx->isChecked(),
                                        ui->pushButtonCaseSensitive->isChecked(),
                                        ui->pushButtonWholeWords->isChecked(),
                                        m_wrap,
                                        false);
    }

    void LUAEditorFindWidget::OnCaseSensitiveToggled(bool InToggled) {
        AZStd::intrusive_ptr<LUAFindReplaceSettings> findReplaceSettings =
                AZ::UserSettings::CreateFind<LUAFindReplaceSettings>(
                    AZ_CRC_CE("LUA Editor Find Settings"), AZ::UserSettings::CT_LOCAL);

        findReplaceSettings->SetCaseSensitive(InToggled);

        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequestBus::Events::Save);
    }

    void LUAEditorFindWidget::OnCaseWholeWordsToggled(bool InToggled) {
        AZStd::intrusive_ptr<LUAFindReplaceSettings> findReplaceSettings =
                AZ::UserSettings::CreateFind<LUAFindReplaceSettings>(
                    AZ_CRC_CE("LUA Editor Find Settings"), AZ::UserSettings::CT_LOCAL);

        findReplaceSettings->SetWholeWords(InToggled);

        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequestBus::Events::Save);
    }

    void LUAEditorFindWidget::OnFindTextReturnPressed() {
        OnFindNextClicked();
    }

    void LUAEditorFindWidget::OnReplacedClicked() {
        if (!m_viewWidget) {
            return;
        }

        m_viewWidget->ReplaceSelectedText(ui->lineEditReplaceText->text());
        OnFindNextClicked();
    }

    void LUAEditorFindWidget::CollectMatchesOnLine(const QString &line, const QString &searchText,
                                                   AZStd::vector<AZStd::pair<int, int>> &matches,
                                                   const QRegularExpression *prebuiltRegex) const {
        if (line.isEmpty() || searchText.isEmpty()) {
            return;
        }

        const bool caseSensitive = ui->pushButtonCaseSensitive->isChecked();
        const bool isRegex = ui->pushButtonRegEx->isChecked();
        const bool wholeWords = ui->pushButtonWholeWords->isChecked();

        if (isRegex || wholeWords) {
            // Prefer the expression built once for the whole scan over compiling one per line.
            const QRegularExpression regex = prebuiltRegex ? *prebuiltRegex : BuildSearchRegex(searchText);

            auto matchIt = regex.globalMatch(line);
            while (matchIt.hasNext()) {
                const QRegularExpressionMatch match = matchIt.next();
                matches.push_back(AZStd::make_pair(static_cast<int>(match.capturedStart()),
                                                   static_cast<int>(match.capturedLength())));
            }
        } else {
            const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
            int pos = 0;
            while ((pos = line.indexOf(searchText, pos, cs)) >= 0) {
                matches.push_back(AZStd::make_pair(pos, searchText.length()));
                pos += searchText.length();
            }
        }
    }

    QRegularExpression LUAEditorFindWidget::BuildSearchRegex(const QString &searchText) const {
        const bool isRegex = ui->pushButtonRegEx->isChecked();
        const bool wholeWords = ui->pushButtonWholeWords->isChecked();
        if (!isRegex && !wholeWords) {
            return QRegularExpression();
        }

        QString pattern = isRegex ? searchText : QRegularExpression::escape(searchText);
        if (wholeWords) {
            pattern = "\\b" + pattern + "\\b";
        }
        return QRegularExpression(pattern, ui->pushButtonCaseSensitive->isChecked()
                                              ? QRegularExpression::NoPatternOption
                                              : QRegularExpression::CaseInsensitiveOption);
    }

    void LUAEditorFindWidget::CollectDocumentResults(LUAViewWidget *view) {
        if (!view) {
            return;
        }

        const auto assetFileName(view->m_Info.m_assetName + ".lua");
        const auto qAssetName = assetFileName.c_str();

        // Save current cursor position to restore it later.
        int originalLine = 0;
        int originalIndex = 0;
        view->GetCursorPosition(originalLine, originalIndex);

        // Start search from the beginning of the document without wrapping.
        view->SetCursorPosition(0, 0);

        const QString searchText = ui->lineEditFindText->text();
        AZStd::vector<ResultEntry> entries;

        LUAViewWidget::FindOperation findOperation =
                view->FindFirst(searchText,
                                ui->pushButtonRegEx->isChecked(),
                                ui->pushButtonCaseSensitive->isChecked(),
                                ui->pushButtonWholeWords->isChecked(),
                                false,
                                true);

        // Process all matches in this document. FindNext advances the operation in place.
        while (findOperation) {
            // FindNext/FindFirst moves the cursor to the end of the match.
            // In QTextDocument, if the match ends at the end of a line, the cursor position
            // might be at the start of the next block.
            // We should use the selection's start position to determine the correct line number.
            auto cursor = findOperation.m_impl->m_cursor;
            int matchLine = cursor.blockNumber() + 1;
            if (cursor.hasSelection()) {
                QTextCursor tempCursor = cursor;
                tempCursor.setPosition(cursor.selectionStart());
                matchLine = tempCursor.blockNumber() + 1;
            }

            // One entry per line: consecutive hits on the same line share a single result row.
            if (entries.empty() || entries.back().m_lineNumber != matchLine) {
                ResultEntry entry;
                entry.m_lineNumber = matchLine;
                entry.m_lineText = view->GetLineText(matchLine - 1);
                CollectMatchesOnLine(entry.m_lineText, searchText, entry.m_matches);
                if (entry.m_matches.empty()) {
                    // A regex that spans multiple lines cannot be re-matched on a single line; keep the hit at column 0.
                    entry.m_matches.push_back(AZStd::make_pair(0, 0));
                }
                entries.push_back(std::move(entry));
            }

            view->FindNext(findOperation);
        }

        // Only register documents that actually contain matches (no "0 hit" rows).
        if (!entries.empty()) {
            ResultDocument doc;
            doc.m_assetId = view->m_Info.m_assetId;
            doc.m_entries = std::move(entries);
            m_resultList[qAssetName] = std::move(doc);
        }

        // Restore original cursor position.
        view->SetCursorPosition(originalLine, originalIndex);
    }

    void LUAEditorFindWidget::FindInOpenDocs() {
        if (!pLUAEditorMainWindow) {
            return;
        }

        for (auto *view: pLUAEditorMainWindow->GetAllViews()) {
            CollectDocumentResults(view);
        }
    }

    void LUAEditorFindWidget::FindInCurrentDoc() {
        CollectDocumentResults(m_viewWidget);
    }

    void LUAEditorFindWidget::ScanDirectory(const QString &searchText) {
        if (m_searchDirectory.isEmpty() || searchText.isEmpty()) {
            return;
        }

        // Build the search expression once for the whole scan instead of once per line.
        const bool useRegex = ui->pushButtonRegEx->isChecked() || ui->pushButtonWholeWords->isChecked();
        const QRegularExpression regex = useRegex ? BuildSearchRegex(searchText) : QRegularExpression();

        int resultLines = 0;
        int filesScanned = 0;

        QDirIterator it(m_searchDirectory, QStringList() << "*.lua", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString filePath = it.next();

            // Keep the UI alive (and abortable) while walking a potentially huge directory tree.
            if ((++filesScanned % 32) == 0 && !ScanStep(filePath)) {
                return;
            }

            // Generated API dumps and other very large files would dominate the scan and the result list.
            if (QFileInfo(filePath).size() > s_maxScannedFileSize) {
                continue;
            }

            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }

            AZStd::vector<ResultEntry> entries;
            QTextStream in(&file);
            int lineNumber = 1;
            while (!in.atEnd()) {
                const QString line = in.readLine();
                if (in.status() != QTextStream::Ok) {
                    // Do not spin on a stream that failed to read (broken/corrupt file).
                    break;
                }

                AZStd::vector<AZStd::pair<int, int>> matches;
                CollectMatchesOnLine(line, searchText, matches, useRegex ? &regex : nullptr);
                if (!matches.empty()) {
                    ResultEntry entry;
                    entry.m_lineNumber = lineNumber;
                    entry.m_lineText = line;
                    entry.m_matches = std::move(matches);
                    entries.push_back(std::move(entry));

                    if (++resultLines >= s_maxResultLines) {
                        m_scanTruncated = true;
                        break;
                    }
                }
                lineNumber++;
            }

            // Only register files that actually contain matches (no "0 hit" rows).
            if (!entries.empty()) {
                ResultDocument doc;
                doc.m_assetId = filePath.toUtf8().data(); // Keep original-case path for file I/O
                doc.m_entries = std::move(entries);
                m_resultList[filePath] = std::move(doc);
            }

            if (m_scanTruncated) {
                break;
            }
        }
    }

    bool LUAEditorFindWidget::ScanStep(const QString &currentFile) {
        if (!m_scanProgress) {
            return true;
        }
        if (m_scanProgress->wasCanceled()) {
            m_scanCancelled = true;
            return false;
        }

        m_scanProgress->setLabelText(tr("Searching \"%1\"...").arg(QDir::toNativeSeparators(currentFile)));

        // Pumping events keeps the progress dialog painting. Guard against the widget being
        // destroyed while the nested event loop runs (the dialog is application modal, so this
        // should not normally happen, but the check is cheap).
        QPointer<LUAEditorFindWidget> selfGuard(this);
        QCoreApplication::processEvents();
        if (!selfGuard) {
            return false;
        }

        if (m_scanProgress->wasCanceled()) {
            m_scanCancelled = true;
            return false;
        }
        return true;
    }

    void LUAEditorFindWidget::BeginScanProgress(const QString &searchText) {
        m_scanCancelled = false;
        m_scanTruncated = false;

        if (!m_scanProgress) {
            m_scanProgress = new QProgressDialog(tr("Searching for \"%1\"...").arg(searchText), tr("Cancel"), 0, 0, this);
            m_scanProgress->setWindowTitle(tr("Find in Files"));
            // Application modal so the find dialog cannot be closed from under the running scan.
            m_scanProgress->setWindowModality(Qt::ApplicationModal);
            m_scanProgress->setMinimumDuration(500); // Don't flash for small directories.
            m_scanProgress->setAutoClose(false);
            m_scanProgress->setAutoReset(false);
        } else {
            m_scanProgress->setLabelText(tr("Searching for \"%1\"...").arg(searchText));
        }
    }

    void LUAEditorFindWidget::EndScanProgress() {
        if (m_scanProgress) {
            m_scanProgress->close();
            delete m_scanProgress;
            m_scanProgress = nullptr;
        }
    }

    int LUAEditorFindWidget::PopulateResultsWidget(FindResults *resultsWidget, const QString &searchText) {
        if (!resultsWidget) {
            return 0;
        }

        // Calculate total matches.
        int totalMatches = 0;
        for (auto iter = m_resultList.begin(); iter != m_resultList.end(); ++iter) {
            for (const auto &entry: iter->m_entries) {
                totalMatches += static_cast<int>(entry.m_matches.size());
            }
        }

        // Now populate the results widget with processed data.
        auto doc = resultsWidget->Document();

        const auto setFoldLevel = [&doc](int lineNum, int foldLevel, int depth) {
            QTBlockState blockState;
            auto block = doc->findBlockByNumber(lineNum);
            AZ_Assert(block.isValid(), "should only be setting fold level on a just added line");
            blockState.m_qtBlockState = block.userState();
            blockState.m_blockState.m_uninitialized = 0;
            blockState.m_blockState.m_folded = 0;
            blockState.m_blockState.m_foldLevel = foldLevel;
            blockState.m_blockState.m_syntaxHighlighterState = depth;
            block.setUserState(blockState.m_qtBlockState);
        };

        int currentLine = 0;
        // Use Qt's %n plural forms for proper internationalization.
        // Languages like Russian, Arabic, etc. have more than 2 plural forms.
        QString hits = tr("%n hit(s)", "", totalMatches);
        //: %n is the number of files containing matches
        QString files = tr("%n file(s)", "", static_cast<int>(m_resultList.size()));

        const QString header = m_scanTruncated
                ? tr("Find \"%1\" (%2 in %3, stopped after %4 matches)")
                          .arg(searchText, hits, files)
                          .arg(s_maxResultLines)
                : tr("Find \"%1\" (%2 in %3)").arg(searchText, hits, files);
        resultsWidget->AppendPlainText(header);
        setFoldLevel(currentLine, 0, 0);

        for (auto iter = m_resultList.begin(); iter != m_resultList.end(); ++iter) {
            ++currentLine;
            const QString fileHits = (iter->m_entries.size() == 1) ? "hit" : "hits";
            QString text("\t\"%1\" (%2 %3)");
            text = text.arg(iter.key()).arg(static_cast<int>(iter->m_entries.size())).arg(fileHits);
            resultsWidget->AppendPlainText(text);
            setFoldLevel(currentLine, 1, 1);

            for (const auto &entry: iter->m_entries) {
                ++currentLine;
                QString lineText = "\t\t\tLine %1: %2";
                lineText = lineText.arg(entry.m_lineNumber).arg(entry.m_lineText);
                resultsWidget->AppendPlainText(lineText);
                setFoldLevel(currentLine, 1, 2);

                auto block = doc->findBlockByNumber(currentLine);
                AZ_Assert(block.isValid(), "should only be setting data on a just added line");
                // Entries always carry at least one match position; fall back to column 0 defensively.
                const int column = entry.m_matches.empty() ? 0 : entry.m_matches[0].first;
                auto assignAssetId = [resultsWidget](const AZStd::string &assetName, const AZStd::string &assetId) {
                    resultsWidget->AssignAssetId(assetName, assetId);
                };
                block.setUserData(aznew FindResultsBlockInfo{
                        iter->m_assetId, iter.key().toUtf8().data(), entry.m_lineNumber, column, assignAssetId
                });
            }

            setFoldLevel(currentLine, 0, 2);
        }

        resultsWidget->FinishedAddingText(searchText,
                                          ui->pushButtonRegEx->isChecked(),
                                          ui->pushButtonWholeWords->isChecked(),
                                          ui->pushButtonCaseSensitive->isChecked());

        return totalMatches;
    }

    bool LUAEditorFindWidget::SearchInDirectory(QWidget *parentWidget, FindResults *resultsWidget,
                                                const QString &searchText) {
        Q_UNUSED(parentWidget);
        if (m_searchDirectory.isEmpty() || !resultsWidget || searchText.isEmpty()) {
            return false;
        }

        m_resultList.clear(); // Clear previous results

        // Scanning an arbitrary directory tree can take a long time; keep the editor responsive and
        // let the user abort instead of having to kill the application.
        BeginScanProgress(searchText);
        ScanDirectory(searchText);
        EndScanProgress();

        if (m_scanCancelled) {
            m_resultList.clear();
            return false;
        }

        // Only report success when something was found, so callers don't open an empty results window.
        const int totalMatches = PopulateResultsWidget(resultsWidget, searchText);
        return totalMatches > 0;
    }

    void LUAEditorFindWidget::OnReplaceAllClicked() {
        if (m_replaceAllRunning) {
            return;
        }

        if (!pLUAEditorMainWindow) {
            AZ_TracePrintf("LUAEditorFindWidget", "Error: pLUAEditorMainWindow is null\n");
            return;
        }

        const QString searchText = ui->lineEditFindText->text();
        const QString replaceWithText = ui->lineEditReplaceText->text();

        if (searchText.isEmpty()) {
            AZ_TracePrintf("LUAEditorFindWidget", "Warning: cannot replace all with empty search text\n");
            return;
        }

        m_replaceAllRunning = true;

        const bool caseSensitive = ui->pushButtonCaseSensitive->isChecked();
        const bool wholeWords = ui->pushButtonWholeWords->isChecked();
        const bool regEx = ui->pushButtonRegEx->isChecked();

        AZStd::vector<LUAViewWidget *> targetViews;
        if (m_searchScope == 0) // Current File
        {
            if (m_viewWidget) {
                targetViews.push_back(m_viewWidget);
            }
        } else // All Open Files
        {
            targetViews = pLUAEditorMainWindow->GetAllViews();
        }

        int totalReplacements = 0;

        for (LUAViewWidget *view: targetViews) {
            if (!view) {
                continue;
            }

            AZ_TracePrintf("LUAEditorFindWidget", "Replacing in: %s\n", view->m_Info.m_assetName.c_str());

            view->SetCursorPosition(0, 0);

            // Search forward without wrapping so the text just inserted by a replacement is never
            // re-matched (which would loop forever when the replacement contains the search text).
            int lastMatchStart = -1;
            while (view->FindFirst(searchText, regEx, caseSensitive, wholeWords, false, true)) {
                const int matchStart = view->GetTextCursor().selectionStart();

                // Guard against matches that do not advance the cursor (e.g. a zero-width regex match
                // combined with an empty replacement), which would otherwise loop forever.
                if (matchStart <= lastMatchStart) {
                    break;
                }
                lastMatchStart = matchStart;

                view->ReplaceSelectedText(replaceWithText);
                totalReplacements++;
            }
        }

        AZ_TracePrintf("LUAEditorFindWidget", "Replace All complete: %d replacements\n", totalReplacements);

        m_replaceAllRunning = false;
    }

    void LUAEditorFindWidget::OnSearchWhereChanged(int index) {
        SetSearchScope(index);
    }

    void LUAEditorFindWidget::SyncScopeActions() const {
        if (m_currentFileAction) {
            m_currentFileAction->setChecked(m_searchScope == 0);
        }
        if (m_openFilesAction) {
            m_openFilesAction->setChecked(m_searchScope == 1);
        }
        if (m_directoryAction) {
            m_directoryAction->setChecked(m_searchScope == 2);
        }
    }

    void LUAEditorFindWidget::SetSearchScope(int scope) {
        const int previousScope = m_searchScope;
        m_searchScope = scope;

        if (m_searchScope == 2) // Directory
        {
            QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory to Search"),
                                                            m_searchDirectory.isEmpty()
                                                                    ? AZ::Utils::GetProjectPath().c_str()
                                                                    : m_searchDirectory);
            if (!dir.isEmpty()) {
                m_searchDirectory = dir;
            } else {
                // The user cancelled the folder picker: revert so we don't stay on "Search in Files..." with no directory.
                m_searchScope = previousScope;
            }
        }

        SyncScopeActions();

        if (m_searchScope == 0) // Current File
        {
            ui->pushButtonFindNext->setEnabled(true);
            ui->pushButtonFindPrevious->setEnabled(true);
        } else // All Open Files or Directory
        {
            ui->pushButtonFindNext->setEnabled(false);
            ui->pushButtonFindPrevious->setEnabled(false);
        }
    }

    void LUAEditorFindWidget::OnFindAllClicked() {
        // Check if we have valid pointers, if not try to get them from the view
        if (!m_viewWidget) {
            AZ_TracePrintf("LUAEditorFindWidget", "Error: m_viewWidget is null\n");
            return;
        }

        // If main window is null we cannot populate a results widget, so bail out.
        if (!pLUAEditorMainWindow) {
            AZ_TracePrintf("LUAEditorFindWidget", "Error: pLUAEditorMainWindow is null\n");
            return;
        }

        const QString searchText = ui->lineEditFindText->text();
        if (searchText.isEmpty()) {
            AZ_TracePrintf("LUAEditorFindWidget", "Search text is empty - returning early\n");
            return;
        }

        // Get the current result index from the FindResultsWidget context menu.
        const int widgetIndex = GetCurrentResultIndex();

        pLUAEditorMainWindow->SetCurrentFindListWidget(widgetIndex);
        FindResults *const resultsWidget = pLUAEditorMainWindow->GetFindResultsWidget(widgetIndex);

        if (!resultsWidget) {
            AZ_TracePrintf("LUAEditorFindWidget", "Error: GetFindResultsWidget returned null for index %d\n",
                           widgetIndex);
            return;
        }

        m_resultList.clear(); // Clear previous results before re-scanning.
        m_scanTruncated = false; // Only a directory scan can truncate; reset for the open-document scopes.

        if (m_searchScope == 0) // Current File
        {
            FindInCurrentDoc();
        } else if (m_searchScope == 1) // All Open Files
        {
            FindInOpenDocs();
        } else if (m_searchScope == 2) // Directory
        {
            BeginScanProgress(searchText);
            ScanDirectory(searchText);
            EndScanProgress();
            if (m_scanCancelled) {
                m_resultList.clear();
                return;
            }
        }
        // else if (m_searchScope == 3) { FindInAllAssets(); }

        if (m_resultList.isEmpty()) {
            AZ_TracePrintf("LUAEditorFindWidget", "No matches found for \"%s\" - keeping previous results\n",
                           searchText.toUtf8().constData());
            return;
        }

        resultsWidget->Clear();
        pLUAEditorMainWindow->ResetSearchClicks();

        PopulateResultsWidget(resultsWidget, searchText);

        // Show and activate the tab widget using the main window's method.
        auto *const luaFindTabWidget = pLUAEditorMainWindow->GetFindTabWidget();
        AZ_Assert(luaFindTabWidget, "LUAEditorMainWindow cant find the FindTabWidget!");

        // Show and activate the tab widget.
        luaFindTabWidget->show();
        luaFindTabWidget->raise();
        luaFindTabWidget->activateWindow();
        luaFindTabWidget->setFocus();

        // Open the find view matching the results widget we just populated.
        pLUAEditorMainWindow->OnOpenFindView(widgetIndex);
    }
}
