#include "musicdownloadwidget.h"
#include "ui_musicdownloadwidget.h"
#include "musicuiobject.h"
#include "musicsettingmanager.h"
#include "musicnetworkthread.h"
#include "musicmydownloadrecordconfigmanager.h"
#include "musicdatadownloadthread.h"
#include "musicttdatadownloadthread.h"
#include "musicbackgrounddownload.h"
#include "musicmessagebox.h"
#include "musicdownloadqueryfactory.h"

#include <QFileDialog>
#include <QLabel>

MusicDownloadTableItem::MusicDownloadTableItem(QWidget *parent)
    : QWidget(parent)
{
    m_infomation = new QLabel(this);
    m_icon = new QLabel(this);
    m_text = new QLabel(this);

    m_text->setGeometry(0, 0, 60, ROW_HEIGHT);
    m_icon->setGeometry(70, 0, 30, ROW_HEIGHT);
    m_infomation->setGeometry(170, 0, 150, ROW_HEIGHT);
}

MusicDownloadTableItem::~MusicDownloadTableItem()
{
    delete m_infomation;
    delete m_icon;
    delete m_text;
}

QString MusicDownloadTableItem::getClassName()
{
    return staticMetaObject.className();
}

void MusicDownloadTableItem::setIcon(const QString &name)
{
    m_icon->setPixmap(QPixmap(name).scaled(28, 18));
}

void MusicDownloadTableItem::setInfomation(const QString &info)
{
    m_infomation->setText(info);
}

void MusicDownloadTableItem::setText(const QString &text)
{
    m_text->setText(text);
}



MusicDownloadTableWidget::MusicDownloadTableWidget(QWidget *parent)
    : MusicAbstractTableWidget(parent)
{
    setColumnCount(1);
    QHeaderView *headerview = horizontalHeader();
    headerview->resizeSection(0, 400);
    MusicUtils::Widget::setTransparent(this, 255);
}

MusicDownloadTableWidget::~MusicDownloadTableWidget()
{
    clearAllItems();
}

QString MusicDownloadTableWidget::getClassName()
{
    return staticMetaObject.className();
}

void MusicDownloadTableWidget::clearAllItems()
{
    while(!m_items.isEmpty())
    {
        delete m_items.takeFirst();
    }
    MusicAbstractTableWidget::clear();
    setColumnCount(1);
}

void MusicDownloadTableWidget::createItem(int bitrate, const QString &type, const QString &info, const QString &icon)
{
    int index = rowCount();
    setRowCount(index + 1);
    setRowHeight(index, ROW_HEIGHT);
    QTableWidgetItem *it = new QTableWidgetItem;
    it->setData(BITRATE_ROLE, bitrate);
    setItem(index, 0,  it);

    MusicDownloadTableItem *item = new MusicDownloadTableItem(this);
    item->setIcon(icon);
    item->setInfomation(info);
    item->setText(type);
    m_items << item;

    setCellWidget(index, 0, item);
}

int MusicDownloadTableWidget::getCurrentBitrate()
{
   int bitrate = -1;
   int row = currentRow();
   if(row == -1)
   {
       return bitrate;
   }

   return item(row, 0)->data(BITRATE_ROLE).toInt();
}

void MusicDownloadTableWidget::listCellClicked(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
}



MusicDownloadWidget::MusicDownloadWidget(QWidget *parent)
    : MusicAbstractMoveWidget(parent),
      ui(new Ui::MusicDownloadWidget)
{
    ui->setupUi(this);

    ui->topTitleCloseButton->setIcon(QIcon(":/functions/btn_close_hover"));
    ui->topTitleCloseButton->setStyleSheet(MusicUIObject::MToolButtonStyle03);
    ui->topTitleCloseButton->setCursor(QCursor(Qt::PointingHandCursor));
    ui->topTitleCloseButton->setToolTip(tr("Close"));

    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);

    ui->downloadPathEdit->setStyleSheet(MusicUIObject::MLineEditStyle01);
    ui->pathChangedButton->setStyleSheet(MusicUIObject::MPushButtonStyle03);
    ui->settingButton->setStyleSheet(MusicUIObject::MPushButtonStyle03);
    ui->downloadButton->setStyleSheet(MusicUIObject::MPushButtonStyle06);

    m_querySingleInfo = false;
    m_downloadThread = M_DOWNLOAD_QUERY_PTR->getQueryThread(this);
    m_queryType = MusicDownLoadQueryThreadAbstract::MusicQuery;

    connect(ui->pathChangedButton, SIGNAL(clicked()), SLOT(downloadDirSelected()));
    connect(m_downloadThread, SIGNAL(downLoadDataChanged(QString)), SLOT(queryAllFinished()));
    connect(ui->topTitleCloseButton, SIGNAL(clicked()), SLOT(close()));
    connect(ui->downloadButton, SIGNAL(clicked()), SLOT(startToDownload()));
}

MusicDownloadWidget::~MusicDownloadWidget()
{
    delete ui;
    delete m_downloadThread;
}

QString MusicDownloadWidget::getClassName()
{
    return staticMetaObject.className();
}

void MusicDownloadWidget::initWidget()
{
    controlEnable(true);
    if(m_queryType == MusicDownLoadQueryThreadAbstract::MovieQuery)
    {
        ui->downloadPathEdit->setText(MOVIE_DIR_FULL);
    }
    else
    {
        ui->downloadPathEdit->setText(M_SETTING_PTR->value(MusicSettingManager::DownloadMusicPathDirChoiced).toString());
    }
}

void MusicDownloadWidget::controlEnable(bool enable)
{
    ui->topTitleCloseButton->setEnabled(enable);
    ui->downloadButton->setEnabled(enable);
    ui->pathChangedButton->setEnabled(enable);
    ui->settingButton->setEnabled(enable);
}

void MusicDownloadWidget::setSongName(const QString &name, MusicDownLoadQueryThreadAbstract::QueryType type)
{
    m_queryType = type;
    initWidget();

    ui->downloadName->setText(MusicUtils::Widget::elidedText(font(), name, Qt::ElideRight, 200));
    m_downloadThread->setQueryAllRecords(true);
    m_downloadThread->startSearchSong(type, name);
}

void MusicDownloadWidget::setSongName(const MusicObject::MusicSongInfomation &info,
                                      MusicDownLoadQueryThreadAbstract::QueryType type)
{
    m_queryType = type;
    m_singleSongInfo = info;
    m_querySingleInfo = true;

    initWidget();
    ui->downloadName->setText(MusicUtils::Widget::elidedText(font(), info.m_songName, Qt::ElideRight, 200));

    if(type == MusicDownLoadQueryThreadAbstract::MusicQuery)
    {
        queryAllFinishedMusic(info.m_songAttrs);
    }
    else if(type == MusicDownLoadQueryThreadAbstract::MovieQuery)
    {
        queryAllFinishedMovie(info.m_songAttrs);
    }
}

void MusicDownloadWidget::show()
{
    setBackgroundPixmap(ui->background, size());
    return MusicAbstractMoveWidget::show();
}

void MusicDownloadWidget::queryAllFinished()
{
    if(!M_NETWORK_PTR->isOnline())
    {
        return;
    }

    ui->viewArea->clearAllItems();
    if(m_queryType == MusicDownLoadQueryThreadAbstract::MusicQuery)
    {
        queryAllFinishedMusic();
    }
    else if(m_queryType == MusicDownLoadQueryThreadAbstract::MovieQuery)
    {
        queryAllFinishedMovie();
    }
}

void MusicDownloadWidget::queryAllFinishedMusic()
{
    MusicObject::MusicSongInfomations musicSongInfos(m_downloadThread->getMusicSongInfos());
    if(!musicSongInfos.isEmpty())
    {
        QString filename = m_downloadThread->getSearchedText();
        QString artistName = filename.split('-').front().trimmed();
        QString songName = filename.split('-').back().trimmed();
        MusicObject::MusicSongInfomation musicSongInfo = musicSongInfos.first();
        foreach(const MusicObject::MusicSongInfomation &var, musicSongInfos)
        {
            if( var.m_singerName.contains(artistName, Qt::CaseInsensitive) &&
                var.m_songName.contains(songName, Qt::CaseInsensitive) )
            {
                musicSongInfo = var;
                break;
            }
        }

        queryAllFinishedMusic(musicSongInfo.m_songAttrs);
    }
}

void MusicDownloadWidget::queryAllFinishedMusic(const MusicObject::MusicSongAttributes &attrs)
{
    foreach(const MusicObject::MusicSongAttribute &attr, attrs)
    {
        if(attr.m_bitrate == MB_32)         ///st
        {
            ui->viewArea->createItem(MB_32, tr("ST"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                     .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                     QString(":/quality/lb_st_quality"));
        }
        else if(attr.m_bitrate == MB_128)   ///sd
        {
            ui->viewArea->createItem(MB_128, tr("SD"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                     .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                     QString(":/quality/lb_sd_quality"));
        }
        else if(attr.m_bitrate == MB_192)   ///hd
        {
            ui->viewArea->createItem(MB_192, tr("HD"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                     .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                     QString(":/quality/lb_hd_quality"));
        }
        else if(attr.m_bitrate == MB_320)   ///sq
        {
            ui->viewArea->createItem(MB_320, tr("SQ"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                     .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                     QString(":/quality/lb_sq_quality"));
        }
        else if(attr.m_bitrate > MB_320)   ///cd
        {
            ui->viewArea->createItem(MB_500, tr("CD"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                     .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                     QString(":/quality/lb_cd_quality"));
        }
        else
        {
            continue;
        }
    }
    resizeWindow();
}

void MusicDownloadWidget::queryAllFinishedMovie()
{
    MusicObject::MusicSongInfomations musicSongInfos(m_downloadThread->getMusicSongInfos());
    if(!musicSongInfos.isEmpty())
    {
        QString filename = m_downloadThread->getSearchedText();
        QString artistName = filename.split('-').front().trimmed();
        QString songName = filename.split('-').back().trimmed();
        MusicObject::MusicSongInfomation musicSongInfo = musicSongInfos.first();
        foreach(const MusicObject::MusicSongInfomation &var, musicSongInfos)
        {
            if( var.m_singerName.contains(artistName, Qt::CaseInsensitive) &&
                var.m_songName.contains(songName, Qt::CaseInsensitive) )
            {
                musicSongInfo = var;
                break;
            }
        }

        queryAllFinishedMovie(musicSongInfo.m_songAttrs);
    }
}

void MusicDownloadWidget::queryAllFinishedMovie(const MusicObject::MusicSongAttributes &attrs)
{
    foreach(const MusicObject::MusicSongAttribute &attr, attrs)
    {
        if(attr.m_bitrate == MB_500)       ///hd
        {
            ui->viewArea->createItem(MB_500, tr("SD"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                     .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                     QString(":/quality/lb_sd_quality"));
        }
        else if(attr.m_bitrate == MB_750)  ///sq
        {
            ui->viewArea->createItem(MB_750, tr("HD"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                     .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                     QString(":/quality/lb_hd_quality"));
        }
        else if(attr.m_bitrate == MB_1000) ///cd
        {
            ui->viewArea->createItem(MB_1000, tr("SQ"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                     .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                     QString(":/quality/lb_sq_quality"));
        }
        else
        {
            continue;
        }
    }
    resizeWindow();
}

void MusicDownloadWidget::resizeWindow()
{
    int delta = ui->viewArea->rowCount();
    delta = (delta == 0) ? 0 : (delta - 1)*ROW_HEIGHT;

    setFixedHeightWidget(this, delta);
    setFixedHeightWidget(ui->backgroundMask, delta);
    setFixedHeightWidget(ui->background, delta);
    setFixedHeightWidget(ui->viewArea, delta);

    setMoveWidget(ui->label2, delta);
    setMoveWidget(ui->downloadPathEdit, delta);
    setMoveWidget(ui->pathChangedButton, delta);
    setMoveWidget(ui->settingButton, delta);
    setMoveWidget(ui->downloadButton, delta);

    setBackgroundPixmap(ui->background, size());
}

void MusicDownloadWidget::setFixedHeightWidget(QWidget *w, int height)
{
    w->setFixedHeight(w->height() + height);
}

void MusicDownloadWidget::setMoveWidget(QWidget *w, int pos)
{
    QRect rect = w->geometry();
    w->move(rect.x(), rect.y() + pos);
}

void MusicDownloadWidget::downloadDirSelected()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::Directory );
    dialog.setViewMode(QFileDialog::Detail);
    if(dialog.exec())
    {
        QString path;
        if(!(path = dialog.directory().absolutePath()).isEmpty())
        {
            if(m_queryType == MusicDownLoadQueryThreadAbstract::MusicQuery)
            {
                M_SETTING_PTR->setValue(MusicSettingManager::DownloadMusicPathDirChoiced, path + "/");
            }
            ui->downloadPathEdit->setText(path + "/");
        }
    }
}

void MusicDownloadWidget::startToDownload()
{
    hide(); ///hide download widget
    if(ui->viewArea->getCurrentBitrate() == -1)
    {
        MusicMessageBox message(tr("Please Select One Item First!"));
        message.exec();
        return;
    }

    if(m_queryType == MusicDownLoadQueryThreadAbstract::MusicQuery)
    {
        m_querySingleInfo ? startToDownloadMusic(m_singleSongInfo) : startToDownloadMusic();
    }
    else if(m_queryType == MusicDownLoadQueryThreadAbstract::MovieQuery)
    {
        m_querySingleInfo ? startToDownloadMovie(m_singleSongInfo) : startToDownloadMovie();
    }
    controlEnable(false);
}

void MusicDownloadWidget::dataDownloadFinished()
{
    emit dataDownloadChanged();
    close();
}

void MusicDownloadWidget::startToDownloadMusic()
{
    MusicObject::MusicSongInfomations musicSongInfos(m_downloadThread->getMusicSongInfos());
    if(!musicSongInfos.isEmpty())
    {
        startToDownloadMusic(musicSongInfos.first());
    }
}

void MusicDownloadWidget::startToDownloadMusic(const MusicObject::MusicSongInfomation &musicSongInfo)
{
    int bitrate = ui->viewArea->getCurrentBitrate();
    MusicObject::MusicSongAttributes musicAttrs = musicSongInfo.m_songAttrs;
    foreach(const MusicObject::MusicSongAttribute &musicAttr, musicAttrs)
    {
        if(musicAttr.m_bitrate == bitrate || musicAttr.m_bitrate > 321)
        {
            if(!M_NETWORK_PTR->isOnline())
            {
                return;
            }

            QString musicSong = musicSongInfo.m_singerName + " - " + musicSongInfo.m_songName;
            QString downloadPrefix = ui->downloadPathEdit->text().isEmpty() ? MUSIC_DIR_FULL : ui->downloadPathEdit->text();
            QString downloadName = QString("%1%2.%3").arg(downloadPrefix).arg(musicSong).arg(musicAttr.m_format);
            ////////////////////////////////////////////////
            MusicDownloadRecords records;
            MusicMyDownloadRecordConfigManager down(this);
            if(!down.readDownloadXMLConfig())
            {
                return;
            }

            down.readDownloadConfig( records );
            MusicDownloadRecord record;
            record.m_name = musicSong;
            record.m_path = QFileInfo(downloadName).absoluteFilePath();
            record.m_size = musicAttr.m_size;
            records << record;
            down.writeDownloadConfig( records );
            ////////////////////////////////////////////////
            QFile file(downloadName);
            if(file.exists())
            {
                for(int i=1; i<99; ++i)
                {
                    if(!QFile::exists(downloadName))
                    {
                        break;
                    }
                    if(i != 1)
                    {
                        musicSong.chop(3);
                    }
                    musicSong += QString("(%1)").arg(i);
                    downloadName = QString("%1%2.%3").arg(downloadPrefix).arg(musicSong).arg(musicAttr.m_format);
                }
            }
            ////////////////////////////////////////////////
            MusicDataDownloadThread *downSong = new MusicDataDownloadThread( musicAttr.m_url, downloadName,
                                                                             MusicDownLoadThreadAbstract::Download_Music, this);
            connect(downSong, SIGNAL(downLoadDataChanged(QString)), SLOT(dataDownloadFinished()));
            downSong->startToDownload();
            break;
        }
    }
}

void MusicDownloadWidget::startToDownloadMovie()
{
    MusicObject::MusicSongInfomations musicSongInfos(m_downloadThread->getMusicSongInfos());
    if(!musicSongInfos.isEmpty())
    {
        startToDownloadMovie(musicSongInfos.first());
    }
}

void MusicDownloadWidget::startToDownloadMovie(const MusicObject::MusicSongInfomation &musicSongInfo)
{
    int bitrate = ui->viewArea->getCurrentBitrate();
    MusicObject::MusicSongAttributes musicAttrs = musicSongInfo.m_songAttrs;
    foreach(const MusicObject::MusicSongAttribute &musicAttr, musicAttrs)
    {
        if(musicAttr.m_bitrate == bitrate)
        {
            if(!M_NETWORK_PTR->isOnline())
            {
                return;
            }

            QString musicSong = musicSongInfo.m_singerName + " - " + musicSongInfo.m_songName;
            QString downloadPrefix = ui->downloadPathEdit->text().isEmpty() ? MOVIE_DIR_FULL : ui->downloadPathEdit->text();
            QString downloadName = QString("%1%2.%3").arg(downloadPrefix).arg(musicSong).arg(musicAttr.m_format);
            ////////////////////////////////////////////////
            QFile file(downloadName);
            if(file.exists())
            {
                for(int i=1; i<99; ++i)
                {
                    if(!QFile::exists(downloadName))
                    {
                        break;
                    }
                    if(i != 1)
                    {
                        musicSong.chop(3);
                    }
                    musicSong += QString("(%1)").arg(i);
                    downloadName = QString("%1%2.%3").arg(downloadPrefix).arg(musicSong).arg(musicAttr.m_format);
                }
            }
            ////////////////////////////////////////////////
            MusicDataDownloadThread* download = new MusicDataDownloadThread(musicAttr.m_url, downloadName,
                                                                            MusicDownLoadThreadAbstract::Download_Video, this);
            connect(download, SIGNAL(downLoadDataChanged(QString)), SLOT(dataDownloadFinished()));
            download->startToDownload();
            break;
        }
    }
}
