#include "mainwindow.h"
#include "features/app_launcher.h"
#include "features/calculator.h"
#include "features/system_commands.h"
#include "features/search.h"
#include <QApplication>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(nullptr)
    , m_searchTimer(nullptr)
{
    setupWindow();
    setupFeatures();
    centerWindow();
}

MainWindow::~MainWindow() {
    for (const auto* feature : m_features) {
        delete feature;
    }
}

void MainWindow::setupWindow() {
    m_ui = new WindowUI(this);
    setCentralWidget(m_ui);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("QMainWindow { background-color: transparent; }");

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(50);

    connect(m_ui, &WindowUI::queryChanged, this, &MainWindow::onQueryChanged);
    connect(m_ui, &WindowUI::itemActivated, this, &MainWindow::onItemActivated);
    connect(m_searchTimer, &QTimer::timeout, this, &MainWindow::performSearch);
}

void MainWindow::setupFeatures() {
    const auto searchFeature = new Search();
    m_features.append(searchFeature);
    connect(searchFeature, &Search::resultsUpdated, this, &MainWindow::performSearch);

    m_features.append(new AppLauncher());
    m_features.append(new Calculator());
    m_features.append(new SystemCommands());
    updateResults();
}

void MainWindow::centerWindow() {
    if (const QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect screenGeometry = screen->geometry();
        const int x = (screenGeometry.width() - width()) / 2;
        const int y = (screenGeometry.height() - height()) / 3;
        move(x, y);
    }
}

void MainWindow::updateResults() {
    m_currentResults.clear();

    for (auto* feature : m_features) {
        if (feature->isEnabled()) {
            auto results = feature->search(m_currentQuery);
            for (const auto& result : results) {
                m_currentResults.append(ResultWithFeature(result, feature));
            }
        }
    }

    QList<FeatureItem> uiResults;
    for (const auto& resultWithFeature : m_currentResults) {
        uiResults.append(resultWithFeature.item);
    }

    m_ui->setResults(uiResults);
    adjustSize();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Escape:
        hide();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        m_ui->activateCurrentItem();
        break;
    case Qt::Key_Down:
        m_ui->selectNextItem();
        break;
    case Qt::Key_Up:
        m_ui->selectPreviousItem();
        break;
    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    m_ui->getSearchEdit()->setFocus();
    m_ui->getSearchEdit()->selectAll();
    centerWindow();
}

void MainWindow::hideEvent(QHideEvent* event) {
    QMainWindow::hideEvent(event);
    m_ui->getSearchEdit()->clear();
    m_currentQuery.clear();
    updateResults();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    event->ignore();
    hide();
}

void MainWindow::onQueryChanged(const QString& query) {
    m_currentQuery = query;
    if (m_searchTimer->isActive()) {
        m_searchTimer->stop();
    }

    if (query.trimmed().isEmpty()) {
        performSearch();
        return;
    }

    m_searchTimer->start();
}

void MainWindow::performSearch() {
    updateResults();
}

void MainWindow::onItemActivated(const int index) {
    if (index >= 0 && index < m_currentResults.size()) {
        const auto& resultWithFeature = m_currentResults[index];
        resultWithFeature.feature->execute(resultWithFeature.item);
        hide();
    }
}