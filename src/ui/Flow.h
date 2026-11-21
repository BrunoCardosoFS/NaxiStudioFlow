#ifndef FLOW_H
#define FLOW_H

#include <QCloseEvent>
#include <QList>
#include <QLocale>
#include <QMainWindow>
#include <QPointer>
#include <QSettings>
#include <QUuid>

#include "core/models.h"
#include "ui/widgets/catalog/CatalogWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Flow;
}
QT_END_NAMESPACE

class PlaylistManager;
class PlaylistItemDelegate;
class MixEditor;

class Flow : public QMainWindow {
  Q_OBJECT

public:
  Flow(QWidget *parent = nullptr);
  ~Flow();

private slots:
  void on_btnFull_clicked();
  void openMixEditorForPlaylistItem(const QUuid &itemUuid);

private:
  Ui::Flow *ui;

  QSettings *settings = new QSettings("NaxStudio", "Flow", this);
  CatalogWidget *catalogWidget = nullptr;
  PlaylistManager *playlistManager = nullptr;
  PlaylistItemDelegate *playlistDelegate = nullptr;
  QPointer<MixEditor> m_mixEditor;

  void saveLayout();
  void restoreLayout();
  void updateClock();
  void updateTransportButtons(PlaylistStatus status);

protected:
  void changeEvent(QEvent *event) override;
  void closeEvent(QCloseEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // FLOW_H
