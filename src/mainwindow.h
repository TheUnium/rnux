#pragma once

#include <QMainWindow>
#include <QCloseEvent>

#include "windowui.h"
#include "features/feature_base.h"

struct ResultWithFeature {
    FeatureItem item;
    FeatureBase* feature;
    ResultWithFeature(const FeatureItem& i, FeatureBase* f) : item(i), feature(f) {}
};

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onQueryChanged(const QString& query);
    void onItemActivated(int index);
    void performSearch();

private:
    void setupFeatures();
    void setupWindow();
    void centerWindow();
    void updateResults();

    WindowUI* m_ui;
    QTimer* m_searchTimer;
    QList<FeatureBase*> m_features;
    QList<ResultWithFeature> m_currentResults;
    QString m_currentQuery;
};