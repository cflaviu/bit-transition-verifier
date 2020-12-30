import qbs

CppApplication {
    consoleApplication: true
    files: "main.cpp"
    cpp.cxxLanguageVersion: "c++17"
    Group {     // Properties for the produced executable
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}
