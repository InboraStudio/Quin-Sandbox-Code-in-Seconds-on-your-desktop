#include "LanguageConfig.h"
#include <QFile>
#include <QDir>

QString LanguageConfig::findMinGWBin()
{
    QStringList candidates = {
        "D:/QTx/Tools/mingw1310_64/bin",
        "C:/Qt/Tools/mingw1310_64/bin",
        "C:/Qt/Tools/mingw1120_64/bin",
        "C:/Qt/Tools/mingw900_64/bin",
    };
    for (const QString &c : candidates)
        if (QFile::exists(c + "/g++.exe"))
            return c;
    return {};
}

// search common JDK locations for javac.exe
QString LanguageConfig::findJavac()
{
    // check PATH first
    QStringList roots = {
        "C:/Program Files/Java",
        "C:/Program Files/Eclipse Adoptium",
        "C:/Program Files/Microsoft",
        "C:/Program Files/OpenJDK",
        "C:/Program Files/Zulu",
        "C:/Program Files/BellSoft/LibericaJDK",
        "C:/Program Files (x86)/Java",
    };
    for (const QString &root : roots) {
        QDir d(root);
        if (!d.exists()) continue;
        for (const QString &sub : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString path = root + "/" + sub + "/bin/javac.exe";
            if (QFile::exists(path)) return path;
        }
    }
    return {};
}

// find java.exe (JRE is enough for running)
QString LanguageConfig::findJava()
{
    QStringList roots = {
        "C:/Program Files/Java",
        "C:/Program Files/Eclipse Adoptium",
        "C:/Program Files/Microsoft",
        "C:/Program Files/OpenJDK",
        "C:/Program Files/Zulu",
        "C:/Program Files (x86)/Java",
    };
    for (const QString &root : roots) {
        QDir d(root);
        if (!d.exists()) continue;
        for (const QString &sub : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString path = root + "/" + sub + "/bin/java.exe";
            if (QFile::exists(path)) return path;
        }
    }
    return "java"; // fall back to PATH
}

// find go binary
QString LanguageConfig::findGo()
{
    QStringList candidates = {
        "C:/Program Files/Go/bin/go.exe",
        "C:/Go/bin/go.exe",
        "C:/tools/go/bin/go.exe",
    };
    for (const QString &p : candidates)
        if (QFile::exists(p)) return p;
    return {}; // empty = not installed
}


QVector<Language> LanguageConfig::all()
{
    QString mingw = findMinGWBin();
    QString gpp   = mingw.isEmpty() ? "g++"  : mingw + "/g++.exe";
    QString gcc   = mingw.isEmpty() ? "gcc"  : mingw + "/gcc.exe";
    QString gld   = mingw.isEmpty() ? "gcc"  : mingw + "/gcc.exe";

    QVector<Language> langs;

    langs.append({
        "C++", "cpp",
        gpp, {"-o", "%OUT%", "%SRC%", "-std=c++17"},
        "", {},
        "", {},
        "#include <iostream>\nusing namespace std;\n\nint main() {\n    cout << \"Hello, Quin Sandbox!\" << endl;\n    return 0;\n}",
        {"int","double","float","char","bool","string","void","auto","return","if","else","for",
         "while","do","class","struct","public","private","protected","new","delete","nullptr",
         "true","false","include","using","namespace","std","cout","cin","endl","const","static",
         "virtual","override","template","typename"}
    });

    langs.append({
        "C", "c",
        gcc, {"-o", "%OUT%", "%SRC%"},
        "", {},
        "", {},
        "#include <stdio.h>\n\nint main() {\n    printf(\"Hello, Quin Sandbox!\\n\");\n    return 0;\n}",
        {"int","double","float","char","void","return","if","else","for","while","do",
         "struct","printf","scanf","include","const","static","sizeof","typedef","NULL",
         "unsigned","long","short","extern","enum","union"}
    });

    langs.append({
        "C#", "cs",
        "csc", {"/nologo", "/out:%OUT%", "%SRC%"},
        "", {},
        "", {},
        "using System;\n\nclass Program {\n    static void Main() {\n        Console.WriteLine(\"Hello, Quin Sandbox!\");\n    }\n}",
        {"using","namespace","class","static","void","public","private","protected","return",
         "int","double","float","string","bool","char","new","this","base","if","else",
         "for","foreach","while","do","try","catch","finally","Console","WriteLine","true","false","null",
         "var","readonly","override","virtual","abstract","interface","enum","struct"}
    });

    langs.append({
        "Python", "py",
        "", {},
        "", {},
        "python", {"%SRC%"},
        "print(\"Hello, Quin Sandbox!\")",
        {"def","class","import","from","return","if","elif","else","for","while","in","not","and","or",
         "True","False","None","print","input","len","range","lambda","try","except","with","as",
         "pass","break","continue","global","nonlocal","yield","async","await","raise","del"}
    });

    langs.append({
        "JavaScript", "js",
        "", {},
        "", {},
        "node", {"%SRC%"},
        "console.log(\"Hello, Quin Sandbox!\");",
        {"var","let","const","function","return","if","else","for","while","do","class","new","this",
         "true","false","null","undefined","console","require","import","export","async","await",
         "try","catch","typeof","instanceof","switch","case","break","continue","default","=>"}
    });

    langs.append({
        "Java", "java",
        "javac", {"%SRC%"},
        "", {},
        "java", {"%CLASS%"},
        "public class Main {\n    public static void main(String[] args) {\n        System.out.println(\"Hello, Quin Sandbox!\");\n    }\n}",
        {"public","private","protected","class","interface","extends","implements","return","void",
         "int","double","float","char","boolean","String","new","this","super","static","final",
         "if","else","for","while","do","try","catch","finally","import","package","true","false",
         "null","System","out","println","abstract","synchronized","throws","throw","enum"}
    });

    langs.append({
        "Go", "go",
        "", {},
        "", {},
        "go", {"run", "%SRC%"},
        "package main\n\nimport \"fmt\"\n\nfunc main() {\n    fmt.Println(\"Hello, Quin Sandbox!\")\n}",
        {"package","import","func","return","if","else","for","range","var","const","type","struct",
         "interface","map","chan","go","defer","select","case","default","break","continue",
         "true","false","nil","fmt","Println","make","len","cap","append","delete"}
    });

    // ASM uses special two-step build in Runner (nasm -> obj, gcc -> exe)
    langs.append({
        "ASM", "asm",
        "nasm", {"-f", "win64", "%SRC%", "-o", "%OBJ%"},
        gld, {"%OBJ%", "-o", "%OUT%"},
        "", {},
        "; NASM x64 Windows\nglobal main\nextern printf\n\nsection .data\n    msg db \"Hello, Quin Sandbox!\", 10, 0\n\nsection .text\nmain:\n    sub rsp, 40\n    lea rcx, [rel msg]\n    call printf\n    xor eax, eax\n    add rsp, 40\n    ret",
        {"global","extern","section","db","dw","dd","dq","resb","equ","mov","add","sub","mul",
         "div","push","pop","call","ret","jmp","je","jne","jl","jg","jle","jge","lea","xor",
         "and","or","not","cmp","test","nop","int","syscall","rsp","rax","rbx","rcx","rdx",
         "rsi","rdi","rbp","r8","r9","r10","r11","eax","ebx","ecx","edx","al","bl"}
    });

    return langs;
}

Language LanguageConfig::byName(const QString &name)
{
    for (const auto &l : all())
        if (l.name == name)
            return l;
    return all().first();
}
