#include "app_launcher.h"
#include <QSettings>
#include <algorithm>

AppLauncher::AppLauncher() {
    loadApplications();
}

QList<FeatureItem> AppLauncher::search(const QString& query) {
    if (query.trimmed().isEmpty()) {
        return m_applications.mid(0, 8); // top 8 apps
    }

    QList<QPair<FeatureItem, int>> scored;
    const QString lowerQuery = query.toLower();

    for (const auto& app : m_applications) {
        if (int score = fuzzyMatch(lowerQuery, app.title.toLower()); score > 0) {
            scored.append({app, score});
        }
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    QList<FeatureItem> results;
    for (const auto&[item, score] : scored) {
        results.append(item);
        if (results.size() >= 8) break;
    }

    return results;
}

void AppLauncher::execute(const FeatureItem& item) {
    QStringList args = QProcess::splitCommand(item.data);
    if (args.isEmpty()) {
        return;
    }

    const QString program = args.takeFirst();
    QProcess::startDetached(program, args);
}

void AppLauncher::loadApplications() {
    m_applications.clear();
    m_seenApps.clear();

    QSet<QString> appDirPaths;

    for (const QString& path : QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation)) {
        appDirPaths.insert(path);
    }
    appDirPaths.insert("/var/lib/flatpak/exports/share/applications");
    appDirPaths.insert(QDir::homePath() + "/.local/share/flatpak/exports/share/applications");
    appDirPaths.insert("/var/lib/snapd/desktop/applications");

    for (const QString& dirPath : appDirPaths) {
        QDir dir(dirPath);
        if (!dir.exists()) {
            continue;
        }

        const QFileInfoList files = dir.entryInfoList({"*.desktop"}, QDir::Files);
        for (const QFileInfo& fileInfo : files) {
            parseDesktopFile(fileInfo.absoluteFilePath());
        }
    }

    std::sort(m_applications.begin(), m_applications.end(),
              [](const FeatureItem& a, const FeatureItem& b) {
                  return a.title.toLower() < b.title.toLower();
              });
}

void AppLauncher::parseDesktopFile(const QString& filePath) {
    QSettings desktopFile(filePath, QSettings::IniFormat);
    desktopFile.beginGroup("Desktop Entry");

    if (desktopFile.value("Type", "Application").toString() != "Application") {
        desktopFile.endGroup();
        return;
    }

    if (desktopFile.value("NoDisplay", false).toBool() || desktopFile.value("Hidden", false).toBool()) {
        desktopFile.endGroup();
        return;
    }

    const QString name = desktopFile.value("Name").toString();
    QString exec = desktopFile.value("Exec").toString();

    if (name.isEmpty() || exec.isEmpty() || m_seenApps.contains(name)) {
        desktopFile.endGroup();
        return;
    }

    m_seenApps.insert(name);

    // fuckass regexp
    // f - the file name
    // F - multiple file names
    // u - a single URL
    // U - multiple URLs
    // d - directory
    // D - directory of first file
    // n - file name without path
    // N - multiple file names without paths
    // i - icon
    // c - translated comment
    // k - desktop file name
    // v - device (eg. for mounting)
    // m - mimetypes
    exec = exec.replace(QRegularExpression("%[fFuUdDnNickvm]"), "").trimmed();

    const QString icon = desktopFile.value("Icon").toString();
    const QString comment = desktopFile.value("Comment").toString();

    m_applications.append(FeatureItem(name, comment, icon, exec, "app"));
    desktopFile.endGroup();
}

int AppLauncher::fuzzyMatch(const QString& query, const QString& text) {
    if (query.isEmpty()) return 1;
    if (text.isEmpty()) return 0;

    int score = 0;
    int queryPos = 0;
    int lastMatchIndex = -1;

    for (int i = 0; i < text.length() && queryPos < query.length(); ++i) {
        if (text[i] == query[queryPos]) {
            int currentScore = 1;
            if (lastMatchIndex == -1) {
                currentScore += 10;
            }
            if (lastMatchIndex == i - 1) {
                currentScore += 5;
            }
            if (i == 0 || text[i-1].isSpace()) {
                currentScore += 8;
            }
            score += currentScore;
            lastMatchIndex = i;
            queryPos++;
        }
    }

    return (queryPos == query.length()) ? score : 0;
}