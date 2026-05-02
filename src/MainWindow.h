#pragma once
#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "LanguageConfig.h"

class CodeEditor;
class OutputPanel;
class Runner;
class TitleBar;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void onRun();
    void onStop();
    void onLanguageChanged(const QString &name);
    void onClearOutput();

private:
    void setupUi();
    void applyTheme();
    void setRunning(bool r);

    TitleBar    *m_titleBar;
    CodeEditor  *m_editor;
    OutputPanel *m_output;
    Runner      *m_runner;
    QComboBox   *m_langBox;
    QPushButton *m_runBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_clearBtn;
    QLabel      *m_statusLabel;

    Language     m_currentLang;
};
