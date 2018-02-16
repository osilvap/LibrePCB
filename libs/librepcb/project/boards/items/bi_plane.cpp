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
#include "bi_plane.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../../circuit/netsignal.h"
#include "../graphicsitems/bgi_plane.h"
#include "../boardplanefragmentsbuilder.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BI_Plane::BI_Plane(Board& board, const BI_Plane& other) :
    BI_Base(board), mUuid(Uuid::createRandom()),
    mLayerName(other.mLayerName), mNetSignal(other.mNetSignal),
    mMinWidth(other.mMinWidth), mMinClearance(other.mMinClearance),
    mKeepOrphans(other.mKeepOrphans), mPriority(other.mPriority),
    mConnectStyle(other.mConnectStyle), mThermalGapWidth(other.mThermalGapWidth),
    mThermalSpokeWidth(other.mThermalSpokeWidth),
    mFragments(other.mFragments) // also copy fragments to avoid the need for a rebuild
{
    mOutline.reset(new Path(*other.mOutline));
    init();
}

BI_Plane::BI_Plane(Board& board, const SExpression& node) :
    BI_Base(board)
{
    mUuid = node.getChildByIndex(0).getValue<Uuid>(true);
    mLayerName = node.getValueByPath<QString>("layer", true);
    Uuid netSignalUuid = node.getValueByPath<Uuid>("net", true);
    mNetSignal = mBoard.getProject().getCircuit().getNetSignalByUuid(netSignalUuid);
    if(!mNetSignal) {
        throw RuntimeError(__FILE__, __LINE__,
            QString(tr("Invalid net signal UUID: \"%1\"")).arg(netSignalUuid.toStr()));
    }
    mMinWidth = node.getValueByPath<Length>("min_width", true);
    mMinClearance = node.getValueByPath<Length>("min_clearance", true);
    mKeepOrphans = node.getValueByPath<bool>("keep_orphans", true);
    mPriority = node.getValueByPath<int>("priority", true);
    if (node.getValueByPath<QString>("connect_style", true) == "none") {
        mConnectStyle = ConnectStyle::None;
    //} else if (node.getValueByPath<QString>("connect_style", true) == "thermal") {
    //    mConnectStyle = ConnectStyle::Thermal;
    } else if (node.getValueByPath<QString>("connect_style", true) == "solid") {
        mConnectStyle = ConnectStyle::Solid;
    } else {
        throw RuntimeError(__FILE__, __LINE__, tr("Unknown plane connect style."));
    }
    mThermalGapWidth = node.getValueByPath<Length>("thermal_gap_width", true);
    mThermalSpokeWidth = node.getValueByPath<Length>("thermal_spoke_width", true);

    mOutline.reset(new Path(node));
    init();
}

//BI_Plane::BI_Plane(Board& board, const QString& layerName, const Length& lineWidth, bool fill,
//                   bool isGrabArea, const Point& startPos) :
//    BI_Base(board)
//{
//    mPolygon.reset(new Polygon(layerName, lineWidth, fill, isGrabArea, startPos));
//    init();
//}

void BI_Plane::init()
{
    mGraphicsItem.reset(new BGI_Plane(*this));
    mGraphicsItem->setPos(getPosition().toPxQPointF());
    mGraphicsItem->setRotation(Angle::deg0().toDeg());

    // connect to the "attributes changed" signal of the board
    connect(&mBoard, &Board::attributesChanged, this, &BI_Plane::boardAttributesChanged);
}

BI_Plane::~BI_Plane() noexcept
{
    mGraphicsItem.reset();
    mOutline.reset();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BI_Plane::addToBoard()
{
    if (isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    // TODO: register to netsignal
    BI_Base::addToBoard(mGraphicsItem.data());

    mGraphicsItem->updateCacheAndRepaint(); // TODO: remove this
}

void BI_Plane::removeFromBoard()
{
    if (!isAddedToBoard()) {
        throw LogicError(__FILE__, __LINE__);
    }
    // TODO: unregister from netsignal
    BI_Base::removeFromBoard(mGraphicsItem.data());
}

void BI_Plane::clear() noexcept
{
    mFragments.clear();
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_Plane::rebuild() noexcept
{
    BoardPlaneFragmentsBuilder builder(*this);
    mFragments = builder.buildFragments();
    mGraphicsItem->updateCacheAndRepaint();
}

void BI_Plane::serialize(SExpression& root) const
{
    root.appendToken(mUuid);
    root.appendTokenChild("layer", mLayerName, false);
    root.appendTokenChild("net", mNetSignal->getUuid(), true);
    root.appendTokenChild("priority", mPriority, true);
    root.appendTokenChild("keep_orphans", mKeepOrphans, false);
    root.appendTokenChild("min_width", mMinWidth, false);
    root.appendTokenChild("min_clearance", mMinClearance, false);
    QString connectStyle;
    switch (mConnectStyle) {
        case ConnectStyle::None:    connectStyle = "none";      break;
        //case ConnectStyle::Thermal: connectStyle = "thermal";   break;
        case ConnectStyle::Solid:   connectStyle = "solid";     break;
        default: throw LogicError(__FILE__, __LINE__);
    }
    root.appendTokenChild("connect_style", connectStyle, true);
    root.appendTokenChild("thermal_gap_width", mThermalGapWidth, false);
    root.appendTokenChild("thermal_spoke_width", mThermalSpokeWidth, false);
    mOutline->serialize(root);
}

/*****************************************************************************************
 *  Inherited from BI_Base
 ****************************************************************************************/

QPainterPath BI_Plane::getGrabAreaScenePx() const noexcept
{
    return mGraphicsItem->sceneTransform().map(mGraphicsItem->shape());
}

bool BI_Plane::isSelectable() const noexcept
{
    return mGraphicsItem->isSelectable();
}

void BI_Plane::setSelected(bool selected) noexcept
{
    BI_Base::setSelected(selected);
    mGraphicsItem->update();
}

/*****************************************************************************************
 *  Operator Overloadings
 ****************************************************************************************/

bool BI_Plane::operator<(const BI_Plane& rhs) const noexcept
{
    // First sort by priority, then by uuid to get a really unique priority order over all
    // existing planes. This way we can ensure that even planes with the same priority
    // will always be filled in the same order. Random order would be dangerous!
    if (mPriority != rhs.mPriority) {
        return mPriority < rhs.mPriority;
    } else {
        return mUuid < rhs.mUuid;
    }
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void BI_Plane::boardAttributesChanged()
{
    mGraphicsItem->updateCacheAndRepaint();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
