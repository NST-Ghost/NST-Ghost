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
#include "translationcore.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QTimer>
#include <QDir>
#include <QDateTime>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <sstream>

// Check if a DLL can be loaded on Windows
static bool canLoadDll(const char* dllName) {
    HMODULE hModule = LoadLibraryExA(dllName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hModule) {
        FreeLibrary(hModule);
        return true;
    }
    return false;
}

// Check Windows dependencies at startup and show helpful dialog if missing
static bool checkWindowsDependencies(const std::string& exeDir) {
    std::vector<std::pair<std::string, std::string>> missingDeps;
    
    // Check Visual C++ Runtime (CRITICAL - almost everything needs this)
    if (!canLoadDll("vcruntime140.dll")) {
        missingDeps.push_back({"Visual C++ Redistributable 2015-2022", 
            "Download from: https://aka.ms/vs/17/release/vc_redist.x64.exe"});
    }
    
    // Check bundled Python DLL
    std::string pythonDll = exeDir + "\\python311.dll";
    struct stat buffer;
    if (stat(pythonDll.c_str(), &buffer) != 0) {
        missingDeps.push_back({"Python 3.11 DLL (python311.dll)", 
            "The bundled Python files are missing. Please re-download NST."});
    }
    
    // Check bundled Python directory
    std::string pythonDir = exeDir + "\\python";
    if (stat(pythonDir.c_str(), &buffer) != 0) {
        missingDeps.push_back({"Bundled Python Directory", 
            "The python/ folder is missing. Please re-download NST."});
    }
    
    if (!missingDeps.empty()) {
        std::stringstream msg;
        msg << "NST detected missing dependencies:\n\n";
        
        for (size_t i = 0; i < missingDeps.size(); i++) {
            msg << (i + 1) << ". " << missingDeps[i].first << "\n";
            msg << "   " << missingDeps[i].second << "\n\n";
        }
        
        msg << "Would you like to continue anyway?\n";
        msg << "(AI features may not work until dependencies are installed)";
        
        int result = MessageBoxA(NULL, msg.str().c_str(), 
            "NST - Missing Dependencies", 
            MB_YESNO | MB_ICONWARNING);
        
        return (result == IDYES);
    }
    
    return true; // All dependencies present
}

// Get executable directory on Windows
static std::string getExeDirectory() {
    char exe_path_buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exe_path_buf, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return ".";
    }
    std::string exe_path(exe_path_buf);
    size_t last_sep = exe_path.find_last_of("\\/");
    return (last_sep != std::string::npos) ? exe_path.substr(0, last_sep) : ".";
}
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
        std::string scripts_path = exe_dir_str + "/pylib";
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
        
        // CRITICAL: Add torch/lib to PATH so c10.dll and other torch DLLs can find their dependencies
        // This is more reliable than os.add_dll_directory() in embedded Python scenarios
        std::string torch_lib = site_packages + "/torch/lib";
        const char* current_path = std::getenv("PATH");
        std::string new_path_str;
        if (current_path) {
            new_path_str = torch_lib + ";" + win_python_home + ";" + std::string(current_path);
        } else {
            new_path_str = torch_lib + ";" + win_python_home;
        }
        
        // Use static storage for PATH (persistent for program lifetime)
        static char s_path_env[32768];
        snprintf(s_path_env, sizeof(s_path_env), "PATH=%s", new_path_str.c_str());
        _putenv(s_path_env);
        
        std::cerr << "[NST] Configured bundled Python (Windows): " << win_python_home << std::endl;
        std::cerr << "[NST] PYTHONPATH: " << pythonpath << std::endl;
        std::cerr << "[NST] Added to PATH: " << torch_lib << std::endl;
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
#ifdef _WIN32
    // Check Windows dependencies FIRST, before anything else
    std::string exeDir = getExeDirectory();
    if (!checkWindowsDependencies(exeDir)) {
        // User chose not to continue
        return 1;
    }
#endif

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

    // Pre-parse command line to check for headless mode
    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "-e" || arg == "--engine" || arg == "-p" || arg == "--project") {
            headless = true;
            break;
        }
    }

    QCoreApplication* app;
    if (headless) {
        std::cerr << "[NST] Starting in Headless (CLI) mode" << std::endl;
        app = new QCoreApplication(argc, argv);
    } else {
        std::cerr << "[NST] Starting in GUI mode" << std::endl;
        app = new QApplication(argc, argv);
    }

    // Set application info
    app->setApplicationName("NST");
    app->setApplicationVersion("1.0.0");

    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("NST Translation Tool - Game Translation Made Easy");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption engineOption(
        QStringList() << "e" << "engine",
        "Game engine name (e.g., rpgm, unity, renpy)",
        "engine"
    );
    QCommandLineOption projectOption(
        QStringList() << "p" << "project",
        "Path to game project directory",
        "path"
    );
    QCommandLineOption deployOption(
        QStringList() << "d" << "deploy",
        "Deploy translated game after loading project"
    );
    QCommandLineOption outputOption(
        QStringList() << "o" << "output",
        "Output directory for deployment (default: original game folder)",
        "path"
    );
    QCommandLineOption backupOption(
        "backup",
        "Create backup before deployment (overrides settings)"
    );
    QCommandLineOption noBackupOption(
        "no-backup",
        "Skip backup creation (overrides settings)"
    );
    QCommandLineOption translateOption(
        QStringList() << "t" << "translate",
        "Translate all files after loading project"
    );

    parser.addOption(engineOption);
    parser.addOption(projectOption);
    parser.addOption(deployOption);
    parser.addOption(outputOption);
    parser.addOption(backupOption);
    parser.addOption(noBackupOption);
    parser.addOption(translateOption);
    
    // We must use parse() or process() to get results
    parser.parse(app->arguments());
    
    if (parser.isSet("help")) {
        std::cerr << parser.helpText().toStdString() << std::endl;
        return 0;
    }
    if (parser.isSet("version")) {
        std::cerr << QCoreApplication::applicationName().toStdString() 
                  << " " << QCoreApplication::applicationVersion().toStdString() << std::endl;
        return 0;
    }

    QString cliEngine = parser.value(engineOption);
    QString cliProject = parser.value(projectOption);
    bool cliDeploy = parser.isSet(deployOption);
    bool cliTranslate = parser.isSet(translateOption);
    QString cliOutput = parser.value(outputOption);
    bool cliBackup = parser.isSet(backupOption);
    bool cliNoBackup = parser.isSet(noBackupOption);

    if (headless && !cliEngine.isEmpty() && !cliProject.isEmpty()) {
        std::cerr << "[NST] Debug: Creating TranslationCore..." << std::endl;
        TranslationCore* core = new TranslationCore(app);
        std::cerr << "[NST] Debug: TranslationCore created." << std::endl;
        
        std::cerr << "[NST] Debug: Loading settings..." << std::endl;
        QSettings settings;
        QVariantMap translationSettings;
        translationSettings["googleApiKey"] = settings.value("apiKey").toString();
        translationSettings["targetLanguage"] = settings.value("targetLanguage", "en").toString();
        translationSettings["googleApi"] = settings.value("googleApi", true).toBool();
        translationSettings["llmProvider"] = settings.value("llmProvider").toString();
        translationSettings["llmApiKey"] = settings.value("llmApiKey").toString();
        translationSettings["llmModel"] = settings.value("llmModel").toString();
        translationSettings["llmBaseUrl"] = settings.value("llmBaseUrl").toString();
        translationSettings["sourceLanguage"] = settings.value("sourceLanguage", "auto").toString();
        core->setTranslationSettings(translationSettings);
        std::cerr << "[NST] Debug: Settings loaded." << std::endl;

        int backupPref = -1;
        if (cliBackup) backupPref = 1;
        if (cliNoBackup) backupPref = 0;
        bool shouldBackup = (backupPref == 1) || (backupPref == -1 && settings.value("deployBackupEnabled", true).toBool());

        std::cerr << "[NST] Debug: Connecting signals..." << std::endl;
        QObject::connect(core, &TranslationCore::translationFinished, [core, cliDeploy, cliProject, cliOutput, shouldBackup]() {
            std::cerr << "[NST] Translation finished." << std::endl;
            if (cliDeploy) {
                std::cerr << "[NST] Starting deployment..." << std::endl;
                QString target = cliOutput.isEmpty() ? cliProject : cliOutput;
                if (core->deployProject(target, shouldBackup)) {
                    std::cerr << "[NST] Deployment successful." << std::endl;
                } else {
                    std::cerr << "[NST] Deployment failed." << std::endl;
                }
            }
            
            // Save workspace to a separate .nst file instead of overwriting game data
            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
            QString workspaceFile = QDir(cliProject).filePath("project_" + timestamp + ".nst");
            if (core->saveWorkspace(workspaceFile)) {
                std::cerr << "[NST] Workspace saved to: " << workspaceFile.toStdString() << std::endl;
            } else {
                std::cerr << "[NST] Failed to save workspace." << std::endl;
            }
            
            QCoreApplication::quit();
        });

        QObject::connect(core, &TranslationCore::errorOccurred, [](const QString &msg) {
            std::cerr << "[NST] ERROR: " << msg.toStdString() << std::endl;
        });
        std::cerr << "[NST] Debug: Signals connected." << std::endl;

        std::cerr << "[NST] Debug: Calling core->loadProject..." << std::endl;
        if (core->loadProject(cliEngine, cliProject)) {
            std::cerr << "[NST] Debug: core->loadProject success." << std::endl;
            if (cliTranslate) {
                std::cerr << "[NST] Starting translation..." << std::endl;
                core->translateAll();
            } else if (cliDeploy) {
                QString target = cliOutput.isEmpty() ? cliProject : cliOutput;
                if (core->deployProject(target, shouldBackup)) {
                    std::cerr << "[NST] Deployment successful." << std::endl;
                } else {
                    std::cerr << "[NST] Deployment failed." << std::endl;
                }
                
                // Save workspace
                QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
                QString workspaceFile = QDir(cliProject).filePath("project_" + timestamp + ".nst");
                core->saveWorkspace(workspaceFile);
                std::cerr << "[NST] Workspace saved to: " << workspaceFile.toStdString() << std::endl;
                
                QCoreApplication::quit();
            } else {
                std::cerr << "[NST] Project loaded. No actions specified." << std::endl;
                QCoreApplication::quit();
            }
        } else {
            return 1;
        }

        return app->exec();
    } else {
        // GUI Mode
        QApplication* guiApp = qobject_cast<QApplication*>(app);
        
        QFile file(":/ui/style.qss");
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream stream(&file);
            guiApp->setStyleSheet(stream.readAll());
            file.close();
        }

        QTranslator translator;
        const QStringList uiLanguages = QLocale::system().uiLanguages();
        for (const QString &locale : uiLanguages) {
            const QString baseName = "NST_" + QLocale(locale).name();
            if (translator.load(":/i18n/" + baseName)) {
                guiApp->installTranslator(&translator);
                break;
            }
        }
        
        MainWindow w;
        w.show();

        return guiApp->exec();
    }
}
