#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCompleter>
#include "tagsmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_open_settings_button_clicked();

    void on_search_input_returnPressed();

    void on_add_new_entry_button_clicked();

    void on_theme_change_button_clicked();

    void on_sync_url_input_returnPressed();

    void on_login_sync_input_returnPressed();

    void on_password_sync_input_returnPressed();

    void on_data_sync_button_clicked();

    void on_back_button_clicked();

    void on_on_mane_page_button_clicked();

    void on_save_note_button_clicked();

    void on_search_input_textChanged(const QString &arg1);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MainWindow *ui;

    TagsManager tagsManager;
    QSet<QString> currentTags;
    QCompleter *tagsCompleter;

    void addTag(const QString &tag);
    void updateCompleter();
    void saveNote();
    QStringList allSearchableEntries();
    void initSearch();
};
#endif // MAINWINDOW_H
