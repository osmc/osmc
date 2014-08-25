/********************************************************************************
** Form generated from reading UI file 'installprogress.ui'
**
** Created: Mon Aug 25 01:45:30 2014
**      by: Qt User Interface Compiler version 4.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INSTALLPROGRESS_H
#define UI_INSTALLPROGRESS_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_InstallProgress
{
public:

    void setupUi(QWidget *InstallProgress)
    {
        if (InstallProgress->objectName().isEmpty())
            InstallProgress->setObjectName(QString::fromUtf8("InstallProgress"));
        InstallProgress->resize(720, 576);
        InstallProgress->setStyleSheet(QString::fromUtf8("background-color: #17394A"));

        retranslateUi(InstallProgress);

        QMetaObject::connectSlotsByName(InstallProgress);
    } // setupUi

    void retranslateUi(QWidget *InstallProgress)
    {
        InstallProgress->setWindowTitle(QApplication::translate("InstallProgress", "Form", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class InstallProgress: public Ui_InstallProgress {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INSTALLPROGRESS_H
