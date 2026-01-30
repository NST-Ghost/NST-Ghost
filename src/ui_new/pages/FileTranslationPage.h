#ifndef NST_UI_FILETRANSLATIONPAGE_H
#define NST_UI_FILETRANSLATIONPAGE_H

#include <QWidget>
#include <QSplitter>

class TranslationServiceManager;

namespace nst::ui {

class FileTranslationViewModel;
class FileListView;
class TranslationTable;
class ProgressIndicator;

/**
 * @brief Page for file-based game translation.
 * 
 * Composes FileListView and TranslationTable components,
 * delegating state management to FileTranslationViewModel.
 */
class FileTranslationPage : public QWidget
{
    Q_OBJECT

public:
    explicit FileTranslationPage(TranslationServiceManager *translationService,
                                  QWidget *parent = nullptr);
    ~FileTranslationPage() override;

    // Open a project
    void openProject(const QString &engineName, const QString &projectPath);

signals:
    void projectLoaded(const QString &path);

private:
    void setupUI();
    void connectSignals();

private:
    FileTranslationViewModel *m_viewModel = nullptr;
    
    // UI Components
    QSplitter *m_mainSplitter = nullptr;
    FileListView *m_fileList = nullptr;
    TranslationTable *m_translationTable = nullptr;
    ProgressIndicator *m_progress = nullptr;
};

} // namespace nst::ui

#endif // NST_UI_FILETRANSLATIONPAGE_H
