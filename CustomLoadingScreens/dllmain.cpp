// dllmain.cpp : Defines the entry point for the DLL application.

// C++ is not my forte, I apologise for any horrors below, but it does work!

#include "pch.h"

#include <fstream>
#include <codecvt>

#include <ShlObj.h>

constexpr auto DIR_SLASH = L"/";
constexpr auto FILE_NULL = L"";

constexpr auto LOADING_SCREENS_LOG = L"mods/CustomLoadingScreens.log";
constexpr auto LOADING_SCREENS_CFG = L"mods/CustomLoadingScreens.cfg";

constexpr auto PACKAGE_EXTENSION       = L".package";
constexpr auto LOADING_SCREENS_FILES   = L"*.package";
constexpr auto ASSOCIATED_MUSIC_SUFFIX = L"_Music.package";
constexpr auto BACKUP_SUBFOLDER        = L"/_backup";

constexpr auto LOADING_SCREENS_UI_FILE     = L"random_loading_screen.package";
constexpr auto LOADING_SCREENS_SPLASH_FILE = L"Splash.package";


inline static void leftTrim(std::wstring& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));
}

inline static void rightTrim(std::wstring& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

inline bool endsWith(std::wstring const& s, std::wstring const& ending)
{
    if (ending.size() > s.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), s.rbegin());
}


static bool dirExists(std::wstring dirName)
{
    DWORD ftyp = GetFileAttributesW(dirName.c_str());

    if (ftyp == INVALID_FILE_ATTRIBUTES) return false;

    return (ftyp & FILE_ATTRIBUTE_DIRECTORY);
}

static bool fileExists(std::wstring dirName)
{
    DWORD ftyp = GetFileAttributesW(dirName.c_str());

    if (ftyp == INVALID_FILE_ATTRIBUTES) return false;

    return !(ftyp & FILE_ATTRIBUTE_DIRECTORY);
}


static std::wofstream LogOpen(bool append)
{
    if (!append) DeleteFile(LOADING_SCREENS_LOG);

    std::wofstream log_file;
    log_file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::generate_header>));
    log_file.open(LOADING_SCREENS_LOG, std::ios_base::out | std::ios_base::app);

    return log_file;
}

static void LogMsg(std::wstring text, bool append = true)
{
    LogOpen(append) << text.c_str() << "\n";
}

static void LogMsg(std::string text, bool append = true)
{
    LogOpen(append) << text.c_str() << "\n";
}

static void ReadCfg(std::wstring &uiPath, std::wstring &screensPath, std::wstring& splashPath, std::wstring& musicPath)
{
    LogMsg("Reading cfg");
    std::wifstream cfg_file(LOADING_SCREENS_CFG);
    cfg_file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::consume_header>));

    PWSTR path;
    SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, NULL, &path);
    std::wstring userDocsPath(path);
    CoTaskMemFree(path);

    // LogMsg(userDocsPath);

    int index = 0;

    wchar_t buf[1024];

    while (cfg_file.good())
    {
        cfg_file.getline(buf, 1024);

        std::wstring line(buf);

        leftTrim(line);

        if (!(line.empty() || line.at(0) == '#'))
        {
            rightTrim(line);

            if (line.at(0) == '~') {
                line = userDocsPath + line.substr(1, line.length() - 1);
            }

            LogMsg(line);

            switch (index)
            {
            case 0:
                uiPath = line;
                break;
            case 1:
                screensPath = line;
                break;
            case 2:
                splashPath = line;
                break;
            case 3:
                musicPath = line;
                break;
            }

            ++index;
        }
    }

    LogMsg("Finished cfg");
}

static bool CopyPackageFile(std::wstring fullSrcPath, std::wstring fullDestPath)
{
    LogMsg(L"Copy " + fullSrcPath);
    LogMsg(L"  -> " + fullDestPath);

    DeleteFileW(fullDestPath.c_str());

    std::ifstream src(fullSrcPath, std::ios::binary);
    std::ofstream dest(fullDestPath, std::ios::binary);

    dest << src.rdbuf();

    return src && dest;
}

static bool SelectLoadingScreen(std::wstring baseDir, std::wstring pattern, std::wstring& selectedFile, std::wstring& selectedMusicFile)
{
    WIN32_FIND_DATAW fileInfo;
    HANDLE hFind;

    LogMsg("Selecting loading screen");

    selectedMusicFile = FILE_NULL;

    std::wstring findPattern = baseDir + DIR_SLASH + pattern;

    int count = 0;
    hFind = FindFirstFileW(findPattern.c_str(), &fileInfo);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::wstring screenPackage = std::wstring(fileInfo.cFileName);

            if (!endsWith(screenPackage, ASSOCIATED_MUSIC_SUFFIX))
            {
                LogMsg(screenPackage);
                ++count;

                // Generate a random number N, such that 0 <= N < count
                if (((rand() / (1.0 * (RAND_MAX + 1))) * count) < 1.0)
                {
                    selectedFile = screenPackage;
                }
            }
        } while (FindNextFileW(hFind, &fileInfo) != 0);

        if (count != 0)
        {
            std::wstring possibleMusicFile = baseDir + DIR_SLASH + selectedFile.substr(0, selectedFile.length() - std::wstring(PACKAGE_EXTENSION).length()) + ASSOCIATED_MUSIC_SUFFIX;

            if (fileExists(possibleMusicFile))
            {
                selectedMusicFile = possibleMusicFile;
            }
        }

        return (count != 0);
    }

    return false;
}

static bool SelectLoadingMusic(std::wstring baseDir, std::wstring pattern, std::wstring& selectedFile)
{
    WIN32_FIND_DATAW fileInfo;
    HANDLE hFind;

    LogMsg("Selecting loading music");

    std::wstring findPattern = baseDir + DIR_SLASH + pattern;

    int count = 0;
    hFind = FindFirstFileW(findPattern.c_str(), &fileInfo);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::wstring musicPackage = std::wstring(fileInfo.cFileName);

            LogMsg(musicPackage);
            ++count;

            // Generate a random number N, such that 0 <= N < count
            if (((rand() / (1.0 * (RAND_MAX + 1))) * count) < 1.0)
            {
                selectedFile = musicPackage;
            }
        } while (FindNextFileW(hFind, &fileInfo) != 0);

        return (count != 0);
    }

    return false;
}

static void BackupSplashPackage(std::wstring musicBackupDir, std::wstring musicDestPath)
{
    if (!dirExists(musicBackupDir))
    {
        CreateDirectoryW(musicBackupDir.c_str(), NULL);

        if (fileExists(musicDestPath))
        {
            LogMsg("Backing-up Splash.package file");

            std::wstring musicBackupPath = musicBackupDir + DIR_SLASH + LOADING_SCREENS_SPLASH_FILE;

            CopyPackageFile(musicDestPath, musicBackupPath);
        }
    }
}

static void RestoreSplashPackage(std::wstring musicBackupDir, std::wstring musicDestPath)
{
    if (dirExists(musicBackupDir))
    {
        std::wstring musicBackupPath = musicBackupDir + DIR_SLASH + LOADING_SCREENS_SPLASH_FILE;

        if (fileExists(musicBackupPath))
        {
            LogMsg("Restoring Splash.package file");

            CopyPackageFile(musicBackupPath, musicDestPath);
        }
    }
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    std::wstring uiPath, screensPath, screensDestPath;
    std::wstring splashPath, musicPath, musicDestPath;

    std::wstring selectedMusicFile = FILE_NULL;

    bool doScreenCleanUp = true;
    bool doMusicCleanUp = true;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        LogMsg("Custom Loading Screens: Entry", false);

        srand((unsigned int)time(NULL)); // Good enough for something that takes as long to load as The Sims 2

        /*
         *  Read the configuration file
        */
        ReadCfg(uiPath, screensPath, splashPath, musicPath);

        /*
         *  Process loading screens
        */
        screensDestPath = uiPath + DIR_SLASH + LOADING_SCREENS_UI_FILE;

        if (dirExists(screensPath))
        {
            LogMsg("Found Loading Screens Directory");

            std::wstring selectedFile;

            if (SelectLoadingScreen(screensPath, LOADING_SCREENS_FILES, selectedFile, selectedMusicFile))
            {
                LogMsg(L"Selected Screen: " + selectedFile);

                std::wstring relSrcPath = screensPath + DIR_SLASH + selectedFile;

                CopyPackageFile(relSrcPath, screensDestPath);

                doScreenCleanUp = false;
            }
        }

        if (doScreenCleanUp) {
            LogMsg("No Loading Screens - performing cleanup");
            DeleteFileW(screensDestPath.c_str());
        }

        /*
         *  Process splash music
         * 
         *  Due to the need to backup the Splash.package file, we can only do this if the "CustomLoadingMusic" sub-folder exists
        */
        if (dirExists(musicPath))
        {
            LogMsg("Found Loading Music Directory");

            std::wstring musicBackupDir = musicPath + BACKUP_SUBFOLDER;

            musicDestPath = splashPath + DIR_SLASH + LOADING_SCREENS_SPLASH_FILE;

            BackupSplashPackage(musicBackupDir, musicDestPath);

            // Any music associated with the loading screen?
            if (!selectedMusicFile.empty())
            {
                LogMsg(L"Selected Associated Music: " + selectedMusicFile);

                std::wstring relSrcPath = screensPath + DIR_SLASH + selectedMusicFile;

                CopyPackageFile(relSrcPath, musicDestPath);

                doMusicCleanUp = false;
            }
            else
            {
                if (SelectLoadingMusic(musicPath, LOADING_SCREENS_FILES, selectedMusicFile))
                {
                    LogMsg(L"Selected Music: " + selectedMusicFile);

                    std::wstring relSrcPath = musicPath + DIR_SLASH + selectedMusicFile;

                    CopyPackageFile(relSrcPath, musicDestPath);

                    doMusicCleanUp = false;
                }
            }

            if (doMusicCleanUp) {
                LogMsg("No Splash Music - performing cleanup");
                DeleteFileW(musicDestPath.c_str());

                RestoreSplashPackage(musicBackupDir, musicDestPath);
            }
        }

        LogMsg("Custom Loading Screens: Exit");
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
