#include "mainapplication.h"
#include <QString>
#include <QStringList>
#include <QStringListIterator>
#include <QTextStream>
#include <QDebug>
#include <stdio.h>
#include "invalidargumentsexception.h"

MainApplication::MainApplication(QStringList options)
{
    console = new QTextStream(stdout, QIODevice::WriteOnly);
    bool optionsParsed = parseOptions(options);
    if (!optionsParsed) {
        throw new InvalidArgumentsException();
    }
}

MainApplication::~MainApplication()
{
    delete console;
}

bool MainApplication::parseOptions(QStringList options)
{
    qDebug() << "MainApplication::parseOptions - Parsing options from command line";
    bool readOptions, readInput, readOutput = false;
    bool errorReading;
    QStringListIterator it(options);
    it.next();
    while (it.hasNext()) {
        QString current = it.next();
        qDebug() << "MainApplication::parseOptions - Processing " << current;
        if (!current.startsWith("-")) {
            // Reading in file name
            readOptions = true;
            if (!readInput) {
                inputFilePath = current;
                readInput = true;
            } else {
                outputFilePath = current;
                readOutput = true;
            }
        } else {
            if (readOptions) {
                // Error: Options listed after files
                qCritical() << "Incorrect arrangement of options - USAGE [options] inputFile outputFile";
                errorReading = true;
            } else {
                // Reading in option
                QString option = current;
                if (!it.hasNext()) {
                    qCritical() << "No option value given for " << option;
                    errorReading = true;
                } else {
                    QString optionValue = it.next();
                    if (optionValue.startsWith('-')) {
                        // Error no option value
                        qCritical() << "No option value given for " << option;
                        errorReading = true;
                    } else {
                        if (option == "-feature") {
                            // TODO: Set feature detector
                        } else if (option == "-tracking") {
                            // TODO: Set tracking method
                        } else if (option == "-outlier") {
                            // TODO: Set outlier rejection method
                        } else if (option == "-global") {
                            // TODO: Set global path estimation method
                        } else if (option == "-optimal") {
                            // TODO: Set optimal path estimation method
                        } else if (option == "-crop") {
                            // TODO: Set crop method
                        } else {
                            // TODO: Error: Unknown option provided
                            qCritical() << "Unknown option:" << option;
                            errorReading = true;
                        }
                    }
                }
            }
        }
    }
    if (!readInput) {
        qCritical() << "No input file given";
        errorReading = true;
    }
    if (!readOutput) {
        qCritical() << "No output file given";
        errorReading = true;
    }
    return !errorReading;
}
