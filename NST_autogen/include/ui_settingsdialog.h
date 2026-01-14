/********************************************************************************
** Form generated from reading UI file 'settingsdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.10.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETTINGSDIALOG_H
#define UI_SETTINGSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SettingsDialog
{
public:
    QHBoxLayout *horizontalLayout_main;
    QListWidget *settingsListWidget;
    QFrame *frame_content;
    QVBoxLayout *verticalLayout_content;
    QStackedWidget *configStackedWidget;
    QWidget *page_general;
    QVBoxLayout *verticalLayout_general;
    QGroupBox *generalGroupBox;
    QFormLayout *formLayout_general;
    QLabel *sourceLanguageLabel;
    QComboBox *sourceLanguageComboBox;
    QLabel *targetLanguageLabel;
    QComboBox *targetLanguageComboBox;
    QLabel *activeModeLabel;
    QComboBox *translatorModeComboBox;
    QLabel *label_enable_ai;
    QCheckBox *enableAiFilterCheckBox;
    QSpacerItem *verticalSpacer_general;
    QWidget *page_translation;
    QVBoxLayout *verticalLayout_translation;
    QScrollArea *scrollArea_translation;
    QWidget *scrollAreaWidgetContents_translation;
    QVBoxLayout *verticalLayout_scroll_translation;
    QGroupBox *googleProGroupBox;
    QFormLayout *formLayout_professional;
    QLabel *googleApiKeyLabel;
    QLineEdit *googleApiKeyEdit;
    QGroupBox *llmProviderGroup;
    QFormLayout *formLayout_ai_powered;
    QLabel *llmProviderLabel;
    QComboBox *llmProviderComboBox;
    QLabel *llmApiKeyLabel;
    QLineEdit *llmApiKeyEdit;
    QLabel *llmModelLabel;
    QComboBox *llmModelComboBox;
    QGroupBox *llmAdvancedGroupBox;
    QFormLayout *formLayout_advanced;
    QLabel *llmBaseUrlLabel;
    QLineEdit *llmBaseUrlEdit;
    QSpacerItem *verticalSpacer_translation;
    QWidget *page_ai_ui;
    QVBoxLayout *verticalLayout_ai_ui;
    QGroupBox *aiFilterDetailsGroupBox;
    QFormLayout *formLayout_ai_settings;
    QLabel *label_ai_sensitivity;
    QDoubleSpinBox *aiFilterThresholdSpinBox;
    QLabel *aiInfoLabel;
    QGroupBox *visualsGroupBox;
    QVBoxLayout *verticalLayout_visuals;
    QCheckBox *enableRelationsCheckBox;
    QSpacerItem *verticalSpacer_ai_ui;
    QWidget *page_plugins;
    QHBoxLayout *horizontalLayout_plugins;
    QListWidget *pluginListWidget;
    QVBoxLayout *verticalLayout_plugins_right;
    QCheckBox *pluginEnabledCheckBox;
    QScrollArea *pluginSettingsScrollArea;
    QWidget *pluginSettingsContainer;
    QFormLayout *formLayout_plugins;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *SettingsDialog)
    {
        if (SettingsDialog->objectName().isEmpty())
            SettingsDialog->setObjectName("SettingsDialog");
        SettingsDialog->resize(750, 550);
        horizontalLayout_main = new QHBoxLayout(SettingsDialog);
        horizontalLayout_main->setSpacing(0);
        horizontalLayout_main->setObjectName("horizontalLayout_main");
        horizontalLayout_main->setContentsMargins(0, 0, 0, 0);
        settingsListWidget = new QListWidget(SettingsDialog);
        new QListWidgetItem(settingsListWidget);
        new QListWidgetItem(settingsListWidget);
        new QListWidgetItem(settingsListWidget);
        new QListWidgetItem(settingsListWidget);
        settingsListWidget->setObjectName("settingsListWidget");
        settingsListWidget->setMaximumSize(QSize(180, 16777215));
        settingsListWidget->setFrameShape(QFrame::NoFrame);

        horizontalLayout_main->addWidget(settingsListWidget);

        frame_content = new QFrame(SettingsDialog);
        frame_content->setObjectName("frame_content");
        frame_content->setFrameShape(QFrame::StyledPanel);
        frame_content->setFrameShadow(QFrame::Raised);
        verticalLayout_content = new QVBoxLayout(frame_content);
        verticalLayout_content->setSpacing(12);
        verticalLayout_content->setObjectName("verticalLayout_content");
        verticalLayout_content->setContentsMargins(12, 12, 12, 12);
        configStackedWidget = new QStackedWidget(frame_content);
        configStackedWidget->setObjectName("configStackedWidget");
        page_general = new QWidget();
        page_general->setObjectName("page_general");
        verticalLayout_general = new QVBoxLayout(page_general);
        verticalLayout_general->setObjectName("verticalLayout_general");
        generalGroupBox = new QGroupBox(page_general);
        generalGroupBox->setObjectName("generalGroupBox");
        formLayout_general = new QFormLayout(generalGroupBox);
        formLayout_general->setObjectName("formLayout_general");
        formLayout_general->setHorizontalSpacing(15);
        formLayout_general->setVerticalSpacing(10);
        sourceLanguageLabel = new QLabel(generalGroupBox);
        sourceLanguageLabel->setObjectName("sourceLanguageLabel");

        formLayout_general->setWidget(0, QFormLayout::ItemRole::LabelRole, sourceLanguageLabel);

        sourceLanguageComboBox = new QComboBox(generalGroupBox);
        sourceLanguageComboBox->setObjectName("sourceLanguageComboBox");

        formLayout_general->setWidget(0, QFormLayout::ItemRole::FieldRole, sourceLanguageComboBox);

        targetLanguageLabel = new QLabel(generalGroupBox);
        targetLanguageLabel->setObjectName("targetLanguageLabel");

        formLayout_general->setWidget(1, QFormLayout::ItemRole::LabelRole, targetLanguageLabel);

        targetLanguageComboBox = new QComboBox(generalGroupBox);
        targetLanguageComboBox->setObjectName("targetLanguageComboBox");

        formLayout_general->setWidget(1, QFormLayout::ItemRole::FieldRole, targetLanguageComboBox);

        activeModeLabel = new QLabel(generalGroupBox);
        activeModeLabel->setObjectName("activeModeLabel");

        formLayout_general->setWidget(2, QFormLayout::ItemRole::LabelRole, activeModeLabel);

        translatorModeComboBox = new QComboBox(generalGroupBox);
        translatorModeComboBox->addItem(QString());
        translatorModeComboBox->addItem(QString());
        translatorModeComboBox->addItem(QString());
        translatorModeComboBox->addItem(QString());
        translatorModeComboBox->setObjectName("translatorModeComboBox");

        formLayout_general->setWidget(2, QFormLayout::ItemRole::FieldRole, translatorModeComboBox);

        label_enable_ai = new QLabel(generalGroupBox);
        label_enable_ai->setObjectName("label_enable_ai");

        formLayout_general->setWidget(3, QFormLayout::ItemRole::LabelRole, label_enable_ai);

        enableAiFilterCheckBox = new QCheckBox(generalGroupBox);
        enableAiFilterCheckBox->setObjectName("enableAiFilterCheckBox");

        formLayout_general->setWidget(3, QFormLayout::ItemRole::FieldRole, enableAiFilterCheckBox);


        verticalLayout_general->addWidget(generalGroupBox);

        verticalSpacer_general = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_general->addItem(verticalSpacer_general);

        configStackedWidget->addWidget(page_general);
        page_translation = new QWidget();
        page_translation->setObjectName("page_translation");
        verticalLayout_translation = new QVBoxLayout(page_translation);
        verticalLayout_translation->setObjectName("verticalLayout_translation");
        scrollArea_translation = new QScrollArea(page_translation);
        scrollArea_translation->setObjectName("scrollArea_translation");
        scrollArea_translation->setWidgetResizable(true);
        scrollArea_translation->setFrameShape(QFrame::NoFrame);
        scrollAreaWidgetContents_translation = new QWidget();
        scrollAreaWidgetContents_translation->setObjectName("scrollAreaWidgetContents_translation");
        scrollAreaWidgetContents_translation->setGeometry(QRect(0, 0, 100, 100));
        verticalLayout_scroll_translation = new QVBoxLayout(scrollAreaWidgetContents_translation);
        verticalLayout_scroll_translation->setObjectName("verticalLayout_scroll_translation");
        googleProGroupBox = new QGroupBox(scrollAreaWidgetContents_translation);
        googleProGroupBox->setObjectName("googleProGroupBox");
        formLayout_professional = new QFormLayout(googleProGroupBox);
        formLayout_professional->setObjectName("formLayout_professional");
        googleApiKeyLabel = new QLabel(googleProGroupBox);
        googleApiKeyLabel->setObjectName("googleApiKeyLabel");

        formLayout_professional->setWidget(0, QFormLayout::ItemRole::LabelRole, googleApiKeyLabel);

        googleApiKeyEdit = new QLineEdit(googleProGroupBox);
        googleApiKeyEdit->setObjectName("googleApiKeyEdit");

        formLayout_professional->setWidget(0, QFormLayout::ItemRole::FieldRole, googleApiKeyEdit);


        verticalLayout_scroll_translation->addWidget(googleProGroupBox);

        llmProviderGroup = new QGroupBox(scrollAreaWidgetContents_translation);
        llmProviderGroup->setObjectName("llmProviderGroup");
        formLayout_ai_powered = new QFormLayout(llmProviderGroup);
        formLayout_ai_powered->setObjectName("formLayout_ai_powered");
        llmProviderLabel = new QLabel(llmProviderGroup);
        llmProviderLabel->setObjectName("llmProviderLabel");

        formLayout_ai_powered->setWidget(0, QFormLayout::ItemRole::LabelRole, llmProviderLabel);

        llmProviderComboBox = new QComboBox(llmProviderGroup);
        llmProviderComboBox->addItem(QString());
        llmProviderComboBox->addItem(QString());
        llmProviderComboBox->setObjectName("llmProviderComboBox");

        formLayout_ai_powered->setWidget(0, QFormLayout::ItemRole::FieldRole, llmProviderComboBox);

        llmApiKeyLabel = new QLabel(llmProviderGroup);
        llmApiKeyLabel->setObjectName("llmApiKeyLabel");

        formLayout_ai_powered->setWidget(1, QFormLayout::ItemRole::LabelRole, llmApiKeyLabel);

        llmApiKeyEdit = new QLineEdit(llmProviderGroup);
        llmApiKeyEdit->setObjectName("llmApiKeyEdit");

        formLayout_ai_powered->setWidget(1, QFormLayout::ItemRole::FieldRole, llmApiKeyEdit);

        llmModelLabel = new QLabel(llmProviderGroup);
        llmModelLabel->setObjectName("llmModelLabel");

        formLayout_ai_powered->setWidget(2, QFormLayout::ItemRole::LabelRole, llmModelLabel);

        llmModelComboBox = new QComboBox(llmProviderGroup);
        llmModelComboBox->setObjectName("llmModelComboBox");

        formLayout_ai_powered->setWidget(2, QFormLayout::ItemRole::FieldRole, llmModelComboBox);

        llmAdvancedGroupBox = new QGroupBox(llmProviderGroup);
        llmAdvancedGroupBox->setObjectName("llmAdvancedGroupBox");
        llmAdvancedGroupBox->setCheckable(true);
        llmAdvancedGroupBox->setChecked(false);
        formLayout_advanced = new QFormLayout(llmAdvancedGroupBox);
        formLayout_advanced->setObjectName("formLayout_advanced");
        llmBaseUrlLabel = new QLabel(llmAdvancedGroupBox);
        llmBaseUrlLabel->setObjectName("llmBaseUrlLabel");

        formLayout_advanced->setWidget(0, QFormLayout::ItemRole::LabelRole, llmBaseUrlLabel);

        llmBaseUrlEdit = new QLineEdit(llmAdvancedGroupBox);
        llmBaseUrlEdit->setObjectName("llmBaseUrlEdit");

        formLayout_advanced->setWidget(0, QFormLayout::ItemRole::FieldRole, llmBaseUrlEdit);


        formLayout_ai_powered->setWidget(3, QFormLayout::ItemRole::SpanningRole, llmAdvancedGroupBox);


        verticalLayout_scroll_translation->addWidget(llmProviderGroup);

        verticalSpacer_translation = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_scroll_translation->addItem(verticalSpacer_translation);

        scrollArea_translation->setWidget(scrollAreaWidgetContents_translation);

        verticalLayout_translation->addWidget(scrollArea_translation);

        configStackedWidget->addWidget(page_translation);
        page_ai_ui = new QWidget();
        page_ai_ui->setObjectName("page_ai_ui");
        verticalLayout_ai_ui = new QVBoxLayout(page_ai_ui);
        verticalLayout_ai_ui->setObjectName("verticalLayout_ai_ui");
        aiFilterDetailsGroupBox = new QGroupBox(page_ai_ui);
        aiFilterDetailsGroupBox->setObjectName("aiFilterDetailsGroupBox");
        formLayout_ai_settings = new QFormLayout(aiFilterDetailsGroupBox);
        formLayout_ai_settings->setObjectName("formLayout_ai_settings");
        label_ai_sensitivity = new QLabel(aiFilterDetailsGroupBox);
        label_ai_sensitivity->setObjectName("label_ai_sensitivity");

        formLayout_ai_settings->setWidget(0, QFormLayout::ItemRole::LabelRole, label_ai_sensitivity);

        aiFilterThresholdSpinBox = new QDoubleSpinBox(aiFilterDetailsGroupBox);
        aiFilterThresholdSpinBox->setObjectName("aiFilterThresholdSpinBox");
        aiFilterThresholdSpinBox->setMaximum(1.000000000000000);
        aiFilterThresholdSpinBox->setSingleStep(0.050000000000000);
        aiFilterThresholdSpinBox->setValue(0.750000000000000);

        formLayout_ai_settings->setWidget(0, QFormLayout::ItemRole::FieldRole, aiFilterThresholdSpinBox);

        aiInfoLabel = new QLabel(aiFilterDetailsGroupBox);
        aiInfoLabel->setObjectName("aiInfoLabel");
        aiInfoLabel->setWordWrap(true);

        formLayout_ai_settings->setWidget(1, QFormLayout::ItemRole::SpanningRole, aiInfoLabel);


        verticalLayout_ai_ui->addWidget(aiFilterDetailsGroupBox);

        visualsGroupBox = new QGroupBox(page_ai_ui);
        visualsGroupBox->setObjectName("visualsGroupBox");
        verticalLayout_visuals = new QVBoxLayout(visualsGroupBox);
        verticalLayout_visuals->setObjectName("verticalLayout_visuals");
        enableRelationsCheckBox = new QCheckBox(visualsGroupBox);
        enableRelationsCheckBox->setObjectName("enableRelationsCheckBox");

        verticalLayout_visuals->addWidget(enableRelationsCheckBox);


        verticalLayout_ai_ui->addWidget(visualsGroupBox);

        verticalSpacer_ai_ui = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_ai_ui->addItem(verticalSpacer_ai_ui);

        configStackedWidget->addWidget(page_ai_ui);
        page_plugins = new QWidget();
        page_plugins->setObjectName("page_plugins");
        horizontalLayout_plugins = new QHBoxLayout(page_plugins);
        horizontalLayout_plugins->setObjectName("horizontalLayout_plugins");
        pluginListWidget = new QListWidget(page_plugins);
        pluginListWidget->setObjectName("pluginListWidget");
        pluginListWidget->setMaximumSize(QSize(150, 16777215));

        horizontalLayout_plugins->addWidget(pluginListWidget);

        verticalLayout_plugins_right = new QVBoxLayout();
        verticalLayout_plugins_right->setObjectName("verticalLayout_plugins_right");
        pluginEnabledCheckBox = new QCheckBox(page_plugins);
        pluginEnabledCheckBox->setObjectName("pluginEnabledCheckBox");
        pluginEnabledCheckBox->setEnabled(false);

        verticalLayout_plugins_right->addWidget(pluginEnabledCheckBox);

        pluginSettingsScrollArea = new QScrollArea(page_plugins);
        pluginSettingsScrollArea->setObjectName("pluginSettingsScrollArea");
        pluginSettingsScrollArea->setWidgetResizable(true);
        pluginSettingsContainer = new QWidget();
        pluginSettingsContainer->setObjectName("pluginSettingsContainer");
        pluginSettingsContainer->setGeometry(QRect(0, 0, 256, 228));
        formLayout_plugins = new QFormLayout(pluginSettingsContainer);
        formLayout_plugins->setObjectName("formLayout_plugins");
        pluginSettingsScrollArea->setWidget(pluginSettingsContainer);

        verticalLayout_plugins_right->addWidget(pluginSettingsScrollArea);


        horizontalLayout_plugins->addLayout(verticalLayout_plugins_right);

        configStackedWidget->addWidget(page_plugins);

        verticalLayout_content->addWidget(configStackedWidget);

        buttonBox = new QDialogButtonBox(frame_content);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout_content->addWidget(buttonBox);


        horizontalLayout_main->addWidget(frame_content);


        retranslateUi(SettingsDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, SettingsDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, SettingsDialog, qOverload<>(&QDialog::reject));
        QObject::connect(settingsListWidget, &QListWidget::currentRowChanged, configStackedWidget, &QStackedWidget::setCurrentIndex);

        settingsListWidget->setCurrentRow(0);
        configStackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(SettingsDialog);
    } // setupUi

    void retranslateUi(QDialog *SettingsDialog)
    {
        SettingsDialog->setWindowTitle(QCoreApplication::translate("SettingsDialog", "Settings", nullptr));

        const bool __sortingEnabled = settingsListWidget->isSortingEnabled();
        settingsListWidget->setSortingEnabled(false);
        QListWidgetItem *___qlistwidgetitem = settingsListWidget->item(0);
        ___qlistwidgetitem->setText(QCoreApplication::translate("SettingsDialog", "General", nullptr));
        QListWidgetItem *___qlistwidgetitem1 = settingsListWidget->item(1);
        ___qlistwidgetitem1->setText(QCoreApplication::translate("SettingsDialog", "Translation", nullptr));
        QListWidgetItem *___qlistwidgetitem2 = settingsListWidget->item(2);
        ___qlistwidgetitem2->setText(QCoreApplication::translate("SettingsDialog", "AI Engine", nullptr));
        QListWidgetItem *___qlistwidgetitem3 = settingsListWidget->item(3);
        ___qlistwidgetitem3->setText(QCoreApplication::translate("SettingsDialog", "Plugins", nullptr));
        settingsListWidget->setSortingEnabled(__sortingEnabled);

        generalGroupBox->setTitle(QCoreApplication::translate("SettingsDialog", "General Settings", nullptr));
        sourceLanguageLabel->setText(QCoreApplication::translate("SettingsDialog", "Source Language:", nullptr));
        targetLanguageLabel->setText(QCoreApplication::translate("SettingsDialog", "Target Language:", nullptr));
        activeModeLabel->setText(QCoreApplication::translate("SettingsDialog", "Active Mode:", nullptr));
        translatorModeComboBox->setItemText(0, QCoreApplication::translate("SettingsDialog", "Quick (Google Free)", nullptr));
        translatorModeComboBox->setItemText(1, QCoreApplication::translate("SettingsDialog", "Professional (Google API)", nullptr));
        translatorModeComboBox->setItemText(2, QCoreApplication::translate("SettingsDialog", "AI-Powered (LLM)", nullptr));
        translatorModeComboBox->setItemText(3, QCoreApplication::translate("SettingsDialog", "Plugins (Lua)", nullptr));

        label_enable_ai->setText(QCoreApplication::translate("SettingsDialog", "AI Features:", nullptr));
        enableAiFilterCheckBox->setText(QCoreApplication::translate("SettingsDialog", "Enable AI Context Filter", nullptr));
        googleProGroupBox->setTitle(QCoreApplication::translate("SettingsDialog", "Google Cloud Translation API", nullptr));
        googleApiKeyLabel->setText(QCoreApplication::translate("SettingsDialog", "API Key:", nullptr));
        llmProviderGroup->setTitle(QCoreApplication::translate("SettingsDialog", "LLM Provider Settings", nullptr));
        llmProviderLabel->setText(QCoreApplication::translate("SettingsDialog", "Provider:", nullptr));
        llmProviderComboBox->setItemText(0, QCoreApplication::translate("SettingsDialog", "Google AI", nullptr));
        llmProviderComboBox->setItemText(1, QCoreApplication::translate("SettingsDialog", "Anthropic", nullptr));

        llmApiKeyLabel->setText(QCoreApplication::translate("SettingsDialog", "API Key:", nullptr));
        llmModelLabel->setText(QCoreApplication::translate("SettingsDialog", "Model:", nullptr));
        llmAdvancedGroupBox->setTitle(QCoreApplication::translate("SettingsDialog", "Advanced", nullptr));
        llmBaseUrlLabel->setText(QCoreApplication::translate("SettingsDialog", "Base URL:", nullptr));
        aiFilterDetailsGroupBox->setTitle(QCoreApplication::translate("SettingsDialog", "AI Filter Configuration", nullptr));
        label_ai_sensitivity->setText(QCoreApplication::translate("SettingsDialog", "Sensitivity:", nullptr));
#if QT_CONFIG(tooltip)
        aiFilterThresholdSpinBox->setToolTip(QCoreApplication::translate("SettingsDialog", "Similarity Threshold (Higher = Stricter/Fewer Skips)", nullptr));
#endif // QT_CONFIG(tooltip)
        aiInfoLabel->setText(QCoreApplication::translate("SettingsDialog", "<html><head/><body><p><span style=\" font-style:italic;\">Details on how the AI interprets the image context.</span></p></body></html>", nullptr));
        visualsGroupBox->setTitle(QCoreApplication::translate("SettingsDialog", "Visualizations", nullptr));
        enableRelationsCheckBox->setText(QCoreApplication::translate("SettingsDialog", "Enable Data Relations Graph (RPG Maker)", nullptr));
        pluginEnabledCheckBox->setText(QCoreApplication::translate("SettingsDialog", "Enable Plugin", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SettingsDialog: public Ui_SettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGSDIALOG_H
