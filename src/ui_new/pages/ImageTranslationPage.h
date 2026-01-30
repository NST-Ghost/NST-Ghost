#ifndef NST_UI_IMAGETRANSLATIONPAGE_H
#define NST_UI_IMAGETRANSLATIONPAGE_H

#include <QWidget>
#include <QSplitter>

class TranslationServiceManager;

namespace nst::ui {

class ImageTranslationViewModel;
class ImageViewer;
class ImageQueue;

/**
 * @brief Page for image translation (OCR + translate + overlay).
 * 
 * Composes ImageQueue and ImageViewer components,
 * delegating state management to ImageTranslationViewModel.
 */
class ImageTranslationPage : public QWidget
{
    Q_OBJECT

public:
    explicit ImageTranslationPage(TranslationServiceManager *translationService,
                                   QWidget *parent = nullptr);
    ~ImageTranslationPage() override;

private:
    void setupUI();
    void connectSignals();

private:
    ImageTranslationViewModel *m_viewModel = nullptr;
    
    // UI Components
    QSplitter *m_mainSplitter = nullptr;
    ImageQueue *m_imageQueue = nullptr;
    ImageViewer *m_imageViewer = nullptr;
    
    // Controls
    QWidget *m_controlPanel = nullptr;
};

} // namespace nst::ui

#endif // NST_UI_IMAGETRANSLATIONPAGE_H
