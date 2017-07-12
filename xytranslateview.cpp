﻿#include "xytranslateview.h"
#include "xymenu.h"
#include "xyinput.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

XYTranslateView::XYTranslateView(QWidget *parent)
    : XYBorderShadowWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
                   | Qt::Tool
                   | Qt::WindowDoesNotAcceptFocus);
    this->setAttribute(Qt::WA_TranslucentBackground);

    mopMenu = NULL;
    mopModel = new XYTranslateModel;
    moFont = qApp->font();
    miCurrentPage = 0;
    miMaxVisibleItem = 7;
    showType = XYTranslateModel::TRANSLATE;
    setMouseTracking(true);
    resize(50, 50);
}

XYTranslateView::~XYTranslateView()
{
    delete mopModel;
}

void XYTranslateView::appendData(const QList<XYTranslateItem *> &list)
{
    mopModel->appendData(list);
}

void XYTranslateView::prependData(const QList<XYTranslateItem *> &list)
{
    mopModel->prependData(list);
}

void XYTranslateView::setData(const QList<XYTranslateItem *> &list)
{
    mopModel->setData(list);
    update();
}

void XYTranslateView::clear()
{
    mopModel->clear();
}

void XYTranslateView::setFont(const QFont &font)
{
    moFont = font;
}

void XYTranslateView::nextPage()
{
    double allPages = mopModel->counts() * 1.0 / miMaxVisibleItem;
    if (allPages > miCurrentPage + 1)
    {
        miCurrentPage++;
    }
}

void XYTranslateView::prePage()
{
    if (miCurrentPage - 1 >= 0)
    {
        miCurrentPage--;
    }
}

int XYTranslateView::itemCount()
{
    return mopModel->counts();
}

QString XYTranslateView::getData(int index)
{
    return mopModel->data(miCurrentPage * miMaxVisibleItem + index - 1, showType);
}

void XYTranslateView::show()
{
    QFontMetrics metrics(moFont);
    int i;
    int max_width = 0;
    for (i = 0; i < miMaxVisibleItem; ++i)
    {
        int index = i + miCurrentPage * miMaxVisibleItem;
        if (mopModel->haveData(index))
        {
            QString data = QString("%1.%2").arg(i + 1).arg(mopModel->data(index, showType));
            max_width = qMax(metrics.width(data), max_width);
        }
        else
        {
            break;
        }
    }
    resize(qMax(max_width + 30, 200), 25 + i * (metrics.height() + 5));
    QWidget::show();
}

void XYTranslateView::close()
{
    if (mopMenu)
    {
        mopMenu->close();
        mopMenu = NULL;
    }
    XYBorderShadowWidget::close();
}

void XYTranslateView::stickItem()
{
    QAction *act = (QAction *)sender();
    XYTranslateItem *item = (XYTranslateItem *)act->data().toLongLong();

    if (item)
    {
        mopModel->stickItem(item);
        update();
    }
    // 这里还应该改变词库对应的词条，置顶属性
}

void XYTranslateView::delItem()
{
    QAction *act = (QAction *)sender();
    XYTranslateItem *item = (XYTranslateItem *)act->data().toLongLong();

    if (item)
    {
        mopModel->delItem(item);
        update();
    }
    // 这里还应该删除词库对应的词条
}

void XYTranslateView::paintEvent(QPaintEvent *event)
{
    XYBorderShadowWidget::paintEvent(event);
    QPainter painter(this);
    painter.setFont(moFont);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    QFontMetrics metrics(moFont);
    int i;
    int max_width = 0;
    for (i = 0; i < miMaxVisibleItem; ++i)
    {
        int index = i + miCurrentPage * miMaxVisibleItem;
        if (mopModel->haveData(index))
        {
            QString data = QString("%1.%2").arg(i + 1).arg(mopModel->data(index, showType));
            mlistCurrentItems[i].rect = QRect(15,
                                              25 + i * (metrics.height() + 5),
                                              metrics.width(data),
                                              metrics.height());
            mlistCurrentItems[i].item = mopModel->getItem(index);
            painter.drawText(mlistCurrentItems[i].rect.x(), mlistCurrentItems[i].rect.y(), data);
            max_width = qMax(metrics.width(data), max_width);
        }
        else
        {
            break;
        }
    }
}

void XYTranslateView::mouseMoveEvent(QMouseEvent *event)
{
    XYBorderShadowWidget::mouseMoveEvent(event);
    int index = getItemIndex(event->pos() + QPoint(0, 10));
    if (index != -1)
    {
        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        setCursor(Qt::ArrowCursor);
    }
}

void XYTranslateView::mouseReleaseEvent(QMouseEvent *event)
{
    XYBorderShadowWidget::mouseReleaseEvent(event);
    if (event->button() == Qt::RightButton)
    {
        int index = getItemIndex(event->pos() + QPoint(0, 10));
        if (index != -1)
        {
            XYTranslateItem *item = contentsItem(event->pos() + QPoint(0, 10));
            XYMenu menu;
            QAction *del = new QAction(QString(QStringLiteral("删除 \"%1\"")).arg(getData(index + 1)), &menu);
            del->setData((long long)item);
            QAction *fixed = new QAction(QString(QStringLiteral("置顶 \"%1\"")).arg(getData(index + 1)), &menu);
            fixed->setData((long long)item);
            connect(del, SIGNAL(triggered()), this, SLOT(delItem()));
            connect(fixed, SIGNAL(triggered()), this, SLOT(stickItem()));
            menu.addAction(fixed);
            menu.addAction(del);
            mopMenu = &menu;
            menu.exec();
            mopMenu = NULL;
        }
    }
    else if (event->button() == Qt::LeftButton)
    {
        int index = getItemIndex(event->pos() + QPoint(0, 10));
        if (index != -1)
        {
            XYInput *input = XYInput::getInstance();
            qApp->postEvent(input->mopLatestWidget, new QKeyEvent(QEvent::KeyPress,
                                                           Qt::Key_unknown,
                                                           Qt::NoModifier,
                                                           getData(index + 1)));
            input->close();
        }
    }
}

XYTranslateItem *XYTranslateView::contentsItem(const QPoint &point)
{
    for (int i = 0; i < 9; ++i)
    {
        if (mlistCurrentItems[i].rect.contains(point))
        {
            return mlistCurrentItems[i].item;
        }
    }
    return NULL;
}

int XYTranslateView::getItemIndex(const QPoint &point)
{
    for (int i = 0; i < 9; ++i)
    {
        if (mlistCurrentItems[i].rect.contains(point))
        {
            return i;
        }
    }
    return -1;
}