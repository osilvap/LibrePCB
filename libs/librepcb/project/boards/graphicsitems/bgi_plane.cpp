/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <QPrinter>
#include "bgi_plane.h"
#include "../items/bi_plane.h"
#include "../board.h"
#include "../../project.h"
#include <librepcb/common/geometry/polygon.h>
#include "../boardlayerstack.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BGI_Plane::BGI_Plane(BI_Plane& plane) noexcept :
    BGI_Base(), mPlane(plane), mLayer(nullptr)
{
    updateCacheAndRepaint();
}

BGI_Plane::~BGI_Plane() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool BGI_Plane::isSelectable() const noexcept
{
    return mLayer && mLayer->isVisible();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BGI_Plane::updateCacheAndRepaint() noexcept
{
    prepareGeometryChange();

    setZValue(getZValueOfCopperLayer(mPlane.getLayerName()));

    mLayer = getLayer(mPlane.getLayerName());

    // get areas
    mAreas.clear();
    for (const Path& r : mPlane.getFragments()) {
        mAreas.append(r.toQPainterPathPx());
    }

    // set shape and bounding rect
    QPainterPathStroker stroker;
    stroker.setWidth(Length::fromMm(0.2).toPx());
    mShape = stroker.createStroke(mPlane.getOutline().toQPainterPathPx());
    mBoundingRect = mShape.boundingRect();

    update();
}

/*****************************************************************************************
 *  Inherited from QGraphicsItem
 ****************************************************************************************/

void BGI_Plane::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    const bool selected = mPlane.isSelected();
    //const bool deviceIsPrinter = (dynamic_cast<QPrinter*>(painter->device()) != 0);
    //const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
    Q_UNUSED(option);

    if (mLayer && mLayer->isVisible()) {
        // draw outline
        painter->setPen(QPen(mLayer->getColor(selected), 0, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(mPlane.getOutline().toQPainterPathPx());

        // draw plane
        painter->setPen(Qt::NoPen);
        painter->setBrush(mLayer->getColor(selected));
        foreach (const QPainterPath& area, mAreas) {
            painter->drawPath(area);
        }
    }

#ifdef QT_DEBUG
    // draw bounding rect
    const GraphicsLayer* layer = mPlane.getBoard().getLayerStack().getLayer(
        GraphicsLayer::sDebugGraphicsItemsBoundingRects);
    if (layer) {
        if (layer->isVisible()) {
            painter->setPen(QPen(layer->getColor(selected), 0));
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(mBoundingRect);
        }
    }
#endif
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

GraphicsLayer* BGI_Plane::getLayer(QString name) const noexcept
{
    if (mPlane.getIsMirrored()) name = GraphicsLayer::getMirroredLayerName(name);
    return mPlane.getBoard().getLayerStack().getLayer(name);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
