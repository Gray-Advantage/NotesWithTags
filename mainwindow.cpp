#include "mainwindow.h"
#include "settingsmanager.h"
#include "themeutils.h"
#include "tagsmanager.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

    if (searchText.isEmpty())
        return;

    QDir dir("notes");
    QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &file : files) {
        QFile f(dir.filePath(file));
        if (!f.open(QIODevice::ReadOnly))
            continue;

        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        QJsonObject obj = doc.object();

        QString title = obj["title"].toString();
        QJsonArray tags = obj["tags"].toArray();

        // Поиск по заголовку или тегу
        if (title.contains(searchText, Qt::CaseInsensitive) ||
            std::any_of(tags.begin(), tags.end(), [&](const QJsonValue &v) {
                return v.toString().contains(searchText, Qt::CaseInsensitive);
            })) {

            // Загрузим данные в форму редактирования
            ui->note_title->setText(title);
            ui->note_text->setPlainText(obj["text"].toString());

            // Очищаем контейнер с тегами
            currentTags.clear();
            QLayoutItem *child;
            while ((child = ui->tags_container->takeAt(0)) != nullptr) {
                delete child->widget();
                delete child;
            }

            // Добавляем теги на страницу редактирования
            for (const QJsonValue &tagVal : tags) {
                addTag(tagVal.toString()); // Функция добавления тега
            }

            ui->pages->setCurrentIndex(3); // перейти на экран редактирования
            return;
        }
    }

    QMessageBox::information(this, "Не найдено", "Заметка по запросу не найдена.");
}



void MainWindow::on_add_new_entry_button_clicked()
{
    currentTags.clear();
    QLayoutItem *child;
    int itemCount = ui->tags_container->count();

    if (itemCount != 1) {
        // Проходим по всем элементам, кроме последнего (пружины)
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


// Settings page


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
    ui->pages->setCurrentIndex(3);
}


void MainWindow::on_back_button_clicked()
{
    ui->pages->setCurrentIndex(0);
}


// Note edit page


void MainWindow::addTag(const QString &tag) {
    if (currentTags.contains(tag)) return; // не добавлять повторно

    currentTags.insert(tag);

    QPushButton *btn = new QPushButton(tag);
    btn->setObjectName(tag); // <-- Важно!
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
    ui->search_input->setText("");  // Очистим поле
    ui->search_input->setText(currentText);  // Возвращаем прежний текст
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

    QStringList tags = QStringList(currentTags.begin(), currentTags.end());

    if (title.isEmpty()) {
        QMessageBox::warning(this, "Пустой заголовок",
                             "Пожалуйста, введите заголовок заметки.");
        ui->note_title->setFocus();
        return;
    }

    QDir().mkpath("notes");
    QString filePath = "notes/" + title + ".json";

    // Убираем проверку на существование файла, чтобы разрешить редактирование с тем же заголовком
    QJsonObject noteJson;
    noteJson["title"] = title;
    noteJson["text"] = text;

    QJsonArray tagsArray;
    for (const QString &tag : tags)
        tagsArray.append(tag);
    noteJson["tags"] = tagsArray;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(noteJson).toJson(QJsonDocument::Indented));
        file.close();
    }

    tagsManager.addTags(tags);
    tagsManager.save();
    updateCompleter();

    ui->pages->setCurrentIndex(0); // Возвращаемся на главную страницу
    QString currentText = ui->search_input->text();
    ui->search_input->setText("");  // Очистим поле
    ui->search_input->setText(currentText);  // Возвращаем прежний текст
}


void MainWindow::on_search_input_textChanged(const QString &text)
{
    // Очищаем текущий контейнер с заметками
    QLayoutItem *child;
    while ((child = ui->note_container->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QString searchText = text.trimmed();
    if (searchText.isEmpty()) return;

    QDir dir("notes");
    QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &file : files) {
        QFile f(dir.filePath(file));
        if (!f.open(QIODevice::ReadOnly))
            continue;

        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        QJsonObject obj = doc.object();

        QString title = obj["title"].toString();
        QJsonArray tags = obj["tags"].toArray();

        // Поиск по заголовку или тегу
        if (title.contains(searchText, Qt::CaseInsensitive) ||
            std::any_of(tags.begin(), tags.end(), [&](const QJsonValue &v) {
                return v.toString().contains(searchText, Qt::CaseInsensitive);
            })) {

            // Создаем кнопку для заголовка заметки
            QHBoxLayout *noteLayout = new QHBoxLayout;

            QLabel *titleLabel = new QLabel(title);
            noteLayout->addWidget(titleLabel);

            // Добавляем кнопки для тегов
            for (const QJsonValue &tagVal : tags) {
                QPushButton *tagButton = new QPushButton(tagVal.toString());
                tagButton->setObjectName(tagVal.toString());
                tagButton->setStyleSheet("border: 1px solid gray; border-radius: 3px; padding: 2px 5px;");
                noteLayout->addWidget(tagButton);

                connect(tagButton, &QPushButton::clicked, this, [=]() {
                    // Действие по нажатию кнопки (например, удаление записи или переход в редактирование)
                });
            }

            // Добавляем "пружину" (спейсер) между тегами и кнопками
            QSpacerItem *spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
            noteLayout->addItem(spacer);  // Этот спейсер создаст промежуток

            // Кнопка редактирования
            QPushButton *editButton = new QPushButton("Редактировать");
            noteLayout->addWidget(editButton);
            connect(editButton, &QPushButton::clicked, this, [=]() {
                // Действие по редактированию заметки
                ui->note_title->setText(title);
                ui->note_text->setPlainText(obj["text"].toString());

                currentTags.clear();
                QLayoutItem *child;
                int itemCount = ui->tags_container->count();

                if (itemCount != 1) {
                    // Проходим по всем элементам, кроме последнего (пружины)
                    for (int i = 0; i < itemCount - 1; ++i) {
                        child = ui->tags_container->takeAt(0);
                        if (child) {
                            delete child->widget();
                            delete child;
                        }
                    }
                }

                // Добавляем теги из заметки
                for (const QJsonValue &tagVal : tags) {
                    addTag(tagVal.toString()); // Функция добавления тега
                }

                ui->pages->setCurrentIndex(3); // Переход на страницу редактирования
            });

            // Кнопка удаления
            QPushButton *deleteButton = new QPushButton("Удалить");
            noteLayout->addWidget(deleteButton);
            connect(deleteButton, &QPushButton::clicked, this, [=]() {
                // Действие по удалению заметки
                QFile::remove(dir.filePath(file));
                on_search_input_textChanged(searchText); // Перезагрузим результаты после удаления
            });

            // Устанавливаем размеры кнопок
            editButton->setMinimumWidth(100);
            deleteButton->setMinimumWidth(70);
            editButton->setMaximumWidth(100);
            deleteButton->setMaximumWidth(70);

            // Устанавливаем растяжение для кнопок и заголовка
            noteLayout->setStretch(noteLayout->indexOf(titleLabel), 4);
            noteLayout->setStretch(noteLayout->indexOf(editButton), 1);
            noteLayout->setStretch(noteLayout->indexOf(deleteButton), 1);

            // Добавляем layout для записи в общий контейнер
            QWidget *noteWidget = new QWidget;
            noteWidget->setLayout(noteLayout);
            noteWidget->setMinimumHeight(50);
            noteWidget->setMaximumHeight(50);
            ui->note_container->addWidget(noteWidget);
        }
    }
}


