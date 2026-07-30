#include "LUAEditorFindWidget.h"
#include "AzCore/UserSettings/UserSettingsComponent.h"
#include "LUAEditorView.hxx"
#include "LUAEditorFindResults.hxx"
#include "Source/LUA/ui_LUAEditorFindWidget.h"

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

    class LUAFindReplaceSettings
            : public AZ::UserSettings {
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

        class LUAFindReplaceSerializationEvents : public AZ::SerializeContext::IEventHandler {
            void OnReadEnd(void *classPtr) override {
                // if(auto* settings = static_cast<LUAFindReplaceSettings*>(classPtr))
                // {
                //
                //     qInfo() << "Finished reading" << settings.;
                // }
            }
        };

    private:
        bool m_wrap = true;
        bool m_caseSensitive = false;
        bool m_regularExpressions = false;
        bool m_wholeWords = false;
        unsigned int m_resultIndex = 0;
    };

    void LUAFindReplaceSettings::Reflect(AZ::ReflectContext *reflection) {
        AZ::SerializeContext *serializeContext = azrtti_cast<AZ::SerializeContext *>(reflection);
        if (serializeContext) {
            serializeContext->Class<LUAFindReplaceSettings, AZ::UserSettings>()
                    ->Version(1)
                    ->EventHandler<LUAFindReplaceSerializationEvents>()
                    ->Field("m_wrap", &LUAFindReplaceSettings::m_wrap)
                    ->Field("m_caseSensitive", &LUAFindReplaceSettings::m_caseSensitive)
                    ->Field("m_regularExpressions", &LUAFindReplaceSettings::m_regularExpressions)
                    ->Field("m_wholeWords", &LUAFindReplaceSettings::m_wholeWords)
                    ->Field("m_resultIndex", &LUAFindReplaceSettings::m_resultIndex);

            AZ::EditContext *editContext = serializeContext->GetEditContext();
            if (editContext) {
                editContext->Class<LUAFindReplaceSettings>("Syntax Colors",
                                                           "Customize the Lua IDE syntax and interface colors.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Font")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

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

            findReplaceSettings->SetCaseSensitive(checked);

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

    void LUAEditorFindWidget::FindInOpenDocs() {
        for (auto *view: pLUAEditorMainWindow->GetAllViews()) {
            auto assetFileName(view->m_Info.m_assetName + ".lua");
            auto qAssetName = assetFileName.c_str();

            // Save current cursor position to restore it later
            int originalLine = 0;
            int originalIndex = 0;
            view->GetCursorPosition(originalLine, originalIndex);

            // Start search from the beginning of the document
            view->SetCursorPosition(0, 0);

            // Check if we've already processed this file
            if (m_resultList.find(qAssetName) == m_resultList.end()) {
                ResultDocument doc;
                doc.m_assetId = view->m_Info.m_assetId;
                m_resultList[qAssetName] = doc;
            }

            // Find all matches in this document
            LUAViewWidget::FindOperation findOperation =
                    view->FindFirst(ui->lineEditFindText->text(),
                                    ui->pushButtonRegEx->isChecked(),
                                    ui->pushButtonCaseSensitive->isChecked(),
                                    ui->pushButtonWholeWords->isChecked(),
                                    false,
                                    true);

            // Process all matches in this document
            while (findOperation) {
                int line = 0;
                int index = 0;
                view->GetCursorPosition(line, index);

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

                ResultEntry entry;
                entry.m_lineNumber = matchLine;
                entry.m_lineText = view->GetLineText(matchLine - 1);

                // Find all matches in the line if regex or whole word mode is used
                if (ui->pushButtonRegEx->isChecked() || ui->pushButtonWholeWords->isChecked()) {
                    QRegularExpression regex(
                        ui->lineEditFindText->text(),
                        ui->pushButtonCaseSensitive->isChecked()
                            ? QRegularExpression::NoPatternOption
                            : QRegularExpression::CaseInsensitiveOption);

                    QRegularExpressionMatch match = regex.match(entry.m_lineText);
                    while (match.hasMatch()) {
                        const int length = static_cast<int>(match.capturedLength());
                        entry.m_matches.push_back(AZStd::make_pair(static_cast<int>(match.capturedStart()), length));
                        match = regex.match(entry.m_lineText, match.capturedStart() + length);
                    }
                } else {
                    // Regular search
                    int pos = 0;
                    while (pos >= 0) {
                        pos = entry.m_lineText.indexOf(ui->lineEditFindText->text(), pos,
                                                       ui->pushButtonCaseSensitive->isChecked()
                                                           ? Qt::CaseSensitive
                                                           : Qt::CaseInsensitive);

                        if (pos >= 0) {
                            entry.m_matches.push_back(
                                AZStd::make_pair(pos, static_cast<int>(ui->lineEditFindText->text().length())));
                            pos++;
                        }
                    }
                }

                // Add to results list
                m_resultList[qAssetName].m_entries.push_back(entry);

                view->FindNext(findOperation);
            }

            // Restore original cursor position
            view->SetCursorPosition(originalLine, originalIndex);
        }
    }

    void LUAEditorFindWidget::FindInCurrentDoc() {
        auto *view = m_viewWidget;
        if (!view) {
            return;
        }

        auto assetFileName(view->m_Info.m_assetName + ".lua");
        auto qAssetName = assetFileName.c_str();

        // Save current cursor position to restore it later
        int originalLine = 0;
        int originalIndex = 0;
        view->GetCursorPosition(originalLine, originalIndex);

        // Start search from the beginning of the document
        view->SetCursorPosition(0, 0);

        ResultDocument doc;
        doc.m_assetId = view->m_Info.m_assetId;
        m_resultList[qAssetName] = doc;

        // Find all matches in this document
        LUAViewWidget::FindOperation findOperation =
                view->FindFirst(ui->lineEditFindText->text(),
                                ui->pushButtonRegEx->isChecked(),
                                ui->pushButtonCaseSensitive->isChecked(),
                                ui->pushButtonWholeWords->isChecked(),
                                false,
                                true);

        // Process all matches in this document
        while (findOperation) {
            int line = 0;
            int index = 0;
            view->GetCursorPosition(line, index);

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

            ResultEntry entry;
            entry.m_lineNumber = matchLine;
            entry.m_lineText = view->GetLineText(matchLine - 1);

            // Find all matches in the line if regex or whole word mode is used
            if (ui->pushButtonRegEx->isChecked() || ui->pushButtonWholeWords->isChecked()) {
                QRegularExpression regex(
                    ui->lineEditFindText->text(),
                    ui->pushButtonCaseSensitive->isChecked()
                        ? QRegularExpression::NoPatternOption
                        : QRegularExpression::CaseInsensitiveOption);

                QRegularExpressionMatch match = regex.match(entry.m_lineText);
                while (match.hasMatch()) {
                    const int length = static_cast<int>(match.capturedLength());
                    entry.m_matches.push_back(AZStd::make_pair(static_cast<int>(match.capturedStart()), length));
                    match = regex.match(entry.m_lineText, match.capturedStart() + length);
                }
            } else {
                // Regular search
                int pos = 0;
                while (pos >= 0) {
                    pos = entry.m_lineText.indexOf(ui->lineEditFindText->text(), pos,
                                                   ui->pushButtonCaseSensitive->isChecked()
                                                       ? Qt::CaseSensitive
                                                       : Qt::CaseInsensitive);

                    if (pos >= 0) {
                        entry.m_matches.push_back(
                            AZStd::make_pair(pos, static_cast<int>(ui->lineEditFindText->text().length())));
                        pos++;
                    }
                }
            }

            // Add to results list
            m_resultList[qAssetName].m_entries.push_back(entry);

            view->FindNext(findOperation);
        }

        // Restore original cursor position
        view->SetCursorPosition(originalLine, originalIndex);
    }

    void LUAEditorFindWidget::FindInDirectory() {
        if (m_searchDirectory.isEmpty()) {
            return;
        }

        const auto searchText = ui->lineEditFindText->text();
        const auto caseSensitive = ui->pushButtonCaseSensitive->isChecked();
        const auto isRegex = ui->pushButtonRegEx->isChecked();
        const auto wholeWords = ui->pushButtonWholeWords->isChecked();

        auto cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        QRegularExpression regex;
        if (isRegex || wholeWords) {
            QString pattern = isRegex ? searchText : QRegularExpression::escape(searchText);
            if (wholeWords) {
                pattern = "\\b" + pattern + "\\b";
            }
            regex.setPattern(pattern);
            if (!caseSensitive) {
                regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
            }
        }

        QDirIterator it(m_searchDirectory, QStringList() << "*.lua", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }

            ResultDocument doc;
            doc.m_assetId = filePath.toUtf8().data(); // Use full path as assetId for now

            QTextStream in(&file);
            int lineNumber = 1;
            while (!in.atEnd()) {
                QString line = in.readLine();
                ResultEntry entry;
                entry.m_lineNumber = lineNumber;
                entry.m_lineText = line;

                if (isRegex || wholeWords) {
                    QRegularExpressionMatchIterator matchIt = regex.globalMatch(line);
                    while (matchIt.hasNext()) {
                        QRegularExpressionMatch match = matchIt.next();
                        entry.m_matches.push_back(AZStd::make_pair(static_cast<int>(match.capturedStart()),
                                                                   static_cast<int>(match.capturedLength())));
                    }
                } else {
                    int pos = 0;
                    while (pos >= 0) {
                        pos = line.indexOf(searchText, pos, cs);
                        if (pos >= 0) {
                            entry.m_matches.push_back(AZStd::make_pair(pos, searchText.length()));
                            pos += searchText.length();
                        }
                    }
                }

                if (!entry.m_matches.empty()) {
                    if (!m_resultList.contains(filePath)) {
                        m_resultList[filePath] = doc;
                    }
                    m_resultList[filePath].m_entries.push_back(entry);
                }
                lineNumber++;
            }
        }
    }

    bool LUAEditorFindWidget::SearchInDirectory(QWidget *parentWidget, FindResults *resultsWidget,
                                                const QString &searchText) {
        if (m_searchDirectory.isEmpty() || !resultsWidget) {
            return false;
        }

        m_resultList.clear();

        const auto caseSensitive = ui->pushButtonCaseSensitive->isChecked();
        const auto isRegex = ui->pushButtonRegEx->isChecked();
        const auto wholeWords = ui->pushButtonWholeWords->isChecked();

        auto cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        QRegularExpression regex;
        if (isRegex || wholeWords) {
            QString pattern = isRegex ? searchText : QRegularExpression::escape(searchText);
            if (wholeWords) {
                pattern = "\\b" + pattern + "\\b";
            }
            regex.setPattern(pattern);
            if (!caseSensitive) {
                regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
            }
        }

        QDirIterator it(m_searchDirectory, QStringList() << "*.lua", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }

            ResultDocument doc;
            doc.m_assetId = filePath.toUtf8().data(); // Keep original-case path for file I/O

            QTextStream in(&file);
            int lineNumber = 1;
            while (!in.atEnd()) {
                QString line = in.readLine();
                ResultEntry entry;
                entry.m_lineNumber = lineNumber;
                entry.m_lineText = line;

                if (isRegex || wholeWords) {
                    QRegularExpressionMatchIterator matchIt = regex.globalMatch(line);
                    while (matchIt.hasNext()) {
                        QRegularExpressionMatch match = matchIt.next();
                        entry.m_matches.push_back(AZStd::make_pair(static_cast<int>(match.capturedStart()),
                                                                   static_cast<int>(match.capturedLength())));
                    }
                } else {
                    int pos = 0;
                    while (pos >= 0) {
                        pos = line.indexOf(searchText, pos, cs);
                        if (pos >= 0) {
                            entry.m_matches.push_back(AZStd::make_pair(pos, searchText.length()));
                            pos += searchText.length();
                        }
                    }
                }

                if (!entry.m_matches.empty()) {
                    if (!m_resultList.contains(filePath)) {
                        m_resultList[filePath] = doc;
                    }
                    m_resultList[filePath].m_entries.push_back(entry);
                }
                lineNumber++;
            }
        }

        int totalMatches = 0;
        for (auto iter = m_resultList.begin(); iter != m_resultList.end(); ++iter) {
            for (auto &entry: iter->m_entries) {
                totalMatches += entry.m_matches.size();
            }
        }

        auto doc = resultsWidget->Document();

        auto setFoldLevel = [&doc](int lineNum, int foldLevel, int depth) {
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
        QString hits = tr("%n hit(s)", "", totalMatches);
        QString files = tr("%n file(s)", "", static_cast<int>(m_resultList.size()));
        QString header = tr("Find \"%1\" (%2 in %3)")
                .arg(searchText, hits, files);
        resultsWidget->AppendPlainText(header);
        setFoldLevel(currentLine, 0, 0);

        for (auto iter = m_resultList.begin(); iter != m_resultList.end(); ++iter) {
            ++currentLine;
            QString fileHits = (iter->m_entries.size() == 1) ? "hit" : "hits";
            QString text("\t\"%1\" (%2 %3)");
            text = text.arg(iter.key()).arg(iter->m_entries.size()).arg(fileHits);
            resultsWidget->AppendPlainText(text);
            setFoldLevel(currentLine, 1, 1);

            for (auto &entry: iter->m_entries) {
                ++currentLine;
                QString lineText = "\t\t\tLine %1: %2";
                lineText = lineText.arg(entry.m_lineNumber).arg(entry.m_lineText);
                resultsWidget->AppendPlainText(lineText);

                if (entry.m_matches.empty()) {
                    setFoldLevel(currentLine, 1, 2);
                    auto block = doc->findBlockByNumber(currentLine);
                    AZ_Assert(block.isValid(), "should only be setting data on a just added line");
                    auto assignAssetId = [resultsWidget](const AZStd::string &assetName, const AZStd::string &assetId) {
                        resultsWidget->AssignAssetId(assetName, assetId);
                    };
                    block.setUserData(aznew FindResultsBlockInfo{
                        iter->m_assetId, iter.key().toUtf8().data(), entry.m_lineNumber, 0, assignAssetId
                    });
                } else {
                    setFoldLevel(currentLine, 1, 2);
                    auto block = doc->findBlockByNumber(currentLine);
                    AZ_Assert(block.isValid(), "should only be setting data on a just added line");
                    auto assignAssetId = [resultsWidget](const AZStd::string &assetName, const AZStd::string &assetId) {
                        resultsWidget->AssignAssetId(assetName, assetId);
                    };
                    block.setUserData(aznew FindResultsBlockInfo{
                        iter->m_assetId, iter.key().toUtf8().data(), entry.m_lineNumber, entry.m_matches[0].first,
                        assignAssetId
                    });
                }
            }

            setFoldLevel(currentLine, 0, 2);
        }

        resultsWidget->FinishedAddingText(searchText, isRegex, wholeWords, caseSensitive);

        return true;
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

            const int advance = static_cast<int>(replaceWithText.size());
            int firstFoundLine = 0;
            int firstFoundIndex = 0;

            if (view->FindFirst(searchText, regEx, caseSensitive, wholeWords, true, true)) {
                view->GetCursorPosition(firstFoundLine, firstFoundIndex);
                view->ReplaceSelectedText(replaceWithText);
                totalReplacements++;

                while (view->FindFirst(searchText, regEx, caseSensitive, wholeWords, true, true)) {
                    int startLine = 0;
                    int startIndex = 0;
                    view->GetCursorPosition(startLine, startIndex);

                    if (startLine == firstFoundLine && startIndex == firstFoundIndex) {
                        break;
                    }

                    view->ReplaceSelectedText(replaceWithText);
                    view->GetCursorPosition(startLine, startIndex);
                    view->SetCursorPosition(startLine, startIndex + advance);

                    totalReplacements++;
                }
            }
        }

        AZ_TracePrintf("LUAEditorFindWidget", "Replace All complete: %d replacements\n", totalReplacements);

        m_replaceAllRunning = false;
    }

    void LUAEditorFindWidget::OnSearchWhereChanged(int index) {
        SetSearchScope(index);
    }

    void LUAEditorFindWidget::SetSearchScope(int scope) {
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
                // If user cancelled, maybe we should revert the scope?
                // But the action is already checked. For now, just keep it.
            }
        }

        if (m_searchScope == 0) // Current File
        {
            ui->pushButtonFindNext->setEnabled(true);
            ui->pushButtonFindPrevious->setEnabled(true);
        } else // All Open Files or All LUA Assets
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

        // If main window is null, try to get it from the view widget (if it has a reference)
        if (!pLUAEditorMainWindow) {
            AZ_TracePrintf("LUAEditorFindWidget", "Warning: pLUAEditorMainWindow is null, trying to get from view\n");
        }

        // Get the current result index from the FindResultsWidget context menu
        const int widgetIndex = GetCurrentResultIndex();

        // Validate that we have a valid main window and results widget before proceeding
        pLUAEditorMainWindow->SetCurrentFindListWidget(widgetIndex);
        FindResults *const resultsWidget = pLUAEditorMainWindow->GetFindResultsWidget(widgetIndex);

        if (!resultsWidget) {
            AZ_TracePrintf("LUAEditorFindWidget", "Error: GetFindResultsWidget returned null for index %d\n",
                           widgetIndex);
            return;
        }

        resultsWidget->Clear();
        pLUAEditorMainWindow->ResetSearchClicks();

        if (!ui->lineEditFindText->text().isEmpty()) {
            m_resultList.clear(); // Clear previous results

            if (m_searchScope == 0) // Current File
            {
                FindInCurrentDoc();
            } else if (m_searchScope == 1) // All Open Files
            {
                FindInOpenDocs();
            } else if (m_searchScope == 2) // Directory
            {
                FindInDirectory();
            }
            // else if (m_searchScope == 2) { FindInAllAssets(); }

            // Calculate total matches
            int totalMatches = 0;
            for (auto iter = m_resultList.begin(); iter != m_resultList.end(); ++iter) {
                for (auto &entry: iter->m_entries) {
                    totalMatches += entry.m_matches.size();
                }
            }

            // Now populate the results widget with processed data
            auto doc = resultsWidget->Document();

            auto setFoldLevel = [&doc](int lineNum, int foldLevel, int depth) {
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

            QString header = tr("Find \"%1\" (%2 in %3)")
                    .arg(ui->lineEditFindText->text(), hits, files);
            resultsWidget->AppendPlainText(header);
            setFoldLevel(currentLine, 0, 0);

            for (auto iter = m_resultList.begin(); iter != m_resultList.end(); ++iter) {
                ++currentLine;
                if (iter->m_entries.size() == 1) {
                    hits = "hit";
                } else {
                    hits = "hits";
                }

                QString text("\t\"%1\" (%2 %3)");
                text = text.arg(iter.key()).arg(iter->m_entries.size()).arg(hits);
                resultsWidget->AppendPlainText(text);
                setFoldLevel(currentLine, 1, 1);

                for (auto &entry: iter->m_entries) {
                    ++currentLine;
                    text = "\t\t\tLine %1: %2";
                    text = text.arg(entry.m_lineNumber).arg(entry.m_lineText);
                    resultsWidget->AppendPlainText(text);

                    // Safety check - ensure we have at least one match before accessing
                    if (entry.m_matches.empty()) {
                        // Use a default position value instead of crashing
                        setFoldLevel(currentLine, 1, 2);

                        auto block = doc->findBlockByNumber(currentLine);
                        AZ_Assert(block.isValid(), "should only be setting data on a just added line");
                        auto assignAssetId = [resultsWidget](const AZStd::string &assetName,
                                                             const AZStd::string &assetId) {
                            resultsWidget->AssignAssetId(assetName, assetId);
                        };
                        block.setUserData(aznew FindResultsBlockInfo{
                            iter->m_assetId, iter.key().toUtf8().data(), entry.m_lineNumber,
                            0, assignAssetId
                        });
                    } else {
                        AZ_Assert(!entry.m_matches.empty(),
                                  "shouldn't be an entry at all if there wasn't at least one match");
                        setFoldLevel(currentLine, 1, 2);

                        auto block = doc->findBlockByNumber(currentLine);
                        AZ_Assert(block.isValid(), "should only be setting data on a just added line");
                        auto assignAssetId = [resultsWidget](const AZStd::string &assetName,
                                                             const AZStd::string &assetId) {
                            resultsWidget->AssignAssetId(assetName, assetId);
                        };
                        block.setUserData(aznew FindResultsBlockInfo{
                            iter->m_assetId, iter.key().toUtf8().data(), entry.m_lineNumber,
                            entry.m_matches[0].first, assignAssetId
                        });
                    }
                }

                setFoldLevel(currentLine, 0, 2);
            }

            resultsWidget->FinishedAddingText(ui->lineEditFindText->text(),
                                              ui->pushButtonRegEx->isChecked(),
                                              ui->pushButtonWholeWords->isChecked(),
                                              ui->pushButtonCaseSensitive->isChecked()
            );

            // Show and activate the tab widget using the main window's method
            auto *const luaFindTabWidget = pLUAEditorMainWindow->GetFindTabWidget();
            AZ_Assert(luaFindTabWidget, "LUAEditorMainWindow cant find the FindTabWidget!");

            // Show and activate the tab widget
            luaFindTabWidget->show();
            luaFindTabWidget->raise();
            luaFindTabWidget->activateWindow();
            luaFindTabWidget->setFocus();

            // Open the specific find view - using default index 0
            pLUAEditorMainWindow->OnOpenFindView(0);
        } else {
            AZ_TracePrintf("LUAEditorFindWidget", "Search text is empty - returning early\n");
        }
    }
}
