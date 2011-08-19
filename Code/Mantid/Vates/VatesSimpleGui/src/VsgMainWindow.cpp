#include "VsgMainWindow.h"

#include "MdViewerWidget.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>

VsgMainWindow::VsgMainWindow(QWidget *parent) : QMainWindow(parent)
{
  this->mdViewer = new MdViewerWidget(this);
  this->setCentralWidget(this->mdViewer);

  this->createActions();
  this->createMenus();
}

VsgMainWindow::~VsgMainWindow()
{

}

void VsgMainWindow::createActions()
{
  // File loading
  this->openAction = new QAction(QApplication::tr("&Open"), this);
  this->openAction->setShortcut(QApplication::tr("Ctrl+O"));
  this->openAction->setStatusTip(QApplication::tr("Open a file for viewing"));
  this->mdViewer->connectLoadDataReaction(this->openAction);

  // Program exit
  this->exitAction = new QAction(QApplication::tr("&Exit"), this);
  this->exitAction->setShortcut(QApplication::tr("Crtl+Q"));
  this->exitAction->setStatusTip(QApplication::tr("Exit the program."));
  QObject::connect(this->exitAction, SIGNAL(triggered()), this, SLOT(close()));
}

void VsgMainWindow::createMenus()
{
  this->fileMenu = this->menuBar()->addMenu(QApplication::tr("&File"));
  this->fileMenu->addAction(this->openAction);
  this->fileMenu->addSeparator();
  this->fileMenu->addAction(this->exitAction);
}
