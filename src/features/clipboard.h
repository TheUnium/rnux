#pragma once

#include "feature_base.h"
#include <QObject>
#include <QClipboard>
#include <QList>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QRandomGenerator>

struct ClipboardItem {
    QString data;
    QString preview;
    QString type; // yhpes : text, or image
    QDateTime timestamp;
    QString filePath;

    bool operator==(const ClipboardItem& other) const {
        return data == other.data &&
               type == other.type &&
               timestamp == other.timestamp;
    }
};

class Clipboard final : public QObject, public FeatureBase {
    Q_OBJECT

public:
    explicit Clipboard(QObject* parent = nullptr);
    ~Clipboard() override;

    [[nodiscard]] QString getName() const override { return "Clipboard"; }
    [[nodiscard]] QString getIcon() const override { return "edit-copy"; }
    QList<FeatureItem> search(const QString& query) override;
    void execute(const FeatureItem& item) override;

private slots:
    void onClipboardChanged();

private:
    void setup();
    void loadHistory();
    void saveHistory();
    [[nodiscard]] static QImage createThumbnail(const QString& imagePath, int size = 64) ;
    void addTextItem(const QString& text);
    void addImageItem(const QImage& image);
    [[nodiscard]] QString storeImage(const QImage& image) const;
    static QImage loadImage(const QString& filePath);

    // encryption/decryption
    void initializeEncryption();
    [[nodiscard]] QByteArray encrypt(const QString& plainText) const;
    [[nodiscard]] QString decrypt(const QByteArray& encryptedData) const;
    static QByteArray deriveKey(const QByteArray& password, const QByteArray& salt) ;
    static QString createPreview(const QString& text) ;

    QClipboard* m_clipboard;
    QList<ClipboardItem> m_history;
    QDir m_storageDir;
    QByteArray m_encryptionKey;
    bool m_encryptionEnabled;
};