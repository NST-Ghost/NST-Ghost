// Python includes - only when HAS_PYTHON is defined
#ifdef HAS_PYTHON
// IMPORTANT: Python.h must be included FIRST, before any Qt headers
// to avoid Qt's "slots" macro conflict with Python's use of "slots"
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#pragma pop_macro("slots")
#endif

// Now we can include Qt headers
#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#endif



#ifdef HAS_PYTHON
// Static storage for environment variables to prevent memory leaks
// (putenv requires the string to persist for the lifetime of the program)
static char s_pythonhome_env[4096];
static char s_pythonpath_env[8192];

// Helper function to configure Python BEFORE pybind11 initialization
// Supports both Linux AppImage and Windows bundled deployments
// Note: This is called BEFORE QApplication, so we can't use Qt APIs
static void configurePythonEnvironment(const char* argv0)
{
    // Check if running from AppImage (APPDIR is set by AppRun script)
    const char* appdir = std::getenv("APPDIR");
    
#ifdef _WIN32
    // Windows: Use GetModuleFileName for reliable exe path (argv[0] is unreliable)
    char exe_path_buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exe_path_buf, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        std::cerr << "[NST] Error: GetModuleFileName failed" << std::endl;
        return;
    }
    std::string exe_path(exe_path_buf);
    size_t last_sep = exe_path.find_last_of("\\/");
    std::string exe_dir_str = (last_sep != std::string::npos) ? exe_path.substr(0, last_sep) : ".";
    
    // Windows: Check for bundled Python in exe_dir/python/
    std::string win_python_home = exe_dir_str + "\\python";
    std::string win_python_dll = win_python_home + "\\python311.dll";
    
    // Use C++ filesystem to check file exists (no Qt dependency)
    struct stat buffer;
    if (stat(win_python_dll.c_str(), &buffer) == 0) {
        // Found bundled Python on Windows
        std::string scripts_path = exe_dir_str + "/scripts";
        std::string site_packages = win_python_home + "/Lib/site-packages";
        std::string stdlib_zip = win_python_home + "/python311.zip";
        std::string stdlib_lib = win_python_home + "/Lib";
        std::string stdlib_dlls = win_python_home + "/DLLs";
        
        // Build complete PYTHONPATH: scripts, stdlib zip, stdlib lib, DLLs, site-packages
        std::string pythonpath = scripts_path + ";" + stdlib_zip + ";" + stdlib_lib + ";" + stdlib_dlls + ";" + site_packages;
        
        // Use static storage to avoid memory leak
        snprintf(s_pythonhome_env, sizeof(s_pythonhome_env), "PYTHONHOME=%s", win_python_home.c_str());
        snprintf(s_pythonpath_env, sizeof(s_pythonpath_env), "PYTHONPATH=%s", pythonpath.c_str());
        
        _putenv(s_pythonhome_env);
        _putenv(s_pythonpath_env);
        
        std::cerr << "[NST] Configured bundled Python (Windows): " << win_python_home << std::endl;
        std::cerr << "[NST] PYTHONPATH: " << pythonpath << std::endl;
        return;
    }
#else
    (void)argv0; // Unused on Linux (uses APPDIR)
#endif
    
    if (appdir && appdir[0] != '\0') {
        // Running from AppImage - configure bundled Python
        std::string appdir_str(appdir);
        
        // Look for bundled Python in standard locations
        std::string python_home;
        std::string python_lib;
        
        // Try common python installation patterns
        std::vector<std::string> python_versions = {"python3.12", "python3.11", "python3.10", "python3"};
        
        for (const auto& pyver : python_versions) {
            std::string test_path = appdir_str + "/usr/lib/" + pyver;
            std::string test_lib = test_path + "/os.py";  // os.py is always present in stdlib
            
            if (QFile::exists(QString::fromStdString(test_lib))) {
                python_lib = test_path;
                python_home = appdir_str + "/usr";
                break;
            }
        }
        
        // If bundled Python found, configure it using environment variables
        if (!python_home.empty()) {
            // Build PYTHONPATH
            std::string scripts_path = appdir_str + "/usr/bin";  // Parent dir so "import scripts" works
            std::string site_packages = python_lib + "/site-packages";
            std::string lib_dynload = python_lib + "/lib-dynload";
            
            std::string pythonpath = scripts_path + ":" + python_lib + ":" + site_packages + ":" + lib_dynload;
            
            // Use static storage to avoid memory leak
            snprintf(s_pythonhome_env, sizeof(s_pythonhome_env), "PYTHONHOME=%s", python_home.c_str());
            snprintf(s_pythonpath_env, sizeof(s_pythonpath_env), "PYTHONPATH=%s", pythonpath.c_str());
            
            putenv(s_pythonhome_env);
            putenv(s_pythonpath_env);
            
            std::cerr << "[NST] Configured bundled Python home: " << python_home << std::endl;
            std::cerr << "[NST] PYTHONPATH: " << pythonpath << std::endl;
        } else {
            std::cerr << "[NST] Warning: APPDIR set but no bundled Python found, using system Python" << std::endl;
#ifdef _WIN32
            _putenv("PYTHONHOME=");
#else
            unsetenv("PYTHONHOME");
#endif
        }
    } else {
        // Not running from AppImage/bundled - use system Python
        // Clear any conflicting environment variables
#ifdef _WIN32
        _putenv("PYTHONHOME=");
#else
        unsetenv("PYTHONHOME");
#endif
        // Let Python auto-discover its paths
        std::cerr << "[NST] Using system Python configuration" << std::endl;
    }
}
#endif // HAS_PYTHON

int main(int argc, char *argv[])
{
#ifdef HAS_PYTHON
    // Configure Python BEFORE creating interpreter
    configurePythonEnvironment(argv[0]);

    std::cerr << "[NST] About to initialize Python interpreter..." << std::endl;
    
    // Initialize Python Interpreter
    pybind11::scoped_interpreter guard{};
    std::cerr << "[NST] Python interpreter initialized successfully" << std::endl;
    
    pybind11::gil_scoped_release release;
    std::cerr << "[NST] GIL released, starting Qt..." << std::endl;
#endif

    std::cerr << "[NST] Creating QApplication..." << std::endl;
    QApplication a(argc, argv);
    std::cerr << "[NST] QApplication created" << std::endl;

    // Fix: Correct resource path for stylesheet (was :/style.qss, needed :/ui/style.qss)
    QFile file(":/ui/style.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        a.setStyleSheet(stream.readAll());
        file.close();
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "NST_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    
    std::cerr << "[NST] Creating MainWindow..." << std::endl;
    MainWindow w;
    std::cerr << "[NST] Showing MainWindow..." << std::endl;
    w.show();

    // QTimer::singleShot(0, &w, &MainWindow::onNewProject); // Optional: Auto-start new project flow

    std::cerr << "[NST] Entering event loop..." << std::endl;
    return a.exec();
}
