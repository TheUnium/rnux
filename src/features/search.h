#pragma once

#include "feature_base.h"
#include <QtNetwork/QNetworkReply>
#include <QTimer>
#include <QJsonArray>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QMutex>
#include <QHash>
#include <QPixmap>
#include <utility>

struct SearchProvider {
    QString name;
    QString shortcut;
    QString iconUrl;
    QString searchUrl;
    QString apiUrl;
    QString description;
    QString cacheType;
    bool hasApi { false };

    SearchProvider(QString  n, QString  s, QString  i,
                   QString  url, QString  desc,
                   QString  api = QString(), const bool hasApiFlag = false,
                   QString  cType = QString())
        : name(std::move(n)), shortcut(std::move(s)), iconUrl(std::move(i)),
          searchUrl(std::move(url)), apiUrl(std::move(api)),
          description(std::move(desc)), cacheType(std::move(cType)), hasApi(hasApiFlag)
    {}
};

struct CacheEntry {
    QString type;
    QString query;
    QDateTime lastModified;
    QList<FeatureItem> results;

    CacheEntry() = default;
    CacheEntry(QString  t, QString  q, QDateTime  lm, const QList<FeatureItem>& r)
        : type(std::move(t)), query(std::move(q)), lastModified(std::move(lm)), results(r) {}
};

class Search final : public QObject, public FeatureBase {
    Q_OBJECT

public:
    explicit Search(QObject* parent = nullptr);
    ~Search() override;

    [[nodiscard]] QString getName() const override { return "Search"; }
    [[nodiscard]] QString getIcon() const override { return "system-search"; }
    QList<FeatureItem> search(const QString& query) override;
    void execute(const FeatureItem& item) override;

signals:
    void resultsUpdated();

private slots:
    void onApiResponse();
    void onSearchTimeout();
    void onIconDownloaded();

private:
    void setupProviders();
    void performApiSearch(const SearchProvider& provider, const QString& query);
    void downloadProviderIcons();
    static QString extractSearchQuery(const QString& fullQuery, const QString& shortcut);
    static QList<FeatureItem> parseNpmResults(const QJsonDocument& doc, const QString& query);
    static QList<FeatureItem> parseCargoResults(const QJsonDocument& doc, const QString& query);
    static QList<FeatureItem> parseGitHubResults(const QJsonDocument& doc, const QString& query);

    static FeatureItem createFeatureItem(const QString& name, const QString& url);

    QNetworkAccessManager* m_networkManager;
    QTimer* m_searchTimer;
    QList<SearchProvider> m_providers;
    QList<FeatureItem> m_currentResults;
    QString m_currentQuery;
    QNetworkReply* m_currentReply { nullptr };

    // cache
    void initializeCache();
    void saveCache();
    void loadCache();
    QList<FeatureItem> getCachedResults(const QString& type, const QString& query);
    void setCachedResults(const QString& type, const QString& query, const QList<FeatureItem>& results);
    void cleanupCache();
    static QString getCacheFilePath();
    QHash<QString, CacheEntry> m_cache;
    QMutex m_cacheMutex;
    static constexpr int MAX_CACHE_ENTRIES = 250;
    static constexpr int CACHE_EXPIRE_HOURS = 24;

    QString m_activeSearchType;
    QString m_activeSearchQuery;

    QHash<QString, QPixmap> m_iconCache;
    QHash<QString, QString> m_iconPaths;
};