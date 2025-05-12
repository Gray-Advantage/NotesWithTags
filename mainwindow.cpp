#include "mainwindow.h"
#include "settingsmanager.h"
#include "themeutils.h"
#include "note.h"
#include "./ui_mainwindow.h"

#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringListModel>
#include <QDebug>
#include <QMessageBox>
#include <QCompleter>
#include <QFileDialog>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    networkManager = new QNetworkAccessManager(this);

    connect(ui->search_input, &QLineEdit::textChanged, this, &MainWindow::on_search_input_textChanged);

    SettingsManager &s = SettingsManager::instance();
    ThemeUtils::applyTheme(s.theme());

    if (s.theme() == "dark") {
        ui->theme_change_button->setText("Сменить на светлую");
    } else {
        ui->theme_change_button->setText("Сменить на тёмную");
    }

    ui->sync_url_input->setText(s.syncUrl());
    ui->login_sync_input->setText(s.login());
    ui->password_sync_input->setText(s.password());
    ui->set_save_place_button->setText(s.saveLocation());

    tagsCompleter = new QCompleter(tagsManager.allTags(), this);
    tagsCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    ui->tags_searcher->setCompleter(tagsCompleter);

    connect(ui->tags_searcher, &QLineEdit::returnPressed, this, [=]() {
        QString text = ui->tags_searcher->text().trimmed();
        if (!text.isEmpty() && !currentTags.contains(text)) {
            addTag(text);
            ui->tags_searcher->clear();
        }
    });

    performSync();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_open_settings_button_clicked()
{
    ui->pages->setCurrentIndex(4);
}

void MainWindow::on_search_input_returnPressed() {
    QString searchText = ui->search_input->text().trimmed();
    if (searchText.isEmpty()) return;

    QList<Note> notes = noteManager.searchNotes(searchText);
    if (notes.isEmpty()) {
        QMessageBox::information(this, "Не найдено", "Заметка по запросу не найдена.");
        return;
    }

    Note note = notes.first();
    ui->note_title->setText(note.title());
    ui->note_text->setPlainText(note.text());

    currentTags.clear();
    QLayoutItem *child;
    while ((child = ui->tags_container->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    for (const QString &tag : note.tags()) {
        addTag(tag);
    }

    ui->pages->setCurrentIndex(3);
}

void MainWindow::on_add_new_entry_button_clicked()
{
    currentTags.clear();
    QLayoutItem *child;
    int itemCount = ui->tags_container->count();

    if (itemCount != 1) {
        for (int i = 0; i < itemCount - 1; ++i) {
            child = ui->tags_container->takeAt(0);
            if (child) {
                delete child->widget();
                delete child;
            }
        }
    }

    ui->note_text->clear();
    ui->note_title->clear();
    ui->pages->setCurrentIndex(3);
}

void MainWindow::on_sync_url_input_returnPressed() {
    SettingsManager::instance().setSyncUrl(ui->sync_url_input->text());
}

void MainWindow::on_login_sync_input_returnPressed() {
    SettingsManager::instance().setLogin(ui->login_sync_input->text());
}

void MainWindow::on_password_sync_input_returnPressed() {
    SettingsManager::instance().setPassword(ui->password_sync_input->text());
}

void MainWindow::on_theme_change_button_clicked()
{
    SettingsManager &s = SettingsManager::instance();
    QString newTheme = s.theme() == "dark" ? "light" : "dark";
    s.setTheme(newTheme);
    ThemeUtils::applyTheme(newTheme);

    ui->theme_change_button->setText(
        newTheme == "dark" ? "Сменить на светлую" : "Сменить на тёмную"
        );
}

void MainWindow::on_data_sync_button_clicked()
{
    performSync();
}

void MainWindow::on_back_button_clicked()
{
    ui->pages->setCurrentIndex(0);
}

void MainWindow::addTag(const QString &tag) {
    if (currentTags.contains(tag)) return;

    currentTags.insert(tag);

    QPushButton *btn = new QPushButton(tag);
    btn->setObjectName(tag);
    btn->setStyleSheet("border: 1px solid gray; border-radius: 3px; padding: 2px 5px;");

    connect(btn, &QPushButton::clicked, this, [=]() {
        currentTags.remove(tag);
        ui->tags_container->removeWidget(btn);
        btn->deleteLater();
    });

    btn->installEventFilter(this);
    ui->tags_container->insertWidget(ui->tags_container->count() - 1, btn);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    QPushButton *btn = qobject_cast<QPushButton *>(obj);
    if (!btn) return false;

    if (event->type() == QEvent::Enter) {
        btn->setText("Удалить?");
    } else if (event->type() == QEvent::Leave) {
        btn->setText(btn->objectName());
    }
    return false;
}

void MainWindow::on_on_mane_page_button_clicked()
{
    ui->pages->setCurrentIndex(0);
    QString currentText = ui->search_input->text();
    ui->search_input->setText("");
    ui->search_input->setText(currentText);
}

void MainWindow::updateCompleter() {
    tagsCompleter->model()->deleteLater();
    tagsCompleter->setModel(new QStringListModel(tagsManager.allTags(), this));
}

void MainWindow::on_save_note_button_clicked() {
    QString title = ui->note_title->text().trimmed();
    QString text = ui->note_text->toPlainText();

    if (text.trimmed().isEmpty()) {
        QMessageBox::warning(this, "Пустая заметка",
                             "Нельзя сохранить пустую заметку. Пожалуйста, введите текст.");
        ui->note_text->setFocus();
        return;
    }

    if (title.isEmpty()) {
        QMessageBox::warning(this, "Пустой заголовок",
                             "Пожалуйста, введите заголовок заметки.");
        ui->note_title->setFocus();
        return;
    }

    QStringList tags = QStringList(currentTags.begin(), currentTags.end());
    Note note(title, text, tags);
    if (note.save()) {
        tagsManager.addTags(tags);
        tagsManager.save();
        updateCompleter();
        syncNote(note);
        ui->pages->setCurrentIndex(0);
        QString currentText = ui->search_input->text();
        ui->search_input->setText("");
        ui->search_input->setText(currentText);
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить заметку.");
    }
}

void MainWindow::on_search_input_textChanged(const QString &text)
{
    QLayoutItem *child;
    while ((child = ui->note_container->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QString searchText = text.trimmed();
    if (searchText.isEmpty()) return;

    QList<Note> notes = noteManager.searchNotes(searchText);
    for (const Note &note : notes) {
        QHBoxLayout *noteLayout = new QHBoxLayout;
        QLabel *titleLabel = new QLabel(note.title());
        noteLayout->addWidget(titleLabel);

        for (const QString &tag : note.tags()) {
            QPushButton *tagButton = new QPushButton(tag);
            tagButton->setObjectName(tag);
            tagButton->setStyleSheet("border: 1px solid gray; border-radius: 3px; padding: 2px 5px;");
            noteLayout->addWidget(tagButton);
        }

        QSpacerItem *spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
        noteLayout->addItem(spacer);

        QPushButton *editButton = new QPushButton("Редактировать");
        noteLayout->addWidget(editButton);
        connect(editButton, &QPushButton::clicked, this, [=]() {
            ui->note_title->setText(note.title());
            ui->note_text->setPlainText(note.text());

            currentTags.clear();
            QLayoutItem *child;
            int itemCount = ui->tags_container->count();

            if (itemCount != 1) {
                for (int i = 0; i < itemCount - 1; ++i) {
                    child = ui->tags_container->takeAt(0);
                    if (child) {
                        delete child->widget();
                        delete child;
                    }
                }
            }

            for (const QString &tag : note.tags()) {
                addTag(tag);
            }

            ui->pages->setCurrentIndex(3);
        });

        QPushButton *deleteButton = new QPushButton("Удалить");
        noteLayout->addWidget(deleteButton);
        connect(deleteButton, &QPushButton::clicked, this, [=]() {
            note.remove();
            on_search_input_textChanged(searchText);
        });

        editButton->setMinimumWidth(100);
        deleteButton->setMinimumWidth(70);
        editButton->setMaximumWidth(100);
        deleteButton->setMaximumWidth(70);

        noteLayout->setStretch(noteLayout->indexOf(titleLabel), 4);
        noteLayout->setStretch(noteLayout->indexOf(editButton), 1);
        noteLayout->setStretch(noteLayout->indexOf(deleteButton), 1);

        QWidget *noteWidget = new QWidget;
        noteWidget->setLayout(noteLayout);
        noteWidget->setMinimumHeight(50);
        noteWidget->setMaximumHeight(50);
        ui->note_container->addWidget(noteWidget);
    }
}

void MainWindow::on_set_save_place_button_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "Выберите папку для сохранения", SettingsManager::instance().saveLocation());
    if (!path.isEmpty()) {
        SettingsManager::instance().setSaveLocation(path);
        ui->set_save_place_button->setText(path);
    }
}

void MainWindow::on_open_register_button_clicked()
{
    SettingsManager &s = SettingsManager::instance();
    QString baseUrl = s.syncUrl();
    if (baseUrl.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите URL синхронизации перед регистрацией.");
        return;
    }
    QUrl url(baseUrl);
    QString registerUrl = url.toString(QUrl::RemovePath | QUrl::StripTrailingSlash) + "/register";
    QDesktopServices::openUrl(QUrl(registerUrl));
}

void MainWindow::syncNote(const Note &note)
{
    SettingsManager &s = SettingsManager::instance();
    if (s.syncUrl().isEmpty() || s.login().isEmpty() || s.password().isEmpty()) {
        ui->sync_warn_label->setText("Настройте параметры синхронизации");
        ui->sync_warn_label_on_settings->setText("Настройте параметры синхронизации");
        return;
    }

    ui->sync_warn_label->setText("Синхронизация...");
    ui->sync_warn_label_on_settings->setText("Синхронизация...");

    QUrl baseUrl(s.syncUrl());
    QString notesUrl = baseUrl.toString(QUrl::RemovePath | QUrl::StripTrailingSlash) + "/notes";

    QJsonObject noteJson;
    noteJson["title"] = note.title();
    noteJson["text"] = note.text();
    QJsonArray tagsArray;
    for (const QString &tag : note.tags())
        tagsArray.append(tag);
    noteJson["tags"] = tagsArray;

    QNetworkRequest request{QUrl(notesUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Login", s.login().toUtf8());
    request.setRawHeader("Password", s.password().toUtf8());

    QJsonDocument doc(noteJson);
    QNetworkReply *reply = networkManager->post(request, doc.toJson(QJsonDocument::Indented));
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleSyncReply(reply);
    });
}

void MainWindow::performSync()
{
    SettingsManager &s = SettingsManager::instance();
    if (s.syncUrl().isEmpty() || s.login().isEmpty() || s.password().isEmpty()) {
        ui->sync_warn_label->setText("Настройте параметры синхронизации");
        ui->sync_warn_label_on_settings->setText("Настройте параметры синхронизации");
        return;
    }

    if (s.saveLocation().isEmpty()) {
        ui->sync_warn_label->setText("Укажите папку для сохранения заметок");
        ui->sync_warn_label_on_settings->setText("Укажите папку для сохранения заметок");
        return;
    }

    ui->sync_warn_label->setText("Синхронизация...");
    ui->sync_warn_label_on_settings->setText("Синхронизация...");

    QString saveLocation = s.saveLocation();
    if (!saveLocation.isEmpty()) {
        QDir dir(saveLocation);
        dir.setNameFilters(QStringList() << "*.json");
        for (const QString &fileName : dir.entryList(QDir::Files)) {
            dir.remove(fileName);
        }
    }
    tagsManager.clearTags();
    tagsManager.save();
    updateCompleter();

    QUrl baseUrl(s.syncUrl());
    QString notesUrl = baseUrl.toString(QUrl::RemovePath | QUrl::StripTrailingSlash) + "/notes";

    QNetworkRequest request{QUrl(notesUrl)};
    request.setRawHeader("Login", s.login().toUtf8());
    request.setRawHeader("Password", s.password().toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleSyncReply(reply);
    });
}

void MainWindow::handleSyncReply(QNetworkReply *reply)
{
    QString errorMessage;
    if (reply->error() != QNetworkReply::NoError) {
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        switch (httpStatus) {
        case 401:
            errorMessage = "Неверные логин или пароль";
            break;
        case 404:
            errorMessage = "Сервер недоступен";
            break;
        default:
            errorMessage = QString("Ошибка сервера: %1").arg(httpStatus);
            break;
        }
        ui->sync_warn_label->setText(errorMessage);
        ui->sync_warn_label_on_settings->setText(errorMessage);
        reply->deleteLater();
        return;
    }

    if (reply->operation() == QNetworkAccessManager::GetOperation) {
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isArray()) {
            QJsonArray notesArray = doc.array();
            qDebug() << "Received" << notesArray.size() << "notes from server";
            for (const QJsonValue &value : notesArray) {
                QJsonObject obj = value.toObject();
                QString title = obj["title"].toString();
                QString text = obj["text"].toString();
                QJsonArray tagsArray = obj["tags"].toArray();
                QStringList tags;
                for (const QJsonValue &tag : tagsArray)
                    tags << tag.toString();

                Note note(title, text, tags);
                if (note.save()) {
                    qDebug() << "Saved note:" << title;
                    tagsManager.addTags(tags);
                } else {
                    qDebug() << "Failed to save note:" << title;
                }
            }
            tagsManager.save();
            updateCompleter();
            ui->sync_warn_label->setText("Синхронизация завершена");
            ui->sync_warn_label_on_settings->setText("Синхронизация завершена");
        } else {
            qDebug() << "Invalid response format:" << responseData;
            ui->sync_warn_label->setText("Неверный формат данных от сервера");
            ui->sync_warn_label_on_settings->setText("Неверный формат данных от сервера");
        }
    } else {
        ui->sync_warn_label->setText("Синхронизация завершена");
        ui->sync_warn_label_on_settings->setText("Синхронизация завершена");
    }

    reply->deleteLater();
}
