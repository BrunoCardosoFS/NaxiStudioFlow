#ifndef FLOW_H
#define FLOW_H

#include <QCloseEvent>
#include <QList>
#include <QLocale>
#include <QMainWindow>
#include <QSettings>

#include "ui/widgets/catalog/CatalogWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Flow;
}
QT_END_NAMESPACE

class Flow : public QMainWindow {
  Q_OBJECT

public:
  Flow(QWidget *parent = nullptr);
  ~Flow();

private slots:
  void on_btnFull_clicked();
  void on_btnPlay_clicked();
  void on_btnPause_clicked();
  void on_btnStop_clicked();

private:
  Ui::Flow *ui;

  QSettings *settings = new QSettings("NaxStudio", "Flow");
  CatalogWidget *catalogWidget = nullptr;

  void saveLayout();
  void restoreLayout();
  void updateClock();

protected:
  void changeEvent(QEvent *event) override;
  void closeEvent(QCloseEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // FLOW_H
