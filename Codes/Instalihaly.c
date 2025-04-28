#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <shlobj.h>
#include <string.h>

#pragma comment(lib, "wininet.lib")

#define MAX_SOFTWARE 2000
#define ITEMS_PER_PAGE 20
#define COLUMNS 2
#define COLUMN_WIDTH 40
#define CURRENT_VERSION "2.1"

typedef struct {
    char name[100];
    char filename[100];
    char url[200];
} Software;

void WaitAndClearConsole(int seconds) {
    Sleep(seconds * 1000);
    system("cls");
}

void GenerateRandomFolderName(char* randomFolder, size_t size) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    srand((unsigned int)time(NULL));
    for (int i = 0; i < 13; i++) {
        randomFolder[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    randomFolder[13] = '\0';
}

void SetConsoleFontSize(short fontSize) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    GetCurrentConsoleFontEx(hConsole, FALSE, &cfi);
    cfi.dwFontSize.Y = fontSize;
    SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);
}

void DownloadSoftwareList(Software* softwareList, int* softwareCount) {
    const char* url = "https://raw.githubusercontent.com/Proseuo/Ujop/refs/heads/main/Codes/software_list.json";
    HINTERNET hInternet, hFile;
    DWORD bytesRead;
    char buffer[4096];
    char* jsonContent = NULL;
    size_t totalSize = 0, bufferSize = 0;

    hInternet = InternetOpen("SoftwareDownloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        printf("Failed to open internet connection.\n");
        return;
    }

    hFile = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        printf("Failed to open URL.\n");
        InternetCloseHandle(hInternet);
        return;
    }

    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (totalSize + bytesRead + 1 > bufferSize) {
            bufferSize = (totalSize + bytesRead + 1) * 2;
            jsonContent = (char*)realloc(jsonContent, bufferSize);
            if (!jsonContent) {
                printf("Memory allocation failed.\n");
                InternetCloseHandle(hFile);
                InternetCloseHandle(hInternet);
                return;
            }
        }
        memcpy(jsonContent + totalSize, buffer, bytesRead);
        totalSize += bytesRead;
    }

    if (jsonContent)
        jsonContent[totalSize] = '\0';

    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    int count = 0;
    char* line = strtok(jsonContent, "\n");
    while (line != NULL) {
        if (strstr(line, "\"name\":")) {
            sscanf(line, " \"name\": \"%[^\"]\"", softwareList[count].name);
        }
        if (strstr(line, "\"filename\":")) {
            sscanf(line, " \"filename\": \"%[^\"]\"", softwareList[count].filename);
        }
        if (strstr(line, "\"url\":")) {
            sscanf(line, " \"url\": \"%[^\"]\"", softwareList[count].url);
            count++;
        }
        line = strtok(NULL, "\n");
    }

    *softwareCount = count;
    free(jsonContent);
}

void PrintSoftwareName(char* name) {
    char displayName[COLUMN_WIDTH];
    int len = strlen(name);
    if (len > COLUMN_WIDTH - 3) {
        strncpy(displayName, name, COLUMN_WIDTH - 4);
        displayName[COLUMN_WIDTH - 4] = '\0';
        strcat(displayName, "...");
    } else {
        strcpy(displayName, name);
    }
    printf("%-30s", displayName);
}

void DownloadFile(const char* url, const char* outputPath, const char* softName, int softwareNumber) {
    HINTERNET hInternet, hFile;
    DWORD bytesRead;
    char buffer[4096];
    DWORD fileSize = 0, downloaded = 0, startTime, elapsedTime;
    double speedKBps;
    int percent;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    hInternet = InternetOpen("Downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        printf("Failed to open internet connection.\n");
        return;
    }

    hFile = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        printf("Failed to open URL.\n");
        InternetCloseHandle(hInternet);
        return;
    }

    DWORD sizeLen = sizeof(fileSize);
    if (!HttpQueryInfo(hFile, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &fileSize, &sizeLen, NULL)) {
        fileSize = 0;
    }

    FILE* file = fopen(outputPath, "wb");
    if (!file) {
        printf("Failed to create file.\n");
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return;
    }

    system("cls");
    printf("Downloading...([%d]%s)\n\n", softwareNumber, softName);

    startTime = GetTickCount();

    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fwrite(buffer, 1, bytesRead, file);
        downloaded += bytesRead;
        elapsedTime = GetTickCount() - startTime;
        if (elapsedTime == 0) elapsedTime = 1;
        speedKBps = (downloaded / 1024.0) / (elapsedTime / 1000.0);

        if (fileSize > 0) {
            percent = (int)((downloaded * 100.0) / fileSize);
            int barWidth = 50;
            int pos = (percent * barWidth) / 100;

            printf("\rProgress: [");
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            for (int i = 0; i < pos; ++i) printf("#");
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            for (int i = pos; i < barWidth; ++i) printf(" ");
            printf("] %d%% | Speed: %.2f KB/s", percent, speedKBps);
        } else {
            printf("\rDownloaded: %.2f KB | Speed: %.2f KB/s", downloaded / 1024.0, speedKBps);
        }
        fflush(stdout);
    }

    fclose(file);
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    system("color 0A");
    printf("\nFile downloaded.\n");
}

int CheckForUpdate() {
    const char* updateUrl = "https://raw.githubusercontent.com/Freeeloy/Instalihaly/refs/heads/main/Downloader/LastUpdate";
    HINTERNET hInternet, hFile;
    DWORD bytesRead;
    char buffer[4096];
    char* jsonContent = NULL;
    size_t totalSize = 0, bufferSize = 0;

    // Set the font size for the update section
    SetConsoleFontSize(20);

    hInternet = InternetOpen("UpdateChecker", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        printf("Failed to open internet connection.\n");
        return 0;
    }

    hFile = InternetOpenUrl(hInternet, updateUrl, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        printf("Failed to open URL.\n");
        InternetCloseHandle(hInternet);
        return 0;
    }

    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (totalSize + bytesRead + 1 > bufferSize) {
            bufferSize = (totalSize + bytesRead + 1) * 2;
            jsonContent = (char*)realloc(jsonContent, bufferSize);
            if (!jsonContent) {
                printf("Memory allocation failed.\n");
                InternetCloseHandle(hFile);
                InternetCloseHandle(hInternet);
                return 0;
            }
        }
        memcpy(jsonContent + totalSize, buffer, bytesRead);
        totalSize += bytesRead;
    }

    if (jsonContent)
        jsonContent[totalSize] = '\0';

    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    char latestVersion[10];
    char downloadUrl[200];
    sscanf(jsonContent, " { \"latest_version\": \"%[^\"]\", \"download_url\": \"%[^\"]\" } ", latestVersion, downloadUrl);
    free(jsonContent);

    if (strcmp(latestVersion, CURRENT_VERSION) != 0) {
        printf("Instalihaly outdated. (Latest version is: %s)\n", latestVersion);
        printf("Updating now...\n");
        Sleep(5000);
        char tempPath[MAX_PATH], folder[MAX_PATH], randomFolder[14], downloadPath[MAX_PATH];
        GetTempPath(MAX_PATH, tempPath);
        GenerateRandomFolderName(randomFolder, sizeof(randomFolder));
        sprintf(folder, "%s%s", tempPath, randomFolder);
        CreateDirectory(folder, NULL);
        sprintf(downloadPath, "%s\\Instalihaly.exe", folder);

        DownloadFile(downloadUrl, downloadPath, "Instalihaly Update", 0);

        ShellExecute(NULL, "open", folder, NULL, NULL, SW_SHOWNORMAL);
        Sleep(5000);
        exit(0);
    } else {
        printf("You are using the latest version (v%s).\n", CURRENT_VERSION);
        WaitAndClearConsole(3);
    }

    return 1;
}

int main() {
    if (!CheckForUpdate()) {
        printf("Unable to check for updates. Continuing...\n");
        Sleep(4000);
        system("pause");
        exit(0);
    }

    // The rest of your main function code here...
    Software softwareList[MAX_SOFTWARE];
    int softwareCount = 0;
    char input[10];
    int choice;
    char downloadPath[MAX_PATH], tempPath[MAX_PATH], folder[MAX_PATH], randomFolder[14];
    int currentPage = 0;

    system("color 0A");
    SetConsoleTitle("Instalihaly");
    SetConsoleFontSize(20);

    DownloadSoftwareList(softwareList, &softwareCount);

    // Displaying software list and handling user choices
    while (1) {
        system("cls");
        printf("===============================================================\n");
        printf("       <<  AVAILABLE SOFTWARES -- LAST UPDATES -- >>\n");
        printf("                 <<  Password : VProv7  >>\n");
        printf("===============================================================\n\n");

        int start = currentPage * ITEMS_PER_PAGE;
        int end = start + ITEMS_PER_PAGE;
        if (end > softwareCount) end = softwareCount;

        int rows = (ITEMS_PER_PAGE + COLUMNS - 1) / COLUMNS;
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < COLUMNS; col++) {
                int i = start + row + col * rows;
                if (i >= end) continue;
                printf("[%d] ", i + 1);
                PrintSoftwareName(softwareList[i].name);
            }
            printf("\n");
        }

        if (end < softwareCount) {
            printf("\n[S] Show More...\n");
        }

        printf("\n[0] Exit\n");
        printf("[B] Back\n");
        printf("===============================================================\n");
        printf("Choose a software to download or press [B] to go back: ");

        fgets(input, sizeof(input), stdin);

        if (input[0] == 'B' || input[0] == 'b') {
            if (currentPage > 0) {
                currentPage--;
                continue;
            } else {
                printf("You are already at the first page.\n");
                system("pause");
                continue;
            }
        }

        if (input[0] == 'S' || input[0] == 's') {
            if (end < softwareCount) {
                currentPage++;
                continue;
            } else {
                printf("No more software to show.\n");
                system("pause");
                continue;
            }
        }

        if (sscanf(input, "%d", &choice) != 1) continue;
        if (choice == 0) break;

        int selectedIndex = choice - 1;
        if (selectedIndex >= start && selectedIndex < end) {
            GetTempPath(MAX_PATH, tempPath);
            sprintf(downloadPath, "%s%s", tempPath, softwareList[selectedIndex].filename);
            DownloadFile(softwareList[selectedIndex].url, downloadPath, softwareList[selectedIndex].filename, selectedIndex + 1);
            GenerateRandomFolderName(randomFolder, sizeof(randomFolder));
            sprintf(folder, "%s%s", tempPath, randomFolder);
            CreateDirectory(folder, NULL);
            char newFilePath[MAX_PATH];
            sprintf(newFilePath, "%s\\%s", folder, softwareList[selectedIndex].filename);
            MoveFile(downloadPath, newFilePath);
            ShellExecute(NULL, "open", folder, NULL, NULL, SW_SHOWNORMAL);
            system("pause");
        }
    }

    return 0;
}
