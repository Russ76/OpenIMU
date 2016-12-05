#include <QFileDialog>
#include "QTableView"
#include <QListWidgetItem>
#include<vector>
#include<QDebug>
#include "widgets/AlgorithmTab.h"
#include "widgets/ResultsTabWidget.h"
#include "AccDataDisplay.h"
#include "QMessageBox"
#include "mainwindow.h"
#include "iostream"
#include <QtConcurrent/QtConcurrentRun>
#include <QByteArray>
#include "widgets/RecordViewWidget.h"

QT_CHARTS_USE_NAMESPACE

const QString frenchText = "French";
const QString englishText = "English";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    //Execute launchApi in a thread
    QtConcurrent::run(MainWindow::launchApi);

    this->setWindowIcon(QIcon("../applicationOpenimu/app/icons/logo.ico"));

    this->grabGesture(Qt::PanGesture);
    this->grabGesture(Qt::PinchGesture);

    this->setWindowTitle(QString::fromUtf8("Open-IMU"));
    this->setMinimumSize(1000,700);

    menu = new ApplicationMenuBar(this);
    statusBar = new QStatusBar(this);
    mainWidget = new MainWidget(this);
    listWidget = new myTreeWidget(this);

    //Set QTreeWidget Column Header
    QTreeWidgetItem* headerItem = new QTreeWidgetItem();
    headerItem->setText(0,QString("Enregistrements"));
    listWidget->setHeaderItem(headerItem);

    tabWidget = new QTabWidget;
    spinnerStatusBar = new QLabel;
    movieSpinnerBar = new QMovie("../applicationOpenimu/app/icons/loaderStatusBar.gif");

    spinnerStatusBar->setMovie(movieSpinnerBar);

    this->setMenuBar(menu);
    this->setStatusBar(statusBar);

    statusBar->setStyleSheet("background-color:rgba(230, 233, 239,0.2);");
    listWidget->setAlternatingRowColors(true);
    listWidget->setStyleSheet("alternate-background-color:#ecf0f1;background-color:white;");

    QPushButton* addRecord = new QPushButton("");
    QIcon img(":/icons/addrecord.png");
    addRecord->setIcon(img);
    addRecord->setIconSize(QSize(20,20));
    addRecord->setCursor(Qt::PointingHandCursor);

    QPushButton* deleteRecord = new QPushButton("");
    QIcon imgd(":/icons/trash.png");
    deleteRecord->setIcon(imgd);
    deleteRecord->setIconSize(QSize(20,20));
    deleteRecord->setCursor(Qt::PointingHandCursor);

    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->addWidget(addRecord);
    vlayout->addWidget(listWidget);
    vlayout->addWidget(deleteRecord);
    mainWidget->mainLayout->addLayout(vlayout);

    listWidget->setMaximumWidth(150);
    tabWidget->setTabsClosable(true);
    homeWidget = new HomeWidget(this);
    tabWidget->addTab(homeWidget,tr("Accueil"));
    tabWidget->setStyleSheet("background: rgb(247, 250, 255,0.6)");
    tabWidget->setCurrentWidget(tabWidget->widget(0));
    tabWidget->grabGesture(Qt::PanGesture);
    tabWidget->grabGesture(Qt::PinchGesture);
    mainWidget->mainLayout->addWidget(tabWidget);

    setCentralWidget(mainWidget);
    setStatusBarText(tr("Prêt"));
    statusBar->setMinimumHeight(20);
    statusBar->addPermanentWidget(spinnerStatusBar);

    connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));
    connect(addRecord, SIGNAL(clicked()), this, SLOT(openRecordDialog()));
    connect(deleteRecord, SIGNAL(clicked()), this, SLOT(deleteRecordFromList()));

    getSavedResultsFromDB();
    getRecordsFromDB();
}

MainWindow::~MainWindow(){
    delete menu ;
}

void MainWindow::onListItemClicked(QTreeWidgetItem* item, int column)
{
    for(int i=0; i<record.m_WimuRecordList.size();i++)
    {
        if(record.m_WimuRecordList.at(i).m_recordName.compare(item->text(column).toStdString()) == 0)
        {
            selectedRecord = record.m_WimuRecordList.at(i);
        }
    }
    setStatusBarText(tr("Prêt"));
}

void MainWindow::onListItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    setStatusBarText(tr("Chargement de l'enregistrement..."));
    startSpinner();
    bool isRecord = false;
    for(int i=0; i<record.m_WimuRecordList.size();i++)
    {

        if(record.m_WimuRecordList.at(i).m_recordName.compare(item->text(column).toStdString()) == 0)
        {

            selectedRecord = record.m_WimuRecordList.at(i);

            getDataFromUUIDFromDB(selectedRecord.m_recordId);


            if(wimuAcquisition.getDataSize()<=0)
            {
                QMessageBox msgBox(
                            QMessageBox::Question,
                            trUtf8("Avertissement"),
                            "L'enregistrement sélectionné est corrompu. Voulez-vous le supprimer?",
                            QMessageBox::Yes | QMessageBox::No);

                msgBox.setButtonText(QMessageBox::Yes, "Oui");
                msgBox.setButtonText(QMessageBox::No, "Non");


                if (msgBox.exec() == QMessageBox::Yes) {
                  deleteRecordFromUUID(selectedRecord.m_recordId);
                  openFile();
                }
            }
            else
            {
               RecordViewWidget* recordTab = new RecordViewWidget(this,wimuAcquisition,selectedRecord);
               addTab(recordTab,selectedRecord.m_recordName);
            }

            isRecord = true;
        }        
    }
    if(!isRecord)
    {
        setStatusBarText(tr("Chargement du résultat..."));
        for(int i=0; i<savedResults.m_algorithmOutputList.size();i++)
        {
            if(savedResults.m_algorithmOutputList.at(i).m_resultName.compare(item->text(column).toStdString()) == 0)
            {
                ResultsTabWidget* resultTab = new ResultsTabWidget(this,savedResults.m_algorithmOutputList.at(i));
                addTab(resultTab,savedResults.m_algorithmOutputList.at(i).m_resultName);

            }
        }
    }
    stopSpinner();
    setStatusBarText(tr("Prêt"));
}

void MainWindow::startSpinner()
{
    spinnerStatusBar->show();
    movieSpinnerBar->start();
}

void MainWindow::stopSpinner(bool playAudio)
{
    movieSpinnerBar->stop();
    spinnerStatusBar->hide();

    if(playAudio)
    {
        Utilities utilities;
        utilities.playAudio();
    }
}

void MainWindow:: openFile(){
    getSavedResultsFromDB();
    getRecordsFromDB();
}

void MainWindow::openRecordDialog()
{
    rDialog = new RecordsDialog(this);
    rDialog->show();
}

void MainWindow::openAlgorithmTab()
{
    algorithmTab = new AlgorithmTab(this,selectedRecord);
    addTab(algorithmTab,"Algorithmes: "+selectedRecord.m_recordName);
}

void MainWindow::setStatusBarText(QString txt, MessageStatus status)
{
    statusBar->showMessage(tr(txt.toStdString().c_str()));

    QString styleSheet = "color: " + Utilities::getColourFromEnum(status) +"; font: 16px;";
    statusBar->setStyleSheet(styleSheet);
}

void MainWindow::deleteRecord()
{
    startSpinner();
    QMessageBox msgBox;
    msgBox.setText("Suppression de l'enregistrement");
    msgBox.setInformativeText("Êtes vous sûr de vouloir supprimer cet enregistrement?");

    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    switch (ret) {
      case QMessageBox::Ok:
        deleteRecordFromUUID(selectedRecord.m_recordId);
        getRecordsFromDB();
        wimuAcquisition.clearData();
        selectedRecord.m_recordId = "";
        tabWidget->removeTab(tabWidget->currentIndex());
        openHomeTab();
          break;
      case QMessageBox::Cancel:
          // Cancel was clicked
          break;
      default:
          // should never be reached
          break;
    }

    stopSpinner(true);
}

void MainWindow::openHomeTab()
{
    homeWidget = new HomeWidget(this);
    addTab(homeWidget,"Accueil");
}

bool MainWindow::getRecordsFromDB()
{
    startSpinner();
    setStatusBarText("Chargement des enregistrements...");

    QNetworkRequest request(QUrl("http://127.0.0.1:5000/records"));
    request.setRawHeader("User-Agent", "ApplicationNameV01");
    request.setRawHeader("Content-Type", "application/json");

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->get(request);

    bool result = connect(manager, SIGNAL(finished(QNetworkReply*)), this ,SLOT(reponseRecue(QNetworkReply*)));

    stopSpinner();
    return result;
}

//Getting results from DB
bool MainWindow::getSavedResultsFromDB()
{
    startSpinner();
    setStatusBarText("Chargement des résultats sauvegardés...");

    QNetworkRequest request(QUrl("http://127.0.0.1:5000/algoResults"));
    request.setRawHeader("User-Agent", "ApplicationNameV01");
    request.setRawHeader("Content-Type", "application/json");

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->get(request);

    bool result;
    QEventLoop loop;
    result = connect(manager, SIGNAL(finished(QNetworkReply*)), &loop,SLOT(quit()));
    loop.exec();
    savedResultsReponse(reply);

    stopSpinner();
    return true;
}

bool MainWindow::getDataFromUUIDFromDB(std::string uuid)
{
    std::string url = "http://127.0.0.1:5000/data?uuid="+uuid;
    QNetworkRequest request(QUrl(QString::fromStdString(url)));
    request.setRawHeader("User-Agent", "ApplicationNameV01");
    request.setRawHeader("Content-Type", "application/json");

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->get(request);
    QEventLoop loop;
    bool result = connect(manager, SIGNAL(finished(QNetworkReply*)), &loop,SLOT(quit()));
    loop.exec();
    reponseRecueAcc(reply);
    return true;
}

void MainWindow::reponseRecueAcc(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError)
   {
       wimuAcquisition.clearData();
       std::string testReponse(reply->readAll());
       CJsonSerializer::Deserialize(&wimuAcquisition, testReponse);

   }
   else
   {
       qDebug() << "error connect";
       qWarning() <<"ErrorNo: "<< reply->error() << "for url: " << reply->url().toString();
       qDebug() << "Request failed, " << reply->errorString();
       qDebug() << "Headers:"<<  reply->rawHeaderList()<< "content:" << reply->readAll();
       qDebug() << reply->readAll();
   }
   delete reply;
}

void MainWindow::savedResultsReponse(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        savedResults.m_algorithmOutputList.clear();
        std::string reponse = reply->readAll().toStdString();

        if(reponse != "")
        {
            savedResults.DeserializeList(reponse);
        }

        setStatusBarText("Prêt");
    }
    else
    {
        setStatusBarText("Erreur lors de la récupération des résultats", MessageStatus::error);
    }
}


void MainWindow::reponseRecue(QNetworkReply* reply)
{
   if (reply->error() == QNetworkReply::NoError)
   {
       std::string testReponse(reply->readAll());
       record.m_WimuRecordList.clear();

       CJsonSerializer::Deserialize(&record, testReponse);

       listWidget->clear();
       for(int i=0; i<record.m_WimuRecordList.size();i++)
       {
           QTreeWidgetItem* top_item = new QTreeWidgetItem();
           top_item->setText(0,QString::fromStdString(record.m_WimuRecordList.at(i).m_recordName));
           top_item->setIcon(0,*(new QIcon(":/icons/file2.png")));

           if(record.m_WimuRecordList.at(i).m_parentId.compare("") == 0 )
           {
               for(int j=0; j<record.m_WimuRecordList.size();j++)
               {
                   if(record.m_WimuRecordList.at(j).m_parentId.compare(record.m_WimuRecordList.at(i).m_recordId ) == 0)
                   {
                       QTreeWidgetItem* child_item = new QTreeWidgetItem;
                       child_item->setText(0,QString::fromStdString(record.m_WimuRecordList.at(j).m_recordName));
                       child_item->setIcon(0,*(new QIcon(":/icons/sliced.png")));
                       top_item->addChild(child_item);
                   }
               }
               for(int w = 0; w<savedResults.m_algorithmOutputList.size();w++)
               {
                   if(savedResults.m_algorithmOutputList.at(w).m_recordId.compare(record.m_WimuRecordList.at(i).m_recordId) == 0)
                   {
                       QTreeWidgetItem* child_item = new QTreeWidgetItem;
                       child_item->setText(0,QString::fromStdString(savedResults.m_algorithmOutputList.at(w).m_resultName));
                       child_item->setIcon(0,*(new QIcon(":/icons/results.png")));
                       top_item->addChild(child_item);
                   }
               }
               listWidget->addTopLevelItem(top_item);
           }
       }

       setStatusBarText("Prêt");
   }
   else
   {
       setStatusBarText("Erreur de connexion lors de la récupération des enregistrements", MessageStatus::error);
       qDebug() << "error connect";
   }
   delete reply;
}

//Delete specific record
bool MainWindow::deleteRecordFromUUID(std::string uuid)
{
    std::string url = "http://127.0.0.1:5000/delete?uuid="+uuid;
    QNetworkRequest request(QUrl(QString::fromStdString(url)));
    request.setRawHeader("User-Agent", "ApplicationNameV01");
    request.setRawHeader("Content-Type", "application/json");

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->get(request);
    QEventLoop loop;
    bool result = connect(manager, SIGNAL(finished(QNetworkReply*)), &loop,SLOT(quit()));
    loop.exec();
    reponseRecueDelete(reply);
    return true;
}

//Rename specific record
bool MainWindow::renameRecordFromUUID(std::string uuid, std::string newname)
{
    std::string url = "http://127.0.0.1:5000/renamerecord/"+uuid+"?name="+newname;

    qDebug() << QString::fromStdString(url);
    QNetworkRequest request(QUrl(QString::fromStdString(url)));
    QByteArray dataByteArray (newname.c_str(),newname.length());
    QByteArray postDataSize = QByteArray::number(dataByteArray.size());

    request.setRawHeader("User-Agent", "ApplicationNameV01");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);


    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkReply *reply = manager->post(request, dataByteArray);

    QEventLoop loop;
    bool result = connect(manager, SIGNAL(finished(QNetworkReply*)), &loop,SLOT(quit()));
    loop.exec();
    reponseRecueRename(reply);
    return true;
}

bool MainWindow::deleteRecordFromList()
{
    QMessageBox msgBox;
    msgBox.setText("Suppression de l'enregistrement");
    msgBox.setInformativeText("Êtes vous sûr de vouloir supprimer cet enregistrement?");


    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    switch (ret) {
      case QMessageBox::Ok:
        deleteRecordFromUUID(selectedRecord.m_recordId);
        getRecordsFromDB();
        break;
    case QMessageBox::Cancel:
        // Cancel was clicked
        break;
    default:
        // should never be reached
        break;
  }
    return true;
}
void MainWindow::reponseRecueDelete(QNetworkReply* reply)
{
   if (reply->error() == QNetworkReply::NoError)
   {
       setStatusBarText(tr("Enregistrement effacé avec succès"), MessageStatus::success);
   }
   else
   {
        setStatusBarText(tr("Échec de la suppression de l'enregistrement"), MessageStatus::error);
   }
}

void MainWindow::reponseRecueRename(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        setStatusBarText(tr("Enregistrement renommé avec succès"), MessageStatus::success);
    }
    else
    {
         setStatusBarText(tr("Échec du changement de nom de l'enregistrement"), MessageStatus::error);
    }
}

void MainWindow::setApplicationInEnglish()
{
    menu->setUncheck(frenchText);
    //TODO: Olivier, insert change language logic here
}

void MainWindow::setApplicationInFrench()
{
    menu->setUncheck(englishText);
    //TODO: Olivier, insert change language logic here
}

void MainWindow::openAbout(){

    aboutDialog = new AboutDialog(this);
    aboutDialog->exec();
}

void MainWindow::openHelp(){
    helpDialog = new HelpDialog(this);
    helpDialog->exec();
}

void MainWindow::addTab(QWidget * tab, std::string label)
{
    int index = 0;
    bool found  = false;
    QString currentTabText;

    for(int i=0; i<tabWidget->count();i++){
        currentTabText = tabWidget->tabText(i);
        if(currentTabText == tr("Accueil")){
            tabWidget->removeTab(i);
        }
    }
    for(int i=0; i<tabWidget->count();i++){
        currentTabText = tabWidget->tabText(i);
        if(currentTabText == QString::fromStdString(label)){
            index = i;
            found = true;
            tabWidget->setCurrentWidget(tabWidget->widget(i));
        }
    }
    if(found)
    {
        tabWidget->setCurrentWidget(tabWidget->widget(index));
    }
    else
    {
        tabWidget->addTab(tab,QString::fromStdString(label));
        tabWidget->setCurrentWidget(tabWidget->widget(tabWidget->count()-1));
    }

    setStatusBarText(tr("Prêt"));
}
void MainWindow::onTabChanged(int index)
{
    if (index == -1) {
        return;
    }

    for(int i=0; i<record.m_WimuRecordList.size();i++)
    {
        if(record.m_WimuRecordList.at(i).m_recordName.compare(tabWidget->tabText(index).toStdString()) == 0)
        {
            selectedRecord = record.m_WimuRecordList.at(i);
        }
    }
}

void MainWindow::closeTab(int index){

    if (index == -1) {
        return;
    }
    QWidget* tabItem = tabWidget->widget(index);
    // Removes the tab at position index from this stack of widgets.
    // The page widget itself is not deleted.
    tabWidget->removeTab(index);

    delete(tabItem);
    tabItem = nullptr;
}

void MainWindow::closeWindow(){
    this->close();
}
