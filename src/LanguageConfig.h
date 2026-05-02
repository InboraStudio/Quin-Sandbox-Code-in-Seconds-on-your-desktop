#pragma once
#include <QString>
#include <QStringList>
#include <QVector>

struct Language {
    QString name;
    QString extension;
    QString compiler;
    QStringList compileArgs;
    QString linker;        // second link step (ASM)
    QStringList linkArgs;
    QString interpreter;
    QStringList runArgs;
    QString helloWorld;
    QStringList keywords;
};

class LanguageConfig {
public:
    static QVector<Language> all();
    static Language byName(const QString &name);

    static QString findMinGWBin();
    static QString findJavac();
    static QString findJava();
    static QString findGo();
};
