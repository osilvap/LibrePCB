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
#include "boardplanefragmentsbuilder.h"
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintpad.h>
#include "items/bi_plane.h"
#include "items/bi_device.h"
#include "items/bi_footprint.h"
#include "items/bi_footprintpad.h"
#include "items/bi_netsegment.h"
#include "items/bi_via.h"
#include "items/bi_netpoint.h"
#include "items/bi_netline.h"
#include "items/bi_polygon.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardPlaneFragmentsBuilder::BoardPlaneFragmentsBuilder(BI_Plane& plane) noexcept :
    mPlane(plane)
{
}

BoardPlaneFragmentsBuilder::~BoardPlaneFragmentsBuilder() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

QVector<Path> BoardPlaneFragmentsBuilder::buildFragments() noexcept
{
    mResult.clear();
    addPlaneOutline();
    clipToBoardOutline();
    subtractOtherObjects();
    ensureMinimumWidth();
    flattenResult();
    if (!mPlane.getKeepOrphans()) {
        removeOrphans();
    }
    return fromClipper(mResult);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BoardPlaneFragmentsBuilder::addPlaneOutline()
{
    mResult.push_back(toClipper(mPlane.getOutline()));
}

void BoardPlaneFragmentsBuilder::clipToBoardOutline()
{
    // determine board area
    ClipperLib::Paths boardArea;
    ClipperLib::Clipper boardAreaClipper;
    foreach (const BI_Polygon* polygon, mPlane.getBoard().getPolygons()) {
        if (polygon->getPolygon().getLayerName() == GraphicsLayer::sBoardOutlines) {
            boardAreaClipper.AddPath(toClipper(polygon->getPolygon().getPath()),
                                     ClipperLib::ptSubject, true);
        }
    }
    boardAreaClipper.Execute(ClipperLib::ctXor, boardArea, ClipperLib::pftEvenOdd,
                             ClipperLib::pftEvenOdd);

    // perform clearance offset
    ClipperLib::ClipperOffset offset(2.0, 5000); // max. 5000nm deviation from perfect arc
    offset.AddPaths(boardArea, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
    offset.Execute(boardArea, -mPlane.getMinClearance().toNm());

    // if we have no board area, abort here
    if (boardArea.empty()) return;

    // clip result to board area
    ClipperLib::Clipper clip;
    clip.AddPaths(mResult, ClipperLib::ptSubject, true);
    clip.AddPaths(boardArea, ClipperLib::ptClip, true);
    clip.Execute(ClipperLib::ctIntersection, mResult, ClipperLib::pftNonZero,
                 ClipperLib::pftNonZero);
}

void BoardPlaneFragmentsBuilder::subtractOtherObjects()
{
    ClipperLib::Clipper c;
    c.AddPaths(mResult, ClipperLib::ptSubject, true);

    // subtract other planes
    foreach (const BI_Plane* plane, mPlane.getBoard().getPlanes()) {
        if (plane == &mPlane) continue;
        if (*plane < mPlane) continue; // ignore planes with lower priority
        if (plane->getLayerName() != mPlane.getLayerName()) continue;
        if (&plane->getNetSignal() == &mPlane.getNetSignal()) continue;
        ClipperLib::Paths expandedPlaneFragments;
        ClipperLib::ClipperOffset o(2.0, 5000); // max. 5000nm deviation from perfect arc
        o.AddPaths(toClipper(plane->getFragments()), ClipperLib::jtRound,
                   ClipperLib::etClosedPolygon);
        o.Execute(expandedPlaneFragments, mPlane.getMinClearance().toNm());
        c.AddPaths(expandedPlaneFragments, ClipperLib::ptClip, true);
    }

    // subtract holes and pads from devices
    foreach (const BI_Device* device, mPlane.getBoard().getDeviceInstances()) {
        for (const Hole& hole : device->getFootprint().getLibFootprint().getHoles()) {
            Point pos = device->getFootprint().mapToScene(hole.getPosition());
            Length dia = hole.getDiameter() + mPlane.getMinClearance() * 2;
            c.AddPath(toClipper(Path::circle(dia).translated(pos)),
                      ClipperLib::ptClip, true);
        }
        foreach (const BI_FootprintPad* pad, device->getFootprint().getPads()) {
            if (!pad->isOnLayer(mPlane.getLayerName())) continue;
            if (pad->getCompSigInstNetSignal() == &mPlane.getNetSignal()) {
                mConnectedNetSignalAreas.push_back(toClipper(pad->getSceneOutline()));
            }
            c.AddPath(createPadCutOut(*pad), ClipperLib::ptClip, true);
        }
    }

    // subtract net segment items
    foreach (const BI_NetSegment* netsegment, mPlane.getBoard().getNetSegments()) {

        // subtract vias
        foreach (const BI_Via* via, netsegment->getVias()) {
            if (&netsegment->getNetSignal() == &mPlane.getNetSignal()) {
                mConnectedNetSignalAreas.push_back(toClipper(via->getSceneOutline()));
            }
            c.AddPath(createViaCutOut(*via), ClipperLib::ptClip, true);
        }

        // subtract netlines
        foreach (const BI_NetLine* netline, netsegment->getNetLines()) {
            if (netline->getLayer().getName() != mPlane.getLayerName()) continue;
            if (&netsegment->getNetSignal() == &mPlane.getNetSignal()) {
                mConnectedNetSignalAreas.push_back(toClipper(netline->getSceneOutline()));
            } else {
                c.AddPath(toClipper(netline->getSceneOutline(mPlane.getMinClearance())),
                          ClipperLib::ptClip, true);
            }
        }
    }

    c.Execute(ClipperLib::ctDifference, mResult, ClipperLib::pftEvenOdd,
              ClipperLib::pftNonZero);
}

void BoardPlaneFragmentsBuilder::ensureMinimumWidth()
{
    ClipperLib::ClipperOffset o1(2.0, 5000); // max. 5000nm deviation from perfect arc
    o1.AddPaths(mResult, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
    o1.Execute(mResult, (mPlane.getMinWidth() / -2).toNm());

    ClipperLib::ClipperOffset o2(2.0, 5000); // max. 5000nm deviation from perfect arc
    o2.AddPaths(mResult, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
    o2.Execute(mResult, (mPlane.getMinWidth() / 2).toNm());
}

void BoardPlaneFragmentsBuilder::flattenResult()
{
    // convert paths to tree
    ClipperLib::PolyTree tree;
    ClipperLib::Clipper c;
    c.AddPaths(mResult, ClipperLib::ptSubject, true);
    c.Execute(ClipperLib::ctXor, tree, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);

    // convert tree to simple paths with cut-ins
    mResult = flattenClipperTree(tree);
}

void BoardPlaneFragmentsBuilder::removeOrphans()
{
    mResult.erase(std::remove_if(mResult.begin(), mResult.end(),
        [this](const ClipperLib::Path& p){
            ClipperLib::Paths intersections;
            ClipperLib::Clipper c;
            c.AddPaths(mConnectedNetSignalAreas, ClipperLib::ptSubject, true);
            c.AddPath(p, ClipperLib::ptClip, true);
            c.Execute(ClipperLib::ctIntersection, intersections, ClipperLib::pftNonZero,
                      ClipperLib::pftNonZero);
            return intersections.empty();
        }),
        mResult.end());
}

/*****************************************************************************************
 *  Helper Methods
 ****************************************************************************************/

ClipperLib::Path BoardPlaneFragmentsBuilder::createPadCutOut(const BI_FootprintPad& pad) const noexcept
{
    bool differentNetSignal = (pad.getCompSigInstNetSignal() != &mPlane.getNetSignal());
    if ((mPlane.getConnectStyle() == BI_Plane::ConnectStyle::None) || differentNetSignal) {
        return toClipper(pad.getSceneOutline(mPlane.getMinClearance()));
    } else {
        return ClipperLib::Path();
    }
}

ClipperLib::Path BoardPlaneFragmentsBuilder::createViaCutOut(const BI_Via& via) const noexcept
{
    bool differentNetSignal = (&via.getNetSignalOfNetSegment() != &mPlane.getNetSignal());
    if ((mPlane.getConnectStyle() == BI_Plane::ConnectStyle::None) || differentNetSignal) {
        return toClipper(via.getSceneOutline(mPlane.getMinClearance()));
    } else {
        return ClipperLib::Path();
    }
}

ClipperLib::Path BoardPlaneFragmentsBuilder::convertHolesToCutIns(
    const ClipperLib::Path& outline, const ClipperLib::Paths& holes) noexcept
{
    ClipperLib::Path path = outline;
    ClipperLib::Paths preparedHoles = prepareHoles(holes);
    for (const ClipperLib::Path& hole : preparedHoles) {
        addCutInToPath(path, hole);
    }
    return path;
}

ClipperLib::Paths BoardPlaneFragmentsBuilder::prepareHoles(const ClipperLib::Paths& holes) noexcept
{
    ClipperLib::Paths preparedHoles;
    for (const ClipperLib::Path& hole : holes) {
        if (hole.size() > 2) {
            preparedHoles.push_back(rotateCutInHole(hole));
        } else {
            qWarning() << "Detected invalid hole in plane, ignoring it.";
        }
    }
    // important: sort holes by the y coordinate of their connection point
    // (to make sure no cut-ins are overlapping in the resulting plane)
    std::sort(preparedHoles.begin(), preparedHoles.end(),
              [](const ClipperLib::Path& p1, const ClipperLib::Path& p2)
              {return p1.front().Y < p2.front().Y;});
    return preparedHoles;
}

ClipperLib::Path BoardPlaneFragmentsBuilder::rotateCutInHole(const ClipperLib::Path& hole) noexcept
{
    ClipperLib::Path p = hole;
    if (p.back() == p.front()) {
        p.pop_back();
    }
    std::rotate(p.begin(), p.begin() + getHoleConnectionPointIndex(p), p.end());
    return p;
}

int BoardPlaneFragmentsBuilder::getHoleConnectionPointIndex(const ClipperLib::Path& hole) noexcept
{
    int index = 0;
    for (size_t i = 1; i < hole.size(); ++i) {
        if (hole.at(i).Y < hole.at(index).Y) {
            index = i;
        }
    }
    return index;
}

void BoardPlaneFragmentsBuilder::addCutInToPath(ClipperLib::Path& outline,
                                                const ClipperLib::Path& hole) noexcept
{
    int index = insertConnectionPointToPath(outline, hole.front());
    if (index >= 0) {
        outline.insert(outline.begin() + index, hole.begin(), hole.end());
    } else {
        qCritical() << "Failed to calculate the connection point of a plane cut-in!";
        qCritical() << "The plane may be invalid (including the Gerber output)!";
    }
}

int BoardPlaneFragmentsBuilder::insertConnectionPointToPath(ClipperLib::Path& path,
                                                            const ClipperLib::IntPoint& p) noexcept
{
    int nearestIndex = -1;
    ClipperLib::IntPoint nearestPoint;
    for (size_t i = 0; i < path.size(); ++i) {
        ClipperLib::cInt y;
        if (calcIntersectionPos(path.at(i), path.at((i + 1) % path.size()), p.X, y)) {
            if ((y <= p.Y) && ((nearestIndex < 0) || (p.Y - y < p.Y - nearestPoint.Y))) {
                nearestIndex = i;
                nearestPoint = ClipperLib::IntPoint(p.X, y);
            }
        }
    }
    if (nearestIndex >= 0) {
        path.insert(path.begin() + nearestIndex + 1, nearestPoint);
        path.insert(path.begin() + nearestIndex + 1, p);
        path.insert(path.begin() + nearestIndex + 1, nearestPoint);
        return nearestIndex + 2;
    } else {
        return -1; // this should actually never happen!
    }
}

bool BoardPlaneFragmentsBuilder::calcIntersectionPos(const ClipperLib::IntPoint& p1,
    const ClipperLib::IntPoint& p2, const ClipperLib::cInt& x, ClipperLib::cInt& y) noexcept
{
    if (((p1.X <= x) && (p2.X > x)) || ((p1.X >= x) && (p2.X < x))) {
        qreal yCalc = p1.Y + (qreal(x - p1.X) * qreal(p2.Y - p1.Y) / qreal(p2.X - p1.X));
        y = qBound(qMin(p1.Y, p2.Y), ClipperLib::cInt(yCalc), qMax(p1.Y, p2.Y));
        return true;
    } else {
        return false;
    }
}

/*****************************************************************************************
 *  Conversion Methods
 ****************************************************************************************/

ClipperLib::Paths BoardPlaneFragmentsBuilder::flattenClipperTree(const ClipperLib::PolyNode& node) noexcept
{
    ClipperLib::Paths paths;
    for (const ClipperLib::PolyNode* outlineChild : node.Childs) { Q_ASSERT(outlineChild);
        Q_ASSERT(!outlineChild->IsHole());
        ClipperLib::Paths holes;
        for (ClipperLib::PolyNode* holeChild : outlineChild->Childs) { Q_ASSERT(holeChild);
            Q_ASSERT(holeChild->IsHole());
            holes.push_back(holeChild->Contour);
            ClipperLib::Paths subpaths = flattenClipperTree(*holeChild);
            paths.insert(paths.end(), subpaths.begin(), subpaths.end());
        }
        paths.push_back(convertHolesToCutIns(outlineChild->Contour, holes));
    }
    return paths;
}

QVector<Path> BoardPlaneFragmentsBuilder::fromClipper(const ClipperLib::Paths& paths) noexcept
{
    QVector<Path> p;
    p.reserve(paths.size());
    for (const ClipperLib::Path& path : paths) {
        p.append(fromClipper(path));
    }
    return p;
}

Path BoardPlaneFragmentsBuilder::fromClipper(const ClipperLib::Path& path) noexcept
{
    Path p;
    for (const ClipperLib::IntPoint& point : path) {
        p.addVertex(fromClipper(point));
    }
    p.close();
    return p;
}

Point BoardPlaneFragmentsBuilder::fromClipper(const ClipperLib::IntPoint& point) noexcept
{
    return Point(point.X, point.Y);
}

ClipperLib::Paths BoardPlaneFragmentsBuilder::toClipper(const QVector<Path>& paths) noexcept
{
    ClipperLib::Paths p;
    p.reserve(paths.size());
    foreach (const Path& path, paths) {
        p.push_back(toClipper(path));
    }
    return p;
}

ClipperLib::Path BoardPlaneFragmentsBuilder::toClipper(const Path& path) noexcept
{
    ClipperLib::Path p;
    for (int i = 0; i < path.getVertices().count(); ++i) {
        const Vertex& vertex = path.getVertices().at(i);
        if ((i == 0) || (vertex.getAngle() == 0)) {
            p.push_back(toClipper(vertex.getPos()));
        } else {
            Path arc = Path::flatArc(path.getVertices().at(i-1).getPos(), vertex.getPos(),
                                     vertex.getAngle(), 5000); // max. 5000nm deviation from perfect arc
            for (int k = 1; k < arc.getVertices().count(); ++k) { // skip first point as it is a duplicate
                p.push_back(toClipper(arc.getVertices().at(k).getPos()));
            }
        }
    }
    // make sure all paths have the same orientation, otherwise we get strange results
    if (ClipperLib::Orientation(p)) {
        ClipperLib::ReversePath(p);
    }
    return p;
}

ClipperLib::IntPoint BoardPlaneFragmentsBuilder::toClipper(const Point& point) noexcept
{
    return ClipperLib::IntPoint(point.getX().toNm(), point.getY().toNm());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
