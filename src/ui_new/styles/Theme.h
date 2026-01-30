#ifndef NST_UI_THEME_H
#define NST_UI_THEME_H

#include <QColor>
#include <QFont>
#include <QString>

namespace nst::ui {

/**
 * @brief Centralized theme configuration for the application.
 * 
 * Provides consistent colors, fonts, and spacing values
 * used throughout the UI.
 */
class Theme
{
public:
    // Color Palette - Dark Theme
    struct Colors {
        // Background colors
        static QColor background()     { return QColor("#1a1a2e"); }
        static QColor surface()        { return QColor("#16213e"); }
        static QColor surfaceLight()   { return QColor("#1f3460"); }
        
        // Primary accent
        static QColor primary()        { return QColor("#e94560"); }
        static QColor primaryHover()   { return QColor("#ff6b8a"); }
        static QColor primaryDark()    { return QColor("#c8374f"); }
        
        // Secondary accent
        static QColor secondary()      { return QColor("#0f3460"); }
        static QColor secondaryHover() { return QColor("#1a4a7a"); }
        
        // Text colors
        static QColor textPrimary()    { return QColor("#eaeaea"); }
        static QColor textSecondary()  { return QColor("#a0a0a0"); }
        static QColor textMuted()      { return QColor("#666666"); }
        
        // Status colors
        static QColor success()        { return QColor("#00d26a"); }
        static QColor warning()        { return QColor("#ffb800"); }
        static QColor error()          { return QColor("#ff5252"); }
        static QColor info()           { return QColor("#00b4d8"); }
        
        // Border
        static QColor border()         { return QColor("#2a2a4a"); }
        static QColor borderHover()    { return QColor("#3a3a6a"); }
    };

    // Spacing values (in pixels)
    struct Spacing {
        static int xs()   { return 4; }
        static int sm()   { return 8; }
        static int md()   { return 16; }
        static int lg()   { return 24; }
        static int xl()   { return 32; }
    };

    // Border radius
    struct Radius {
        static int sm()   { return 4; }
        static int md()   { return 8; }
        static int lg()   { return 12; }
        static int full() { return 9999; }
    };

    // Font configuration
    struct Fonts {
        static QFont body();
        static QFont title();
        static QFont heading();
        static QFont mono();
    };

    // Load stylesheet for the application
    static QString loadStylesheet();
};

} // namespace nst::ui

#endif // NST_UI_THEME_H
